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

/// Read every whitespace-separated token of `text` as a T. False if any token
/// is not one: a stray word must not end the read quietly and drop everything
/// after it. (Before 2026-11-22 it did, so three trefoil rows followed by the
/// text of a Git LFS pointer checked as a trefoil.)
template<typename T>
bool ReadAllNumbers(const std::string& text, std::vector<T>& out)
{
    std::istringstream in(text);
    T v;
    while (in >> v) { out.push_back(v); }
    return in.eof();   // stopped at the end, not at a token that is not a T
}

/// How the text of one --invariance input turned out.
enum class Parse
{
    Ok,           ///< a diagram with crossings, ready to check
    NoCrossings,  ///< a well-formed 3D embedding whose projection has no crossings
    Invalid       ///< not a PD code or 3D embedding at all
};

struct Input
{
    Parse       parse;
    PD_T        pd;
    std::string why;   ///< for NoCrossings and Invalid: what was found
};

/// Build the "before" diagram from a file's text. Auto-detects format: a '.'
/// anywhere means a 3-column 3D embedding (projected via FromCoordinates);
/// otherwise a 5-column signed PD code.
Input BuildDiagram(const std::string& text)
{
    const auto invalid = [](std::string why)
    {
        return Input{ Parse::Invalid, PD_T::InvalidDiagram(), std::move(why) };
    };

    // An unfetched Git LFS file: say so, rather than what it fails to parse as.
    // (The header is the one test/run_tests.py and klut_lfs_pointer_check.py
    // look for.)
    if (text.rfind("version https://git-lfs.github.com/spec/", 0) == 0)
    {
        return invalid("a Git LFS pointer, not the data (run `git lfs pull`)");
    }

    if (text.find('.') != std::string::npos)   // 3D embedding (floats)
    {
        std::vector<double> coords;
        if (!ReadAllNumbers(text, coords))
        {
            return invalid("not a 3D embedding: it holds a token that is not a number");
        }
        if (coords.empty()) { return invalid("no numbers in it"); }
        if (coords.size() % 3 != 0)
        {
            return invalid("not a 3D embedding: " + std::to_string(coords.size())
                           + " numbers, not a multiple of 3");
        }

        Int nverts = static_cast<Int>(coords.size() / 3);
        // Drop a duplicated closing vertex if present (closed-polygon marker).
        const double* first = coords.data();
        const double* last  = coords.data() + 3 * (nverts - 1);
        if (nverts > 1 && first[0] == last[0] && first[1] == last[1]
                       && first[2] == last[2])
        {
            --nverts;
        }

        auto [pd, unlinks] = PD_T::FromCoordinates(coords.data(), nverts);
        if (pd.ValidQ()) { return Input{ Parse::Ok, std::move(pd), "" }; }

        // FromCoordinates reports a crossingless curve as an invalid diagram
        // plus one unlink per component, and a failure as an invalid diagram
        // with no unlinks.
        if (unlinks.Size() > 0)
        {
            return Input{ Parse::NoCrossings, std::move(pd),
                          "a 3D embedding with no crossings (an unlink of "
                          + std::to_string(unlinks.Size())
                          + " component(s)): nothing to simplify" };
        }
        return invalid("the 3D embedding did not project to a diagram"
                       " (FromCoordinates failed)");
    }

    std::vector<Int> ints;                      // 5-col signed PD code (ints)
    if (!ReadAllNumbers(text, ints))
    {
        return invalid("not a PD code: it holds a token that is not an integer");
    }
    if (ints.empty()) { return invalid("no numbers in it"); }
    if (ints.size() % 5 != 0)
    {
        return invalid("not a PD code: " + std::to_string(ints.size())
                       + " integers, not a multiple of 5");
    }

    PD_T pd = BuildPD(ints, static_cast<Int>(ints.size() / 5));
    if (!pd.ValidQ()) { return invalid("the PD code does not describe a diagram"); }
    return Input{ Parse::Ok, std::move(pd), "" };
}

/// The --invariance verdict on one input. `status` is what its RESULT line
/// says:
///   pass     HOMFLY preserved by simplification
///   fail     HOMFLY changed: a real bug
///   skip     a well-formed input this oracle cannot verify: a split link that
///            does not reassemble (it needs the delta rule; --cross-check in
///            run_tests.py verifies it with Regina), or a 3D embedding with no
///            crossings (an unlink: there is nothing to simplify)
///   invalid  not a test input at all: the file cannot be opened, or it is not
///            a 5-column PD code or a 3-column 3D embedding. An unfetched Git
///            LFS pointer lands here. Until 2026-11-22 these were `skip`, and
///            run_tests.py counts a skip as a pass, so a pointer passed.
struct Verdict
{
    std::string status;
    Int         crossings = 0;
    std::string note;   ///< for stderr: why it is not a pass
};

Verdict CheckText(const std::string& text)
{
    Input input = BuildDiagram(text);
    if (input.parse == Parse::Invalid)     { return { "invalid", 0, input.why }; }
    if (input.parse == Parse::NoCrossings) { return { "skip",    0, input.why }; }

    const Int crossings = input.pd.CrossingCount();
    InvarianceResult r = CheckInvariance(input.pd);

    if (r.status == InvStatus::Preserved) { return { "pass", crossings, "" }; }
    if (r.status == InvStatus::Changed)
    {
        return { "fail", crossings,
                 "HOMFLY changed\n    before: " + PolyToString(r.before)
                 + "\n    after : " + PolyToString(r.after) };
    }
    return { "skip", crossings,
             "split link / not reassemblable — needs the delta rule"
             " (verify with --cross-check)" };
}

