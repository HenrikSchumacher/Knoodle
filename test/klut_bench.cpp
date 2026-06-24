/**
 * @file klut_bench.cpp
 * @brief Throughput benchmark for the KLUT *Identify* path (klut_identify.hpp),
 *        modeled on the polygonal-knot enumeration workload: a firehose of small,
 *        NON-minimal diagrams, each handed to ki::Identify (decompose -> pass-only
 *        Simplify -> direct lookup -> Reapr escalation on miss) and usually
 *        resolved to an already-known knot.
 *
 * What it measures, per item, as separate stages:
 *   construct   - build a fresh PD_T from a stored signed PD code (stands in for
 *                 "build a diagram from the line-arrangement combinatorics") and
 *                 wrap it in a single-diagram PDC.
 *   identify    - ki::Identify on that PDC: pass-only seed Simplify, direct table
 *                 lookup, and Reapr escalation when the pass-fixpoint is a
 *                 non-minimal <=13 diagram absent from the table. The escalation
 *                 rate (reapr calls / item) is reported alongside.
 *
 * Inputs: KLUT minimal diagrams (reconstructed from the shipped key tables) are
 * perturbed by one Reapr embed+reproject into small non-minimal diagrams of the
 * SAME knot -- so Identify has real work to do (klut_e2e showed Reapr is a no-op
 * on already-minimal diagrams, which would make this measure nothing).
 *
 * Alternate input (--polygon-edges=N): a firehose of fresh random equilateral
 * N-gons from the progressive action-angle sampler, projected to PDs and
 * Identified. Generation and classification are timed separately. This is the
 * real polygonal-knot enumeration workload (mostly unknots, a tail of small
 * knots, occasional >13-crossing diagrams that escalate or stay Unidentified).
 *
 * Parallel scaling uses the REENTRANT path: subtables pre-loaded once
 * (LoadSubtables, avoiding the lazy-load race); ki::Identify's lookups use a
 * thread-local buffer (FindID(buffer, c), NOT the shared-buffer FindID(pd)) over
 * the shared read-only table, and each worker thread owns its own Reapr.
 *
 * Build: test/Makefile target klut_bench (UMFPACK + Accelerate, like inflate_check).
 */

#include "../Knoodle.hpp"
#include "../tools/klut_identify.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using Int     = std::int64_t;
using Real    = double;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, float>;
using Klut    = Knoodle::Klut;
using Clock   = std::chrono::steady_clock;
using CodeInt = Klut::CodeInt;
namespace ki  = klut_identify;

