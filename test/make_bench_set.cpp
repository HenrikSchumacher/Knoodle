/**
 * @file make_bench_set.cpp
 * @brief Build a frozen, reproducible benchmark dataset of KNOT diagrams for the
 *        invariant-throughput comparison (KLUT vs libhomfly vs Regina).
 *
 * Firehose model, done reproducibly:
 *   For each crossing number n in [c-min, c-max] (plantri vertex count V = n+2):
 *     1. Stream every quadrangulation out of the vendored `plantri` (PlantriStream)
 *        and seeded RESERVOIR-sample (Algorithm R) `reservoir-mult * per-crossing`
 *        RAW graphs -- a uniform sample over the shadows at n, with no dependence on
 *        plantri's internal enumeration order (so neither "first N" nor a res/mod
 *        slice, both of which are structurally biased). Dualization is DEFERRED:
 *        QuadToShadow is std::map-heavy and the n=13 stream is ~1.4e9 graphs, so
 *        the hot path is just a read + one PRNG draw per graph.
 *     2. Dualize the survivors (QuadToShadow) and apply the KNOTS-ONLY filter: keep
 *        a shadow iff it is single-component. Component count is a property of the
 *        shadow alone (the strand tracing goes straight through each crossing,
 *        independent of over/under), so AssignAndTrace(sh, 0) reports it with no
 *        sign choice. Keep up to `per-crossing` knot-shadows (we over-sample raw
 *        graphs because ~half of them dualize to multi-component links).
 *     3. For each retained shadow, draw `masks-per-shadow` iid Bernoulli(1/2)
 *        over/under masks from the same seeded PRNG -> AssignAndTrace -> 5-column
 *        signed PD code.
 *
 * plantri's output for a fixed V (no res/mod) is deterministic, so the whole run
 * is reproducible from (plantri binary, seed): same file, byte for byte. Nothing
 * here touches Reapr, so unlike the klut_bench pool this set is seed-reproducible.
 *
 * Output (text): a provenance header, then `count N`, then N lines, each
 *   nc  x0 x1 x2 x3 s0  x0 x1 x2 x3 s1  ...        (nc crossings, 5 ints each)
 * matching the 5-column signed PD lingua franca (PD_T::FromSignedPDCode, Regina
 * regina_pd, libhomfly via KnoodleJenkins). Parsed by bench_io.hpp / bench_io.py.
 *
 * Build: test/Makefile target make_bench_set (only needs plantri_gen.hpp + the
 * vendored plantri binary at runtime; no Knoodle core, no libhomfly).
 */

#include "plantri_gen.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

std::uint64_t SplitMix64(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// A retained diagram: its crossing count and flattened 5-col signed PD code.
struct Diagram { int nc; std::vector<Int> pd; };

} // namespace