Verdict CheckFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in) { return { "invalid", 0, "cannot open" }; }
    std::stringstream ss;
    ss << in.rdbuf();
    return CheckText(ss.str());
}

/// --invariance [files...]: run the invariance check on each input — a 5-col PD
/// code or a 3-col 3D embedding (auto-detected) — or one diagram on stdin if no
/// files. Reports per-item and an aggregate; exit nonzero on any failure.
int Invariance(int argc, char* argv[])
{
    std::vector<std::string> files;
    for (int i = 2; i < argc; ++i) { files.emplace_back(argv[i]); }

    long passed = 0, failed = 0, skipped = 0, invalid = 0;

    // One machine-readable line per item on stdout:
    //   RESULT \t pass|fail|skip|invalid \t <crossings> \t <label>
    // (tab-separated so labels/paths may contain spaces; see Verdict for what
    // each status means). Failure details and skip/invalid notes go to stderr
    // so stdout stays parseable.
    auto report = [&](const std::string& label, const Verdict& v)
    {
        std::cout << "RESULT\t" << v.status << "\t" << v.crossings << "\t" << label << "\n";

        if      (v.status == "pass") { ++passed; return; }
        else if (v.status == "fail") { ++failed;  std::cerr << "  FAIL "; }
        else if (v.status == "skip") { ++skipped; std::cerr << "  SKIP "; }
        else                         { ++invalid; std::cerr << "  INVALID "; }
        std::cerr << label;
        if (v.crossings > 0) { std::cerr << " (" << v.crossings << " crossings)"; }
        std::cerr << ": " << v.note << "\n";
    };

    if (files.empty())
    {
        std::stringstream ss;
        ss << std::cin.rdbuf();
        report("stdin", CheckText(ss.str()));
    }
    else
    {
        for (const std::string& f : files) { report(f, CheckFile(f)); }
    }

    std::cout << "\ninvariance: " << passed << " preserved, " << failed
              << " changed, " << skipped << " skipped, " << invalid << " invalid\n";
    // Exit 1 on a genuine HOMFLY change or an input that is not one; a skip
    // alone exits 0. Either way the code stays in {0,1}, so callers can tell
    // a finished run from a crash.
    return (failed == 0 && invalid == 0) ? 0 : 1;
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

    // Input handling for --invariance: anything that is not a PD code or a 3D
    // embedding is `invalid`, never `skip`. run_tests.py counts a skip as a
    // pass, so before this an unfetched Git LFS pointer (and an unopenable
    // path, and real rows followed by junk) passed as tested.
    {
        const std::string trefoil_pd = "0 4 1 3 1\n2 0 3 5 1\n4 2 5 1 1\n";
        const std::string lfs_pointer =
            "version https://git-lfs.github.com/spec/v1\n"
            "oid sha256:4d7a214614ab2935c943f9e0ff69d22eadbb8f32b1258daaa5e2ca24d17e2393\n"
            "size 42\n";
        // 12-vertex trefoil, (sin t + 2 sin 2t, cos t - 2 cos 2t, -sin 3t).
        const std::string trefoil_3d =
            "0.00 -1.00 -0.00\n2.23 -0.13 -1.00\n2.60 1.50 -0.00\n1.00 2.00 1.00\n"
            "-0.87 0.50 0.00\n-1.23 -1.87 -1.00\n-0.00 -3.00 -0.00\n1.23 -1.87 1.00\n"
            "0.87 0.50 0.00\n-1.00 2.00 -1.00\n-2.60 1.50 -0.00\n-2.23 -0.13 1.00\n";
        const std::string square_3d =
            "0.0 0.0 0.0\n1.0 0.0 0.0\n1.0 1.0 0.0\n0.0 1.0 0.0\n";

        struct Case { std::string what; Verdict got; std::string expected; };
        const std::vector<Case> cases = {
            { "Git LFS pointer",               CheckText(lfs_pointer),                "invalid" },
            { "empty input",                   CheckText(""),                         "invalid" },
            { "blank lines only",              CheckText("\n  \n\t\n"),               "invalid" },
            { "PD rows, then a stray word",    CheckText(trefoil_pd + "oops\n"),      "invalid" },
            { "PD code with 9 integers",       CheckText("0 4 1 3 1\n2 0 3 5\n"),     "invalid" },
            { "embedding, then a stray word",  CheckText(trefoil_3d + "oops\n"),      "invalid" },
            { "embedding with 5 numbers",      CheckText("1.0 2.0 3.0\n4.0 5.0\n"),   "invalid" },
            { "a path that does not exist",
              CheckFile("/nonexistent/homfly_check_selftest.tsv"),                    "invalid" },
            { "crossingless 3D embedding",     CheckText(square_3d),                  "skip"    },
            { "trefoil PD code (control)",     CheckText(trefoil_pd),                 "pass"    },
            { "trefoil 3D embedding (control)", CheckText(trefoil_3d),                "pass"    },
        };

        std::cout << "\n=== --invariance input handling (unreadable input is invalid) ===\n";
        for (const Case& c : cases)
        {
            const bool ok = (c.got.status == c.expected);
            std::cout << "  " << (ok ? "PASS" : "FAIL") << "  " << c.what
                      << ": " << c.got.status;
            if (!c.got.note.empty()) { std::cout << " (" << c.got.note << ")"; }
            std::cout << "\n";
            if (!ok)
            {
                std::cout << "    expected " << c.expected << "\n";
                ++failures;
            }
        }
    }

    return failures == 0 ? 0 : 1;
}
