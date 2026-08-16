// polygon_levels_check -- every simplification tier must return the same knot.
//
// A simplifier may fail to simplify. It may not return a different knot. This
// holds every tier of PlanarDiagramComplex::Simplify to that, on a corpus built
// so the tiers actually fire.
//
// WHY A NEW CORPUS. local_moves_check runs the same contract over
// data/diagrams/hardunknots and reports that the local tiers change the answer
// on 1 of 581 diagrams. That is the corpus being made of the wrong thing: hard
// unknots are *constructed* to resist local moves. GitHub #33 -- R_IIa
// half-applying on a locked diagram and silently returning a different knot --
// survived in the tree because of that gap. Random equilateral polygons of a
// few hundred edges are full of easy local structure, so the tiers engage.
//
// IN-PROCESS ONLY, no knoodlesimplify. This is a test of the LIBRARY. The CLI
// is a separate contract with its own suite; shelling out would make a failure
// ambiguous between "the library computed the wrong answer" and "the tool
// passed the wrong arguments".
//
// THE ORACLE IS AGREEMENT, not a stored answer. Every configuration below is a
// simplification of the same knot, so they must all agree with each other. That
// needs no knowledge of what the knot IS -- which matters, because the raw
// projections here run to 370 crossings and nothing can name those. HOMFLY is
// used when the output is small enough to trust; the single-variable Alexander
// |det| fingerprint works at any size and covers the rest.
//
// Build: `make polygon_levels_check` in test/.

#include "homfly_invariance.hpp"
#include "link_alexander.hpp"

#include <algorithm>
#include <complex>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using Real       = double;   // homfly_invariance.hpp defines Int/PD_T/PDC_T/Polynomial
using Cplx       = std::complex<double>;
using Link_T     = Knoodle::LinkEmbedding<Real, Int, float>;
using LinkAlex_T = knoodle_test::LinkAlexander<Cplx, Int>;
using LinkFP     = LinkAlex_T::Value;

/// Above this, libhomfly cannot be trusted to finish or to be right, and a
/// wrong answer is indistinguishable from an unusable one. Reading a knot type
/// off a large diagram is what cost a session on 2026-08-14.
constexpr Int kVerifiableCap = 40;

/// The tiers, mirroring knoodlesimplify's --simplify-level ladder
/// (tools/knoodlesimplify.cpp BuildSimplifyArgs), plus two the CLI cannot
/// select.
///
/// Note levels 4-6 deliberately keep local_opt_level = 0: Henrik's measurements
/// found the local pass does not pay once rerouting is engaged, so the 3->4
/// boundary is NOT a superset. Local and reroute are orthogonal regimes, which
/// is exactly why the crossed rows at the bottom are worth running -- GitHub #33
/// lived in that corner, at local_opt_level = 4 on a locked diagram.
struct Config
{
    const char * name;
    int          local_opt_level;
    bool         rerouteQ;
    bool         disconnectQ;
};

static const Config kConfigs[] = {
    { "s0 local=0",            0, false, false },
    { "s1 local=1 (R_I)",      1, false, false },
    { "s2 local=2 (R_I+R_II)", 2, false, false },
    { "s3 local=4 (all local)",4, false, false },
    { "s4 reroute",            0, true,  false },
    { "s5 reroute+disconnect", 0, true,  true  },
    { "x  local=2 + reroute",  2, true,  false },
    { "x  local=4 + reroute",  4, true,  false },
};

static constexpr std::size_t kConfigCount = sizeof(kConfigs)/sizeof(kConfigs[0]);

/// What one configuration produced. Both oracles are optional: HOMFLY needs a
/// small diagram, the Alexander fingerprint needs a connected one.
struct Outcome
{
    Int        crossings   = -1;
    bool       homflyQ     = false;
    Polynomial homfly;
    bool       alexQ       = false;
    LinkFP     alex;
};

static Outcome Evaluate( const PD_T & pd, const LinkAlex_T & alexander )
{
    Outcome o;
    o.crossings = pd.ValidQ() ? pd.CrossingCount() : Int(0);

    if( o.crossings <= kVerifiableCap )
    {
        bool ok = false;
        // A diagram simplified out of existence is the unknot; libhomfly is
        // not asked about a diagram with no crossings.
        if( !pd.ValidQ() || o.crossings == Int(0) )
        {
            o.homfly  = Polynomial{ {{0,0},1} };
            o.homflyQ = true;
        }
        else
        {
            PD_T copy(pd);
            Polynomial h = HomflyOfPossiblySplit(copy.ToJenkinsCodeString(), ok);
            if( ok ) { o.homfly = std::move(h); o.homflyQ = true; }
        }
    }

    if( pd.ValidQ() && o.crossings > Int(0) )
    {
        LinkFP fp = alexander(pd);
        if( fp.ok ) { o.alex = std::move(fp); o.alexQ = true; }
    }
    else
    {
        // The unknot's |det| is 1 at every sample point, i.e. log10 = 0 --
        // which is exactly what LinkAlexander returns for a trivial diagram.
        o.alex.logdet.assign(5, 0.0);
        o.alex.ok = true;
        o.alexQ   = true;
    }

    return o;
}

