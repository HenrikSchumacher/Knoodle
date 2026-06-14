/**
 * @file plantri_check.cpp
 * @brief Exhaustive generator-based correctness test: enumerate planar knot/link
 *        diagrams with the vendored `plantri`, then verify that simplification
 *        preserves two invariants on every one. No Python, no Regina, no tools/.
 *
 * Pipeline (a self-contained C++ port of test/run_tests.py's Tier 2):
 *
 *   plantri -E V  ->  edge_code graphs (planar quadrangulations)
 *      -> quad_to_shadow:   dual = a 4-valent graph (the knot/link shadow)
 *      -> assign + trace:   each 2^n over/under assignment -> a signed PD code
 *      -> Simplify, then check two invariants of the result:
 *           (1) link-component count (cheap, HOMFLY-free): ColorCount must not
 *               change;
 *           (2) per-sublink HOMFLY: colors are persistent component labels, so
 *               for EVERY color subset the corresponding sublink's HOMFLY must
 *               be preserved (the full subset is the whole-link check). With
 *               --no-homfly only (1) runs, for fast high-crossing torture runs.
 *
 * The HOMFLY oracle (vendored libhomfly + split-link delta rule) and the
 * component/sublink helpers live in homfly_invariance.hpp, shared with
 * homfly_check.cpp, so this test uses the exact oracle that is itself
 * cross-validated against libhomfly's published reference polynomials.
 *
 * For crossing number n, plantri is asked for V = n + 2 vertices:
 *   simple     : plantri -q -E V        (simple quadrangulations; V even, >= 8)
 *   no-r1      : plantri -Q -c2 -E V     (all quads, no length-2 cycles)
 *   everything : plantri -Q -E V         (all quadrangulations)
 *
 * Any change in component count or a sublink HOMFLY is a genuine simplification
 * bug; exit is nonzero iff one is found. plantri output is streamed (bounded
 * memory at any V); Ctrl-C stops early with a summary.
 *
 * Build: test/Makefile target plantri_check (light config, links vendored
 * libhomfly; needs the vendored plantri binary at runtime).
 */

#include "homfly_invariance.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

namespace {

// Set by SIGINT so an all-night run can be stopped with Ctrl-C and still print
// its summary. A volatile sig_atomic_t is the only state a handler may touch.
volatile std::sig_atomic_t g_stop = 0;
extern "C" void OnSigint(int) { g_stop = 1; }

double Secs(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}

std::string Shq(const std::string& s)
{
    std::string out = "'";
    for (char ch : s) { out += (ch == '\'') ? std::string("'\\''") : std::string(1, ch); }
    out += "'";
    return out;
}

//==============================================================================
// plantri invocation + streaming edge_code parser  (port of run_tests.py)
//==============================================================================

using Graph = std::vector<std::vector<int>>;   // per vertex: CW edge indices

/// Streams graphs out of `plantri -E` one at a time, so memory stays bounded no
/// matter how many graphs a high vertex count produces (V=15 yields hundreds of
/// millions — buffering the whole stdout would OOM). Reads records directly off
/// the pipe: a 1-byte body size (or 0 then a 4-byte little-endian size), then
/// that many bytes of vertex sections separated by 0xFF.
class PlantriStream
{
public:
    ~PlantriStream() { Close(); }

    bool Open(const std::string& plantri, const std::string& mode, int V)
    {
        const std::string flags = (mode == "simple")  ? "-q -E"
                                : (mode == "no-r1")    ? "-Q -c2 -E"
                                                       : "-Q -E";   // "everything"
        const std::string cmd = Shq(plantri) + " " + flags + " " + std::to_string(V)
                              + " 2>/dev/null";
        f_ = ::popen(cmd.c_str(), "r");
        if (!f_) { return false; }

        static const std::string HEADER = ">>edge_code<<";
        std::vector<char> hb(HEADER.size());
        if (std::fread(hb.data(), 1, HEADER.size(), f_) != HEADER.size()) { return false; }
        return std::equal(HEADER.begin(), HEADER.end(), hb.begin());
    }

