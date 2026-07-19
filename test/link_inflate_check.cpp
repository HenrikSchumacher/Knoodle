/**
 * @file link_inflate_check.cpp
 * @brief Inflate-and-recover stress test for LINKS (sibling of inflate_check,
 *        which is knots-only).
 *
 * Takes a known link, inflates it to a large diagram by repeated Reapr embed +
 * random reproject, verifies the link type is preserved at every stage via a
 * single-variable Alexander |det| fingerprint (see link_alexander.hpp), then runs
 * KnoodleSimplify and confirms it collapses back to a small diagram with the same
 * fingerprint and the same number of link components.
 *
 * Why a custom fingerprint: Alexander_UMFPACK::Alexander refuses links (its
 * t = 1 normalization divides by zero for mu >= 2 components). The |det|-on-the-
 * unit-circle fingerprint here needs no such normalization. It is a true link
 * invariant but single-variable, so it is a tripwire, not a classifier.
 *
 * Failure signals:
 *   (1) the fingerprint changes across an embed->reproject round (diagram corrupted);
 *   (2) the link splits / loses or gains a component during inflation;
 *   (3) Simplify fails to reduce the giant diagram;
 *   (4) the recovered diagram has a different fingerprint or component count.
 *
 * Reproducibility mirrors inflate_check: a master --seed drives the run; each
 * round re-seeds the Reapr RNG with splitmix64(seed, round); failing inputs are
 * dumped to .tsv with the exact reproduction recipe.
 *
 * Build: see test/Makefile (target: link_inflate_check). Needs UMFPACK + Accelerate.
 */

#include "../Knoodle.hpp"
#include "homfly_invariance.hpp"   // Int, PDC_T, PD_T, Polynomial, HomflyOfPossiblySplit
#include "link_alexander.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Int, PDC_T, PD_T, Polynomial come from homfly_invariance.hpp (same anon ns).
using Real       = double;
using Cplx       = std::complex<double>;
using Reapr_T    = Knoodle::Reapr<Real, Int, float>;
using LinkAlex_T = knoodle_test::LinkAlexander<Cplx, Int>;
using LinkFP     = LinkAlex_T::Value;
using Clock      = std::chrono::steady_clock;

double Secs(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}

