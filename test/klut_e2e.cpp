/**
 * @file klut_e2e.cpp
 * @brief Tier 4: end-to-end test of the real knoodlesimplify | knoodleidentify
 *        pipeline against the KLUT (Knot LookUp Table).
 *
 * Tiers 1-3 (test/klut_check.cpp) validate the table and its in-process reader.
 * This tier exercises the actual *command-line tools* a user runs:
 *
 *     reconstruct(key) -> knoodlesimplify --streaming-mode | knoodleidentify
 *
 * For a deterministic sample of keys across crossing numbers it:
 *   1. reconstructs the diagram from the MacLeod code (FromMacLeodCode),
 *   2. emits it as a knoodlesimplify-input block (5-column signed PD code,
 *      the exact format knoodlesimplify itself writes),
 *   3. runs the whole batch through the real two-stage pipeline once, and
 *   4. checks each knot's reported name against its own table entry.
 *
 * Why this is not redundant with Tiers 1-3: simplification is part of the
 * query (the table stores keys of *simplified, canonicalized* diagrams). The
 * pipeline re-embeds each diagram in 3D and reprojects (Reapr), so it generally
 * lands on a *different* minimal diagram of the same knot than the input key.
 * That diagram's key is in the table only if generation happened to capture it.
 * So this tier measures the pipeline's empirical coverage:
 *
 *   - exact name match  -> the round-trip works end to end (the common case);
 *   - <notfound:N>      -> a clean minimal diagram the table never stored
 *                          (a coverage gap, not a correctness bug; reported,
 *                          not a hard failure);
 *   - a *different* knot name, an unknot, a link, etc. -> a real identification
 *                          bug (hard failure).
 *
 * Reprojection is orientation-preserving and never mirrors, so the symmetry
 * coset is preserved too: an exact match includes the "coset" field. A
 * coset-only difference is reported separately as a soft finding.
 *
 * Self-contained: needs only the KLUT data and the two built tools. No UMFPACK,
 * no libhomfly, no external reference data. Build: tools/Makefile target
 * klut_e2e (same flags as knoodleidentify). Run from the test/ directory.
 */

#include "../Knoodle.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Int   = std::int64_t;
using PD_T  = Knoodle::PlanarDiagram<Int>;
using Clock = std::chrono::steady_clock;

