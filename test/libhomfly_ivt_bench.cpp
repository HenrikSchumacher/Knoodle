/**
 * @file libhomfly_ivt_bench.cpp
 * @brief Invariant-throughput driver #2 of 3 (libhomfly). Reads the frozen
 *        benchmark dataset (make_bench_set), and for each diagram times:
 *          construct  - build a PD_T from the stored signed PD code
 *          compute    - encode to Jenkins code and run libhomfly (via the shared
 *                       oracle HomflyOfPossiblySplit) on the RAW diagram -- no
 *                       simplification, so cost tracks the DIAGRAM crossing count
 *        and emits one per-diagram result row (bench_io schema):
 *          index  nc  construct_ns  compute_ns  result_key
 *
 * result_key is the HOMFLY polynomial in a canonical, parseable form:
 *   "l,m,coef;l,m,coef;..."  (terms in std::map order; empty coeffs dropped)
 * libhomfly's (L,M) are term-for-term identical to Regina's homflyLM(l,m), so the
 * Regina cross-check (Phase 4) parses this back into a regina.Laurent2 and
 * compares. The construct/compute split mirrors klut_ivt_bench for fairness.
 *
 * Build: test/Makefile target libhomfly_ivt_bench (links the vendored libhomfly
 * objects, like homfly_check / plantri_check).
 */

#include "bench_io.hpp"
#include "homfly_invariance.hpp"   // KnoodleJenkins, HomflyOfPossiblySplit, Polynomial, BuildPD

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using Clock = std::chrono::steady_clock;

namespace {

double NsSince(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double, std::nano>(b - a).count();
}

// Canonical, parseable serialization of a (L,M)->coef polynomial. Polynomial is a
// std::map, so iteration is already in canonical (l,m) order.
std::string PolyKey(const Polynomial& p)
{
    std::string s;
    for (const auto& [lm, c] : p)
    {
        if (c == 0) { continue; }
        s += std::to_string(lm.first) + ',' + std::to_string(lm.second)
           + ',' + std::to_string(c) + ';';
    }
    return s.empty() ? std::string("0") : s;
}

} // namespace

int main(int argc, char* argv[])
{
    std::string dataset_path, out_path;
    std::size_t warmup = 1000;
    std::size_t limit  = 0;

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        auto v = [&](const std::string& p) { return a.substr(p.size()); };
        if      (a.rfind("--dataset=", 0) == 0) dataset_path = v("--dataset=");
        else if (a.rfind("--out=", 0) == 0)     out_path = v("--out=");
        else if (a.rfind("--warmup=", 0) == 0)  warmup = std::stoull(v("--warmup="));
        else if (a.rfind("--limit=", 0) == 0)   limit = std::stoull(v("--limit="));
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "libhomfly_ivt_bench: libhomfly invariant-throughput driver.\n\n"
                "  --dataset=PATH     frozen benchmark dataset (required)\n"
                "  --out=PATH         per-diagram result TSV (default: stdout)\n"
                "  --warmup=N         untimed warmup computations (default 1000)\n"
                "  --limit=N          process only first N diagrams (0 = all)\n";
            return 0;
        }
    }
    if (dataset_path.empty()) { std::cerr << "error: --dataset=PATH required\n"; return 2; }

    bench_io::DataSet ds;
    try { ds = bench_io::Load(dataset_path); }
    catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 2; }
    std::size_t n = ds.diagrams.size();
    if (limit && limit < n) { n = limit; }
    std::cerr << "libhomfly_ivt_bench: " << n << " diagrams from " << dataset_path << "\n";

    auto homfly_one = [&](const bench_io::Diagram& d, double& construct_ns,
                          double& compute_ns) -> std::string
    {
        const Int rows = static_cast<Int>(d.pd.size() / 5);
        auto t0 = Clock::now();
        PD_T pd = BuildPD(d.pd, rows);
        auto t1 = Clock::now();
        const std::string jenkins = KnoodleJenkins(pd);
        bool ok = true;
        Polynomial poly = HomflyOfPossiblySplit(jenkins, ok);
        auto t2 = Clock::now();
        construct_ns = NsSince(t0, t1);
        compute_ns   = NsSince(t1, t2);
        return ok ? PolyKey(poly) : std::string("MALFORMED");
    };

    for (std::size_t i = 0; i < warmup && n > 0; ++i)
    {
        double c, k;
        homfly_one(ds.diagrams[i % n], c, k);
    }

    std::ofstream fout;
    std::ostream* os = &std::cout;
    if (!out_path.empty()) { fout.open(out_path); if (!fout) { std::cerr << "error: cannot open " << out_path << "\n"; return 2; } os = &fout; }
    *os << bench_io::ResultHeader() << "\n";

    double total_compute = 0;
    const auto W0 = Clock::now();
    for (std::size_t i = 0; i < n; ++i)
    {
        const bench_io::Diagram& d = ds.diagrams[i];
        double construct_ns = 0, compute_ns = 0;
        const std::string key = homfly_one(d, construct_ns, compute_ns);
        total_compute += compute_ns;
        *os << i << '\t' << d.nc << '\t' << construct_ns << '\t'
            << compute_ns << '\t' << key << '\n';
    }
    const auto W1 = Clock::now();
    const double wall = std::chrono::duration<double>(W1 - W0).count();

    std::cerr << "libhomfly_ivt_bench: homfly " << (total_compute / n) << " ns/item (mean), "
              << (n / (total_compute * 1e-9)) << " items/s (compute-only); "
              << "wall " << wall << " s\n";
    return 0;
}