/// Stored references, one row per polygon: name, raw crossing count, and the
/// HOMFLY of the simplified diagram.
///
/// Agreement between configurations is the primary oracle and needs no stored
/// answer. But agreement is blind to UNIFORM drift: if a change made every tier
/// return the same wrong knot, they would still agree. These pin the answer
/// across time as well as across configurations.
struct Reference { Int crossings; std::string homfly; };

static std::map<std::string,Reference> LoadReferences( const fs::path & p, bool & foundQ )
{
    std::map<std::string,Reference> refs;
    foundQ = false;
    std::ifstream in(p);
    if( !in ) { return refs; }
    foundQ = true;
    std::string line;
    while( std::getline(in,line) )
    {
        if( line.empty() || line[0] == '#' ) { continue; }
        const auto t1 = line.find('\t');
        if( t1 == std::string::npos ) { continue; }
        const auto t2 = line.find('\t', t1+1);
        if( t2 == std::string::npos ) { continue; }
        refs[line.substr(0,t1)] = Reference{
            Int(std::stoll(line.substr(t1+1, t2-t1-1))), line.substr(t2+1) };
    }
    return refs;
}

int main( int argc, char ** argv )
{
    fs::path dir = "polygons";
    bool updateQ = false;
    for( int i = 1; i < argc; ++i )
    {
        std::string a(argv[i]);
        if      ( a.rfind("--dir=",0) == 0 )  { dir = a.substr(6); }
        else if ( a == "--update-expected" )  { updateQ = true; }
    }

    std::vector<fs::path> files;
    if( fs::is_directory(dir) )
    {
        for( const auto & e : fs::directory_iterator(dir) )
        { if( e.path().extension() == ".crd" ) { files.push_back(e.path()); } }
    }
    std::sort(files.begin(), files.end());

    if( files.empty() )
    {
        std::printf("error: no .crd fixtures in '%s'\n", dir.string().c_str());
        std::printf("       run `make polygons && ./polygons/make_polygons polygons`\n");
        return 2;
    }

    const fs::path ref_path = dir / "expected.tsv";
    bool have_refs = false;
    const auto refs = LoadReferences(ref_path, have_refs);
    if( !have_refs && !updateQ )
    {
        std::printf("  NOTE: no %s -- configurations are still compared against\n"
                    "        each other, but the answer is not pinned across time.\n"
                    "        Regenerate with --update-expected.\n\n",
                    ref_path.string().c_str());
    }
    std::vector<std::string> new_refs;

    const LinkAlex_T alexander{};

    std::printf("polygon_levels_check: %zu polygons x %zu configurations\n\n",
                files.size(), kConfigCount);

    Int    passed = 0, failed = 0;
    Int    unchecked = 0;
    // COVERAGE, not correctness: how often the tiers actually diverge. If they
    // never do, a green run says nothing -- which is precisely the state
    // local_moves_check reports on the hard-unknot corpus (1 of 581).
    Int    exercised = 0, considered = 0;
    std::vector<std::string> failures;

    for( const auto & f : files )
    {
        const std::string name = f.stem().string();

        Link_T L = Link_T::FromFile(f);
        if( !L.ValidQ() )
        {
            std::printf("  SKIP  %-16s could not read coordinates\n", name.c_str());
            ++unchecked;
            continue;
        }

        auto [raw, unlinks] = PD_T::FromLinkEmbedding(L);
        (void)unlinks;
        if( !raw.ValidQ() )
        {
            std::printf("  SKIP  %-16s projection declined it\n", name.c_str());
            ++unchecked;
            continue;
        }

        std::vector<Outcome> outs;
        outs.reserve(kConfigCount);

        for( const Config & c : kConfigs )
        {
            PD_T  copy(raw);
            PDC_T pdc(std::move(copy));

            auto args = PDC_T::Simplify_Args_T{};
            args.splitQ          = true;
            args.local_opt_level = static_cast<Knoodle::UInt8>(c.local_opt_level);
            args.rerouteQ        = c.rerouteQ;
            args.disconnectQ     = c.disconnectQ;
            pdc.Simplify(args);

            outs.push_back(Evaluate(pdc.ToSingleDiagram(), alexander));
        }

        // Did the tiers actually do different things here?
        bool divergedQ = false;
        for( std::size_t i = 1; i < outs.size(); ++i )
        { if( outs[i].crossings != outs[0].crossings ) { divergedQ = true; } }
        ++considered;
        if( divergedQ ) { ++exercised; }

        // Every configuration simplified the same knot, so every oracle that
        // applies must agree with every other. Compare each against the first
        // configuration that produced a usable value of that kind.
        const Outcome * ref_h = nullptr;
        const Outcome * ref_a = nullptr;
        std::size_t     ref_hi = 0, ref_ai = 0;
        for( std::size_t i = 0; i < outs.size(); ++i )
        {
            if( !ref_h && outs[i].homflyQ ) { ref_h = &outs[i]; ref_hi = i; }
            if( !ref_a && outs[i].alexQ   ) { ref_a = &outs[i]; ref_ai = i; }
        }

        std::string cx;
        for( const auto & o : outs ) { cx += " " + std::to_string(o.crossings); }

        bool okQ = true;
        for( std::size_t i = 0; i < outs.size(); ++i )
        {
            const Outcome & o = outs[i];

            if( ref_h && o.homflyQ && !(o.homfly == ref_h->homfly) )
            {
                std::printf("  FAIL  %-16s %s: HOMFLY differs from %s\n"
                            "          %s : %s\n          %s : %s\n",
                            name.c_str(), kConfigs[i].name, kConfigs[ref_hi].name,
                            kConfigs[ref_hi].name, PolyToString(ref_h->homfly).c_str(),
                            kConfigs[i].name,      PolyToString(o.homfly).c_str());
                failures.push_back(name + " " + kConfigs[i].name
                                   + ": HOMFLY differs from " + kConfigs[ref_hi].name);
                okQ = false;
            }

            if( ref_a && o.alexQ && !LinkAlex_T::Equal(o.alex, ref_a->alex) )
            {
                std::printf("  FAIL  %-16s %s: Alexander |det| differs from %s\n"
                            "          %s : [%s]\n          %s : [%s]\n",
                            name.c_str(), kConfigs[i].name, kConfigs[ref_ai].name,
                            kConfigs[ref_ai].name, LinkAlex_T::ToString(ref_a->alex).c_str(),
                            kConfigs[i].name,      LinkAlex_T::ToString(o.alex).c_str());
                failures.push_back(name + " " + kConfigs[i].name
                                   + ": Alexander differs from " + kConfigs[ref_ai].name);
                okQ = false;
            }

            if( !o.homflyQ && !o.alexQ )
            {
                std::printf("  NOTE  %-16s %s: no oracle applies (%lld crossings)\n",
                            name.c_str(), kConfigs[i].name, (long long)o.crossings);
                ++unchecked;
            }
        }

        // Cross-time check against the stored reference, and the raw crossing
        // count with it -- if the projection of a fixed .crd ever changes, the
        // fixture stopped being the diagram it was recorded as.
        const Outcome * best = ref_h ? ref_h : nullptr;
        if( best )
        {
            const std::string h = PolyToString(best->homfly);
            if( updateQ )
            {
                new_refs.push_back(name + "\t" + std::to_string(outs[0].crossings)
                                   + "\t" + h);
            }
            else
            {
                auto it = refs.find(name);
                if( it == refs.end() )
                {
                    if( have_refs )
                    {
                        std::printf("  NOTE  %-16s no stored reference\n", name.c_str());
                        ++unchecked;
                    }
                }
                else
                {
                    if( it->second.crossings != outs[0].crossings )
                    {
                        std::printf("  FAIL  %-16s raw projection changed: recorded "
                                    "%lld crossings, got %lld\n", name.c_str(),
                                    (long long)it->second.crossings,
                                    (long long)outs[0].crossings);
                        failures.push_back(name + ": raw crossing count changed");
                        okQ = false;
                    }
                    if( it->second.homfly != h )
                    {
                        std::printf("  FAIL  %-16s HOMFLY differs from the stored "
                                    "reference\n          stored : %s\n          now    : %s\n",
                                    name.c_str(), it->second.homfly.c_str(), h.c_str());
                        failures.push_back(name + ": HOMFLY differs from stored reference");
                        okQ = false;
                    }
                }
            }
        }

        if( okQ ) { ++passed; } else { ++failed; }
        std::printf("  %-5s %-16s crossings:%s\n", okQ ? "ok" : "FAIL",
                    name.c_str(), cx.c_str());
    }

    std::printf("\n%s\n", std::string(70,'-').c_str());
    std::printf("the tiers produced different diagrams on %lld of %lld polygons"
                " (coverage, not correctness)\n",
                (long long)exercised, (long long)considered);
    if( considered > 0 && exercised * Int(2) < considered )
    {
        std::printf("  NOTE: the tiers rarely diverge on this corpus, so a green"
                    " run here says less than it looks.\n");
    }
    std::printf("%lld polygons agreed across all %zu configurations, %lld disagreed"
                ", %lld unchecked\n",
                (long long)passed, kConfigCount, (long long)failed,
                (long long)unchecked);

    if( updateQ )
    {
        std::ofstream out(ref_path);
        out << "# Reference answers for polygon_levels_check, regenerated with\n"
            << "# ./polygon_levels_check --update-expected\n"
            << "#\n"
            << "# Agreement between configurations is the primary oracle; these pin\n"
            << "# the answer across TIME as well, so that a change making every tier\n"
            << "# return the same wrong knot is still caught.\n"
            << "#\n"
            << "# name\traw-crossings\thomfly\n";
        for( const auto & r : new_refs ) { out << r << "\n"; }
        std::printf("\nwrote %zu references to %s\n",
                    new_refs.size(), ref_path.string().c_str());
    }

    if( !failures.empty() )
    {
        std::printf("\nfailures:\n");
        for( const auto & s : failures ) { std::printf("  - %s\n", s.c_str()); }
    }
    return failures.empty() ? 0 : 1;
}