namespace {

double Secs(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}

//==============================================================================
// Name parsing:  K[c, i, j, "coset"]
//==============================================================================

struct NameFields { int c = 0, i = 0, j = 0; std::string coset; bool ok = false; };

NameFields ParseName(const std::string& name)
{
    NameFields f;
    if (!name.starts_with("K[") || name.empty() || name.back() != ']') { return f; }
    const std::string in = name.substr(2, name.size() - 3);
    std::size_t c1 = in.find(','),
                c2 = c1 == std::string::npos ? c1 : in.find(',', c1 + 1),
                c3 = c2 == std::string::npos ? c2 : in.find(',', c2 + 1);
    if (c3 == std::string::npos) { return f; }
    try {
        f.c = std::stoi(in.substr(0, c1));
        f.i = std::stoi(in.substr(c1 + 1, c2 - c1 - 1));
        f.j = std::stoi(in.substr(c2 + 1, c3 - c2 - 1));
    } catch (...) { return f; }
    f.coset = in.substr(c3 + 1);
    f.ok = true;
    return f;
}

/// Same knot (c,i,j) ignoring the coset?
bool SameKnot(const NameFields& a, const NameFields& b)
{
    return a.ok && b.ok && a.c == b.c && a.i == b.i && a.j == b.j;
}

//==============================================================================
// Shell quoting
//==============================================================================

std::string Shq(const std::string& s)
{
    std::string out = "'";
    for (char ch : s) { out += (ch == '\'') ? std::string("'\\''") : std::string(1, ch); }
    out += "'";
    return out;
}

//==============================================================================
// Sampling: choose `per` evenly-spaced global key indices in [0, total).
//==============================================================================

std::vector<long> SampleIndices(long total, long per, long seed)
{
    std::vector<long> idx;
    if (total <= 0) { return idx; }
    if (per >= total)
    {
        for (long k = 0; k < total; ++k) { idx.push_back(k); }
        return idx;
    }
    // Evenly spaced; `seed` shifts the phase within a stride so different runs
    // can sample different keys without an RNG.
    const long stride_num = total;            // pick at k*total/per + phase
    long last = -1;
    for (long k = 0; k < per; ++k)
    {
        long g = (k * stride_num) / per + (seed % std::max<long>(1, total / per));
        if (g >= total) { g = total - 1; }
        if (g <= last) { g = last + 1; }       // keep strictly increasing
        if (g >= total) { break; }
        idx.push_back(g);
        last = g;
    }
    return idx;
}

//==============================================================================
// Per-crossing emission
//==============================================================================

struct Stats
{
    long sampled       = 0;
    long recon_fail    = 0;   // key didn't reconstruct to a knot (hard)
    long exact         = 0;   // identified as its own table name (incl. coset)
    long coset_only    = 0;   // right knot (c,i,j), different coset (soft finding)
    long notfound      = 0;   // <notfound:N> -- coverage gap (soft)
    long wrong_knot    = 0;   // a different K[...] knot (hard)
    long became_unknot = 0;   // over-simplified to the unknot (hard)
    long other_anomaly = 0;   // link / unidentified / invalid / multi-summand (hard)
    long missing_line  = 0;   // pipeline produced fewer output lines than knots (hard)
};

/// Append the sampled diagrams for crossing number `c` to the input file, and
/// record their expected names in `expected`. Returns false on a fatal I/O
/// problem (missing data files are non-fatal: that crossing is skipped).
bool EmitCrossing(const std::string& dir, int c, long per, long seed,
                  std::ostream& input, std::vector<std::string>& expected,
                  Stats& st)
{
    const std::string cc = (c < 10 ? "0" : "") + std::to_string(c);
    std::ifstream vstream(dir + "/Klut_Values_" + cc + ".tsv");
    std::ifstream kstream(dir + "/Klut_Keys_" + cc + ".bin", std::ios::binary);
    if (!vstream || !kstream)
    {
        std::cout << "  c=" << c << ": (data missing, skipping)\n";
        return true;
    }

    // Read the value file: name + cumulative key index, so a global key index
    // maps back to the name whose bucket contains it.
    std::vector<std::string> names;
    std::vector<long>        cum;      // cum[k] = first global index of names[k]
    long total = 0;
    std::string name;
    long count;
    while (vstream >> name >> count)
    {
        names.push_back(name);
        cum.push_back(total);
        total += count;
    }
    if (total == 0)
    {
        std::cout << "  c=" << c << ": (empty value file, skipping)\n";
        return true;
    }

    const std::vector<long> idx = SampleIndices(total, per, seed);

    std::vector<Knoodle::UInt8> key(static_cast<std::size_t>(c));
    std::size_t name_cursor = 0;
    long emitted = 0;

    for (long g : idx)
    {
        // Advance the name cursor to the bucket containing global index g.
        while (name_cursor + 1 < cum.size() && cum[name_cursor + 1] <= g) { ++name_cursor; }
        const std::string& this_name = names[name_cursor];

        kstream.seekg(static_cast<std::streamoff>(g) * c, std::ios::beg);
        if (!kstream.read(reinterpret_cast<char*>(key.data()), c))
        {
            std::cout << "  c=" << c << ": key read failed at index " << g << "\n";
            return false;
        }

        PD_T pd = PD_T::FromMacLeodCode(key.data(), Int(c), Int(0));
        if (!pd.ValidQ() || pd.LinkComponentCount() != Int(1))
        {
            ++st.recon_fail;
            std::cout << "  FAIL c=" << c << " name=" << this_name
                      << ": key did not reconstruct to a knot.\n";
            continue;
        }

        // Emit one knot block in knoodlesimplify's own output format: a 'k'
        // marker (the first is treated as optional by the reader), an 's'
        // summand marker, then one 5-column signed-PD row per crossing.
        input << "k\ns\n";
        auto pd_code = pd.template PDCode<Int, {.signQ = true, .colorQ = false}>();
        const Int n = pd.CrossingCount();
        for (Int x = 0; x < n; ++x)
        {
            input << pd_code(x, 0);
            for (Int col = 1; col < 5; ++col) { input << '\t' << pd_code(x, col); }
            input << '\n';
        }

        expected.push_back(this_name);
        ++emitted;
        ++st.sampled;
    }

    std::cout << "  c=" << c << ": sampled " << emitted << " of " << total << " keys\n";
    return true;
}

//==============================================================================
// Output comparison
//==============================================================================

void Compare(const std::vector<std::string>& expected,
             const std::string& out_path, Stats& st)
{
    std::ifstream out(out_path);
    std::string line;
    std::size_t i = 0;

    for (; i < expected.size() && std::getline(out, line); ++i)
    {
        // Trim trailing CR/space.
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        const std::string& want = expected[i];

        if (line == want) { ++st.exact; continue; }

        if (line == "Unknot")
        {
            ++st.became_unknot;
            if (st.became_unknot <= 20)
                std::cout << "  FAIL: " << want << " simplified to the Unknot.\n";
            continue;
        }
        if (line.rfind("<notfound:", 0) == 0)
        {
            ++st.notfound;
            continue;   // soft: coverage gap
        }
        if (line.find('#') != std::string::npos ||
            line.rfind("<link:", 0) == 0 ||
            line.rfind("<unidentified:", 0) == 0 ||
            line == "<invalid>")
        {
            ++st.other_anomaly;
            if (st.other_anomaly <= 20)
                std::cout << "  FAIL: " << want << " -> '" << line << "' (anomalous).\n";
            continue;
        }

        // A different K[...] name: same knot but other coset, or a wrong knot.
        NameFields w = ParseName(want), g = ParseName(line);
        if (SameKnot(w, g))
        {
            ++st.coset_only;
            if (st.coset_only <= 20)
                std::cout << "  NOTE: " << want << " -> '" << line
                          << "' (same knot, different coset).\n";
        }
        else
        {
            ++st.wrong_knot;
            if (st.wrong_knot <= 20)
                std::cout << "  FAIL: " << want << " -> '" << line
                          << "' (WRONG KNOT).\n";
        }
    }

    // Any expected knots with no corresponding output line.
    if (i < expected.size())
    {
        st.missing_line += static_cast<long>(expected.size() - i);
        std::cout << "  FAIL: pipeline produced " << i << " output lines for "
                  << expected.size() << " input knots.\n";
    }
}

} // namespace