    /// Read the next graph into `g`. Returns false at end of stream.
    bool Next(Graph& g)
    {
        int c = std::fgetc(f_);
        if (c == EOF) { return false; }
        std::size_t body = static_cast<std::size_t>(c);
        if (body == 0)   // extended header: 4-byte little-endian size
        {
            unsigned char b4[4];
            if (std::fread(b4, 1, 4, f_) != 4) { return false; }
            body = static_cast<std::size_t>(b4[0])
                 | (static_cast<std::size_t>(b4[1]) << 8)
                 | (static_cast<std::size_t>(b4[2]) << 16)
                 | (static_cast<std::size_t>(b4[3]) << 24);
        }
        std::vector<unsigned char> buf(body);
        if (body && std::fread(buf.data(), 1, body, f_) != body) { return false; }

        g.clear();
        std::vector<int> cur;
        for (std::size_t k = 0; k < body; ++k)
        {
            const unsigned char b = buf[k];
            if (b == 0xFF) { if (!cur.empty()) { g.push_back(cur); cur.clear(); } }
            else { cur.push_back(static_cast<int>(b)); }
        }
        if (!cur.empty()) { g.push_back(cur); }
        return true;
    }

    // Stop early: closing the pipe sends plantri SIGPIPE on its next write.
    void Close() { if (f_) { ::pclose(f_); f_ = nullptr; } }

private:
    FILE* f_ = nullptr;
};

//==============================================================================
// quadrangulation -> knot-shadow (dual)  (port of quad_to_shadow)
//==============================================================================

using Crossing = std::array<int,4>;            // CW half-edge ids at a crossing
using Dart     = std::pair<int,int>;           // (vertex, position)

struct Shadow
{
    std::vector<Crossing> crossings;
    std::map<int,int>     partner;             // half-edge -> its partner
    bool                  ok = false;
};

Shadow QuadToShadow(const Graph& vertices)
{
    Shadow sh;
    const std::size_t nv = vertices.size();

    // Each edge index appears twice; record its two (vertex, position) darts.
    std::map<int, std::vector<Dart>> edge_to_darts;
    for (std::size_t v = 0; v < nv; ++v)
        for (std::size_t p = 0; p < vertices[v].size(); ++p)
            edge_to_darts[vertices[v][p]].push_back({static_cast<int>(v),
                                                     static_cast<int>(p)});

    // Next dart in the same face: from (v0,p0) along edge e to (v1,p1), step to
    // the CW predecessor at v1.
    std::map<Dart, Dart> dart_next;
    for (const auto& [e, darts] : edge_to_darts)
    {
        if (darts.size() != 2) { return sh; }   // malformed -> skip graph
        const auto [v0, p0] = darts[0];
        const auto [v1, p1] = darts[1];
        const int deg1 = static_cast<int>(vertices[v1].size());
        const int deg0 = static_cast<int>(vertices[v0].size());
        dart_next[{v0, p0}] = {v1, (p1 - 1 + deg1) % deg1};
        dart_next[{v1, p1}] = {v0, (p0 - 1 + deg0) % deg0};
    }

    // Trace faces; each must have exactly 4 sides (quadrangulation).
    std::set<Dart> visited;
    std::vector<std::vector<Dart>> faces;
    for (std::size_t v = 0; v < nv; ++v)
        for (std::size_t p = 0; p < vertices[v].size(); ++p)
        {
            Dart d0{static_cast<int>(v), static_cast<int>(p)};
            if (visited.count(d0)) { continue; }
            std::vector<Dart> face;
            Dart d = d0;
            while (!visited.count(d))
            {
                visited.insert(d);
                face.push_back(d);
                auto it = dart_next.find(d);
                if (it == dart_next.end()) { return sh; }
                d = it->second;
            }
            if (face.size() != 4) { return sh; }   // not a quadrangulation
            faces.push_back(std::move(face));
        }

    // Dual: each face is a crossing; half-edge id = 4*f + i. Two half-edges that
    // border the same primal edge are partners.
    std::map<int, std::vector<int>> edge_to_halfedges;
    sh.crossings.reserve(faces.size());
    for (std::size_t f = 0; f < faces.size(); ++f)
    {
        const int base = 4 * static_cast<int>(f);
        sh.crossings.push_back({base, base + 1, base + 2, base + 3});
        for (int i = 0; i < 4; ++i)
        {
            const auto [v, p] = faces[f][static_cast<std::size_t>(i)];
            const int primal_edge = vertices[v][p];
            edge_to_halfedges[primal_edge].push_back(base + i);
        }
    }
    for (const auto& [e, hes] : edge_to_halfedges)
    {
        if (hes.size() != 2) { return sh; }
        sh.partner[hes[0]] = hes[1];
        sh.partner[hes[1]] = hes[0];
    }

    sh.ok = true;
    return sh;
}

//==============================================================================
// crossing assignment + strand tracing -> signed PD code (port)
//==============================================================================

struct Traced { std::vector<Int> pd; int n_components = 0; bool ok = false; };

Traced AssignAndTrace(const Shadow& sh, std::uint64_t mask)
{
    Traced out;
    const int n = static_cast<int>(sh.crossings.size());

    std::map<int, std::pair<int,int>> he_to_crossing;   // half-edge -> (crossing,pos)
    for (int c = 0; c < n; ++c)
        for (int p = 0; p < 4; ++p)
            he_to_crossing[sh.crossings[static_cast<std::size_t>(c)][static_cast<std::size_t>(p)]] = {c, p};

    auto next_halfedge = [&](int h) -> int
    {
        const auto [c, p] = he_to_crossing.at(h);
        const int exit_he = sh.crossings[static_cast<std::size_t>(c)][static_cast<std::size_t>((p + 2) % 4)];
        return sh.partner.at(exit_he);
    };

    std::set<int> all_he;
    for (const auto& cr : sh.crossings) for (int h : cr) { all_he.insert(h); }

    std::map<std::pair<int,int>, int>  arc_at;
    std::map<std::pair<int,int>, bool> is_arrival;
    std::set<int> visited;
    int arc_count = 0;
    int n_components = 0;

    for (int start : all_he)
    {
        if (visited.count(start)) { continue; }
        ++n_components;

        std::vector<int> path;
        int h = start;
        while (!visited.count(h))
        {
            visited.insert(h);
            visited.insert(sh.partner.at(h));
            path.push_back(h);
            h = next_halfedge(h);
        }

        const int k = static_cast<int>(path.size());
        const int base = arc_count;
        for (int i = 0; i < k; ++i)
        {
            const auto [c_idx, p] = he_to_crossing.at(path[static_cast<std::size_t>(i)]);
            const int exit_pos = (p + 2) % 4;
            arc_at[{c_idx, p}]        = base + ((i - 1 + k) % k);
            is_arrival[{c_idx, p}]    = true;
            arc_at[{c_idx, exit_pos}] = base + i;
            is_arrival[{c_idx, exit_pos}] = false;
        }
        arc_count += k;
    }

    auto arrival = [&](int c, int p) -> bool
    {
        auto it = is_arrival.find({c, p});
        return it != is_arrival.end() && it->second;
    };
    auto arc = [&](int c, int p) -> int
    {
        auto it = arc_at.find({c, p});
        if (it == arc_at.end()) { out.ok = false; return 0; }
        return it->second;
    };

    out.ok = true;
    out.pd.reserve(static_cast<std::size_t>(5 * n));
    for (int c = 0; c < n; ++c)
    {
        const int p_a = arrival(c, 0) ? 0 : 2;     // strand A occupies {0,2}
        const int p_b = arrival(c, 1) ? 1 : 3;     // strand B occupies {1,3}
        const int bit = static_cast<int>((mask >> c) & 1ULL);
        const int p_u = (bit == 0) ? p_a : p_b;    // under-strand arrival
        const int p_o = (bit == 0) ? p_b : p_a;    // over-strand arrival
        const int sign = (p_o == (p_u + 1) % 4) ? 1 : -1;
        const int ccw[4] = { p_u, (p_u + 3) % 4, (p_u + 2) % 4, (p_u + 1) % 4 };
        for (int q : ccw) { out.pd.push_back(arc(c, q)); }
        out.pd.push_back(sign);
    }
    out.n_components = n_components;
    return out;
}

//==============================================================================
// Driver
//==============================================================================

struct Stats
{
    long tested = 0, preserved = 0, changed = 0, skipped = 0, recon_fail = 0;
    long knots = 0, links = 0;
    long changed_knots = 0, changed_links = 0;   // HOMFLY changed (knot/link)
    long comp_changed = 0;                        // link-component count changed
};

/// Masks to test for an n-crossing shadow: all 2^n, or a random K-sample.
std::vector<std::uint64_t> ChooseMasks(int n, const std::string& assignments,
                                       std::mt19937_64& rng)
{
    const std::uint64_t total = (n >= 63) ? 0 : (std::uint64_t(1) << n);
    std::vector<std::uint64_t> masks;
    if (assignments == "all" || total == 0)
    {
        for (std::uint64_t m = 0; m < total; ++m) { masks.push_back(m); }
        return masks;
    }
    const std::uint64_t k = std::strtoull(assignments.c_str(), nullptr, 10);
    if (k >= total)
    {
        for (std::uint64_t m = 0; m < total; ++m) { masks.push_back(m); }
        return masks;
    }
    std::set<std::uint64_t> chosen;
    std::uniform_int_distribution<std::uint64_t> dist(0, total - 1);
    while (chosen.size() < k) { chosen.insert(dist(rng)); }
    masks.assign(chosen.begin(), chosen.end());
    return masks;
}

/// Write a changed diagram's 5-column signed PD code to its own file in `dir`
/// (named by V/graph/mask/components) — a reproducer directly consumable by
/// `homfly_check --invariance <dir>/*.tsv` or by knoodlesimplify.
void DumpChanged(const std::string& dir, const std::vector<Int>& pd, int n,
                 int V, long g, std::uint64_t mask, int ncomp)
{
    const std::string path = dir + "/V" + std::to_string(V) + "_g" + std::to_string(g)
                           + "_m" + std::to_string(mask) + "_" + std::to_string(ncomp)
                           + "comp.tsv";
    std::ofstream f(path);
    if (!f) { return; }
    for (int c = 0; c < n; ++c)
        for (int j = 0; j < 5; ++j)
            f << pd[static_cast<std::size_t>(5 * c + j)] << (j < 4 ? '\t' : '\n');
}

} // namespace

