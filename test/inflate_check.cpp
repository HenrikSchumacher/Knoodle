/**
 * @file inflate_check.cpp
 * @brief Randomized stress test: inflate a known knot to ~100k crossings by
 *        repeated Reapr-embed + random-reproject, verify the knot type is
 *        preserved at every (huge) stage via the fast Alexander value, then
 *        run KnoodleSimplify and confirm it collapses back to a small diagram
 *        with the same Alexander and HOMFLY invariants.
 *
 * The Alexander value (determinant-style, via sparse UMFPACK) is fast even at
 * 100k crossings, so it serves as a tripwire: if any embed→reproject round
 * changes it, a bug corrupted the diagram. KnoodleSimplify failing to reduce
 * the giant diagram, or the recovered diagram having a different HOMFLY, are
 * the other two failure signals.
 *
 * Reproducibility: the run is driven by a master --seed; each round re-seeds the
 * Reapr RNG with splitmix64(seed, round), so any failing round is reproducible
 * from (its input diagram, that derived seed). On failure the input diagram is
 * dumped to a .tsv and the exact reproduction recipe is printed.
 *
 * Alexander is knots-only (no multivariable Alexander in Knoodle), so this test
 * operates on single-component knots throughout.
 *
 * Build: see test/Makefile (target: inflate_check). Needs UMFPACK + Accelerate.
 */

#include "../Knoodle.hpp"   // KNOODLE_USE_UMFPACK supplied by the Makefile (-D)

extern "C" {
#include "vendor/libhomfly/homfly.h"
}
extern "C" void knoodle_gc_free_all(void);

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using Int     = std::int64_t;
using Real    = double;
using Cplx    = std::complex<double>;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, float>;
using Alex_T  = Knoodle::Alexander_UMFPACK<Cplx, Int>;
using Clock   = std::chrono::steady_clock;

