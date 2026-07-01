/**
 * @file klut_ivt_bench.cpp
 * @brief Invariant-throughput driver #1 of 3 (KLUT). Reads the frozen benchmark
 *        dataset (make_bench_set), and for each diagram times:
 *          construct  - build a PD_T from the stored signed PD code (+ wrap in PDC)
 *          identify   - ki::IdentifyInto: pass-only Simplify, table lookup, Reapr
 *                       escalation on a non-minimal <=13 miss
 *        and emits one per-diagram result row (bench_io schema):
 *          index  nc  construct_ns  compute_ns  result_key
 *
 * This driver is ALSO the labeling oracle: result_key = "<reduced_c>:<label>",
 * whose leading integer is the identified knot's reduced crossing number. The
 * report (Phase 4) joins the three engines' rows by `index` and bins every
 * engine's timing by THIS reduced crossing number -- so the random firehose is
 * reported stratified by true knot complexity without a separate labeling pass.
 *
 * Single-threaded (apples-to-apples with the HOMFLY drivers). One Reapr + scratch
 * PDCs + result are reused across diagrams (the realistic reentrant path).
 *
 * Build: test/Makefile target klut_ivt_bench (light config, like plantri_check --
 * ki::Identify's Reapr uses TopologicalNumbering + MCF, no UMFPACK/Alexander).
 */

#include "../Knoodle.hpp"
#include "../tools/klut_identify.hpp"
#include "bench_io.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using Real    = double;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, float>;
using Klut    = Knoodle::Klut;
using Clock   = std::chrono::steady_clock;
namespace ki  = klut_identify;

namespace {

double NsSince(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double, std::nano>(b - a).count();
}

// Render an IdentifyResult as "<reduced_c>:<label>". The leading integer is the
// reduced crossing number used to bin all three engines' timings in the report.
std::string ResultKey(const ki::IdentifyResult& R)
{
    if (R.status == ki::IdentifyResult::Status::LinkOutOfScope) { return "-1:LINK"; }
    if (R.summands.empty()) { return "0:U"; }   // reduced to the unknot

    Int reduced_c = 0;
    std::string label;
    for (std::size_t i = 0; i < R.summands.size(); ++i)
    {
        const auto& s = R.summands[i];
        reduced_c += s.crossings;
        if (i) { label += '+'; }
        switch (s.kind)
        {
            case ki::Summand::Kind::Identified:
                label += std::to_string(s.crossings) + "_id"
                       + std::to_string(static_cast<long long>(s.id));
                break;
            case ki::Summand::Kind::Unidentified:
                label += std::to_string(s.crossings) + "_UNID";
                break;
            case ki::Summand::Kind::Error:
                label += std::to_string(s.crossings) + "_ERR";
                break;
        }
    }
    return std::to_string(static_cast<long long>(reduced_c)) + ":" + label;
}

} // namespace

int main(int argc, char* argv[])
{
    std::string dataset_path;
    std::string out_path;
    std::string klut_dir = "../data/Klut";
    std::size_t warmup = 1000;
    std::size_t limit  = 0;             // 0 = all
    ki::IdentifyParams q{};             // n0=1, cap=2, rot=5, max_cx=13 defaults

    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        auto v = [&](const std::string& p) { return a.substr(p.size()); };
        if      (a.rfind("--dataset=", 0) == 0)  dataset_path = v("--dataset=");
        else if (a.rfind("--out=", 0) == 0)      out_path = v("--out=");
        else if (a.rfind("--klut-dir=", 0) == 0) klut_dir = v("--klut-dir=");
        else if (a.rfind("--warmup=", 0) == 0)   warmup = std::stoull(v("--warmup="));
        else if (a.rfind("--limit=", 0) == 0)    limit = std::stoull(v("--limit="));
        else if (a.rfind("--n0=", 0) == 0)       q.n0 = static_cast<ki::Size_T>(std::stoull(v("--n0=")));
        else if (a.rfind("--cap=", 0) == 0)      q.cap = static_cast<ki::Size_T>(std::stoull(v("--cap=")));
        else if (a.rfind("--rot=", 0) == 0)      q.rot = static_cast<ki::Size_T>(std::stoull(v("--rot=")));
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "klut_ivt_bench: KLUT invariant-throughput driver.\n\n"
                "  --dataset=PATH     frozen benchmark dataset (required)\n"
                "  --out=PATH         per-diagram result TSV (default: stdout)\n"
                "  --klut-dir=PATH    KLUT table dir (default ../data/Klut)\n"
                "  --warmup=N         untimed warmup identifies (default 1000)\n"
                "  --limit=N          process only first N diagrams (0 = all)\n"
                "  --n0/--cap/--rot   ki::Identify escalation schedule\n";
            return 0;
        }
    }
    if (dataset_path.empty()) { std::cerr << "error: --dataset=PATH required\n"; return 2; }

    bench_io::DataSet ds;
    try { ds = bench_io::Load(dataset_path); }
    catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 2; }
    std::size_t n = ds.diagrams.size();
    if (limit && limit < n) { n = limit; }
    std::cerr << "klut_ivt_bench: " << n << " diagrams from " << dataset_path << "\n";

    Klut klut{ std::filesystem::path(klut_dir), static_cast<Knoodle::Size_T>(q.max_cx) };
    klut.LoadSubtables();

    Reapr_T reapr{};
    PDC_T work, temp;
    ki::IdentifyResult R;

    auto identify_one = [&](const bench_io::Diagram& d, double& construct_ns,
                            double& compute_ns) -> std::string
    {
        const Int rows = static_cast<Int>(d.pd.size() / 5);
        auto t0 = Clock::now();
        work.Clear();
        work.Push(PD_T::FromSignedPDCode(d.pd.data(), rows, false, true));
        auto t1 = Clock::now();
        ki::IdentifyInto(klut, work, temp, reapr, R, q);
        auto t2 = Clock::now();
        construct_ns = NsSince(t0, t1);
        compute_ns   = NsSince(t1, t2);
        return ResultKey(R);
    };

    // Warmup: touch the table / allocator / branch predictors before timing.
    for (std::size_t i = 0; i < warmup && n > 0; ++i)
    {
        double c, k;
        identify_one(ds.diagrams[i % n], c, k);
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
        const std::string key = identify_one(d, construct_ns, compute_ns);
        total_compute += compute_ns;
        *os << i << '\t' << d.nc << '\t' << construct_ns << '\t'
            << compute_ns << '\t' << key << '\n';
    }
    const auto W1 = Clock::now();
    const double wall = std::chrono::duration<double>(W1 - W0).count();

    std::cerr << "klut_ivt_bench: identify " << (total_compute / n) << " ns/item (mean), "
              << (n / (total_compute * 1e-9)) << " items/s (identify-only); "
              << "wall " << wall << " s\n";
    return 0;
}
