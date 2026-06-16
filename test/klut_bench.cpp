/**
 * @file klut_bench.cpp
 * @brief Throughput benchmark for the KLUT identify path, modeled on the
 *        polygonal-knot enumeration workload: a firehose of small, NON-minimal
 *        diagrams, each Simplified, canonicalized to a MacLeod key, and looked
 *        up (usually to be discarded as an already-known knot).
 *
 * What it measures, per item, as separate stages:
 *   construct   - build a fresh PD_T from a stored signed PD code (stands in for
 *                 "build a diagram from the line-arrangement combinatorics")
 *   simplify    - PlanarDiagramComplex::Simplify (Reapr-based; the suspected
 *                 dominant cost -- the table only stores simplified diagrams)
 *   canonical   - PlanarDiagram::WriteMacLeodCode (canonical key computation)
 *   lookup      - Klut::FindID(key) = pack + one hash probe (expected ~free)
 *
 * Inputs: KLUT minimal diagrams (reconstructed from the shipped key tables) are
 * perturbed by one Reapr embed+reproject into small non-minimal diagrams of the
 * SAME knot -- so Simplify has real work to do (klut_e2e showed Reapr is a no-op
 * on already-minimal diagrams, which would make this measure nothing).
 *
 * Parallel scaling uses the REENTRANT path: subtables pre-loaded once
 * (LoadSubtables, avoiding the lazy-load race), each thread with its own scratch
 * buffer calling FindID(buffer, c) (NOT the shared-buffer FindID(pd) overload),
 * over the shared read-only table.
 *
 * Build: test/Makefile target klut_bench (UMFPACK + Accelerate, like inflate_check).
 */

#include "../Knoodle.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
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
// knot throughout), to probe how Simplify behaves on LARGE inputs.
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
// -> same diagrams -> same audit outcomes (Simplify itself stays stochastic, but
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

struct Stage { double construct = 0, simplify = 0, canonical = 0, lookup = 0; };

// Run `iters` identify chains over the pool (cycled), accumulating per-stage time.
// Returns the stage totals; `found` counts lookups that resolved to a real id.
Reapr_T MakeReapr(const PDC_T::Simplify_Args_T& args)
{
    return Reapr_T({
        .permute_randomQ     = args.permute_randomQ,
        .energy              = args.energy,
        .ortho_draw_settings = {
            .randomize_bends          = args.randomize_bends,
            .randomize_virtual_edgesQ = args.randomize_virtual_edgesQ,
            .compaction_method        = args.compaction_method
        },
        .scaling             = args.scaling
    });
}

Stage RunChain(const std::vector<Item>& pool, Klut& klut, std::size_t iters,
               std::size_t start, std::size_t stride, std::atomic<std::size_t>* correct,
               const PDC_T::Simplify_Args_T& args, bool reuse_reapr = false)
{
    Stage s;
    std::array<CodeInt, Klut::max_crossing_count> buf{};
    std::size_t hits = 0;
    Reapr_T reapr = MakeReapr(args);   // reused across items when reuse_reapr
    for (std::size_t i = 0; i < iters; ++i)
    {
        const Item& it = pool[(start + i * stride) % pool.size()];

        auto t0 = Clock::now();
        PD_T pd = PD_T::FromSignedPDCode(it.code.data(),
                      static_cast<Int>(it.code.size() / 5), false, true);
        auto t1 = Clock::now();

        PDC_T pdc{ std::move(pd) };
        if (reuse_reapr) { pdc.Simplify(reapr, args); } else { pdc.Simplify(args); }
        PD_T simp = pdc.ToSingleDiagram();
        auto t2 = Clock::now();

        const Int c = simp.CrossingCount();
        bool ok = simp.ValidQ() && c >= Int(3) && c <= Int(Klut::max_crossing_count);
        if (ok) { simp.template WriteMacLeodCode<CodeInt>(buf.data()); }
        auto t3 = Clock::now();

        if (ok)
        {
            auto [cc, id] = klut.FindID(buf.data(), c);
            (void)cc;
            // Correct = identified as the same knot the perturbed diagram came
            // from. A cheaper Simplify that under-reduces yields not_found or a
            // >13 diagram (ok=false) -> counted wrong, exactly what we want to see.
            if (id == it.src_id && id != Klut::not_found) { ++hits; }
        }
        auto t4 = Clock::now();

        s.construct += Secs(t0, t1);
        s.simplify  += Secs(t1, t2);
        s.canonical += Secs(t2, t3);
        s.lookup    += Secs(t3, t4);
    }
    if (correct) { correct->fetch_add(hits, std::memory_order_relaxed); }
    return s;
}

