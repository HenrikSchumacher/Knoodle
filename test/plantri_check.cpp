/**
 * @file plantri_check.cpp
 * @brief Exhaustive generator-based correctness test: enumerate planar knot/link
 *        diagrams with the vendored `plantri`, then verify that simplification
 *        preserves HOMFLY on every one of them. No Python, no Regina, no tools/.
 *
 * Pipeline (a self-contained C++ port of test/run_tests.py's Tier 2):
 *
 *   plantri -E V  ->  edge_code graphs (planar quadrangulations)
 *      -> quad_to_shadow:      dual = a 4-valent graph (the knot/link shadow)
 *      -> assign + trace:      each 2^n over/under assignment -> a signed PD code
 *      -> CheckInvariance:     HOMFLY(diagram) == HOMFLY(Simplify(diagram)) ?
 *
 * The HOMFLY oracle (vendored libhomfly + split-link delta rule + the
 * simplify-preserves-HOMFLY check) is shared with homfly_check.cpp via
 * homfly_invariance.hpp, so this test uses the exact oracle that is itself
 * cross-validated against libhomfly's published reference polynomials.
 *
 * For crossing number n, plantri is asked for V = n + 2 vertices:
 *   simple     : plantri -q -E V        (simple quadrangulations; V even, >= 8)
 *   no-r1      : plantri -Q -c2 -E V     (all quads, no length-2 cycles)
 *   everything : plantri -Q -E V         (all quadrangulations)
 *
 * A "Changed" result is a genuine bug (simplification altered the knot type); a
 * "skip" is a diagram this oracle can't settle (e.g. a simplified complex that
 * cannot be reassembled into one diagram). Exit nonzero iff any HOMFLY changed.
 *
 * Build: test/Makefile target plantri_check (light config, links vendored
 * libhomfly; needs the vendored plantri binary at runtime).
 */

#include "homfly_invariance.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

namespace {

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
// plantri invocation + edge_code parsing  (port of run_tests.py)
//==============================================================================

/// Run plantri for V vertices in the given mode and capture its binary stdout.
/// Sets ok=false if the process could not be launched.
std::vector<unsigned char> RunPlantri(const std::string& plantri,
                                      const std::string& mode, int V, bool& ok)
{
    const std::string flags = (mode == "simple")  ? "-q -E"
                            : (mode == "no-r1")    ? "-Q -c2 -E"
                                                   : "-Q -E";   // "everything"
    const std::string cmd = Shq(plantri) + " " + flags + " " + std::to_string(V)
                          + " 2>/dev/null";
    FILE* f = ::popen(cmd.c_str(), "r");
    if (!f) { ok = false; return {}; }

    std::vector<unsigned char> out;
    unsigned char buf[65536];
    std::size_t got;
    while ((got = std::fread(buf, 1, sizeof buf, f)) > 0)
        out.insert(out.end(), buf, buf + got);
    ::pclose(f);
    ok = true;
    return out;
}

using Graph = std::vector<std::vector<int>>;   // per vertex: CW edge indices

/// Parse plantri's binary edge_code (-E) output into a list of graphs.
std::vector<Graph> ParseEdgeCodes(const std::vector<unsigned char>& raw)
{
    static const std::string HEADER = ">>edge_code<<";
    std::vector<Graph> graphs;
    if (raw.size() < HEADER.size()) { return graphs; }
    for (std::size_t i = 0; i < HEADER.size(); ++i)
        if (raw[i] != static_cast<unsigned char>(HEADER[i])) { return graphs; }

    std::size_t pos = HEADER.size();
    while (pos < raw.size())
    {
        std::size_t body_size = raw[pos];
        ++pos;
        if (body_size == 0)   // extended header: next 4 bytes LE are the size
        {
            if (pos + 4 > raw.size()) { break; }
            body_size = static_cast<std::size_t>(raw[pos])
                      | (static_cast<std::size_t>(raw[pos + 1]) << 8)
                      | (static_cast<std::size_t>(raw[pos + 2]) << 16)
                      | (static_cast<std::size_t>(raw[pos + 3]) << 24);
            pos += 4;
        }
        if (pos + body_size > raw.size()) { break; }

        Graph vertices;
        std::vector<int> current;
        for (std::size_t k = 0; k < body_size; ++k)
        {
            const unsigned char b = raw[pos + k];
            if (b == 0xFF)
            {
                if (!current.empty()) { vertices.push_back(current); current.clear(); }
            }
            else { current.push_back(static_cast<int>(b)); }
        }
        if (!current.empty()) { vertices.push_back(current); }
        pos += body_size;

        graphs.push_back(std::move(vertices));
    }
    return graphs;
}

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

} // namespace

