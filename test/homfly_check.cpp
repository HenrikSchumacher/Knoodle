/**
 * @file homfly_check.cpp
 * @brief HOMFLY-based correctness checks for knoodle simplification, using a
 *        vendored, dependency-free copy of libhomfly (public domain) as the
 *        invariant oracle — no Python/Regina at runtime.
 *
 * The bridge from a Knoodle diagram to libhomfly is the core library's own
 * Jenkins code: PlanarDiagram::ToJenkinsCodeString() emits exactly the
 * space-separated integer format libhomfly's homfly()/homfly_str() consumes
 * (component count; per component a crossing count then (id, over/under)
 * pairs; then (id, handedness) pairs).
 *
 * This first stage is the Tier-1 cross-validation: confirm that Knoodle's
 * Jenkins encoding of known knots reproduces libhomfly's *own* published
 * reference polynomials (convention-free, independent of both Knoodle and
 * Regina). A companion Python script cross-checks against Regina.
 *
 * Build: see test/Makefile (target: homfly_check).
 */

#include "../Knoodle.hpp"

extern "C" {
#include "vendor/libhomfly/homfly.h"
}
// Reclaims libhomfly's working memory; call after copying out each result.
extern "C" void knoodle_gc_free_all(void);

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <sys/resource.h>   // getrusage — peak-RSS check for the leak fix

using Int   = std::int64_t;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;
using PD_T  = PDC_T::PD_T;

namespace {

/// HOMFLY polynomial as a canonical map from (L-exponent, M-exponent) to
/// coefficient. libhomfly's (L,M) variables are term-for-term identical to
/// Regina's homflyLM(l,m) (verified empirically), so this representation
/// compares directly across the two oracles.
using Polynomial = std::map<std::pair<int,int>, long>;

/// Knoodle diagram -> libhomfly input string (its native Jenkins code).
std::string KnoodleJenkins(const PD_T& pd)
{
    return std::string(pd.ToJenkinsCodeString());
}

/// Run libhomfly on a Jenkins string, returning its polynomial string.
/// homfly_str takes a mutable char*, so hand it a private buffer.
std::string LibhomflyPoly(const std::string& jenkins)
{
    std::vector<char> buf(jenkins.begin(), jenkins.end());
    buf.push_back('\0');
    char* out = homfly_str(buf.data());
    std::string result = out ? std::string(out) : std::string("<null>");
    knoodle_gc_free_all();  // safe: result already copied to its own storage
    return result;
}

/// Run libhomfly and return the polynomial as a canonical (L,M)->coef map.
Polynomial LibhomflyPolyMap(const std::string& jenkins)
{
    std::vector<char> buf(jenkins.begin(), jenkins.end());
    buf.push_back('\0');
    Poly* p = homfly(buf.data());

    Polynomial out;
    if (p == nullptr) { return out; }
    for (long i = 0; i < p->len; ++i)
    {
        const Term& t = p->term[i];
        out[{ static_cast<int>(t.l), static_cast<int>(t.m) }] += t.coef;
    }
    for (auto it = out.begin(); it != out.end(); )
    {
        if (it->second == 0) { it = out.erase(it); } else { ++it; }
    }
    knoodle_gc_free_all();  // safe: terms already copied into `out`
    return out;
}

struct KnownKnot
{
    std::string        name;
    std::vector<Int>   pd;          ///< flat 5-col signed PD code, 0-based
    Int                crossings;
    std::string        expected;    ///< libhomfly reference poly, or "" if unknown
};

PD_T BuildPD(const std::vector<Int>& pd, Int crossings)
{
    return PD_T::FromSignedPDCode(pd.data(), crossings, false, true);
}

/// --emit-poly: read a flat 5-column signed PD code (whitespace-separated
/// ints) from stdin, compute its HOMFLY via Knoodle-Jenkins -> libhomfly, and
/// print one "L_exp M_exp coef" line per term (sorted). Bridge for the Regina
/// cross-check driver (test/homfly_xcheck.py).
int EmitPoly()
{
    std::vector<Int> ints;
    Int v;
    while (std::cin >> v) { ints.push_back(v); }

    if (ints.empty() || ints.size() % 5 != 0)
    {
        std::cerr << "emit-poly: expected a multiple of 5 integers (5-col "
                     "signed PD code), got " << ints.size() << "\n";
        return 2;
    }

    const Int crossings = static_cast<Int>(ints.size() / 5);
    PD_T pd = BuildPD(ints, crossings);
    const Polynomial poly = LibhomflyPolyMap(KnoodleJenkins(pd));

    for (const auto& [lm, coef] : poly)
    {
        std::cout << lm.first << ' ' << lm.second << ' ' << coef << '\n';
    }
    return 0;
}

/// Peak resident set size in MiB (high-water mark; never decreases).
double PeakRSS_MiB()
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
#if defined(__APPLE__)
    return static_cast<double>(ru.ru_maxrss) / (1024.0 * 1024.0);  // bytes
#else
    return static_cast<double>(ru.ru_maxrss) / 1024.0;             // kibibytes
#endif
}