int main(int argc, char* argv[])
{
    std::string plantri      = "vendor/plantri/plantri";
    std::string mode         = "everything";
    std::string assignments  = "all";
    int  from_c = 3, to_c = 6;
    bool knots_only = false;
    bool do_homfly = true;          // --no-homfly => component-count check only (fast)
    long seed = 42;
    long max_fail_print = 25;
    long max_diagrams = 0;          // global cap (0 = unlimited)
    long max_graphs   = 0;          // per-vertex-count graph cap (0 = unlimited)
    long progress_every = 200000;   // emit a progress line every N diagrams
    std::string dump_changed;       // file to dump changed diagrams' PD codes to

    for (int k = 1; k < argc; ++k)
    {
        std::string a(argv[k]);
        if      (a.rfind("--plantri=", 0) == 0)              plantri = a.substr(10);
        else if (a.rfind("--plantri-mode=", 0) == 0)         mode = a.substr(15);
        else if (a.rfind("--crossing-assignments=", 0) == 0) assignments = a.substr(23);
        else if (a.rfind("--from-crossing=", 0) == 0)        from_c = std::stoi(a.substr(16));
        else if (a.rfind("--up-to-crossing=", 0) == 0)       to_c = std::stoi(a.substr(17));
        else if (a == "--knots-only")                        knots_only = true;
        else if (a == "--no-homfly")                         do_homfly = false;
        else if (a.rfind("--seed=", 0) == 0)                 seed = std::stol(a.substr(7));
        else if (a.rfind("--max-diagrams=", 0) == 0)         max_diagrams = std::stol(a.substr(15));
        else if (a.rfind("--max-graphs=", 0) == 0)           max_graphs = std::stol(a.substr(13));
        else if (a.rfind("--progress-every=", 0) == 0)       progress_every = std::stol(a.substr(17));
        else if (a.rfind("--dump-changed=", 0) == 0)         dump_changed = a.substr(15);
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "plantri_check -- exhaustive plantri-generated simplification test.\n\n"
                "Enumerates planar knot/link diagrams with the vendored plantri and\n"
                "checks that Simplify preserves two invariants on each: the link-\n"
                "component count (cheap, HOMFLY-free) and the HOMFLY polynomial.\n"
                "Exit nonzero iff either changed. plantri output is streamed, so\n"
                "memory stays bounded at any crossing number; Ctrl-C stops early and\n"
                "still prints a summary.\n\n"
                "  --plantri=PATH              plantri binary (default vendor/plantri/plantri)\n"
                "  --plantri-mode=MODE         everything | no-r1 | simple (default everything)\n"
                "  --crossing-assignments=A    all, or an integer K random sample (default all)\n"
                "  --from-crossing=N           first crossing number (default 3)\n"
                "  --up-to-crossing=N          last crossing number (default 6)\n"
                "  --no-homfly                 component-count check only (fast; for high cx)\n"
                "  --knots-only                skip multi-component (link) diagrams\n"
                "  --max-diagrams=N            stop after N diagrams total (0 = unlimited)\n"
                "  --max-graphs=N              cap graphs per crossing number (0 = unlimited)\n"
                "  --progress-every=N          progress line every N diagrams (default 200000)\n"
                "  --dump-changed=DIR          write each changed diagram's PD code into DIR\n"
                "                              (one .tsv per diagram; feed to homfly_check)\n"
                "  --seed=N                    RNG seed for K-sampling (default 42)\n\n"
                "All-night torture test at 13 crossings (component check only is far\n"
                "faster than HOMFLY there; sample signs and cap the work):\n"
                "  ./plantri_check --from-crossing=13 --up-to-crossing=13 \\\n"
                "                  --crossing-assignments=64 --no-homfly --max-diagrams=50000000\n"
                "Note: 'simple' mode only yields even V>=8 (crossing number >= 6);\n"
                "use 'everything'/'no-r1' for smaller diagrams.\n";
            return 0;
        }
        else { std::cerr << "unknown option: " << a << "\n"; return 2; }
    }

    std::signal(SIGINT, OnSigint);   // Ctrl-C -> graceful stop with a summary

    std::cout << "=== plantri_check (mode=" << mode << ", c=" << from_c << ".."
              << to_c << ", assignments=" << assignments
              << (knots_only ? ", knots-only" : "") << ") ===\n";

    // Vertex counts: simple mode needs even V>=8; others V>=4. V = n + 2.
    std::vector<int> v_values;
    for (int V = (mode == "simple" ? 8 : 4); V <= to_c + 2; ++V)
    {
        const int n = V - 2;
        if (n < from_c || n > to_c) { continue; }
        if (mode == "simple" && (V % 2 != 0)) { continue; }
        v_values.push_back(V);
    }
    if (v_values.empty())
    {
        std::cout << "  (no vertex counts in range for this mode)\n";
        if (mode == "simple")
            std::cout << "  simple mode starts at 6 crossings; try --plantri-mode=everything\n";
        return 0;
    }

    if (!dump_changed.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(dump_changed, ec);
        if (ec) { std::cerr << "cannot create dir " << dump_changed << "\n"; return 2; }
    }
    long dumped = 0;
    const long dump_cap = 2000;   // avoid flooding on a huge torture run

    std::mt19937_64 rng(static_cast<std::uint64_t>(seed));
    Stats st;
    std::map<int, std::pair<long,long>> per_c;   // crossing -> (tested, changed)
    const auto t0 = Clock::now();

    bool stopped = false;   // diagram budget hit, or Ctrl-C
    for (int V : v_values)
    {
        if (stopped) { break; }
        const int n = V - 2;
        PlantriStream ps;
        if (!ps.Open(plantri, mode, V))
        { std::cerr << "  could not run plantri for V=" << V << "\n"; return 2; }
        const auto tv = Clock::now();

        long v_tested = 0, v_changed = 0, bad_graphs = 0, g_idx = 0;
        Graph g;
        while (ps.Next(g))
        {
            if (g_stop) { stopped = true; break; }
            if (max_graphs && g_idx >= max_graphs) { break; }
            const long this_g = g_idx++;

            Shadow sh = QuadToShadow(g);
            if (!sh.ok) { ++bad_graphs; continue; }

            for (std::uint64_t mask : ChooseMasks(n, assignments, rng))
            {
                if (g_stop) { stopped = true; break; }
                Traced tr = AssignAndTrace(sh, mask);
                if (!tr.ok) { continue; }
                const bool is_link = (tr.n_components > 1);
                if (knots_only && is_link) { continue; }

                PD_T pd = BuildPD(tr.pd, n);
                if (!pd.ValidQ()) { ++st.recon_fail; continue; }
                (is_link ? st.links : st.knots)++;
                ++st.tested; ++v_tested;

                // One Simplify, two invariants:
                //  (1) component count (cheap, HOMFLY-free) — Simplify must not
                //      change the number of link components (colors);
                //  (2) per-sublink HOMFLY (optional) — colors are persistent
                //      labels, so EVERY color-subset sublink must keep its HOMFLY
                //      (the full subset is the whole-link check). Strictly
                //      stronger than checking only the whole link.
                const Int comp_before = pd.ColorCount();   // = link-component count
                SimplifyMeasure m = SimplifyAndMeasure(pd, do_homfly);
                const bool comp_bug = (comp_before != m.comp_after);

                bool homfly_bug = false;
                if (do_homfly)
                {
                    if (m.reassembly_failed) { ++st.skipped; }
                    else
                    {
                        const std::set<Int> cset = DiagramColors(pd);
                        const std::vector<Int> cols(cset.begin(), cset.end());
                        const int k = static_cast<int>(cols.size());
                        bool skip = false;
                        const Polynomial unknot{ {{0,0},1} };
                        if (m.reduced_to_unknot)
                        {
                            bool okb = true;                 // whole link must be the unknot
                            if (SublinkHomfly(pd, cols, okb) != unknot) { homfly_bug = true; }
                            if (!okb) { skip = true; }
                        }
                        else if (k >= 1 && k <= 16)
                        {
                            for (std::uint32_t bits = 1;
                                 bits < (std::uint32_t(1) << k) && !homfly_bug && !skip; ++bits)
                            {
                                std::vector<Int> subset;
                                for (int i = 0; i < k; ++i)
                                    if (bits & (std::uint32_t(1) << i)) subset.push_back(cols[i]);
                                bool okb = true, oka = true;
                                Polynomial hb = SublinkHomfly(pd, subset, okb);
                                Polynomial ha = SublinkHomfly(m.single_after, subset, oka);
                                if (!okb || !oka) { skip = true; }
                                else if (hb != ha) { homfly_bug = true; }
                            }
                        }
                        else { skip = true; }   // too many components to enumerate

                        if (skip) { ++st.skipped; }
                        else if (homfly_bug)
                        { ++st.changed; (is_link ? st.changed_links : st.changed_knots)++; }
                    }
                }
                if (comp_bug) { ++st.comp_changed; }
                if (!comp_bug && !homfly_bug) { ++st.preserved; }

                if (comp_bug || homfly_bug)
                {
                    ++v_changed;
                    if (!dump_changed.empty() && dumped < dump_cap)
                    { DumpChanged(dump_changed, tr.pd, n, V, this_g, mask, tr.n_components); ++dumped; }
                    if (st.comp_changed + st.changed <= max_fail_print)
                    {
                        std::cout << "  BUG  V" << V << " g" << this_g << " m" << mask << " "
                                  << tr.n_components << "comp (" << n << "cx):";
                        if (comp_bug)
                            std::cout << " components " << comp_before << "->" << m.comp_after << ";";
                        if (homfly_bug) std::cout << " a sublink's HOMFLY changed;";
                        std::cout << "\n";
                    }
                }

                if (progress_every && st.tested % progress_every == 0)
                    std::cerr << "  ... V" << V << " g" << this_g << " | " << st.tested
                              << " diagrams, " << st.comp_changed << " comp / "
                              << st.changed << " HOMFLY changed ["
                              << Secs(t0, Clock::now()) << "s]\n";

                if (max_diagrams && st.tested >= max_diagrams) { stopped = true; break; }
            }
            if (stopped) { break; }
        }
        ps.Close();   // SIGPIPEs plantri if we stopped early
        per_c[n] = { v_tested, v_changed };
        std::cout << "  V=" << V << " (" << n << " crossings): " << g_idx
                  << " graphs, " << v_tested << " diagrams, " << v_changed
                  << " changed";
        if (bad_graphs) { std::cout << ", " << bad_graphs << " non-quad skipped"; }
        std::cout << "  [" << Secs(tv, Clock::now()) << "s]\n";
    }

    const auto t1 = Clock::now();
    if (stopped)
        std::cout << (g_stop ? "\n(interrupted by Ctrl-C — partial results below)\n"
                             : "\n(stopped at --max-diagrams budget)\n");
    std::cout << "\nplantri_check: " << st.tested << " diagrams ("
              << st.knots << " knots, " << st.links << " links) in "
              << Secs(t0, t1) << "s\n"
              << "  component count changed : " << st.comp_changed
              << "  (Simplify dropped/added a link component)\n";
    if (do_homfly)
        std::cout << "  HOMFLY changed (knot)   : " << st.changed_knots << "\n"
                  << "  HOMFLY changed (link)   : " << st.changed_links << "\n"
                  << "  skipped (HOMFLY)        : " << st.skipped << "  (not computable)\n";
    else
        std::cout << "  (HOMFLY check skipped — component-count check only)\n";
    std::cout << "  reconstruction failures : " << st.recon_fail << "\n";

    // ToSingleDiagram reassembly is faithful for split links (verified), so any
    // change — component count or HOMFLY — is a genuine simplification bug.
    const long bugs = st.comp_changed + st.changed;
    if (bugs == 0)
        std::cout << "PASS: simplification preserved every invariant checked.\n";
    else
        std::cout << "FAIL: " << bugs << " bug(s) — " << st.comp_changed
                  << " component-count, " << st.changed << " HOMFLY.\n";
    return bugs == 0 ? 0 : 1;
}