int main(int argc, char* argv[])
{
    std::string dir       = "../data/Klut";
    std::string simplify  = "../tools/knoodlesimplify";
    std::string identify  = "../tools/knoodleidentify";
    int  from_c = 3, to_c = 13;
    long per    = 100;
    long seed   = 0;
    bool keep_temp = false;

    for (int k = 1; k < argc; ++k)
    {
        std::string a(argv[k]);
        if      (a.rfind("--data-dir=", 0) == 0)       dir = a.substr(11);
        else if (a.rfind("--simplify=", 0) == 0)       simplify = a.substr(11);
        else if (a.rfind("--identify=", 0) == 0)       identify = a.substr(11);
        else if (a.rfind("--per-crossing=", 0) == 0)   per = std::stol(a.substr(15));
        else if (a.rfind("--from-crossing=", 0) == 0)  from_c = std::stoi(a.substr(16));
        else if (a.rfind("--up-to-crossing=", 0) == 0) to_c = std::stoi(a.substr(17));
        else if (a.rfind("--seed=", 0) == 0)           seed = std::stol(a.substr(7));
        else if (a == "--keep-temp")                   keep_temp = true;
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "klut_e2e -- Tier 4 end-to-end pipeline test.\n\n"
                "Samples KLUT keys, reconstructs each diagram, and runs the batch\n"
                "through the real knoodlesimplify | knoodleidentify pipeline,\n"
                "checking each knot is identified as its own table name.\n\n"
                "  --data-dir=PATH       KLUT data dir (default ../data/Klut)\n"
                "  --simplify=PATH       knoodlesimplify binary (default ../tools/...)\n"
                "  --identify=PATH       knoodleidentify binary (default ../tools/...)\n"
                "  --per-crossing=N      keys sampled per crossing number (default 100)\n"
                "  --from-crossing=N     first crossing number (default 3)\n"
                "  --up-to-crossing=N    last crossing number (default 13)\n"
                "  --seed=N              shift which keys are sampled (default 0)\n"
                "  --keep-temp           do not delete the temp input/output files\n";
            return 0;
        }
        else { std::cerr << "unknown option: " << a << "\n"; return 2; }
    }

    std::cout << "=== KLUT end-to-end pipeline (c=" << from_c << ".." << to_c
              << ", " << per << " keys/crossing) ===\n";

    const std::string in_path  = "klut_e2e_input.tsv";
    const std::string out_path = "klut_e2e_output.txt";
    const std::string sim_log  = "klut_e2e_simplify.log";
    const std::string id_log   = "klut_e2e_identify.log";

    std::vector<std::string> expected;
    Stats st;

    const auto t0 = Clock::now();
    {
        std::ofstream input(in_path);
        if (!input) { std::cerr << "cannot write " << in_path << "\n"; return 2; }
        for (int c = from_c; c <= to_c; ++c)
            if (!EmitCrossing(dir, c, per, seed, input, expected, st)) { return 2; }
    }
    std::cout << "  emitted " << expected.size() << " knots ["
              << Secs(t0, Clock::now()) << "s]\n";

    if (expected.empty()) { std::cerr << "no keys sampled\n"; return 2; }

    // Resolve the data dir to an absolute path for the identify stage, since it
    // resolves relative paths against the tool's own location, not our cwd.
    std::error_code ec;
    std::string abs_dir = std::filesystem::weakly_canonical(dir, ec).string();
    if (ec || abs_dir.empty()) { abs_dir = dir; }

    const std::string cmd =
        Shq(simplify) + " --streaming-mode --quiet < " + Shq(in_path) +
        " 2> " + Shq(sim_log) + " | " +
        Shq(identify) + " --expanded --quiet --data-dir=" + Shq(abs_dir) +
        " > " + Shq(out_path) + " 2> " + Shq(id_log);

    std::cout << "  running pipeline...\n";
    const auto p0 = Clock::now();
    const int rc = std::system(cmd.c_str());
    const auto p1 = Clock::now();
    if (rc != 0)
        std::cout << "  (pipeline exit code " << rc
                  << "; see " << sim_log << " / " << id_log << ")\n";

    Compare(expected, out_path, st);
    std::cout << "  pipeline + compare [" << Secs(p0, p1) << "s]\n";

    if (!keep_temp)
    {
        std::error_code rm;
        for (const std::string& f : {in_path, out_path, sim_log, id_log,
                                     std::string("knoodlesimplify.log")})
            std::filesystem::remove(f, rm);
    }

    const long hard = st.recon_fail + st.wrong_knot + st.became_unknot
                    + st.other_anomaly + st.missing_line;
    const double pct = st.sampled ? 100.0 * double(st.exact) / double(st.sampled) : 0.0;

    std::cout << "\nklut_e2e: " << st.sampled << " keys sampled\n"
              << "  exact name match      : " << st.exact
              << "  (" << pct << "% round-tripped)\n"
              << "  same knot, other coset: " << st.coset_only
              << "  (soft; reprojection should preserve coset)\n"
              << "  not found in table    : " << st.notfound
              << "  (coverage gap; not a hard failure)\n"
              << "  reconstruction failures: " << st.recon_fail << "\n"
              << "  WRONG KNOT            : " << st.wrong_knot << "\n"
              << "  over-simplified to unknot: " << st.became_unknot << "\n"
              << "  other anomalies       : " << st.other_anomaly << "\n"
              << "  missing output lines  : " << st.missing_line << "\n";
    std::cout << (hard == 0 ? "PASS: every sampled knot identified as itself or "
                              "flagged as a coverage gap.\n"
                            : (std::to_string(hard) + " hard failure(s).\n"));
    return hard == 0 ? 0 : 1;
}
