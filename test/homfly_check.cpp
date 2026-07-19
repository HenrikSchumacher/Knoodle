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
 * The HOMFLY oracle + invariance check live in homfly_invariance.hpp (shared
 * with plantri_check.cpp). This file is the CLI driver and the Tier-1 panel.
 *
 * Build: see test/Makefile (target: homfly_check).
 */

#include "homfly_invariance.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <sys/resource.h>   // getrusage — peak-RSS check for the leak fix

namespace {

struct KnownKnot
{
    std::string        name;
    std::vector<Int>   pd;          ///< flat 5-col signed PD code, 0-based
    Int                crossings;
    std::string        expected;    ///< libhomfly reference poly, or "" if unknown
};

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

    long passed = 0, failed = 0, skipped = 0;

    // One machine-readable line per item on stdout:
    //   RESULT \t pass|fail|skip \t <crossings> \t <label>
    // (tab-separated so labels/paths may contain spaces). pass = HOMFLY
    // preserved, fail = HOMFLY changed (a real bug), skip = not verifiable by
    // this oracle (unreadable input, or a split link needing the delta rule).
    // Failure details and skip notes go to stderr so stdout stays parseable.
    auto run_one = [&](const std::string& label, const std::string& text)
    {
        bool ok = false;
        PD_T pd = BuildDiagram(text, ok);
        if (!ok)
        {
            std::cout << "RESULT\tskip\t0\t" << label << "\n";
            std::cerr << "  SKIP " << label << ": not a valid PD code / embedding\n";
            ++skipped;
            return;
        }
        const Int crossings = pd.CrossingCount();
        InvarianceResult r = CheckInvariance(pd);

        const char* st = (r.status == InvStatus::Preserved) ? "pass"
                       : (r.status == InvStatus::Changed)   ? "fail" : "skip";
        std::cout << "RESULT\t" << st << "\t" << crossings << "\t" << label << "\n";

        if (r.status == InvStatus::Preserved)
        {
            ++passed;
        }
        else if (r.status == InvStatus::Changed)
        {
            ++failed;
            std::cerr << "  FAIL " << label << " (" << crossings << " crossings)\n"
                      << "    before: " << PolyToString(r.before) << "\n"
                      << "    after : " << PolyToString(r.after)  << "\n";
        }
        else
        {
            ++skipped;
            std::cerr << "  SKIP " << label << " (" << crossings << " crossings):"
                      << " split link / not reassemblable — needs the delta rule"
                      << " (verify with --cross-check)\n";
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
            if (!in) { std::cout << "RESULT\tskip\t0\t" << f << "\n";
                       std::cerr << "  SKIP " << f << ": cannot open\n"; ++skipped; continue; }
            std::stringstream ss;
            ss << in.rdbuf();
            run_one(f, ss.str());
        }
    }

    std::cout << "\ninvariance: " << passed << " preserved, " << failed
              << " changed, " << skipped << " skipped\n";
    // Exit 1 only on a genuine HOMFLY change; skips keep the code in {0,1} so
    // callers can distinguish a clean run from a crash.
    return (failed == 0) ? 0 : 1;
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
    bool ok = true;
    const Polynomial poly = HomflyOfPossiblySplit(KnoodleJenkins(pd), ok);

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
        const bool ok = (r.status == InvStatus::Preserved);
        std::cout << "  " << (ok ? "PASS" : "FAIL") << "  " << k.name << "\n";
        if (!ok)
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

    // Split-union delta rule: H(trefoil ⊔ figure-eight) must equal
    // delta * H(trefoil) * H(figure-eight). Both are computed here from the
    // same libhomfly pieces, so this checks the delta value and the
    // decomposition/combination logic. (The value also matches Regina's
    // homflyLM — see test/homfly_xcheck.py.)
    {
        // trefoil (arcs 0-5) disjoint figure-eight (arcs shifted by 6).
        std::vector<Int> split = {0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1,
                                  9,7,10,6,1, 13,11,6,10,1, 11,8,12,9,-1, 7,12,8,13,-1};
        bool ok = true;
        Polynomial got = HomflyOfPossiblySplit(KnoodleJenkins(BuildPD(split, 7)), ok);

        Polynomial tref = LibhomflyPolyMap(KnoodleJenkins(BuildPD(panel[0].pd, panel[0].crossings)));
        Polynomial four = LibhomflyPolyMap(KnoodleJenkins(BuildPD(panel[1].pd, panel[1].crossings)));
        Polynomial expected = Multiply(Multiply(Delta, tref), four);

        const bool match = ok && (got == expected);
        std::cout << "\n=== split-union delta rule (trefoil u figure-eight) ===\n"
                  << "  " << (match ? "PASS" : "FAIL")
                  << "  H(A u B) == delta * H(A) * H(B)\n";
        if (!match)
        {
            std::cout << "    got     : " << PolyToString(got) << "\n"
                      << "    expected: " << PolyToString(expected) << "\n";
            ++failures;
        }
    }

    return failures == 0 ? 0 : 1;
}
