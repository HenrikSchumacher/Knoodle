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
#include "knot_determinant.hpp"
#include "pass_fixtures.hpp"

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

    // ---- AfterDiagram: "drawing minus W" ------------------------------
    // (descriptors below are written against the trefoil, so only run them
    //  when that is the diagram under test)
    // The second of the two deletions. Built from the descriptor alone, with
    // no help from PassSimplifier::Reroute, so it is an independent statement
    // of what the applier is supposed to return. Crossing count must be
    // (before - interior crossings of W + corridor crossings), and the result
    // must be a legal diagram.
    if( diagram.CrossingCount() == Int(3) )
    {
        auto after = [&](const Deco_T::PassMove_T & mv, Int expect_cx,
                         const char * name)
        {
            std::string why;
            PD_T ad = deco.AfterDiagram(diagram, mv, why);
            if (!why.empty())
            {
                std::printf("  after %s: %s\n", name, why.c_str());
                ok = false;
                return;
            }
            const bool cleanQ = ad.CheckAll();
            const bool countQ = (ad.CrossingCount() == expect_cx);

            // The move is an isotopy, so the determinant must survive it.
            // CheckAll alone is far too weak here: it passes for a legal
            // diagram of the WRONG knot, which is exactly how a wrong splice
            // presents.
            const Int det_before = DeterminantModP(diagram);
            const Int det_after  = DeterminantModP(ad);
            // The determinant is defined up to sign, and which sign the
            // minor comes out with depends on the row/column dropped, so
            // compare up to negation mod p.
            constexpr Int P = Int(1000003);
            const bool detQ = (det_before == det_after)
                           || (det_before == (P - det_after) % P);

            std::printf("  after %s: %lld crossings, CheckAll %s, det %lld -> %lld %s\n",
                name, (long long)ad.CrossingCount(), cleanQ ? "PASS" : "FAIL",
                (long long)det_before, (long long)det_after,
                (countQ ? (detQ ? "OK" : "*** DETERMINANT CHANGED ***")
                        : "*** wrong crossing count ***"));
            if (!cleanQ || !countQ || !detQ) { ok = false; }
        };

        // 2-arc strand (1 interior crossing), corridor of 1: 3 - 1 + 1
        after({ {1, 3}, 1, {6}, {false}, 3 }, Int(3), "doc-example");
        after({ {11, 1}, 11, {7}, {true}, 1 }, Int(3), "two-arc-strand");
    }

    // ---- multi-crossing corridors, on a real diagram -------------------
    // The trefoil and figure-eight cannot host these (see pass_fixtures.hpp),
    // so they run on the 73-crossing reproducer from PR #30. The k = 8 case is
    // the one that triggers the arc-label aliasing in Reroute.
    if( diagram.CrossingCount() == Int(3) )   // run once, not per grid size
    {
        PD_T big = PD_T::FromSignedPDCode(
            zf061098_underpass.data(),
            Int(zf061098_underpass.size() / 5), false, false );

        OrthoDraw_T Hb(big, Int(-1), settings);
        Deco_T db(Hb, Int(2));

        const Int det_big = DeterminantModP(big);
        constexpr Int P = Int(1000003);

        auto big_case = [&](const char * spec, Int expect_cx, const char * name)
        {
            Deco_T::PassMove_T mv; std::string err, why;
            if (!Deco_T::PassMove_T::Parse(spec, mv, err))
            { std::printf("  big %s: parse %s\n", name, err.c_str()); ok = false; return; }

            auto pr = db.RoutePassMove(big, mv);
            if (!pr.validQ)
            { std::printf("  big %s: route rejected (%s)\n", name, pr.why.c_str()); ok = false; return; }

            PD_T ad = db.AfterDiagram(big, mv, why);
            if (!why.empty())
            { std::printf("  big %s: AfterDiagram %s\n", name, why.c_str()); ok = false; return; }

            const Int det_after = DeterminantModP(ad);
            const bool cleanQ = ad.CheckAll();
            const bool countQ = (ad.CrossingCount() == expect_cx);
            const bool detQ   = (det_big == det_after)
                             || (det_big == (P - det_after) % P);

            std::printf("  big %-14s k=%zu: %lld cx, CheckAll %s, det %s\n",
                name, mv.cross.size(), (long long)ad.CrossingCount(),
                cleanQ ? "PASS" : "FAIL", detQ ? "preserved" : "*** CHANGED ***");
            if (!cleanQ || !countQ || !detQ) { ok = false; }
        };

        // the aliasing case: 8 corridor crossings replacing 8 strand crossings
        big_case("strand=81,83,85,87,89,91,93,95,97 depart=80"
                 " cross=7:u,42:u,8:u,230:u,131:u,281:u,260:u,290:u land=96",
                 Int(73), "aliasing k=8");
        // harvested shorter corridors on the same diagram
        big_case("strand=1,3,5 depart=0 cross=51:o,122:o land=4",
                 Int(73), "harvested k=2");
        big_case("strand=1,3,5,7 depart=0 cross=51:o,122:o,239:o land=6",
                 Int(73), "harvested k=3");
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
        auto pr = deco.RoutePassMove(diagram, mv);
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
    // Doc worked example: W = arc 0, under arcs 3, 1, 4; depart f1 and land
    // f0 are the two faces flanking arc 0, so the strand leaves and arrives
    // on the port it already occupies (check 4).
    expect({ {1, 3}, 1, {6}, {false}, 3 }, true, 1, "doc-example");

    // The doc's cautionary variant: chain rule holds (f3 -> f2 via darc 3,
    // land L(9)=f2) but f2 neither flanks arc 5 nor is a quadrant at the head
    // anchor — spec check 4 must reject it.
    expect({ {11}, 11, {3}, {false}, 9 }, false, 0, "land-not-canonical");

    // Wrong port: this is what the doc example used to say. The chain rule
    // holds and L(1)={1,6} IS a quadrant at arc 5's head anchor, so the old
    // check 4 accepted it -- but {1,6} sits between arcs 0 and 3, on the far
    // side of the crossing from arc 5's port, so the rerouted strand would
    // attach somewhere the move promised not to touch.
    expect({ {11}, 11, {7}, {false}, 1 }, false, 0, "land-wrong-port");

    // Both anchors the same crossing: the strand is the whole component, so
    // it leaves and returns to one crossing and "which port" is not well
    // posed (R_I curls and relatives).
    expect({ {1, 3, 5, 7, 9, 11}, 1, {}, {}, 1 }, false, 0, "anchors-coincide");

    // Two-arc strand (arcs 5 then 0), corridor f3 -> f1 crossing arc 3.
    expect({ {11, 1}, 11, {7}, {true}, 1 }, true, 1, "two-arc-strand");

    // A corridor longer than the strand it replaces. The applier rebuilds the
    // strand out of the labels the move frees, so there is nowhere to put the
    // extra crossings -- check 6. (This descriptor used to be the face-revisit
    // case; a face revisit needs k >= 2, hence a strand of >= 3 arcs, and no
    // such move exists on a trefoil or a figure-eight once lengthening is
    // barred. Multi-crossing coverage lives in the handoff reproducer.)
    expect({ {1}, 0, {4, 10, 3, 9}, {false, false, false, false}, 0 },
           false, 0, "lengthening-corridor");

    // kind=middlepass: the same mixed-tag corridor must be ACCEPTED once
    // per-crossing tags are declared (check 5 dropped).
    // kind=middlepass drops the uniform-tag rule (check 5) but not check 6:
    // it has no more room for extra crossings than a plain pass does.
    {
        Deco_T::PassMove_T mp{ {1}, 1, {6, 3, 9}, {false, true, false}, 0 };
        mp.middlepassQ = true;
        expect(mp, false, 0, "middlepass-still-bounded");
    }

    // Margin regression (the mv0009 bug class): the doc-example corridor
    // must route no matter WHICH face OrthoDraw draws as the exterior —
    // with a margin, a corridor through the drawn exterior goes around
    // the diagram instead of failing in a disconnected clip.
    for (Int ext = 0; ext < 5; ++ext)
    {
        OrthoDraw_T Hx(diagram, ext, settings);
        Deco_T dx(Hx, Int(2));
        auto pr = dx.RoutePassMove(diagram, { {1, 3}, 1, {6}, {false}, 3 });
        if (!pr.validQ)
        {
            std::printf("  exterior=%lld: REJECTED (%s)\n",
                (long long)ext, pr.why.c_str());
            ok = false;
        }
    }
    std::printf("  exterior-independence sweep: 5 exteriors\n");

    // Rejections:
    expect({ {1}, 1, {6, 3, 9}, {false, true, false}, 0 }, false, 0, "mixed-tags");

    // Arc-disjointness: crossing one arc twice is not something an applier can
    // carry out (the first crossing splits it, so the label's extent changes),
    // and FindShortestPath cannot emit it either -- it keeps a visited set on
    // arcs. This is the descriptor the old face-revisit case used.
    expect({ {11}, 11, {3, 2}, {false, false}, 11 }, false, 0, "cross-arc-twice");
    expect({ {11}, 11, {11}, {false}, 10 },        false, 0, "cross-own-strand");
    expect({ {11, 3}, 11, {7}, {true}, 1 },        false, 0, "broken-strand");
    expect({ {}, 7, {}, {}, 7 },                   false, 0, "empty-strand");
    expect({ {11}, 11, {2}, {false}, 3 },          false, 0, "wrong-chain-start");
    expect({ {11}, 9, {}, {}, 9 },                 false, 0, "depart-not-canonical");

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