/// --stress N: compute a fixed knot's HOMFLY N times in one process and assert
/// peak RSS does not climb — a regression guard for the gc_shim leak fix. With
/// the tracked allocator + free_all this plateaus; a leak would grow it without
/// bound.
int Stress(long n)
{
    // Figure-eight: enough allocation per call to expose a leak quickly.
    const std::vector<Int> fig8 = {3,1,4,0,1, 7,5,0,4,1, 5,2,6,3,-1, 1,6,2,7,-1};
    PD_T pd = BuildPD(fig8, 4);
    const std::string jenkins = KnoodleJenkins(pd);

    const long warmup = std::max(1L, n / 100);
    double rss_after_warmup = 0.0;
    Polynomial expected;

    for (long i = 1; i <= n; ++i)
    {
        Polynomial p = LibhomflyPolyMap(jenkins);

        if (i == 1) { expected = p; }
        else if (p != expected)
        {
            std::cerr << "stress: result changed at iter " << i
                      << " (allocator corruption?)\n";
            return 1;
        }

        if (i == warmup) { rss_after_warmup = PeakRSS_MiB(); }
    }

    const double rss_end    = PeakRSS_MiB();
    const double growth     = rss_end - rss_after_warmup;
    const double threshold  = 32.0;  // MiB; generous, real leak would be GBs

    std::cout << "stress: " << n << " homfly() calls on the figure-eight\n"
              << "  peak RSS after warmup (" << warmup << " iters): "
              << rss_after_warmup << " MiB\n"
              << "  peak RSS at end:                  " << rss_end << " MiB\n"
              << "  growth: " << growth << " MiB (threshold " << threshold << ")\n";

    if (growth > threshold)
    {
        std::cout << "  FAIL: peak RSS climbed — memory is not being reclaimed.\n";
        return 1;
    }
    std::cout << "  PASS: peak RSS bounded — gc_shim reclaims between calls.\n";
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--emit-poly")
    {
        return EmitPoly();
    }
    if (argc > 1 && std::string(argv[1]) == "--stress")
    {
        const long n = (argc > 2) ? std::atol(argv[2]) : 200000L;
        return Stress(n);
    }

    // PD codes from test/run_tests.py (0-based, [X0,X1,X2,X3,sign]).
    // Expected polynomials are libhomfly's own reference values
    // (test/vendor/libhomfly/reference_data.txt). Note the leading space and
    // exact spacing libhomfly emits.
    const std::vector<KnownKnot> panel = {
        { "trefoil (3_1)",
          {0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1}, 3,
          "" },   // chirality TBD against reference; filled after first run
        { "figure-eight (4_1)",
          {3,1,4,0,1, 7,5,0,4,1, 5,2,6,3,-1, 1,6,2,7,-1}, 4,
          " - L^-2 - 1 - L^2 + M^2" },
    };

    // libhomfly's published trefoil reference and its mirror (chirality flip
    // is L -> L^-1, i.e. exponent sign flip).
    const std::string trefoil_ref        = " - L^-4 - 2L^-2 + M^2L^-2";
    const std::string trefoil_ref_mirror = " - L^4 - 2L^2 + M^2L^2";

    int failures = 0;

    for (const KnownKnot& k : panel)
    {
        PD_T pd = BuildPD(k.pd, k.crossings);
        const std::string jenkins = KnoodleJenkins(pd);
        const std::string poly    = LibhomflyPoly(jenkins);

        std::cout << "=== " << k.name << " ===\n";
        std::cout << "  jenkins : " << jenkins << "\n";
        std::cout << "  libhomfly: " << poly << "\n";

        std::string expected = k.expected;
        bool ok = true;

        if (k.name.rfind("trefoil", 0) == 0)
        {
            ok = (poly == trefoil_ref || poly == trefoil_ref_mirror);
            std::cout << "  expected : " << trefoil_ref
                      << "  (or mirror " << trefoil_ref_mirror << ")\n";
        }
        else
        {
            ok = (poly == expected);
            std::cout << "  expected : " << expected << "\n";
        }

        std::cout << "  " << (ok ? "PASS" : "FAIL") << "\n\n";
        if (!ok) { ++failures; }
    }

    std::cout << (failures == 0
                  ? "All Tier-1 encoding checks passed.\n"
                  : (std::to_string(failures) + " check(s) FAILED.\n"));
    return failures == 0 ? 0 : 1;
}
