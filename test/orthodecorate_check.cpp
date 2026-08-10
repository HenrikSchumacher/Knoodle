// OrthoDecorate regression test (Phases 1 + 2).
//
// Phase 1: face-cell identification (full coverage, zero orphan cells),
// the L-infinity clearance distance transform, and single-face A* routing.
// Phase 2: darc-level face lookup (LeftFace/RightFace), portals, and
// multi-face routing (RouteAcrossDarcs) under the descriptor convention of
// docs/move-descriptor.md — sweep over every interior darc, an invalid
// chain that must be rejected, and a two-crossing route, with path
// integrity checked (unit-adjacency; occupied cells exactly at the
// recorded crossing indices).
//
// Cases: trefoil and figure-eight, each at a tight 4x2 grid and the
// standard 20x20 drawing grid. Exit 0 iff everything passes.
// Build: `make orthodecorate_check` in tools/.

#include "../Knoodle.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

using Int         = std::int64_t;
using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
using Deco_T      = Knoodle::OrthoDecorate<PD_T>;

static int run_case(const char * name, std::vector<Int> pd, Int n,
                    Int xg, Int yg)
{
    std::printf("=== %s (grid %lld x %lld) ===\n",
        name, (long long)xg, (long long)yg);

    PD_T diagram = PD_T::FromSignedPDCode(pd.data(), n);

    OrthoDraw_T::Settings_T settings{};
    settings.x_grid_size = xg;
    settings.y_grid_size = yg;

    OrthoDraw_T H(diagram, Int(-1), settings);

    Deco_T deco(H);

    std::printf("grid: %lld x %lld, stretch (%lld, %lld)\n",
        (long long)deco.GridWidth(), (long long)deco.GridHeight(),
        (long long)deco.StretchX(),  (long long)deco.StretchY());

    // 1. Face map: count cells per face, expect every face to own cells
    const Int face_count = H.FaceCount();
    std::vector<long long> cells(static_cast<std::size_t>(face_count), 0);
    long long occupied_cells = 0;
    long long orphan_cells   = 0;

    for (Int y = 0; y < deco.GridHeight(); ++y)
    {
        for (Int x = 0; x < deco.GridWidth(); ++x)
        {
            Int f = deco.FaceAt(x, y);
            if      (f >= 0)         ++cells[static_cast<std::size_t>(f)];
            else if (deco.OccupiedQ(x, y)) ++occupied_cells;
            else                     ++orphan_cells;
        }
    }

    std::printf("faces: %lld, occupied cells: %lld, orphan cells: %lld\n",
        (long long)face_count, occupied_cells, orphan_cells);

    int empty_faces = 0;
    for (Int f = 0; f < face_count; ++f)
    {
        std::printf("  face %lld: %lld cells\n", (long long)f,
            cells[static_cast<std::size_t>(f)]);
        if (cells[static_cast<std::size_t>(f)] == 0) ++empty_faces;
    }

    // 2+3. Distance field + A* routing inside the biggest interior face
    // (the exterior face's in-grid clip may be disconnected corner pockets,
    // so routing across it can legitimately fail).
    Int big_face = -1;
    for (Int f = 0; f < face_count; ++f)
    {
        if (f == H.ExteriorFace()) continue;
        if (big_face < 0 || cells[static_cast<std::size_t>(f)] >
                            cells[static_cast<std::size_t>(big_face)]) big_face = f;
    }

    std::array<Int,2> start{-1,-1}, goal{-1,-1};
    for (Int y = 0; y < deco.GridHeight(); ++y)
    {
        for (Int x = 0; x < deco.GridWidth(); ++x)
        {
            if (deco.FaceAt(x, y) == big_face)
            {
                if (start[0] < 0) start = {x, y};
                goal = {x, y};
            }
        }
    }

    auto path = deco.RouteThroughFace(big_face, start, goal);

    std::printf("route in face %lld from (%lld,%lld) to (%lld,%lld): %zu points\n",
        (long long)big_face,
        (long long)start[0], (long long)start[1],
        (long long)goal[0],  (long long)goal[1],
        path.size());

    bool ok = (face_count > 0) && (empty_faces == 0)
           && (occupied_cells > 0) && !path.empty()
           && (path.front() == start) && (path.back() == goal);

    // ---- Phase 2: multi-face routing ----

    // Path integrity: consecutive cells unit-adjacent; occupied cells appear
    // exactly at the recorded crossing indices.
    auto check_route = [&](const Deco_T::MultiRoute_T & mr,
                           std::size_t n_crossings, const char * name) -> bool
    {
        if (!mr.validQ) { std::printf("  route %s: INVALID\n", name); return false; }
        if (mr.crossing_indices.size() != n_crossings)
        { std::printf("  route %s: wrong crossing count\n", name); return false; }

        for (std::size_t i = 1; i < mr.path.size(); ++i)
        {
            Int dx = mr.path[i][0] - mr.path[i-1][0];
            Int dy = mr.path[i][1] - mr.path[i-1][1];
            if (std::abs(dx) + std::abs(dy) != 1)
            { std::printf("  route %s: gap at %zu\n", name, i); return false; }
        }

        std::size_t ci = 0;
        for (std::size_t i = 0; i < mr.path.size(); ++i)
        {
            bool occ = deco.OccupiedQ(mr.path[i][0], mr.path[i][1]);
            bool is_crossing = (ci < mr.crossing_indices.size()
                && mr.crossing_indices[ci] == static_cast<Int>(i));
            if (occ != is_crossing)
            { std::printf("  route %s: occupancy mismatch at %zu\n", name, i); return false; }
            if (is_crossing) ++ci;
        }

        std::printf("  route %s: %zu points, %zu crossings OK\n",
            name, mr.path.size(), n_crossings);
        return true;
    };

    // Sweep: for every darc with a nonempty portal between two interior
    // faces, route from beside its first portal point to beside its last.
    int routes_tested = 0;
    for (Int da = 0; da < 2 * H.MaxArcCount(); ++da)
    {
        Int fl = deco.LeftFace(da), fr = deco.RightFace(da);
        if (fl < 0 || fr < 0) continue;
        if (fl == H.ExteriorFace() || fr == H.ExteriorFace()) continue;

        auto portal = deco.Portal(da);
        if (portal.empty())
        { std::printf("  darc %lld: EMPTY portal\n", (long long)da); ok = false; continue; }

        auto mr = deco.RouteAcrossDarcs(
            portal.front().left, portal.back().right, {da});
        char nm[32]; std::snprintf(nm, sizeof nm, "darc%lld", (long long)da);
        if (!check_route(mr, 1, nm)) ok = false;
        ++routes_tested;
    }
    std::printf("  single-crossing sweep: %d routes\n", routes_tested);

    // Invalid chain must be rejected: two darcs whose faces don't chain.
    {
        bool found = false;
        for (Int da = 0; da < 2 * H.MaxArcCount() && !found; ++da)
        {
            for (Int db = 0; db < 2 * H.MaxArcCount() && !found; ++db)
            {
                if (deco.LeftFace(da) < 0 || deco.LeftFace(db) < 0) continue;
                if (deco.RightFace(da) == deco.LeftFace(db)) continue;
                auto pa = deco.Portal(da);
                if (pa.empty()) continue;
                auto mr = deco.RouteAcrossDarcs(
                    pa.front().left, pa.front().right, {da, db});
                if (mr.validQ)
                { std::printf("  invalid chain (%lld,%lld) ACCEPTED\n",
                    (long long)da, (long long)db); ok = false; }
                found = true;
            }
        }
    }

    // Two-crossing route: find darcs da, db with R(da) == L(db), all three
    // faces interior, and route across both.
    {
        bool tested = false;
        for (Int da = 0; da < 2 * H.MaxArcCount() && !tested; ++da)
        {
            for (Int db = 0; db < 2 * H.MaxArcCount() && !tested; ++db)
            {
                if ((da / 2) == (db / 2)) continue;
                Int f0 = deco.LeftFace(da), f1 = deco.RightFace(da);
                Int f2 = deco.RightFace(db);
                if (f0 < 0 || f1 < 0 || f2 < 0) continue;
                if (deco.LeftFace(db) != f1) continue;
                if (f0 == H.ExteriorFace() || f1 == H.ExteriorFace()
                    || f2 == H.ExteriorFace()) continue;

                auto pa = deco.Portal(da), pb = deco.Portal(db);
                if (pa.empty() || pb.empty()) continue;

                auto mr = deco.RouteAcrossDarcs(
                    pa.front().left, pb.back().right, {da, db});
                if (!check_route(mr, 2, "two-crossing")) ok = false;
                tested = true;
            }
        }
        if (!tested) std::printf("  (no interior two-crossing chain in this diagram)\n");
    }

    std::printf(ok ? "CASE OK\n" : "CASE FAILED\n");
    return ok ? 0 : 1;
}

