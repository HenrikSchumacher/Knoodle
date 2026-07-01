/**
 * @file plantri_gen.hpp
 * @brief Shared knot-diagram generator primitives: stream shadows out of the
 *        vendored `plantri`, dualize each quadrangulation to a 4-valent knot/link
 *        shadow, and turn an over/under mask into a 5-column signed PD code.
 *
 * Extracted verbatim from plantri_check.cpp so that test/driver *and* the
 * benchmark-set generator (make_bench_set.cpp) share ONE implementation of the
 *   plantri -E V  ->  QuadToShadow (dual)  ->  AssignAndTrace(mask)  ->  signed PD
 * pipeline. plantri_check.cpp remains the regression guard: it exercises this
 * exact code path exhaustively, so any behavioural drift here is caught there.
 *
 * Header-only, internal linkage (anonymous namespace), like homfly_invariance.hpp:
 * include into ONE translation unit each (the tests/tools are separate binaries).
 *
 * Note on component count: AssignAndTrace traces "straight through" every
 * crossing (position p -> p+2), which is a property of the SHADOW alone and is
 * independent of the over/under `mask`. So Traced::n_components computed with any
 * mask (e.g. mask 0) is the diagram's link-component count -- the cheap
 * knots-only filter (keep iff n_components == 1) needs no sign choice.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using Int = std::int64_t;

//==============================================================================
// plantri invocation + streaming edge_code parser  (port of run_tests.py)
//==============================================================================

/// Shell-quote a string so it can be embedded in a `popen` command line.
inline std::string Shq(const std::string& s)
{
    std::string out = "'";
    for (char ch : s) { out += (ch == '\'') ? std::string("'\\''") : std::string(1, ch); }
    out += "'";
    return out;
}

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

    bool Open(const std::string& plantri, const std::string& mode, int V,
              long shard_res, long shard_mod)
    {
        const std::string flags = (mode == "simple")  ? "-q -E"
                                : (mode == "no-r1")    ? "-Q -c2 -E"
                                                       : "-Q -E";   // "everything"
        // plantri's own res/mod: `plantri ... V res/mod` emits a disjoint,
        // exhaustive 1/mod slice of the graphs at V -- the natural unit for
        // fanning a sweep across cluster jobs (one SLURM array task per res).
        const std::string shard = (shard_mod > 0)
            ? (" " + std::to_string(shard_res) + "/" + std::to_string(shard_mod)) : "";
        const std::string cmd = Shq(plantri) + " " + flags + " " + std::to_string(V)
                              + shard + " 2>/dev/null";
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

inline Shadow QuadToShadow(const Graph& vertices)
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

inline Traced AssignAndTrace(const Shadow& sh, std::uint64_t mask)
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

} // namespace