// Named Simplify presets to sweep (cost vs. correctness). Defaults are the
// knoodlesimplify path; the cheaper ones dial down the stochastic Reapr work.
struct Preset { const char* name; PDC_T::Simplify_Args_T args; bool reuse = false; };

std::vector<Preset> Presets()
{
    auto base = [] { return PDC_T::Simplify_Args_T{}; };
    const auto TOPO = PDC_T::Compaction_T::TopologicalNumbering;
    std::vector<Preset> v;
    { Preset p{"default", base(), false};                                    v.push_back(p); }
    { auto a = base(); a.rotation_trials = 1;          Preset p{"rot=1", a, false};            v.push_back(p); }
    { Preset p{"reuse-reapr", base(), true};                                 v.push_back(p); }
    { auto a = base(); a.compaction_method = TOPO;     Preset p{"topo-compaction", a, false};  v.push_back(p); }
    { auto a = base(); a.compaction_method = TOPO;     Preset p{"reuse+topo", a, true};        v.push_back(p); }
    { auto a = base(); a.compaction_method = TOPO; a.rotation_trials = 1;
                                                       Preset p{"reuse+topo+rot1", a, true};   v.push_back(p); }
    { auto a = base(); a.compaction_method = TOPO; a.rerouteQ = false;
                                                       Preset p{"reuse+topo,noreroute", a, true}; v.push_back(p); }
    // Skip the per-pass ConditionalCompress (threshold=100 => never compress a
    // <=100-crossing diagram); also try skipping the initial compress.
    { auto a = base(); a.compression_threshold = 100;  Preset p{"thresh=100", a, false}; v.push_back(p); }
    { auto a = base(); a.compression_threshold = 100; a.compress_initialQ = false;
                                                       Preset p{"thresh=100,noinit", a, false}; v.push_back(p); }
    { auto a = base(); a.compression_threshold = 100; a.compress_initialQ = false;
      a.compaction_method = TOPO;                      Preset p{"reuse+topo+thresh100,noinit", a, true}; v.push_back(p); }
    return v;
}