// Phase 3: pass-move descriptors on the trefoil (the worked example of
// docs/move-descriptor.md plus rejection cases). Face cycles, from
// FaceDarcs: f0{0,4,8} (exterior), f1{1,6}, f2{2,9}, f3{3,11,7}, f4{5,10}.
static int run_pass_tests(Int xg, Int yg)
{
    std::printf("=== trefoil pass moves (grid %lld x %lld) ===\n",
        (long long)xg, (long long)yg);

    std::vector<Int> trefoil = {
        0, 4, 1, 3, 1,
        2, 0, 3, 5, 1,
        4, 2, 5, 1, 1,
    };
    PD_T diagram = PD_T::FromSignedPDCode(trefoil.data(), Int(3));

    OrthoDraw_T::Settings_T settings{};
    settings.x_grid_size = xg;
    settings.y_grid_size = yg;
    OrthoDraw_T H(diagram, Int(-1), settings);
    Deco_T deco(H);

    bool ok = true;

    auto integrity = [&](const Deco_T::MultiRoute_T & mr) -> bool
    {
        for (std::size_t i = 1; i < mr.path.size(); ++i)
        {
            Int dx = mr.path[i][0] - mr.path[i-1][0];
            Int dy = mr.path[i][1] - mr.path[i-1][1];
            if (std::abs(dx) + std::abs(dy) != 1) return false;
        }
        std::size_t ci = 0;
        for (std::size_t i = 0; i < mr.path.size(); ++i)
        {
            bool occ = deco.OccupiedQ(mr.path[i][0], mr.path[i][1]);
            bool is_x = (ci < mr.crossing_indices.size()
                && mr.crossing_indices[ci] == static_cast<Int>(i));
            if (occ != is_x) return false;
            if (is_x) ++ci;
        }
        return true;
    };

    auto expect = [&](const Deco_T::PassMove_T & mv, bool should_pass,
                      std::size_t n_cross, const char * name)
    {
        auto pr = deco.RoutePassMove(mv);
        if (pr.validQ != should_pass)
        {
            std::printf("  pass %s: expected %s, got %s\n", name,
                should_pass ? "VALID" : "REJECT",
                pr.validQ ? "VALID" : "REJECT");
            ok = false;
            return;
        }
        if (!pr.validQ) { std::printf("  pass %s: rejected OK\n", name); return; }

        bool good = integrity(pr.route)
            && pr.route.crossing_indices.size() == n_cross
            && pr.over.size() == n_cross
            && deco.OccupiedQ(pr.tail_anchor[0], pr.tail_anchor[1])
            && deco.OccupiedQ(pr.head_anchor[0], pr.head_anchor[1]);

        if (!good)
        {
            std::printf("  pass %s: VALID but integrity failed\n", name);
            ok = false;
            return;
        }
        std::printf("  pass %s: %zu points, %zu crossings OK\n",
            name, pr.route.path.size(), n_cross);
    };

    // The spec's worked example: reroute arc 5 (darc 11, runs c2 -> c0),
    // leave through L(7)=f3, cross arc 3 under (darc 7: f3 -> f1), land
    // through L(1)=f1.
    expect({ {11}, 7, {7}, {false}, 1 }, true, 1, "doc-example");

    // The doc's cautionary variant: chain rule holds (f3 -> f2 via darc 3,
    // land L(9)=f2) but f2 is not a quadrant at the head anchor c0 — spec
    // check 4 must reject it.
    expect({ {11}, 7, {3}, {false}, 9 }, false, 0, "land-not-at-anchor");

    // Two-arc strand (arcs 5 then 0), corridor f3 -> f1 crossing arc 3.
    expect({ {11, 1}, 7, {7}, {false}, 1 }, true, 1, "two-arc-strand");

    // Face-revisiting corridor: f3 -> f2 -> f3 crossing arc 1 twice.
    expect({ {11}, 7, {3, 2}, {false, false}, 11 }, true, 2, "face-revisit");

    // Rejections:
    expect({ {11}, 7, {3, 2}, {false, true}, 11 }, false, 0, "mixed-tags");
    expect({ {11}, 7, {11}, {false}, 10 },         false, 0, "cross-own-strand");
    expect({ {11, 3}, 7, {7}, {false}, 1 },        false, 0, "broken-strand");
    expect({ {}, 7, {}, {}, 7 },                   false, 0, "empty-strand");
    expect({ {11}, 7, {2}, {false}, 3 },           false, 0, "wrong-chain-start");
    expect({ {11}, 9, {}, {}, 9 },                 false, 0, "depart-not-at-anchor");

    std::printf(ok ? "CASE OK\n" : "CASE FAILED\n");
    return ok ? 0 : 1;
}

int main()
{
    // Right-hand trefoil (3_1), 0-based arcs, 5-col signed PD rows
    std::vector<Int> trefoil = {
        0, 4, 1, 3, 1,
        2, 0, 3, 5, 1,
        4, 2, 5, 1, 1,
    };

    // Figure-eight (4_1), 0-based arcs; signs +1,+1,-1,-1
    std::vector<Int> fig8 = {
        3, 1, 4, 0, 1,
        7, 5, 0, 4, 1,
        5, 2, 6, 3, -1,
        1, 6, 2, 7, -1,
    };

    int rc = 0;
    rc |= run_case("trefoil, tight grid",   trefoil, 3, 4, 2);
    rc |= run_case("trefoil, drawing grid", trefoil, 3, 20, 20);
    rc |= run_case("fig8, tight grid",      fig8,    4, 4, 2);
    rc |= run_case("fig8, drawing grid",    fig8,    4, 20, 20);
    rc |= run_pass_tests(4, 2);
    rc |= run_pass_tests(20, 20);

    std::printf(rc == 0 ? "PROBE OK\n" : "PROBE FAILED\n");
    return rc;
}