namespace {

double Secs(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}

std::uint64_t SplitMix64(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// One perturbed pool item: a non-minimal signed PD code + its crossing count and
// the source knot's minimal crossing number (for a coarse correctness check).
struct Item
{
    std::vector<Int> code;    // 5-col signed PD, flattened
    Int              nc = 0;  // crossing count of the perturbed diagram
    Int              c_min = 0;
    Klut::ID_T       src_id = Klut::not_found;  // (c_min, src_id) = the true knot
};

// Reconstruct minimal KLUT diagrams: sample `per_c` knots from each crossing
// number c in [3, c_max], read their first stored key, FromMacLeodCode.
std::vector<PD_T> SampleMinimals(const std::string& klut_dir, Int c_max, Int per_c)
{
    std::vector<PD_T> out;
    for (Int c = 3; c <= c_max; ++c)
    {
        const std::string path = klut_dir + "/Klut_Keys_"
                               + (c < 10 ? "0" : "") + std::to_string(c) + ".bin";
        std::ifstream in(path, std::ios::binary);
        if (!in) { continue; }
        for (Int k = 0; k < per_c; ++k)
        {
            std::vector<Knoodle::UInt8> key(static_cast<std::size_t>(c));
            in.seekg(static_cast<std::streamoff>(k) * c);
            if (!in.read(reinterpret_cast<char*>(key.data()), c)) { break; }
            PD_T pd = PD_T::FromMacLeodCode(key.data(), Int(c), Int(0));
            if (pd.ValidQ()) { out.push_back(std::move(pd)); }
        }
    }
    return out;
}

std::vector<Int> SignedPDCode(const PD_T& pd_in)
{
    PD_T pd(pd_in);
    auto code = pd.template PDCode<Int, {.signQ = true, .colorQ = false}>();
    const Int c = pd.CrossingCount();
    std::vector<Int> flat;
    flat.reserve(static_cast<std::size_t>(5 * c));
    for (Int i = 0; i < c; ++i)
        for (int j = 0; j < 5; ++j) { flat.push_back(code(i, j)); }
    return flat;
}

// Perturb each minimal into a small non-minimal diagram of the same knot via one
// Reapr embed+reproject. Keep results that stay a single-component diagram with
// crossing count in (c_min, cap]; retry a few seeds to fill the pool.
std::vector<Item> BuildPool(const std::vector<PD_T>& minimals, Int cap,
                            std::uint64_t seed0, int tries_per, Klut& klut)
{
    std::vector<Item> pool;
    Reapr_T reapr{};
    std::array<CodeInt, Klut::max_crossing_count> kbuf{};
    for (const PD_T& m_in : minimals)
    {
        PD_T m(m_in);
        const Int c_min = m.CrossingCount();
        // The true knot id: the minimal table diagram's own key.
        m.template WriteMacLeodCode<CodeInt>(kbuf.data());
        auto [mc, src_id] = klut.FindID(kbuf.data(), c_min);
        (void)mc;
        for (int t = 0; t < tries_per; ++t)
        {
            reapr.RandomEngine() =
                Knoodle::PRNG_T(SplitMix64(seed0 + static_cast<std::uint64_t>(
                    pool.size() * 131 + t)));
            auto emb = reapr.Embedding(m);
            emb.Rotate(reapr.RandomRotation());
            auto [pd_new, unlinks] = PD_T::FromLinkEmbedding(emb);
            if (!pd_new.ValidQ() || unlinks.Size() > Int(0)
                || pd_new.LinkComponentCount() > Int(1)
                || pd_new.DiagramComponentCount() > Int(1)) { continue; }
            const Int nc = pd_new.CrossingCount();
            if (nc <= c_min || nc > cap) { continue; }
            pool.push_back(Item{ SignedPDCode(pd_new), nc, c_min, src_id });
            break;
        }
    }
    return pool;
}

// Inflate each minimal to >= target crossings by repeated Reapr reproject (same
// knot throughout), to probe how Identify behaves on LARGE inputs.
std::vector<Item> BuildInflatedPool(const std::vector<PD_T>& minimals, Int target,
                                    std::uint64_t seed0, Klut& klut)
{
    std::vector<Item> pool;
    Reapr_T reapr{};
    std::array<CodeInt, Klut::max_crossing_count> kbuf{};
    for (const PD_T& m_in : minimals)
    {
        PD_T m(m_in);
        const Int c_min = m.CrossingCount();
        m.template WriteMacLeodCode<CodeInt>(kbuf.data());
        auto [mc, src_id] = klut.FindID(kbuf.data(), c_min);
        (void)mc;
        PD_T pd(m);
        for (int round = 1; pd.CrossingCount() < target && round <= 60; ++round)
        {
            reapr.RandomEngine() = Knoodle::PRNG_T(
                SplitMix64(seed0 + static_cast<std::uint64_t>(pool.size() * 131 + round)));
            auto emb = reapr.Embedding(pd);
            emb.Rotate(reapr.RandomRotation());
            auto [pd_new, unlinks] = PD_T::FromLinkEmbedding(emb);
            if (!pd_new.ValidQ() || unlinks.Size() > Int(0)
                || pd_new.LinkComponentCount() > Int(1)
                || pd_new.DiagramComponentCount() > Int(1)) { break; }
            pd = std::move(pd_new);
        }
        if (pd.CrossingCount() > c_min)
        {
            pool.push_back(Item{ SignedPDCode(pd), pd.CrossingCount(), c_min, src_id });
        }
    }
    return pool;
}

// Pool serialization. The pool is generated by perturbing KLUT minimals through
// Reapr, whose embedding step pulls from OrthoDraw's own entropy-seeded RNG
// (OrthoDraw.hpp; InitializedRandomEngine uses std::random_device with no seed
// hook), so the generated set is NOT reproducible from the benchmark side alone.
// To get a reproducible failing set, save the pool once and reload it: same file
// -> same diagrams -> same audit outcomes (Identify itself stays stochastic, but
// the input population is pinned). Text format, one item per line:
//   nc <tab> c_min <tab> src_id <tab> n_codes <tab> code...
bool SavePool(const std::vector<Item>& pool, const std::string& path)
{
    std::ofstream out(path);
    if (!out) { return false; }
    out << "klut_bench_pool 1 " << pool.size() << "\n";
    for (const auto& it : pool)
    {
        out << it.nc << '\t' << it.c_min << '\t'
            << static_cast<long long>(it.src_id) << '\t' << it.code.size();
        for (Int v : it.code) { out << '\t' << v; }
        out << '\n';
    }
    return static_cast<bool>(out);
}

std::vector<Item> LoadPool(const std::string& path)
{
    std::vector<Item> pool;
    std::ifstream in(path);
    if (!in) { return pool; }
    std::string magic; int ver = 0; std::size_t n = 0;
    if (!(in >> magic >> ver >> n) || magic != "klut_bench_pool") { return pool; }
    pool.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        Item it; std::size_t ncode = 0; long long src = 0;
        if (!(in >> it.nc >> it.c_min >> src >> ncode)) { break; }
        it.src_id = static_cast<Klut::ID_T>(src);
        it.code.resize(ncode);
        for (std::size_t j = 0; j < ncode; ++j) { in >> it.code[j]; }
        pool.push_back(std::move(it));
    }
    return pool;
}

struct Stage { double construct = 0, identify = 0; std::size_t reapr_calls = 0; };

// True iff Identify resolved `it` to exactly the source knot: a single Identified
// summand at the source's crossing number and id.
bool IdentifiedAsSource(const ki::IdentifyResult& r, const Item& it)
{
    return r.status == ki::IdentifyResult::Status::Knot
        && r.summands.size() == 1
        && r.summands[0].kind == ki::Summand::Kind::Identified
        && r.summands[0].crossings == it.c_min
        && r.summands[0].id == it.src_id;
}

// Run `iters` identify chains over the pool (cycled), accumulating per-stage time
// and the escalation count. Returns the stage totals; `correct` counts items that
// resolved to the source knot. One Reapr and one set of scratch PDCs / result are
// reused across all items via ki::IdentifyInto (the realistic reentrant firehose
// path) and are therefore thread-local to this chain. The per-item input PD_T is
// always freshly built (each pool item is a different diagram).
Stage RunChain(const std::vector<Item>& pool, Klut& klut, std::size_t iters,
               std::size_t start, std::size_t stride, std::atomic<std::size_t>* correct,
               const ki::IdentifyParams& q)
{
    Stage s;
    std::size_t hits = 0;
    Reapr_T reapr{};
    PDC_T work, temp;            // scratch, reused across items
    ki::IdentifyResult R;        // result, reused across items
    for (std::size_t i = 0; i < iters; ++i)
    {
        const Item& it = pool[(start + i * stride) % pool.size()];
        const Int rows = static_cast<Int>(it.code.size() / 5);

        auto t0 = Clock::now();
        work.Clear();
        work.Push(PD_T::FromSignedPDCode(it.code.data(), rows, false, true));
        auto t1 = Clock::now();

        ki::IdentifyInto(klut, work, temp, reapr, R, q);
        auto t2 = Clock::now();

        s.reapr_calls += static_cast<std::size_t>(R.reapr_calls);
        if (IdentifiedAsSource(R, it)) { ++hits; }
        s.construct += Secs(t0, t1);
        s.identify  += Secs(t1, t2);
    }
    if (correct) { correct->fetch_add(hits, std::memory_order_relaxed); }
    return s;
}

// Escalation-schedule presets to sweep (cost vs. correctness): the initial Reapr
// embedding_trials handed to ki::Identify before doubling. A larger n0 spends more
// up front but may resolve a stubborn fixpoint in one escalation instead of two.
struct Preset { const char* name; ki::IdentifyParams q; };

std::vector<Preset> Presets()
{
    std::vector<Preset> v;
    v.push_back(Preset{"n0=1",  ki::IdentifyParams{ .n0 = ki::Size_T(1) }});
    v.push_back(Preset{"n0=2",  ki::IdentifyParams{ .n0 = ki::Size_T(2) }});
    v.push_back(Preset{"n0=4",  ki::IdentifyParams{ .n0 = ki::Size_T(4) }});
    v.push_back(Preset{"n0=8",  ki::IdentifyParams{ .n0 = ki::Size_T(8) }});
    return v;
}

void ReportStages(const Stage& s, std::size_t iters, double wall)
{
    const double total = s.construct + s.identify;
    auto line = [&](const char* name, double t) {
        std::cout << "    " << name << "  "
                  << (t / iters * 1e9) << " ns/item   "
                  << (100.0 * t / total) << " %\n";
    };
    std::cout << "  per-stage (single thread, " << iters << " items):\n";
    line("construct", s.construct);
    line("identify ", s.identify);
    std::cout << "  escalation: " << (static_cast<double>(s.reapr_calls) / iters)
              << " reapr calls/item\n";
    std::cout << "  end-to-end: " << (wall / iters * 1e9) << " ns/item   "
              << (iters / wall) << " items/s\n";
}

// Map a --compaction= name to the OrthoDraw compaction method. This is a Reapr
// setting which, per PDC::Simplify ("the options of the Reapr instance override
// some of the options in args"), governs the embedding compaction inside
// ki::Identify. Length_MCF (min-cost-flow) is the default; TopologicalNumbering
// is the cheap alternative.
PDC_T::Compaction_T ParseCompaction(const std::string& s, bool& ok)
{
    using C = PDC_T::Compaction_T;
    ok = true;
    if (s == "length-mcf" || s == "mcf" || s == "default")    return C::Length_MCF;
    if (s == "topo" || s == "topological-numbering")          return C::TopologicalNumbering;
    if (s == "topo-ordering" || s == "topological-ordering")  return C::TopologicalOrdering;
    if (s == "length-clp" || s == "clp")                      return C::Length_CLP;
    if (s == "area-clp" || s == "area-length-clp")            return C::AreaAndLength_CLP;
    ok = false;
    return C::Length_MCF;
}

const char* CompactionName(PDC_T::Compaction_T c)
{
    using C = PDC_T::Compaction_T;
    switch (c)
    {
        case C::TopologicalNumbering: return "topo";
        case C::TopologicalOrdering:  return "topo-ordering";
        case C::Length_MCF:           return "length-mcf";
        case C::Length_CLP:           return "length-clp";
        case C::AreaAndLength_CLP:    return "area-clp";
        default:                      return "unknown";
    }
}

} // namespace