namespace {

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

//==============================================================================
// Alexander fingerprint (the tripwire)
//==============================================================================

// Evaluate the Alexander polynomial at several fixed arguments. These values
// are knot invariants (independent of the diagram), so two diagrams of the same
// knot agree. Avoid arguments that are roots of the Alexander polynomial.
const std::vector<Cplx> kAlexArgs = {
    Cplx(-1.0), Cplx(2.0), Cplx(3.0), Cplx(5.0), Cplx(-2.0)
};

struct AlexFingerprint
{
    std::vector<Cplx>         mant;
    std::vector<std::int64_t> exp;
};

AlexFingerprint Alexander(const Alex_T& alex, const PD_T& pd)
{
    AlexFingerprint f;
    f.mant.resize(kAlexArgs.size());
    f.exp.resize(kAlexArgs.size());
    alex.Alexander(pd, kAlexArgs.data(),
                   static_cast<std::int64_t>(kAlexArgs.size()),
                   f.mant.data(), f.exp.data(), false);
    return f;
}

// Compare with a relative tolerance — the value carries ~1e-5 float noise at
// 100k crossings, while distinct knots differ by O(1), so 1e-3 is safe.
bool AlexEqual(const AlexFingerprint& a, const AlexFingerprint& b)
{
    constexpr double tol = 1e-3;
    if (a.mant.size() != b.mant.size()) { return false; }
    for (std::size_t i = 0; i < a.mant.size(); ++i)
    {
        // Fold the base-10 exponent into the mantissa for comparison.
        Cplx va = a.mant[i] * std::pow(10.0, static_cast<double>(a.exp[i]));
        Cplx vb = b.mant[i] * std::pow(10.0, static_cast<double>(b.exp[i]));
        const double scale = std::abs(va) + std::abs(vb) + 1.0;
        if (std::abs(va - vb) > tol * scale) { return false; }
    }
    return true;
}

std::string AlexToString(const AlexFingerprint& f)
{
    std::string s;
    for (std::size_t i = 0; i < f.mant.size(); ++i)
    {
        Cplx v = f.mant[i] * std::pow(10.0, static_cast<double>(f.exp[i]));
        s += (i ? ", " : "") + std::to_string(v.real());
    }
    return s;
}

//==============================================================================
// HOMFLY of a knot (connected, single component) via vendored libhomfly
//==============================================================================

using Polynomial = std::map<std::pair<int,int>, long>;

// Returns false in `ok` if pd is not a single-component connected diagram
// (libhomfly only handles connected diagrams; a knot always is).
Polynomial HomflyKnot(const PD_T& pd, bool& ok)
{
    ok = false;
    if (pd.LinkComponentCount() > Int(1) || pd.DiagramComponentCount() > Int(1))
    {
        return {};
    }
    std::string jenkins = std::string(pd.ToJenkinsCodeString());
    std::vector<char> buf(jenkins.begin(), jenkins.end());
    buf.push_back('\0');
    Poly* p = homfly(buf.data());
    Polynomial out;
    if (p != nullptr)
    {
        for (long i = 0; i < p->len; ++i)
        {
            out[{ static_cast<int>(p->term[i].l), static_cast<int>(p->term[i].m) }]
                += p->term[i].coef;
        }
    }
    knoodle_gc_free_all();
    ok = true;
    return out;
}

//==============================================================================
// Starting knots
//==============================================================================

PD_T BuiltinKnot(const std::string& name)
{
    if (name == "figure_eight" || name == "4_1")
    {
        std::vector<Int> pd = {3,1,4,0,1, 7,5,0,4,1, 5,2,6,3,-1, 1,6,2,7,-1};
        return PD_T::FromSignedPDCode(pd.data(), Int(4), false, true);
    }
    std::vector<Int> tref = {0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1};
    return PD_T::FromSignedPDCode(tref.data(), Int(3), false, true);
}

// Reconstruct a knot from the KLUT MacLeod-code key table for crossing number c
// (the index-th key). Returns an invalid diagram if the table is unavailable.
PD_T KlutKnot(const std::string& klut_dir, int c, long index)
{
    std::string path = klut_dir + "/Klut_Keys_"
                     + (c < 10 ? "0" : "") + std::to_string(c) + ".bin";
    std::ifstream in(path, std::ios::binary);
    if (!in) { return PD_T::InvalidDiagram(); }
    in.seekg(static_cast<std::streamoff>(index) * c);
    std::vector<Knoodle::UInt8> key(static_cast<std::size_t>(c));
    if (!in.read(reinterpret_cast<char*>(key.data()), c)) { return PD_T::InvalidDiagram(); }
    return PD_T::FromMacLeodCode(key.data(), Int(c), Int(0));
}

//==============================================================================
// Trial config + result
//==============================================================================

struct Config
{
    std::uint64_t seed       = 1;
    Int           target     = 100000;   // inflate until crossings >= target
    int           max_rounds = 60;
    Int           reduce_to  = 200;       // simplified must be <= this
    Int           homfly_max = 40;        // compute HOMFLY if recovered <= this
    bool          verbose    = false;
};

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

// Returns true on pass.
bool RunTrial(PD_T start, const Config& cfg, const Alex_T& alex)
{
    std::cout << "=== trial seed=" << cfg.seed
              << "  start: " << start.CrossingCount() << " crossings ===\n";

    if (start.LinkComponentCount() > Int(1))
    {
        std::cout << "  FAIL: starting diagram is a link, not a knot (Alexander "
                     "is knots-only).\n";
        return false;
    }

    const AlexFingerprint alex0 = Alexander(alex, start);
    bool homfly_ok0 = false;
    const Polynomial homfly0 = HomflyKnot(start, homfly_ok0);
    std::cout << "  Alexander(start): " << AlexToString(alex0) << "\n";

    Reapr_T reapr{};
    PD_T pd = std::move(start);
    PD_T prev = pd;  // input to the current round (for failure dumps)

    const auto t_inflate0 = Clock::now();
    int round = 0;
    while (pd.CrossingCount() < cfg.target && round < cfg.max_rounds)
    {
        ++round;
        if (pd.DiagramComponentCount() > Int(1) || pd.CrossingCount() == 0)
        {
            std::cout << "  FAIL: diagram became split/trivial before round "
                      << round << " (unexpected for a knot).\n";
            return false;
        }

        const std::uint64_t seed_k = SplitMix64(cfg.seed + static_cast<std::uint64_t>(round));
        reapr.RandomEngine() = Knoodle::PRNG_T(seed_k);

        prev = pd;
        auto emb = reapr.Embedding(pd);
        emb.Rotate(reapr.RandomRotation());
        auto [pd_new, unlinks] = PD_T::FromLinkEmbedding(emb);

        if (!pd_new.ValidQ() || unlinks.Size() > Int(0)
            || pd_new.LinkComponentCount() > Int(1))
        {
            const std::string dump = "inflate_fail_seed" + std::to_string(cfg.seed)
                                   + "_round" + std::to_string(round) + ".tsv";
            DumpPD(prev, dump);
            std::cout << "  FAIL: round " << round << " produced an invalid/split/"
                         "multi-component projection (unlinks=" << unlinks.Size()
                      << ").\n  reproduce: load " << dump << ", Reapr seed "
                      << seed_k << ", one embed+reproject.\n";
            return false;
        }
        pd = std::move(pd_new);

        // Tripwire: the Alexander value must not change.
        const AlexFingerprint alex_k = Alexander(alex, pd);
        if (!AlexEqual(alex_k, alex0))
        {
            const std::string dump = "inflate_fail_seed" + std::to_string(cfg.seed)
                                   + "_round" + std::to_string(round) + ".tsv";
            DumpPD(prev, dump);
            std::cout << "  FAIL: Alexander changed at round " << round
                      << " (" << pd.CrossingCount() << " crossings)\n"
                      << "    before: " << AlexToString(alex0) << "\n"
                      << "    after : " << AlexToString(alex_k) << "\n"
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
        std::cout << "  FAIL: Simplify returned no diagram (the knot is "
                     "non-trivial; it should not vanish).\n";
        DumpPD(pd, "inflate_fail_seed" + std::to_string(cfg.seed) + "_simplify.tsv");
        return false;
    }
    PD_T recovered = pdc.ToSingleDiagram();
    const Int rec_c = recovered.CrossingCount();
    std::cout << "  Simplify: " << inflated << " -> " << rec_c
              << " crossings (" << Secs(t_simp0, t_simp1) << "s)\n";

    if (rec_c > cfg.reduce_to)
    {
        std::cout << "  FAIL: Simplify did not reduce the diagram (" << rec_c
                  << " > " << cfg.reduce_to << ") — strong bug signal.\n";
        DumpPD(pd, "inflate_fail_seed" + std::to_string(cfg.seed) + "_simplify.tsv");
        return false;
    }

    // Invariant checks on the recovered diagram.
    const AlexFingerprint alex_rec = Alexander(alex, recovered);
    if (!AlexEqual(alex_rec, alex0))
    {
        std::cout << "  FAIL: Alexander of recovered diagram differs.\n"
                  << "    start    : " << AlexToString(alex0) << "\n"
                  << "    recovered: " << AlexToString(alex_rec) << "\n";
        return false;
    }

    if (rec_c <= cfg.homfly_max && homfly_ok0)
    {
        bool ok = false;
        const Polynomial homfly_rec = HomflyKnot(recovered, ok);
        if (ok && homfly_rec != homfly0)
        {
            std::cout << "  FAIL: HOMFLY of recovered diagram differs from start.\n";
            return false;
        }
        std::cout << "  HOMFLY preserved (recovered " << rec_c << " crossings).\n";
    }

    std::cout << "  PASS: Alexander preserved through inflation and recovery.\n";
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    Config cfg;
    std::string start_spec = "trefoil";
    std::string klut_dir   = "../data/Klut";
    int n_trials = 1;

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        auto val = [&](const std::string& p) { return a.substr(p.size()); };
        if      (a.rfind("--seed=", 0) == 0)        cfg.seed = std::stoull(val("--seed="));
        else if (a.rfind("--target=", 0) == 0)      cfg.target = std::stoll(val("--target="));
        else if (a.rfind("--max-rounds=", 0) == 0)  cfg.max_rounds = std::stoi(val("--max-rounds="));
        else if (a.rfind("--reduce-to=", 0) == 0)   cfg.reduce_to = std::stoll(val("--reduce-to="));
        else if (a.rfind("--trials=", 0) == 0)      n_trials = std::stoi(val("--trials="));
        else if (a.rfind("--start=", 0) == 0)       start_spec = val("--start=");
        else if (a.rfind("--klut-dir=", 0) == 0)    klut_dir = val("--klut-dir=");
        else if (a == "--verbose")                  cfg.verbose = true;
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "inflate_check — inflate a knot to ~100k crossings, verify the\n"
                "Alexander value is preserved each round, then Simplify and check\n"
                "it collapses back with the same Alexander/HOMFLY.\n\n"
                "Options:\n"
                "  --start=SPEC     trefoil (default) | figure_eight | klut:C[:i]\n"
                "                   | pd:FILE  (klut:C reconstructs a C-crossing knot)\n"
                "  --seed=N         master RNG seed (default 1); trial k uses seed+k\n"
                "  --trials=N       run N trials with consecutive seeds (default 1)\n"
                "  --target=N       inflate until >= N crossings (default 100000)\n"
                "  --max-rounds=N   cap on inflation rounds (default 60)\n"
                "  --reduce-to=N    recovered diagram must be <= N crossings (default 200)\n"
                "  --klut-dir=PATH  KLUT data dir for --start=klut (default ../data/Klut)\n"
                "  --verbose        print per-round crossing counts\n";
            return 0;
        }
    }