void ReportStages(const Stage& s, std::size_t iters, double wall)
{
    const double total = s.construct + s.simplify + s.canonical + s.lookup;
    auto line = [&](const char* name, double t) {
        std::cout << "    " << name << "  "
                  << (t / iters * 1e9) << " ns/item   "
                  << (100.0 * t / total) << " %\n";
    };
    std::cout << "  per-stage (single thread, " << iters << " items):\n";
    line("construct ", s.construct);
    line("simplify  ", s.simplify);
    line("canonical ", s.canonical);
    line("lookup    ", s.lookup);
    std::cout << "  end-to-end: " << (wall / iters * 1e9) << " ns/item   "
              << (iters / wall) << " items/s\n";
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
    std::size_t audit_n = 0;     // --audit=N: classify N outcomes (correct/wrong-knot/notfound/oor)
    Int inflate_target = 0;      // --inflate=N: inflate inputs to ~N crossings
    Int  prof_ct = 0;            // --compress-threshold=N for the breakdown/profile path
    bool prof_noinit = false;    // --no-compress-initial
    bool prof_nopermute = false; // --no-permute: deterministic pass order (permute_randomQ=false)
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
        else if (a.rfind("--audit=", 0) == 0)     audit_n = std::stoull(v("--audit="));
        else if (a.rfind("--inflate=", 0) == 0)   inflate_target = std::stoll(v("--inflate="));
        else if (a.rfind("--compress-threshold=", 0) == 0) prof_ct = std::stoll(v("--compress-threshold="));
        else if (a == "--no-compress-initial")    prof_noinit = true;
        else if (a == "--no-permute")             prof_nopermute = true;
        else if (a.rfind("--pool-file=", 0) == 0) pool_file = v("--pool-file=");
    }

    std::cout << "klut_bench: KLUT identify-path throughput\n"
              << "  klut-dir=" << klut_dir << "  c_max=" << c_max
              << "  per_c=" << per_c << "  cap=" << cap << "  iters=" << iters << "\n\n";

    // Build the table and pre-load (single-threaded) so parallel reads are safe.
    Klut klut{ std::filesystem::path(klut_dir), static_cast<Knoodle::Size_T>(c_max) };
    const auto tL0 = Clock::now();
    klut.LoadSubtables();
    const auto tL1 = Clock::now();
    std::cout << "  subtables loaded in " << Secs(tL0, tL1) << " s\n";

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

    PDC_T::Simplify_Args_T default_args{};
    default_args.compression_threshold = prof_ct;
    if (prof_noinit) { default_args.compress_initialQ = false; }
    if (prof_nopermute) { default_args.permute_randomQ = false; }

    // Profiling mode: nothing but identify chains in a tight loop, so an external
    // sampler (`sample <pid>`) sees a clean Simplify-dominated process.
    if (profile_n > 0)
    {
        std::cout << "  profile mode: " << profile_n << " chains (sample me now)\n";
        std::atomic<std::size_t> c{0};
        Stage ps = RunChain(pool, klut, profile_n, 0, 1, &c, default_args, false);
        std::cout << "  done (" << ps.simplify << " s in simplify)\n";
        return 0;
    }

    // Audit mode: classify each identify outcome. The "correct%" column lumps
    // together three very different misses:
    //   WRONG-KNOT       - valid id but a DIFFERENT knot: a real Simplify
    //                       correctness bug (must be zero).
    //   not_found (<=13)  - a pass-reduction end-state with <=13 crossings whose
    //                       key is NOT in the KLUT. The table is supposed to
    //                       contain every pass-fixpoint key (Klutter records
    //                       pass-fixpoints with embedding_trials=0), so this is a
    //                       genuine TABLE GAP, not benign under-reduction.
    //   out-of-range      - end-state reduced to >13 cx (beyond the table's range)
    //                       or <3 / invalid: outside what the KLUT can key.
    // Dumps the distinct inputs that EVER simplify to the wrong knot, and the
    // missing <=13 end-states (their keys are the table gaps), for repro.
    if (audit_n > 0)
    {
        std::array<CodeInt, Klut::max_crossing_count> buf{};
        std::size_t n_correct = 0, n_wrong = 0, n_notfound = 0, n_oor = 0, n_wrong_uniq = 0;
        std::vector<char> wrong_seen(pool.size(), 0);
        std::ofstream dump("klut_bench_wrongknot.tsv");
        std::ofstream ndump("klut_bench_notfound.tsv");   // <=13 end-states missing from KLUT
        std::size_t nf_dumped = 0; const std::size_t nf_cap = 500;
        for (std::size_t i = 0; i < audit_n; ++i)
        {
            const std::size_t idx = i % pool.size();
            const Item& it = pool[idx];
            PD_T pd = PD_T::FromSignedPDCode(it.code.data(),
                          static_cast<Int>(it.code.size() / 5), false, true);
            PDC_T pdc{ std::move(pd) };
            pdc.Simplify(default_args);
            PD_T simp = pdc.ToSingleDiagram();
            const Int c = simp.CrossingCount();
            if (!(simp.ValidQ() && c >= Int(3) && c <= Int(Klut::max_crossing_count)))
            {
                ++n_oor; continue;   // reduced to >13 (or <3 / invalid): under-reduction
            }
            simp.template WriteMacLeodCode<CodeInt>(buf.data());
            auto [cc, id] = klut.FindID(buf.data(), c);
            (void)cc;
            if (id == it.src_id) { ++n_correct; }
            else if (id == Klut::not_found || id == Klut::error || id == Klut::invalid)
            {
                // A <=13-crossing Simplify end-state whose key is NOT in the KLUT.
                // If the table covers all pass-reduction end-states, this is a gap.
                // Dump the end-state diagram (its key is the missing one) + input.
                ++n_notfound;
                if (nf_dumped < nf_cap)
                {
                    ++nf_dumped;
                    auto end_code = SignedPDCode(simp);
                    ndump << "# src_id=" << it.src_id << " c_min=" << it.c_min
                          << "  end_state_c=" << c << "  (perturbed " << it.nc << " cx)\n";
                    ndump << "# end-state (missing key) PD:\n";
                    const Int er = static_cast<Int>(end_code.size() / 5);
                    for (Int r = 0; r < er; ++r)
                        ndump << end_code[5*r] << '\t' << end_code[5*r+1] << '\t'
                              << end_code[5*r+2] << '\t' << end_code[5*r+3] << '\t'
                              << end_code[5*r+4] << '\n';
                    ndump << "# input (perturbed) PD:\n";
                    const Int ir = static_cast<Int>(it.code.size() / 5);
                    for (Int r = 0; r < ir; ++r)
                        ndump << it.code[5*r] << '\t' << it.code[5*r+1] << '\t'
                              << it.code[5*r+2] << '\t' << it.code[5*r+3] << '\t'
                              << it.code[5*r+4] << '\n';
                }
            }
            else                     // valid id, but a DIFFERENT knot: real bug
            {
                ++n_wrong;
                if (!wrong_seen[idx])
                {
                    wrong_seen[idx] = 1; ++n_wrong_uniq;
                    dump << "# src_id=" << it.src_id << " c_min=" << it.c_min
                         << "  got_id=" << id << " simp_c=" << c
                         << "  (perturbed " << it.nc << " cx)\n";
                    const Int rows = static_cast<Int>(it.code.size() / 5);
                    for (Int r = 0; r < rows; ++r)
                        dump << it.code[5*r] << '\t' << it.code[5*r+1] << '\t'
                             << it.code[5*r+2] << '\t' << it.code[5*r+3] << '\t'
                             << it.code[5*r+4] << '\n';
                }
            }
        }
        const double tot = static_cast<double>(audit_n);
        std::cout << "  audit (" << audit_n << " trials over " << pool.size()
                  << " diagrams, default args, stochastic Simplify):\n"
                  << "    correct (== source knot)        : " << n_correct
                  << "  (" << 100.0 * n_correct / tot << "%)\n"
                  << "    WRONG KNOT (valid id != source) : " << n_wrong
                  << "   <- real Simplify failures; " << n_wrong_uniq
                  << " distinct inputs -> klut_bench_wrongknot.tsv\n"
                  << "    not_found (<=13, non-minimal)   : " << n_notfound
                  << "  (missing pass-fixpoint -- TABLE GAP; -> klut_bench_notfound.tsv)\n"
                  << "    out-of-range (>13 cx / invalid) : " << n_oor
                  << "  (end-state beyond table range)\n";
        return 0;
    }

    // Single-thread: per-stage breakdown (default Simplify settings).
    std::atomic<std::size_t> correct{0};
    const auto t0 = Clock::now();
    Stage s = RunChain(pool, klut, iters, 0, 1, &correct, default_args);
    const auto t1 = Clock::now();
    ReportStages(s, iters, Secs(t0, t1));
    std::cout << "  correct-identify rate: " << (100.0 * correct.load() / iters) << " %\n\n";

    // Sweep Simplify presets: cost vs. correctness. The winner is the cheapest
    // preset that keeps correctness ~100%.
    std::cout << "  Simplify preset sweep (single thread, " << iters << " items):\n";
    std::cout << "    " << std::string(26, ' ')
              << "ns/item    items/s    correct%\n";
    for (const auto& p : Presets())
    {
        std::atomic<std::size_t> c{0};
        const auto q0 = Clock::now();
        RunChain(pool, klut, iters, 0, 1, &c, p.args, p.reuse);
        const auto q1 = Clock::now();
        const double w = Secs(q0, q1);
        char buf[160];
        std::snprintf(buf, sizeof buf, "    %-24s  %8.0f  %9.0f   %7.3f",
                      p.name, w / iters * 1e9, iters / w, 100.0 * c.load() / iters);
        std::cout << buf << "\n";
    }
    std::cout << "\n";

    // Parallel scaling.
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
                         static_cast<std::size_t>(T), &f, default_args);
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
