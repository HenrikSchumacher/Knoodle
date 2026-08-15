// local_moves_check -- the ArcSimplifier's local patterns must preserve the knot.
//
// A simplifier is allowed to fail to simplify. It is not allowed to return a
// different knot. This test holds the local-move tiers to that, on a corpus
// where the answer is known for every element: data/diagrams/hardunknots, 21
// diagrams that are all the UNKNOT and all chosen to be hard to recognize as
// such. Any output with a nontrivial HOMFLY is a soundness failure, full stop.
//
// It exists because of upstream issue 12, found 2026-08-14. `local_opt_level=4`
// enables the assisted R_IIa patterns, which call `SwitchCrossing`. A
// PlanarDiagram is LOCKED BY DEFAULT, a locked diagram refuses that call, and
// `ArcSimplifier/Helpers.hpp` discards the refusal:
//
//     void SwitchCrossing( const Int c_ ) { (void) pd.SwitchCrossing(c_); }
//
// The surrounding R_IIa move does four Reconnects and two DeactivateCrossings,
// none of which are lock-guarded, so the move HALF-APPLIES. R_IIa preserves the
// knot only as a whole; half of it is a crossing change. `Monster` is the
// element of this corpus that trips it: a 10-crossing unknot becomes a
// 6-crossing knot, silently.
//
// Levels 0 and 2 are checked as controls, so a failure here says which tier
// broke rather than just "something did".
//
// KNOWN FAILURES are listed below and reported as XFAIL. As elsewhere in this
// suite, a known failure that starts passing is an XPASS and fails the run, so
// the entry gets removed rather than quietly masking a later regression.
//
// Build: `make local_moves_check` in test/.

#include "homfly_invariance.hpp"

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/// HOMFLY is only trustworthy on a diagram small enough for libhomfly to
/// finish; above this we cannot tell a wrong answer from an unusable one, and
/// saying so is better than guessing. (Reading a knot type off a large diagram
/// is what cost a session's worth of wrong conclusions on 2026-08-14.)
constexpr Int kVerifiableCap = 40;

/// The tiers to exercise. 4 is the one that enables assisted R_Ia/R_IIa; the
/// others are controls.
constexpr int kLevels[] = { 0, 2, 4 };

/// The HOMFLY of the unknot is 1: a single term at L^0 M^0 with coefficient 1.
static bool IsUnknotPolynomial( const Polynomial & p )
{
    Int nonzero = 0;
    for( const auto & [key,coeff] : p )
    {
        if( coeff == 0 ) { continue; }
        ++nonzero;
        if( key != std::pair<int,int>{0,0} || coeff != 1 ) { return false; }
    }
    return nonzero == 1;
}

/// (fixture, level) pairs that are known to be wrong. Remove an entry when it
/// is fixed -- the test will tell you, by failing with XPASS.
static bool KnownFailure( const std::string & name, int level )
{
    return (name == "Monster") && (level == 4);   // upstream issue 12
}

static bool ReadPDCode( const fs::path & p, std::vector<Int> & code, Int & rows )
{
    code.clear(); rows = 0;
    std::ifstream in(p);
    if( !in ) { return false; }

    std::string line;
    while( std::getline(in,line) )
    {
        if( line.find_first_not_of(" \t\r") == std::string::npos ) { continue; }
        std::istringstream s(line);
        long long v; Int k = 0;
        while( s >> v ) { code.push_back(Int(v)); ++k; }
        if( k != Int(5) ) { return false; }        // 5-column signed PD code
        ++rows;
    }
    return rows > Int(0);
}

int main( int argc, char ** argv )
{
    const fs::path dir = (argc > 1) ? argv[1] : "../data/diagrams/hardunknots";

    std::vector<fs::path> files;
    if( fs::is_directory(dir) )
    {
        for( const auto & e : fs::directory_iterator(dir) )
        { if( e.path().extension() == ".tsv" ) { files.push_back(e.path()); } }
    }
    std::sort(files.begin(),files.end());

    if( files.empty() )
    {
        std::printf("error: no .tsv fixtures in '%s'\n", dir.string().c_str());
        return 2;
    }

    std::printf("local_moves_check: %zu known unknots from %s\n\n",
                files.size(), dir.string().c_str());

    Int passed = 0, failed = 0, xfailed = 0, skipped = 0;
    std::vector<std::string> failures;

    for( const auto & f : files )
    {
        const std::string name = f.stem().string();

        std::vector<Int> code; Int rows = 0;
        if( !ReadPDCode(f,code,rows) )
        {
            std::printf("  SKIP  %-16s (not a 5-column PD code)\n", name.c_str());
            ++skipped;
            continue;
        }

        const PD_T input = PD_T::FromSignedPDCode(code.data(), rows, false, false);

        for( int level : kLevels )
        {
            PD_T  copy(input);
            PDC_T pdc(std::move(copy));

            auto args = PDC_T::Simplify_Args_T{};
            args.local_opt_level = static_cast<Knoodle::UInt8>(level);
            pdc.Simplify(args);

            const PD_T out = pdc.ToSingleDiagram();

            // Simplified entirely away, or down to a farfalle: that is the
            // unknot and needs no oracle.
            const bool goneQ = !out.ValidQ() || (out.CrossingCount() <= Int(1));

            bool unknotQ = true;
            bool checkedQ = true;

            if( !goneQ )
            {
                if( out.CrossingCount() > kVerifiableCap )
                {
                    checkedQ = false;             // too big to trust; not a pass
                }
                else
                {
                    bool ok = false;
                    const Polynomial h =
                        HomflyOfPossiblySplit(out.ToJenkinsCodeString(), ok);
                    if( !ok ) { checkedQ = false; }
                    else      { unknotQ = IsUnknotPolynomial(h); }
                }
            }

            const std::string what =
                name + " local_opt_level=" + std::to_string(level);
            const bool knownQ = KnownFailure(name,level);

            if( !checkedQ )
            {
                std::printf("  SKIP  %-34s output has %lld crossings, above the "
                            "verifiable cap\n", what.c_str(),
                            (long long)out.CrossingCount());
                ++skipped;
                continue;
            }

            if( unknotQ && !knownQ ) { ++passed; continue; }

            if( unknotQ && knownQ )
            {
                std::printf("  XPASS %-34s marked as a known failure but it "
                            "passed; remove the entry\n", what.c_str());
                failures.push_back(what + ": XPASS -- remove the known-failure entry");
                ++failed;
                continue;
            }

            if( knownQ )
            {
                std::printf("  XFAIL %-34s a known unknot came back with %lld "
                            "crossings and a nontrivial HOMFLY\n"
                            "        known: upstream issue 12 (R_IIa half-applies "
                            "on a locked diagram)\n",
                            what.c_str(), (long long)out.CrossingCount());
                ++xfailed;
                continue;
            }

            std::printf("  FAIL  %-34s a known unknot came back with %lld "
                        "crossings and a nontrivial HOMFLY\n",
                        what.c_str(), (long long)out.CrossingCount());
            failures.push_back(what + ": simplification changed the knot type");
            ++failed;
        }
    }

    std::printf("\n%s\n", std::string(70,'-').c_str());
    std::printf("passed %lld, failed %lld, skipped %lld, known-failing %lld\n",
                (long long)passed, (long long)failed,
                (long long)skipped, (long long)xfailed);

    if( !failures.empty() )
    {
        std::printf("\nfailures:\n");
        for( const auto & s : failures ) { std::printf("  - %s\n", s.c_str()); }
    }
    return failures.empty() ? 0 : 1;
}