int main(int argc, char* argv[])
{
    std::string plantri = "vendor/plantri/plantri";
    std::string mode    = "everything";   // simple | no-r1 | everything
    std::string out_path;
    int  c_min = 3, c_max = 13;
    std::size_t per_crossing    = 20000;  // target knot-shadows kept per crossing number
    std::size_t reservoir_mult  = 3;      // raw-graph reservoir = mult * per_crossing (covers link drop)
    std::size_t masks_per_shadow = 1;     // iid sign assignments per retained shadow
    std::uint64_t seed = 20260701ULL;

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        auto val = [&](const std::string& p) { return a.substr(p.size()); };
        if      (a.rfind("--plantri=", 0) == 0)          plantri = val("--plantri=");
        else if (a.rfind("--plantri-mode=", 0) == 0)     mode = val("--plantri-mode=");
        else if (a.rfind("--out=", 0) == 0)              out_path = val("--out=");
        else if (a.rfind("--from-crossing=", 0) == 0)    c_min = std::stoi(val("--from-crossing="));
        else if (a.rfind("--up-to-crossing=", 0) == 0)   c_max = std::stoi(val("--up-to-crossing="));
        else if (a.rfind("--per-crossing=", 0) == 0)     per_crossing = std::stoull(val("--per-crossing="));
        else if (a.rfind("--reservoir-mult=", 0) == 0)   reservoir_mult = std::stoull(val("--reservoir-mult="));
        else if (a.rfind("--masks-per-shadow=", 0) == 0) masks_per_shadow = std::stoull(val("--masks-per-shadow="));
        else if (a.rfind("--seed=", 0) == 0)             seed = std::stoull(val("--seed="));
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "make_bench_set: build a frozen, reproducible knot-diagram dataset.\n\n"
                "  --out=PATH                  output dataset file (required)\n"
                "  --from-crossing=N           smallest crossing number (default 3)\n"
                "  --up-to-crossing=N          largest crossing number (default 13)\n"
                "  --per-crossing=K            target knot-shadows kept per crossing number (default 20000)\n"
                "  --reservoir-mult=M          raw-graph reservoir = M*K, covers the link drop (default 3)\n"
                "  --masks-per-shadow=M        iid sign assignments per shadow (default 1)\n"
                "  --seed=N                    master PRNG seed (default 20260701)\n"
                "  --plantri-mode=MODE         simple | no-r1 | everything (default everything)\n"
                "  --plantri=PATH              plantri binary (default vendor/plantri/plantri)\n\n"
                "Streams plantri shadows, keeps single-component (knot) shadows, uniformly\n"
                "reservoir-samples K per crossing number, and assigns iid Bernoulli(1/2)\n"
                "over/under masks. Reproducible from (plantri binary, seed).\n";
            return 0;
        }
    }

    if (out_path.empty()) { std::cerr << "error: --out=PATH is required (see --help)\n"; return 2; }

    // "simple" quadrangulations exist only for even V >= 8 (=> even n >= 6); the
    // other modes start at V=4 (n=2). Mirror plantri_check's V range.
    const int v_start = (mode == "simple") ? 8 : 4;

    std::vector<Diagram> dataset;
    std::cerr << "make_bench_set: mode=" << mode << " c=" << c_min << ".." << c_max
              << " per_crossing=" << per_crossing << " masks/shadow=" << masks_per_shadow
              << " seed=" << seed << "\n";

    for (int V = v_start; V <= c_max + 2; ++V)
    {
        const int n = V - 2;
        if (n < c_min || n > c_max) { continue; }

        PlantriStream stream;
        if (!stream.Open(plantri, mode, V, 0, 0))
        {
            std::cerr << "  V=" << V << " (" << n << "cx): plantri stream unavailable, skipping\n";
            continue;
        }

        // Per-crossing-number PRNG: one deterministic stream for BOTH reservoir
        // decisions and (afterwards) the sign masks. Reproducible because plantri's
        // output order for a fixed V is fixed.
        std::mt19937_64 rng(SplitMix64(seed + static_cast<std::uint64_t>(n)));

        // Reservoir-sample RAW graphs uniformly over the ENTIRE plantri stream
        // (Algorithm R), then dualize + knots-only filter ONLY the survivors. The
        // n=13 stream is ~1.4e9 graphs, and QuadToShadow is std::map-heavy, so we
        // must keep it off the hot path: the per-graph cost here is just a read +
        // one PRNG draw. We over-sample by `reservoir_mult` because ~half of the
        // survivors will be links (dropped by the knots-only filter).
        const std::size_t cap_res = reservoir_mult * per_crossing;
        std::vector<Graph> reservoir;
        reservoir.reserve(cap_res);
        std::size_t graphs = 0;

        Graph g;
        while (stream.Next(g))
        {
            if (reservoir.size() < cap_res)
            {
                reservoir.push_back(g);
            }
            else
            {
                std::uniform_int_distribution<std::size_t> pick(0, graphs);  // item index = graphs
                const std::size_t j = pick(rng);
                if (j < cap_res) { reservoir[j] = g; }
            }
            ++graphs;
        }
        stream.Close();

        // Dualize survivors; keep single-component (knot) shadows up to per_crossing
        // and assign iid Bernoulli(1/2) over/under masks. Reservoir-slot order is
        // uncorrelated with knot type, so the first per_crossing knots are a uniform
        // sample of knot-shadows.
        const std::uint64_t mask_hi =
            (n >= 63) ? ~0ULL : ((std::uint64_t(1) << n) - 1);
        std::uniform_int_distribution<std::uint64_t> mask_dist(0, mask_hi);

        std::size_t knot_shadows = 0, written = 0;
        for (const Graph& gg : reservoir)
        {
            if (knot_shadows >= per_crossing) { break; }
            Shadow sh = QuadToShadow(gg);
            if (!sh.ok || static_cast<int>(sh.crossings.size()) != n) { continue; }
            Traced probe = AssignAndTrace(sh, 0);   // component count is sign-independent
            if (!probe.ok || probe.n_components != 1) { continue; }
            ++knot_shadows;
            for (std::size_t m = 0; m < masks_per_shadow; ++m)
            {
                Traced tr = AssignAndTrace(sh, mask_dist(rng));
                if (!tr.ok) { continue; }
                dataset.push_back(Diagram{ n, std::move(tr.pd) });
                ++written;
            }
        }
        if (knot_shadows < per_crossing && graphs > cap_res)
        {
            std::cerr << "  V=" << V << " (" << n << "cx): NOTE only " << knot_shadows
                      << " knot-shadows from a full reservoir (raise --reservoir-mult "
                         "for the full " << per_crossing << ")\n";
        }

        std::cerr << "  V=" << V << " (" << n << "cx): " << graphs << " graphs scanned, "
                  << reservoir.size() << " in reservoir, " << knot_shadows
                  << " knot-shadows kept, " << written << " diagrams\n";
    }

    std::ofstream out(out_path);
    if (!out) { std::cerr << "error: cannot open output " << out_path << "\n"; return 2; }
    out << "# knoodle_bench_set v1\n"
        << "# seed=" << seed << " mode=" << mode
        << " c_min=" << c_min << " c_max=" << c_max
        << " per_crossing=" << per_crossing
        << " masks_per_shadow=" << masks_per_shadow << "\n"
        << "# firehose: uniform reservoir sample of single-component (knot) plantri"
           " shadows, iid Bernoulli(1/2) signs\n"
        << "count " << dataset.size() << "\n";
    for (const Diagram& d : dataset)
    {
        out << d.nc;
        for (Int v : d.pd) { out << '\t' << v; }
        out << '\n';
    }
    if (!out) { std::cerr << "error: write failed on " << out_path << "\n"; return 2; }

    std::cerr << "make_bench_set: wrote " << dataset.size() << " diagrams to " << out_path << "\n";
    return 0;
}
