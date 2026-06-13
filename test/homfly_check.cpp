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
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
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

/// Human-readable form of a (L,M)->coef polynomial, for diagnostics.
std::string PolyToString(const Polynomial& p)
{
    if (p.empty()) { return "0"; }
    std::string s;
    for (const auto& [lm, c] : p)
    {
        if (!s.empty()) { s += " + "; }
        s += std::to_string(c) + " L^" + std::to_string(lm.first)
                               + " M^" + std::to_string(lm.second);
    }
    return s;
}

//==============================================================================
// Invariance check
//==============================================================================
//
// HOMFLY is a link invariant, so simplification must not change it. We compare
// homfly(input) to homfly(whole simplified diagram). The simplified complex is
// reassembled into one diagram with PlanarDiagramComplex::ToSingleDiagram(),
// which connect-sums the pieces back together (and converts anelli to
// farfalle) — HOMFLY-faithful for knots and *non-split* links. All our test
// corpora are non-split (link-table entries are prime; plantri gives connected
// diagrams), so no product/δ bookkeeping is needed. A split-link input would
// need that and is out of scope here.

/// Simplify args mirroring knoodlesimplify's default (Reapr / level-6) path;
/// the struct's defaults already set splitQ/rerouteQ/disconnectQ = true.
PDC_T::Simplify_Args_T SimplifyArgs()
{
    return PDC_T::Simplify_Args_T{};
}

/// HOMFLY of the simplified diagram as a whole. Returns the unknot polynomial
/// {(0,0):1} when the input simplifies to the trivial knot.
Polynomial SimplifiedHomfly(const PD_T& pd)
{
    PD_T  copy(pd);
    PDC_T pdc(std::move(copy));
    pdc.Simplify(SimplifyArgs());

    if (!pdc.ValidQ() || pdc.DiagramCount() == Int(0))
    {
        return Polynomial{ {{0, 0}, 1} };  // unknot
    }
    return LibhomflyPolyMap(KnoodleJenkins(pdc.ToSingleDiagram()));
}

struct InvarianceResult
{
    bool       ok;
    Polynomial before;
    Polynomial after;
};

InvarianceResult CheckInvariance(const PD_T& pd)
{
    Polynomial before = LibhomflyPolyMap(KnoodleJenkins(pd));
    Polynomial after  = SimplifiedHomfly(pd);
    const bool ok = (before == after);
    return { ok, std::move(before), std::move(after) };
}

/// Build the "before" diagram from a file's text. Auto-detects format: a '.'
/// anywhere means a 3-column 3D embedding (projected via FromKnotEmbedding);
/// otherwise a 5-column signed PD code. Sets ok=false on a malformed shape.
PD_T BuildDiagram(const std::string& text, bool& ok)
{
    ok = false;

    if (text.find('.') != std::string::npos)   // 3D embedding (floats)
    {
        std::istringstream in(text);
        std::vector<double> coords;
        double v;
        while (in >> v) { coords.push_back(v); }
        if (coords.empty() || coords.size() % 3 != 0) { return PD_T::InvalidDiagram(); }

        Int nverts = static_cast<Int>(coords.size() / 3);
        // Drop a duplicated closing vertex if present (closed-polygon marker).
        const double* first = coords.data();
        const double* last  = coords.data() + 3 * (nverts - 1);
        if (nverts > 1 && first[0] == last[0] && first[1] == last[1]
                       && first[2] == last[2])
        {
            --nverts;
        }

        auto [pd, unlinks] = PD_T::FromKnotEmbedding(coords.data(), nverts);
        (void)unlinks;
        ok = pd.ValidQ();
        return pd;
    }

    std::istringstream in(text);                // 5-col signed PD code (ints)
    std::vector<Int> ints;
    Int v;
    while (in >> v) { ints.push_back(v); }
    if (ints.empty() || ints.size() % 5 != 0) { return PD_T::InvalidDiagram(); }

    ok = true;
    return BuildPD(ints, static_cast<Int>(ints.size() / 5));
}

/// --invariance [files...]: run the invariance check on each input — a 5-col PD
/// code or a 3-col 3D embedding (auto-detected) — or one diagram on stdin if no
/// files. Reports per-item and an aggregate; exit nonzero on any failure.
int Invariance(int argc, char* argv[])
{
    std::vector<std::string> files;
    for (int i = 2; i < argc; ++i) { files.emplace_back(argv[i]); }

    long passed = 0, failed = 0, unreadable = 0;

    auto run_one = [&](const std::string& label, const std::string& text)
    {
        bool ok = false;
        PD_T pd = BuildDiagram(text, ok);
        if (!ok)
        {
            std::cerr << "  SKIP " << label << ": not a valid PD code / embedding\n";
            ++unreadable;
            return;
        }
        const Int crossings = pd.CrossingCount();
        InvarianceResult r = CheckInvariance(pd);
        if (r.ok)
        {
            ++passed;
        }
        else
        {
            ++failed;
            std::cout << "  FAIL " << label << " (" << crossings << " crossings)\n"
                      << "    before: " << PolyToString(r.before) << "\n"
                      << "    after : " << PolyToString(r.after)  << "\n";
        }
    };

    if (files.empty())
    {
        std::stringstream ss;
        ss << std::cin.rdbuf();
        run_one("stdin", ss.str());
    }
    else
    {
        for (const std::string& f : files)
        {
            std::ifstream in(f);
            if (!in) { std::cerr << "  SKIP " << f << ": cannot open\n"; ++unreadable; continue; }
            std::stringstream ss;
            ss << in.rdbuf();
            run_one(f, ss.str());
        }
    }

    std::cout << "\ninvariance: " << passed << " preserved, " << failed
              << " changed, " << unreadable << " unreadable\n";
    return (failed == 0 && unreadable == 0) ? 0 : 1;
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
    if (argc > 1 && std::string(argv[1]) == "--invariance")
    {
        return Invariance(argc, argv);
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

    // Invariance self-test: simplify each panel knot and confirm HOMFLY is
    // unchanged (HOMFLY is a link invariant; simplification must preserve it).
    std::cout << "\n=== invariance (simplify must preserve HOMFLY) ===\n";
    for (const KnownKnot& k : panel)
    {
        InvarianceResult r = CheckInvariance(BuildPD(k.pd, k.crossings));
        std::cout << "  " << (r.ok ? "PASS" : "FAIL") << "  " << k.name << "\n";
        if (!r.ok)
        {
            std::cout << "    before: " << PolyToString(r.before) << "\n"
                      << "    after : " << PolyToString(r.after)  << "\n";
            ++failures;
        }
    }

    // Sensitivity guard: the oracle must distinguish distinct knots, otherwise
    // the invariance equality check would be vacuous (always "preserved").
    {
        Polynomial tref = LibhomflyPolyMap(KnoodleJenkins(BuildPD(panel[0].pd, panel[0].crossings)));
        Polynomial four = LibhomflyPolyMap(KnoodleJenkins(BuildPD(panel[1].pd, panel[1].crossings)));
        const bool distinct = (tref != four);
        std::cout << "\n=== discrimination (oracle separates distinct knots) ===\n"
                  << "  " << (distinct ? "PASS" : "FAIL")
                  << "  trefoil HOMFLY != figure-eight HOMFLY\n";
        if (!distinct) { ++failures; }
    }

    return failures == 0 ? 0 : 1;
}