int main(int argc, char* argv[])
{
    std::string klut_dir = "../data/Klut";
    Int    c_max   = 13;
    Int    per_c   = 80;     // minimal knots sampled per crossing number
    Int    cap     = 24;     // keep perturbed diagrams up to this many crossings
    std::size_t iters = 200000;
    std::size_t profile_n = 0;   // --profile=N: run only N chains, then exit (for `sample`)
    std::size_t audit_n = 0;     // --audit=N: classify N outcomes (correct/wrong/unidentified/error)
    Int inflate_target = 0;      // --inflate=N: inflate inputs to ~N crossings
    ki::Size_T n0  = 1;          // --n0=N: initial Reapr embedding_trials in ki::Identify
    ki::Size_T cap_escalate = ki::IdentifyParams{}.cap; // --escalate-cap=N: max escalation rounds
    ki::Size_T rot_trials   = ki::IdentifyParams{}.rot; // --rot=N: reprojections per embedding
    Int polygon_edges = 0;       // --polygon-edges=N: random-polygon firehose mode (N-gon knots)
    std::uint64_t polygon_seed = 20260617ULL; // --polygon-seed=N: action-angle sampler seed
    std::string compaction = "length-mcf"; // --compaction=NAME: Reapr compaction method
    int randomize_bends = -1;    // --randomize-bends=N: Reapr bend-layout trials (-1=keep default 4)
    double ssn_tolerance = -1;   // --ssn-tolerance=X: Reapr energy-min tolerance (>0 to override)
    long   ssn_max_iter  = -1;   // --ssn-max-iter=N: Reapr energy-min max iterations (>=0 to override)
    double scaling       = -1;   // --scaling=X: Reapr embedding scaling (>0 to override)
    std::string pool_file;       // --pool-file=PATH: load pool if it exists, else build + save
    std::vector<int> thread_counts = {1, 2, 4, 8};

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        auto v = [&](const std::string& p) { return a.substr(p.size()); };
        if      (a.rfind("--klut-dir=", 0) == 0) klut_dir = v("--klut-dir=");
        else if (a.rfind("--c-max=", 0) == 0)    c_max = std::stoll(v("--c-max="));
        else if (a.rfind("--per-c=", 0) == 0)    per_c = std::stoll(v("--per-c="));
        else if (a.rfind("--cap=", 0) == 0)      cap = std::stoll(v("--cap="));
        else if (a.rfind("--iters=", 0) == 0)    iters = std::stoull(v("--iters="));
        else if (a.rfind("--profile=", 0) == 0)  profile_n = std::stoull(v("--profile="));
        else if (a.rfind("--audit=", 0) == 0)    audit_n = std::stoull(v("--audit="));
        else if (a.rfind("--inflate=", 0) == 0)  inflate_target = std::stoll(v("--inflate="));
        else if (a.rfind("--n0=", 0) == 0)       n0 = static_cast<ki::Size_T>(std::stoull(v("--n0=")));
        else if (a.rfind("--escalate-cap=", 0) == 0) cap_escalate = static_cast<ki::Size_T>(std::stoull(v("--escalate-cap=")));
        else if (a.rfind("--rot=", 0) == 0) rot_trials = static_cast<ki::Size_T>(std::stoull(v("--rot=")));
        else if (a.rfind("--polygon-edges=", 0) == 0) polygon_edges = std::stoll(v("--polygon-edges="));
        else if (a.rfind("--polygon-seed=", 0) == 0)  polygon_seed = std::stoull(v("--polygon-seed="));
        else if (a.rfind("--compaction=", 0) == 0) compaction = v("--compaction=");
        else if (a.rfind("--randomize-bends=", 0) == 0) randomize_bends = std::stoi(v("--randomize-bends="));
        else if (a.rfind("--ssn-tolerance=", 0) == 0) ssn_tolerance = std::stod(v("--ssn-tolerance="));
        else if (a.rfind("--ssn-max-iter=", 0) == 0)  ssn_max_iter = std::stol(v("--ssn-max-iter="));
        else if (a.rfind("--scaling=", 0) == 0)       scaling = std::stod(v("--scaling="));
        else if (a.rfind("--pool-file=", 0) == 0) pool_file = v("--pool-file=");
    }

    const ki::IdentifyParams idp{ .n0 = n0, .cap = cap_escalate, .rot = rot_trials,
                                  .max_cx = static_cast<Int>(c_max) };

    std::cout << "klut_bench: KLUT Identify-path throughput\n"
              << "  klut-dir=" << klut_dir << "  c_max=" << c_max
              << "  per_c=" << per_c << "  cap=" << cap << "  iters=" << iters
              << "  n0=" << static_cast<long long>(n0) << "\n\n";

    // Build the table and pre-load (single-threaded) so parallel reads are safe.
    Klut klut{ std::filesystem::path(klut_dir), static_cast<Knoodle::Size_T>(c_max) };
    const auto tL0 = Clock::now();
    klut.LoadSubtables();
    const auto tL1 = Clock::now();
    std::cout << "  subtables loaded in " << Secs(tL0, tL1) << " s\n";

    // Polygon-firehose mode: instead of the perturbed KLUT pool, generate fresh
    // random equilateral polygons (single-component knots) with the progressive
    // action-angle sampler, project each to a PD, and Identify it. Generation and
    // classification are timed separately (generation is usually the cheap part).
    // The pool is not used in this mode. Seedable for a reproducible diagram
    // stream (the Reapr escalation inside Identify remains stochastic).
    if (polygon_edges > 0)
    {
        bool comp_ok = true;
        const auto comp = ParseCompaction(compaction, comp_ok);
        if (!comp_ok) { std::cerr << "unknown --compaction=" << compaction << "\n"; return 2; }
        Reapr_T::Settings_T rset{};
        rset.ortho_draw_settings.compaction_method = comp;
        if (randomize_bends >= 0) { rset.ortho_draw_settings.randomize_bends = randomize_bends; }
        if (ssn_tolerance > 0)    { rset.tolerance    = static_cast<Real>(ssn_tolerance); }
        if (ssn_max_iter  >= 0)   { rset.SSN_max_iter  = static_cast<Knoodle::Size_T>(ssn_max_iter); }
        if (scaling > 0)          { rset.scaling       = static_cast<Real>(scaling); }

        using Sampler_T = Knoodle::ActionAngleSampler<Real, Int, Knoodle::PRNG_T, true>;
        Sampler_T sampler{ Knoodle::PRNG_T(polygon_seed) };
        Reapr_T reapr{ rset };
        PDC_T work, temp;
        ki::IdentifyResult R;

        double t_gen = 0, t_classify = 0;
        std::size_t reapr_calls = 0;
        std::size_t n_identified = 0, n_unident = 0, n_error = 0,
                    n_unknot = 0, n_link = 0, n_invalid = 0;
        Int gen_c_min = -1, gen_c_max = 0; double gen_c_sum = 0;
        std::array<std::size_t, Klut::max_crossing_count + 1> hist{};  // identified-prime crossings

        std::cout << "\n  polygon firehose: " << iters << " random "
                  << polygon_edges << "-gons (seed " << polygon_seed << ")\n"
                  << "    reapr: compaction=" << CompactionName(comp)
                  << " randomize_bends=" << rset.ortho_draw_settings.randomize_bends
                  << " scaling=" << rset.scaling
                  << " tolerance=" << rset.tolerance
                  << " SSN_max_iter=" << rset.SSN_max_iter << "\n"
                  << "    identify: n0=" << static_cast<long long>(idp.n0)
                  << " escalate-cap=" << static_cast<long long>(idp.cap)
                  << " rot=" << static_cast<long long>(idp.rot) << "\n";

        const auto P0 = Clock::now();
        for (std::size_t i = 0; i < iters; ++i)
        {
            auto t0 = Clock::now();
            auto L = sampler.RandomEquilateralLink<Real, Int, float>(Int(1), polygon_edges);
            auto [pd, unlinks] = PD_T::FromLinkEmbedding(L);
            (void)unlinks;
            auto t1 = Clock::now();

            const bool gen_ok = pd.ValidQ();
            if (gen_ok)
            {
                const Int gc = pd.CrossingCount();
                gen_c_min = (gen_c_min < 0) ? gc : std::min(gen_c_min, gc);
                gen_c_max = std::max(gen_c_max, gc);
                gen_c_sum += static_cast<double>(gc);
                work.Clear();
                work.Push(std::move(pd));
                ki::IdentifyInto(klut, work, temp, reapr, R, idp);
            }
            auto t2 = Clock::now();

            t_gen      += Secs(t0, t1);
            t_classify += Secs(t1, t2);

            if (!gen_ok) { ++n_invalid; continue; }
            reapr_calls += static_cast<std::size_t>(R.reapr_calls);

            if (R.status == ki::IdentifyResult::Status::LinkOutOfScope) { ++n_link; continue; }
            if (R.summands.empty()) { ++n_unknot; continue; }

            bool all_ident = true, any_unident = false, any_error = false;
            for (const auto& sm : R.summands)
            {
                if (sm.kind == ki::Summand::Kind::Identified)
                {
                    if (sm.crossings >= Int(0) && sm.crossings <= Int(Klut::max_crossing_count))
                        ++hist[static_cast<std::size_t>(sm.crossings)];
                }
                else { all_ident = false; }
                if (sm.kind == ki::Summand::Kind::Unidentified) { any_unident = true; }
                if (sm.kind == ki::Summand::Kind::Error)        { any_error = true; }
            }
            if      (all_ident)   { ++n_identified; }
            else if (any_error)   { ++n_error; }
            else if (any_unident) { ++n_unident; }
        }
        const auto P1 = Clock::now();
        const double wall   = Secs(P0, P1);
        const double tot    = t_gen + t_classify;
        const std::size_t n_valid = iters - n_invalid;

        const double classify_tput = (t_classify > 0) ? iters / t_classify : 0.0;
        const double genclass_tput = (tot > 0)        ? iters / tot        : 0.0;

        std::cout << "  generated crossings: ";
        if (gen_c_min < 0) { std::cout << "(none valid)"; }
        else { std::cout << gen_c_min << ".." << gen_c_max << " (avg "
                         << (gen_c_sum / static_cast<double>(n_valid ? n_valid : 1)) << ")"; }
        std::cout << "\n  throughput (single thread, " << iters << " polygons):\n"
                  << "    classify only        : " << classify_tput << " polygons/s\n"
                  << "    generate + classify  : " << genclass_tput << " polygons/s\n"
                  << "  average timing:\n"
                  << "    generate   " << (t_gen / iters * 1e9) << " ns/item   "
                  << (100.0 * t_gen / tot) << " %\n"
                  << "    classify   " << (t_classify / iters * 1e9) << " ns/item   "
                  << (100.0 * t_classify / tot) << " %\n"
                  << "    total      " << (tot / iters * 1e9) << " ns/item\n"
                  << "  end-to-end (wall incl. overhead): " << (wall / iters * 1e9)
                  << " ns/item   " << (iters / wall) << " polygons/s\n"
                  << "  escalation: " << (static_cast<double>(reapr_calls) / iters)
                  << " reapr calls/item\n"
                  << "  outcomes: identified " << n_identified
                  << ", unidentified " << n_unident << ", error " << n_error
                  << ", unknot " << n_unknot << ", link " << n_link
                  << ", invalid " << n_invalid << "\n";
        std::cout << "  identified-prime crossing histogram:";
        bool any = false;
        for (std::size_t c = 3; c < hist.size(); ++c)
            if (hist[c]) { std::cout << "  " << c << "cx:" << hist[c]; any = true; }
        std::cout << (any ? "" : "  (none)") << "\n";
        return 0;
    }

    // Build (or reload) the perturbed pool. With --pool-file=PATH, an existing
    // file is reloaded verbatim (reproducible failing set across runs); otherwise
    // the pool is built and, if a path was given, saved there for next time.
    std::vector<Item> pool;
    if (!pool_file.empty() && std::filesystem::exists(pool_file))
    {
        pool = LoadPool(pool_file);
        std::cout << "  pool loaded from " << pool_file
                  << " (" << pool.size() << " diagrams)\n";
    }
    else
    {
        std::cout << "  building pool...\n";
        auto minimals = SampleMinimals(klut_dir, c_max, per_c);
        pool = inflate_target > 0
            ? BuildInflatedPool(minimals, inflate_target, 0xC0FFEE, klut)
            : BuildPool(minimals, cap, 0xC0FFEE, 8, klut);
        if (!pool_file.empty())
        {
            if (SavePool(pool, pool_file))
                std::cout << "  pool saved to " << pool_file
                          << " (reload with --pool-file= for a reproducible set)\n";
            else
                std::cerr << "  warning: could not save pool to " << pool_file << "\n";
        }
    }
    if (pool.empty()) { std::cerr << "empty pool\n"; return 2; }

    Int nc_min = pool[0].nc, nc_max = pool[0].nc;
    double nc_sum = 0;
    for (const auto& it : pool) { nc_min = std::min(nc_min, it.nc); nc_max = std::max(nc_max, it.nc); nc_sum += it.nc; }
    std::cout << "  pool: " << pool.size() << " diagrams, perturbed crossings "
              << nc_min << ".." << nc_max << " (avg " << (nc_sum / pool.size()) << ")\n\n";

    // Profiling mode: nothing but identify chains in a tight loop, so an external
    // sampler (`sample <pid>`) sees a clean Identify-dominated process.
    if (profile_n > 0)
    {
        std::cout << "  profile mode: " << profile_n << " chains (sample me now)\n";
        std::atomic<std::size_t> c{0};
        Stage ps = RunChain(pool, klut, profile_n, 0, 1, &c, idp);
        std::cout << "  done (" << ps.identify << " s in identify, "
                  << ps.reapr_calls << " reapr calls)\n";
        return 0;
    }

    // Audit mode: classify each Identify outcome. A pool item is a perturbed copy
    // of one prime KLUT minimal, so the only correct answer is a single Identified
    // summand at the source (c_min, src_id). Everything else is a real failure:
    //   WRONG KNOT     - a single valid id that is a DIFFERENT knot, OR an
    //                    unexpected multi-summand split: a Simplify/Identify
    //                    correctness bug (must be zero).
    //   UNIDENTIFIED   - escalation cap exhausted on a >13-crossing end-state:
    //                    Reapr never recovered a <=13 diagram. Under the corrected
    //                    KLUT contract every prime knot <=13 has its minimal in the
    //                    table, so this is an escalation FAILURE, not a table gap.
    //   ERROR          - cap exhausted on a <=13 end-state whose key is absent: a
    //                    table gap or a stuck pass-fixpoint (should be zero).
    //   LINK / COMP-ERR- a single-component perturbation reported as a link or a
    //                    component-count change: a decomposition bug.
    // Dumps the distinct inputs that ever miss, by class, for repro.
    if (audit_n > 0)
    {
        std::size_t n_correct = 0, n_wrong = 0, n_unident = 0, n_error = 0,
                    n_link = 0, n_comperr = 0, n_unknot = 0;
        std::size_t n_wrong_uniq = 0, n_unident_uniq = 0;
        std::vector<char> wrong_seen(pool.size(), 0), unident_seen(pool.size(), 0);
        std::ofstream wdump("klut_bench_wrongknot.tsv");
        std::ofstream udump("klut_bench_unidentified.tsv");

        auto dump_input = [](std::ofstream& os, const Item& it, const std::string& note) {
            os << "# " << note << "  src_c_min=" << it.c_min
               << " src_id=" << static_cast<long long>(it.src_id)
               << " perturbed_c=" << it.nc << "\n";
            const Int rows = static_cast<Int>(it.code.size() / 5);
            for (Int r = 0; r < rows; ++r)
                os << it.code[5*r] << '\t' << it.code[5*r+1] << '\t'
                   << it.code[5*r+2] << '\t' << it.code[5*r+3] << '\t'
                   << it.code[5*r+4] << '\n';
        };

        Reapr_T reapr{};
        for (std::size_t i = 0; i < audit_n; ++i)
        {
            const std::size_t idx = i % pool.size();
            const Item& it = pool[idx];
            PD_T pd = PD_T::FromSignedPDCode(it.code.data(),
                          static_cast<Int>(it.code.size() / 5), false, true);
            PDC_T pdc{ std::move(pd) };
            auto r = ki::Identify(klut, std::move(pdc), reapr, idp);

            if (r.component_error) { ++n_comperr; }

            if (r.status == ki::IdentifyResult::Status::LinkOutOfScope) { ++n_link; continue; }
            if (r.summands.empty()) { ++n_unknot; continue; }  // reduced to the unknot (a knotted item should not)

            if (IdentifiedAsSource(r, it)) { ++n_correct; continue; }

            // A miss. Classify by the worst summand kind present (Error worst,
            // then Unidentified, then a wrong-but-valid Identified / bad split).
            bool has_error = false, has_unident = false, has_wrong = false;
            for (const auto& s : r.summands)
            {
                if (s.kind == ki::Summand::Kind::Error)        { has_error = true; }
                else if (s.kind == ki::Summand::Kind::Unidentified) { has_unident = true; }
                else { has_wrong = true; }   // Identified, but not the lone source knot
            }
            if (has_error)        { ++n_error; }
            else if (has_unident)
            {
                ++n_unident;
                if (!unident_seen[idx]) { unident_seen[idx] = 1; ++n_unident_uniq;
                                          dump_input(udump, it, "UNIDENTIFIED"); }
            }
            else if (has_wrong)
            {
                ++n_wrong;
                if (!wrong_seen[idx]) { wrong_seen[idx] = 1; ++n_wrong_uniq;
                                        dump_input(wdump, it, "WRONG-KNOT"); }
            }
        }
        const double tot = static_cast<double>(audit_n);
        std::cout << "  audit (" << audit_n << " trials over " << pool.size()
                  << " diagrams, ki::Identify, stochastic Reapr):\n"
                  << "    correct (== source knot)         : " << n_correct
                  << "  (" << 100.0 * n_correct / tot << "%)\n"
                  << "    WRONG KNOT (valid id != source)  : " << n_wrong
                  << "   <- real Identify failures; " << n_wrong_uniq
                  << " distinct inputs -> klut_bench_wrongknot.tsv\n"
                  << "    UNIDENTIFIED (>13 after escalate): " << n_unident
                  << "   <- escalation never recovered <=13; " << n_unident_uniq
                  << " distinct inputs -> klut_bench_unidentified.tsv\n"
                  << "    ERROR (<=13 end-state, no key)   : " << n_error
                  << "  (table gap / stuck fixpoint)\n"
                  << "    link / component-error           : " << n_link
                  << " / " << n_comperr << "  (single-knot input misreported)\n"
                  << "    reduced-to-unknot                : " << n_unknot << "\n";
        return 0;
    }

    // Single-thread: per-stage breakdown (default escalation schedule).
    std::atomic<std::size_t> correct{0};
    const auto t0 = Clock::now();
    Stage s = RunChain(pool, klut, iters, 0, 1, &correct, idp);
    const auto t1 = Clock::now();
    ReportStages(s, iters, Secs(t0, t1));
    std::cout << "  correct-identify rate: " << (100.0 * correct.load() / iters) << " %\n\n";

    // Sweep escalation schedules: cost vs. correctness. The winner is the cheapest
    // initial n0 that keeps correctness ~100%.
    std::cout << "  escalation-schedule sweep (single thread, " << iters << " items):\n";
    std::cout << "    " << std::string(26, ' ')
              << "ns/item    items/s    reapr/item  correct%\n";
    for (const auto& p : Presets())
    {
        std::atomic<std::size_t> c{0};
        const auto q0 = Clock::now();
        Stage ss = RunChain(pool, klut, iters, 0, 1, &c, p.q);
        const auto q1 = Clock::now();
        const double w = Secs(q0, q1);
        char buf[200];
        std::snprintf(buf, sizeof buf, "    %-24s  %8.0f  %9.0f   %9.4f   %7.3f",
                      p.name, w / iters * 1e9, iters / w,
                      static_cast<double>(ss.reapr_calls) / iters,
                      100.0 * c.load() / iters);
        std::cout << buf << "\n";
    }
    std::cout << "\n";

    // Parallel scaling. Each worker owns its own Reapr (RunChain constructs one);
    // the table is shared read-only and looked up reentrantly.
    std::cout << "  parallel scaling (end-to-end items/s):\n";
    double base = 0;
    for (int T : thread_counts)
    {
        if (T > static_cast<int>(std::thread::hardware_concurrency()) && T > 1) { continue; }
        std::atomic<std::size_t> f{0};
        const std::size_t per = iters / static_cast<std::size_t>(T);
        const auto p0 = Clock::now();
        std::vector<std::thread> ths;
        for (int t = 0; t < T; ++t)
            ths.emplace_back([&, t] {
                RunChain(pool, klut, per, static_cast<std::size_t>(t) * 7 + 1,
                         static_cast<std::size_t>(T), &f, idp);
            });
        for (auto& th : ths) { th.join(); }
        const auto p1 = Clock::now();
        const double ips = (per * static_cast<std::size_t>(T)) / Secs(p0, p1);
        if (T == 1) { base = ips; }
        std::cout << "    " << T << " thread(s): " << ips << " items/s"
                  << (base > 0 ? "   " + std::to_string(ips / base) + "x" : "") << "\n";
    }

    // Projection.
    const double ips1 = iters / Secs(t0, t1);
    std::cout << "\n  projection @ single-thread rate: 1e10 items ~ "
              << (1e10 / ips1 / 3600.0) << " core-hours\n";
    return 0;
}