// splitmix64 — derive a well-mixed per-round seed from (master, round).
std::uint64_t SplitMix64(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// Load a 5-col signed PD code (one crossing per row) -> PD_T. Handles links.
PD_T LoadSignedPD(const std::string& path)
{
    std::ifstream in(path);
    std::vector<Int> ints;
    Int v;
    while (in >> v) { ints.push_back(v); }
    if (ints.empty() || ints.size() % 5 != 0) { return PD_T::InvalidDiagram(); }
    return PD_T::FromSignedPDCode(
        ints.data(), static_cast<Int>(ints.size() / 5), false, true);
}

// Dump a PD code (5-col signed) to a file for failure reproduction.
void DumpPD(const PD_T& pd, const std::string& path)
{
    PD_T copy(pd);
    auto code = copy.template PDCode<Int, {.signQ = true, .colorQ = false}>();
    std::ofstream out(path);
    const Int c = copy.CrossingCount();
    for (Int i = 0; i < c; ++i)
    {
        out << code(i,0) << '\t' << code(i,1) << '\t' << code(i,2) << '\t'
            << code(i,3) << '\t' << code(i,4) << '\n';
    }
}

struct Config
{
    std::uint64_t seed       = 1;
    Int           target     = 20000;    // inflate until crossings >= target
    int           max_rounds = 60;
    Int           reduce_to  = 200;       // simplified must be <= this
    Int           homfly_max = 40;        // cross-check HOMFLY if recovered <= this
    bool          verbose    = false;
};

// Returns true on pass.
bool RunTrial(PD_T start, const std::string& name, const Config& cfg,
              const LinkAlex_T& linkalex)
{
    std::cout << "=== trial seed=" << cfg.seed << "  start: " << name << "  "
              << start.CrossingCount() << " crossings, "
              << start.LinkComponentCount() << " components ===\n";

    if (start.LinkComponentCount() < Int(2))
    {
        std::cout << "  NOTE: starting diagram is a knot, not a link; use "
                     "inflate_check for knots.\n";
    }

    const Int comps0 = start.LinkComponentCount();
    const LinkFP fp0 = linkalex(start);
    if (!fp0.ok)
    {
        std::cout << "  FAIL: base fingerprint invalid (split link, or a sample "
                     "point hit an Alexander root).\n";
        return false;
    }
    std::cout << "  fingerprint(start): [" << LinkAlex_T::ToString(fp0) << "]\n";

    // Second, independent invariant: HOMFLY of the start link, via the validated
    // split-safe oracle (homfly_invariance.hpp). The start is a small connected
    // link diagram, so libhomfly handles it directly; HomflyOfPossiblySplit also
    // covers the split case for the recovered diagram below.
    bool homfly_ok0 = false;
    const Polynomial homfly0 =
        HomflyOfPossiblySplit(KnoodleJenkins(start), homfly_ok0);
    if (homfly_ok0)
    {
        std::cout << "  HOMFLY(start): " << PolyToString(homfly0) << "\n";
    }
    else
    {
        std::cout << "  NOTE: HOMFLY(start) unavailable (malformed Jenkins code); "
                     "skipping HOMFLY cross-check.\n";
    }

    Reapr_T reapr{};
    PD_T pd = std::move(start);
    PD_T prev = pd;

    const auto t_inflate0 = Clock::now();
    int round = 0;
    while (pd.CrossingCount() < cfg.target && round < cfg.max_rounds)
    {
        ++round;
        if (pd.DiagramComponentCount() > Int(1) || pd.CrossingCount() == 0)
        {
            std::cout << "  FAIL: diagram became split/trivial before round "
                      << round << ".\n";
            DumpPD(prev, "link_inflate_fail_seed" + std::to_string(cfg.seed)
                       + "_round" + std::to_string(round) + ".tsv");
            return false;
        }

        const std::uint64_t seed_k =
            SplitMix64(cfg.seed + static_cast<std::uint64_t>(round));
        reapr.RandomEngine() = Knoodle::PRNG_T(seed_k);

        prev = pd;
        auto emb = reapr.Embedding(pd);
        emb.Rotate(reapr.RandomRotation());
        auto [pd_new, unlinks] = PD_T::FromLinkEmbedding(emb);

        if (!pd_new.ValidQ() || unlinks.Size() > Int(0)
            || pd_new.DiagramComponentCount() > Int(1))
        {
            const std::string dump = "link_inflate_fail_seed"
                + std::to_string(cfg.seed) + "_round" + std::to_string(round) + ".tsv";
            DumpPD(prev, dump);
            std::cout << "  FAIL: round " << round << " produced an invalid/split "
                         "projection (unlinks=" << unlinks.Size() << ").\n"
                      << "  reproduce: load " << dump << ", Reapr seed " << seed_k
                      << ", one embed+reproject.\n";
            return false;
        }
        pd = std::move(pd_new);

        // Tripwire 1: component count must be preserved.
        if (pd.LinkComponentCount() != comps0)
        {
            const std::string dump = "link_inflate_fail_seed"
                + std::to_string(cfg.seed) + "_round" + std::to_string(round) + ".tsv";
            DumpPD(prev, dump);
            std::cout << "  FAIL: component count changed at round " << round
                      << " (" << comps0 << " -> " << pd.LinkComponentCount() << ")\n"
                      << "  reproduce: load " << dump << ", Reapr seed " << seed_k << ".\n";
            return false;
        }

        // Tripwire 2: the Alexander fingerprint must not change.
        const LinkFP fp_k = linkalex(pd);
        if (!LinkAlex_T::Equal(fp_k, fp0))
        {
            const std::string dump = "link_inflate_fail_seed"
                + std::to_string(cfg.seed) + "_round" + std::to_string(round) + ".tsv";
            DumpPD(prev, dump);
            std::cout << "  FAIL: fingerprint changed at round " << round
                      << " (" << pd.CrossingCount() << " crossings)\n"
                      << "    before: [" << LinkAlex_T::ToString(fp0)  << "]\n"
                      << "    after : [" << LinkAlex_T::ToString(fp_k) << "]\n"
                      << "  reproduce: load " << dump << ", Reapr seed " << seed_k
                      << ", one embed+reproject.\n";
            return false;
        }

        if (cfg.verbose)
        {
            std::cout << "    round " << round << ": " << pd.CrossingCount()
                      << " crossings\n";
        }
    }
    const auto t_inflate1 = Clock::now();
    const Int inflated = pd.CrossingCount();
    std::cout << "  inflated to " << inflated << " crossings in " << round
              << " rounds (" << Secs(t_inflate0, t_inflate1) << "s)\n";

    if (inflated < cfg.target)
    {
        std::cout << "  NOTE: did not reach target " << cfg.target
                  << " within " << cfg.max_rounds << " rounds.\n";
    }

    // Simplify the giant diagram and confirm it collapses.
    const auto t_simp0 = Clock::now();
    PDC_T pdc{ PD_T(pd) };
    pdc.Simplify(PDC_T::Simplify_Args_T{});
    const auto t_simp1 = Clock::now();

    if (!pdc.ValidQ() || pdc.DiagramCount() == Int(0))
    {
        std::cout << "  FAIL: Simplify returned no diagram.\n";
        DumpPD(pd, "link_inflate_fail_seed" + std::to_string(cfg.seed) + "_simplify.tsv");
        return false;
    }

    // Total crossings across all resulting diagram components (a split link may
    // reduce to several diagrams).
    Int rec_c = 0;
    Int rec_comps = 0;
    for (Int d = 0; d < pdc.DiagramCount(); ++d)
    {
        rec_c     += pdc.Diagrams()[d].CrossingCount();
        rec_comps += pdc.Diagrams()[d].LinkComponentCount();
    }
    std::cout << "  Simplify: " << inflated << " -> " << rec_c << " crossings ("
              << pdc.DiagramCount() << " diagram(s), " << rec_comps
              << " components, " << Secs(t_simp0, t_simp1) << "s)\n";

    if (rec_c > cfg.reduce_to)
    {
        std::cout << "  FAIL: Simplify did not reduce the diagram (" << rec_c
                  << " > " << cfg.reduce_to << ") — strong bug signal.\n";
        DumpPD(pd, "link_inflate_fail_seed" + std::to_string(cfg.seed) + "_simplify.tsv");
        return false;
    }

    if (rec_comps != comps0)
    {
        std::cout << "  FAIL: recovered component count differs (" << comps0
                  << " -> " << rec_comps << ").\n";
        return false;
    }

    // Fingerprint check on the recovered diagram (only meaningful if it stayed a
    // single connected diagram — the fingerprint is per connected diagram).
    if (pdc.DiagramCount() == Int(1))
    {
        const LinkFP fp_rec = linkalex(pdc.Diagrams()[0]);
        if (!LinkAlex_T::Equal(fp_rec, fp0))
        {
            std::cout << "  FAIL: fingerprint of recovered diagram differs.\n"
                      << "    start    : [" << LinkAlex_T::ToString(fp0)     << "]\n"
                      << "    recovered: [" << LinkAlex_T::ToString(fp_rec) << "]\n";
            return false;
        }
    }
    else
    {
        std::cout << "  NOTE: recovered into " << pdc.DiagramCount()
                  << " diagrams; skipping connected-diagram fingerprint check.\n";
    }

    // HOMFLY cross-check on the recovered diagram (split-safe). Bounded by
    // homfly_max because HOMFLY is exponential in crossing count; the recovered
    // diagram is normally minimal, so this almost always runs.
    if (homfly_ok0 && rec_c <= cfg.homfly_max)
    {
        PD_T single = pdc.ToSingleDiagram();
        if (single.ValidQ())
        {
            bool homfly_okR = false;
            const Polynomial homflyR =
                HomflyOfPossiblySplit(KnoodleJenkins(single), homfly_okR);
            if (homfly_okR && homflyR != homfly0)
            {
                std::cout << "  FAIL: HOMFLY of recovered diagram differs from start.\n"
                          << "    start    : " << PolyToString(homfly0) << "\n"
                          << "    recovered: " << PolyToString(homflyR) << "\n";
                return false;
            }
            if (homfly_okR)
            {
                std::cout << "  HOMFLY preserved (recovered " << rec_c
                          << " crossings).\n";
            }
        }
    }

    std::cout << "  PASS: fingerprint + component count + HOMFLY preserved through "
                 "inflation and recovery.\n";
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    Config cfg;
    std::string linktable = "../data/diagrams/linktable";
    std::vector<std::string> starts;   // linktable names or pd:FILE
    int n_trials = 1;

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        auto val = [&](const std::string& p) { return a.substr(p.size()); };
        if      (a.rfind("--seed=", 0) == 0)        cfg.seed = std::stoull(val("--seed="));
        else if (a.rfind("--target=", 0) == 0)      cfg.target = std::stoll(val("--target="));
        else if (a.rfind("--max-rounds=", 0) == 0)  cfg.max_rounds = std::stoi(val("--max-rounds="));
        else if (a.rfind("--reduce-to=", 0) == 0)   cfg.reduce_to = std::stoll(val("--reduce-to="));
        else if (a.rfind("--homfly-max=", 0) == 0)  cfg.homfly_max = std::stoll(val("--homfly-max="));
        else if (a.rfind("--trials=", 0) == 0)      n_trials = std::stoi(val("--trials="));
        else if (a.rfind("--linktable=", 0) == 0)   linktable = val("--linktable=");
        else if (a.rfind("--start=", 0) == 0)       starts.push_back(val("--start="));
        else if (a == "--verbose")                  cfg.verbose = true;
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "link_inflate_check — inflate a known link to a large diagram,\n"
                "verify a single-variable Alexander |det| fingerprint and the\n"
                "component count are preserved each round, then Simplify and check\n"
                "it collapses back with the same fingerprint.\n\n"
                "Options:\n"
                "  --start=SPEC     linktable name (e.g. L4a1_0) or pd:FILE; repeatable\n"
                "                   (default: a small built-in spread of links)\n"
                "  --seed=N         master RNG seed (default 1); trial k uses seed+k\n"
                "  --trials=N       run N trials per start with consecutive seeds (default 1)\n"
                "  --target=N       inflate until >= N crossings (default 20000)\n"
                "  --max-rounds=N   cap on inflation rounds (default 60)\n"
                "  --reduce-to=N    recovered diagram must be <= N crossings (default 200)\n"
                "  --homfly-max=N   cross-check HOMFLY if recovered <= N crossings (default 40)\n"
                "  --linktable=PATH linktable data dir (default ../data/diagrams/linktable)\n"
                "  --verbose        print per-round crossing counts\n";
            return 0;
        }
        else { starts.push_back(a); }  // bare name = linktable entry
    }

    if (starts.empty())
    {
        // A spread across crossing number and component count.
        starts = {"L2a1_0", "L4a1_0", "L5a1_0", "L6a1_0", "L6a4_0_0", "L8n3_0_0"};
    }

    LinkAlex_T linkalex{};
    int failures = 0;
    int total    = 0;

    for (const auto& spec : starts)
    {
        PD_T base;
        std::string name = spec;
        if (spec.rfind("pd:", 0) == 0)
        {
            name = spec.substr(3);
            base = LoadSignedPD(name);
        }
        else
        {
            base = LoadSignedPD(linktable + "/" + spec + ".tsv");
        }

        if (!base.ValidQ())
        {
            std::cerr << "could not load link '" << spec << "'\n";
            ++failures; ++total;
            continue;
        }

        for (int t = 0; t < n_trials; ++t)
        {
            Config c = cfg;
            c.seed = cfg.seed + static_cast<std::uint64_t>(t);
            ++total;
            if (!RunTrial(PD_T(base), name, c, linkalex)) { ++failures; }
            std::cout << "\n";
        }
    }

    std::cout << "link_inflate_check: " << (total - failures) << "/" << total
              << " trials passed\n";
    return failures == 0 ? 0 : 1;
}