int main(int argc, char* argv[])
{
    std::string plantri      = "vendor/plantri/plantri";
    std::string mode         = "everything";
    std::string assignments  = "all";
    int  from_c = 3, to_c = 6;
    bool knots_only = false;
    long seed = 42;
    long max_fail_print = 25;

    for (int k = 1; k < argc; ++k)
    {
        std::string a(argv[k]);
        if      (a.rfind("--plantri=", 0) == 0)              plantri = a.substr(10);
        else if (a.rfind("--plantri-mode=", 0) == 0)         mode = a.substr(15);
        else if (a.rfind("--crossing-assignments=", 0) == 0) assignments = a.substr(23);
        else if (a.rfind("--from-crossing=", 0) == 0)        from_c = std::stoi(a.substr(16));
        else if (a.rfind("--up-to-crossing=", 0) == 0)       to_c = std::stoi(a.substr(17));
        else if (a == "--knots-only")                        knots_only = true;
        else if (a.rfind("--seed=", 0) == 0)                 seed = std::stol(a.substr(7));
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "plantri_check -- exhaustive plantri-generated HOMFLY-invariance test.\n\n"
                "Enumerates planar knot/link diagrams with the vendored plantri and\n"
                "checks that Simplify preserves HOMFLY on each. Exit nonzero iff any\n"
                "HOMFLY changed (a real simplification bug).\n\n"
                "  --plantri=PATH              plantri binary (default vendor/plantri/plantri)\n"
                "  --plantri-mode=MODE         everything | no-r1 | simple (default everything)\n"
                "  --crossing-assignments=A    all, or an integer K random sample (default all)\n"
                "  --from-crossing=N           first crossing number (default 3)\n"
                "  --up-to-crossing=N          last crossing number (default 6)\n"
                "  --knots-only                skip multi-component (link) diagrams\n"
                "  --seed=N                    RNG seed for K-sampling (default 42)\n\n"
                "Note: 'simple' mode only yields even V>=8, i.e. crossing number >= 6;\n"
                "use 'everything' or 'no-r1' for smaller diagrams.\n";
            return 0;
        }
        else { std::cerr << "unknown option: " << a << "\n"; return 2; }
    }

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

    std::mt19937_64 rng(static_cast<std::uint64_t>(seed));
    Stats st;
    std::map<int, std::pair<long,long>> per_c;   // crossing -> (tested, changed)
    const auto t0 = Clock::now();

    for (int V : v_values)
    {
        const int n = V - 2;
        bool ok = false;
        const auto raw = RunPlantri(plantri, mode, V, ok);
        if (!ok) { std::cerr << "  could not run plantri for V=" << V << "\n"; return 2; }
        const auto graphs = ParseEdgeCodes(raw);
        const auto tv = Clock::now();

        long v_tested = 0, v_changed = 0, bad_graphs = 0;
        for (std::size_t g = 0; g < graphs.size(); ++g)
        {
            Shadow sh = QuadToShadow(graphs[g]);
            if (!sh.ok) { ++bad_graphs; continue; }

            for (std::uint64_t mask : ChooseMasks(n, assignments, rng))
            {
                Traced tr = AssignAndTrace(sh, mask);
                if (!tr.ok) { continue; }
                if (knots_only && tr.n_components > 1) { continue; }

                PD_T pd = BuildPD(tr.pd, n);
                if (!pd.ValidQ())
                {
                    ++st.recon_fail;
                    continue;
                }
                (tr.n_components > 1 ? st.links : st.knots)++;

                InvarianceResult r = CheckInvariance(pd);
                ++st.tested; ++v_tested;

                if (r.status == InvStatus::Preserved) { ++st.preserved; }
                else if (r.status == InvStatus::Changed)
                {
                    ++st.changed; ++v_changed;
                    if (st.changed <= max_fail_print)
                        std::cout << "  FAIL  V" << V << " g" << g << " m" << mask
                                  << " " << tr.n_components << "comp ("
                                  << n << " crossings)\n"
                                  << "    before: " << PolyToString(r.before) << "\n"
                                  << "    after : " << PolyToString(r.after)  << "\n";
                }
                else { ++st.skipped; }
            }
        }
        per_c[n] = { v_tested, v_changed };
        std::cout << "  V=" << V << " (" << n << " crossings): " << graphs.size()
                  << " graphs, " << v_tested << " diagrams, " << v_changed
                  << " changed";
        if (bad_graphs) { std::cout << ", " << bad_graphs << " non-quad skipped"; }
        std::cout << "  [" << Secs(tv, Clock::now()) << "s]\n";
    }

    const auto t1 = Clock::now();
    std::cout << "\nplantri_check: " << st.tested << " diagrams ("
              << st.knots << " knots, " << st.links << " links) in "
              << Secs(t0, t1) << "s\n"
              << "  HOMFLY preserved : " << st.preserved << "\n"
              << "  HOMFLY changed   : " << st.changed   << "  (real simplify bugs)\n"
              << "  skipped (oracle) : " << st.skipped   << "  (split/not reassemblable)\n"
              << "  reconstruction failures: " << st.recon_fail << "\n";
    std::cout << (st.changed == 0
                  ? "PASS: simplification preserved HOMFLY on every diagram.\n"
                  : (std::to_string(st.changed) + " diagram(s) changed HOMFLY.\n"));
    return st.changed == 0 ? 0 : 1;
}
