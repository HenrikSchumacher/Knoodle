/**
 * @file bench_io.hpp
 * @brief Shared reader for the frozen benchmark dataset (make_bench_set output)
 *        and the common per-diagram result-row schema used by the C++ drivers
 *        (klut_ivt_bench, libhomfly_ivt_bench). A matching Python reader lives in
 *        bench_io.py for the Regina driver.
 *
 * Dataset format (text):
 *   # ... provenance comment lines (seed, mode, crossing range) ...
 *   count N
 *   nc  x0 x1 x2 x3 s0  x0 x1 x2 x3 s1  ...      (N lines; nc crossings, 5 ints each)
 *
 * Result-row schema (each driver emits one TSV, joined across engines by `index`):
 *   index  nc  construct_ns  compute_ns  result_key
 * See ResultHeader(). result_key is an engine-specific canonical token: for KLUT
 * the reduced-knot label "c:id"; for libhomfly a parseable HOMFLY term string
 * "l,m,coeff;..." (also consumed by the Regina cross-check).
 */

#pragma once

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bench_io {

using Int = std::int64_t;

struct Diagram
{
    int              nc = 0;   // crossing count
    std::vector<Int> pd;       // 5*nc flattened signed PD (x0,x1,x2,x3,sign per crossing)
};

struct DataSet
{
    std::vector<std::string> provenance;  // '#' header lines
    std::vector<Diagram>     diagrams;
};

/// Load a dataset file. Throws std::runtime_error on open/parse failure.
inline DataSet Load(const std::string& path)
{
    DataSet ds;
    std::ifstream in(path);
    if (!in) { throw std::runtime_error("bench_io::Load: cannot open " + path); }

    // Accepts one or more "count N" blocks, so per-crossing files concatenated
    // with `cat` load as a single dataset (indices run globally across blocks).
    std::string line;
    std::size_t expected = 0;
    bool any_count = false;
    while (std::getline(in, line))
    {
        if (line.empty()) { continue; }
        if (line[0] == '#') { ds.provenance.push_back(line); continue; }

        std::istringstream ss(line);
        std::string tok;
        ss >> tok;
        if (tok == "count")
        {
            std::size_t block = 0;
            if (!(ss >> block)) { throw std::runtime_error("bench_io::Load: bad 'count' line in " + path); }
            expected += block;
            any_count = true;
            ds.diagrams.reserve(expected);
            continue;
        }

        Diagram d;
        try { d.nc = std::stoi(tok); }
        catch (...) { throw std::runtime_error("bench_io::Load: expected 'count N' or a diagram in " + path); }
        if (d.nc <= 0) { throw std::runtime_error("bench_io::Load: bad crossing count in " + path); }
        d.pd.resize(static_cast<std::size_t>(5 * d.nc));
        for (auto& v : d.pd) { ss >> v; }
        if (!ss) { throw std::runtime_error("bench_io::Load: malformed diagram line in " + path); }
        ds.diagrams.push_back(std::move(d));
    }
    if (any_count && ds.diagrams.size() != expected)
    {
        throw std::runtime_error("bench_io::Load: diagram count mismatch in " + path);
    }
    return ds;
}

/// TSV header line for a driver's per-diagram result file (see file comment).
inline const char* ResultHeader()
{
    return "# index\tnc\tconstruct_ns\tcompute_ns\tresult_key";
}

} // namespace bench_io