    int failures = 0;
    for (int t = 0; t < n_trials; ++t)
    {
        Config c = cfg;
        c.seed = cfg.seed + static_cast<std::uint64_t>(t);

        PD_T start;
        if (start_spec.rfind("klut:", 0) == 0)
        {
            // klut:C  or  klut:C:i
            std::string rest = start_spec.substr(5);
            auto colon = rest.find(':');
            int cnum = std::stoi(colon == std::string::npos ? rest : rest.substr(0, colon));
            long idx = (colon == std::string::npos) ? 0 : std::stol(rest.substr(colon + 1));
            start = KlutKnot(klut_dir, cnum, idx);
            if (!start.ValidQ())
            {
                std::cerr << "could not load KLUT knot " << start_spec
                          << " from " << klut_dir << "\n";
                return 2;
            }
        }
        else if (start_spec.rfind("pd:", 0) == 0)
        {
            std::ifstream in(start_spec.substr(3));
            std::vector<Int> ints; Int v;
            while (in >> v) ints.push_back(v);
            if (ints.empty() || ints.size() % 5 != 0) { std::cerr << "bad pd file\n"; return 2; }
            start = PD_T::FromSignedPDCode(ints.data(), static_cast<Int>(ints.size()/5), false, true);
        }
        else
        {
            start = BuiltinKnot(start_spec);
        }

        Alex_T alex;  // fresh per trial (caches per-diagram normalization)
        if (!RunTrial(std::move(start), c, alex)) { ++failures; }
        std::cout << "\n";
    }

    std::cout << "inflate_check: " << (n_trials - failures) << "/" << n_trials
              << " trials passed\n";
    return failures == 0 ? 0 : 1;
}
