#pragma once

#include <queue>
#include <numeric>   // std::lcm
#include <algorithm> // std::min, std::max
#include <limits>
#include <unordered_map>
#include <cmath>     // std::hypot, std::abs
#include <utility>   // std::swap
#include <vector>
#include <string>    // PassRoute_T::why
#include <cstdint>   // OverlayKind underlying type

namespace Knoodle
{
    template<class PD_T_>
    class OrthoDecorate
    {
    public:

        using PD_T        = PD_T_;
        using Int         = typename PD_T::Int;
        using OrthoDraw_T = OrthoDraw<PD_T>;
        using Point_T     = std::array<Int,2>;
        using Path_T      = std::vector<Point_T>;

    private:

        OrthoDraw_T & H;    // Reference to computed OrthoDraw (non-const: ArcLines() caches lazily)

        Int margin;         // free ring around the drawing (see constructor)
        Int n_x;            // grid width:  Width()  * x_grid_size + 2 + 2*margin
        Int n_y;            // grid height: Height() * y_grid_size + 1 + 2*margin
        Int x_grid_size;
        Int y_grid_size;
        Int stretch_x;      // lcm / x_grid_size
        Int stretch_y;      // lcm / y_grid_size

        // Lazy-computed face map
        mutable bool              face_map_ready = false;
        mutable Tensor1<Int,Int>  face_map;      // n_x*n_y → face_id (-1=free, -2=occupied)
        mutable Tensor1<bool,Int> occupied;       // n_x*n_y → is arc/crossing cell?

        // Per-face distance field cache
        struct DistanceField
        {
            std::vector<Int> dist;  // stretched bbox grid → L∞ distance
            Int x0, y0;            // bbox origin in original grid (with 1-cell expansion)
            Int w, h;              // bbox size in original grid (expanded)
            Int sw, sh;            // stretched dimensions
            Int max_dist;
        };
        mutable std::unordered_map<Int, DistanceField> dist_cache;

        // Lazy darc -> face lookup (PD face indices, same numbering as F_dA)
        mutable bool             darc_faces_ready = false;
        mutable Tensor1<Int,Int> dA_F;

        //======================================================================
        // Grid indexing
        //======================================================================

        Int GridIndex(Int x, Int y) const
        {
            return x + n_x * (n_y - Int(1) - y);
        }

        Int StretchedIndex(Int sx, Int sy, Int sw) const
        {
            return sx + sw * sy;
        }

        //======================================================================
        // Rasterize arc polylines onto the occupied grid
        //======================================================================

        // NOTE: We deliberately do NOT use ArcLines() here. ArcLines() shortens
        // an arc's endpoints by x_gap_size/y_gap_size wherever the arc goes
        // *under* a crossing (that is how the drawing shows the strand
        // interruption). For face walls those gaps are corridors of free cells
        // that connect the faces on either side of the understrand, so the
        // flood fill would leak through every under-crossing. The vertex chain
        // ArcVertices() + VertexCoordinates() is the true, gapless geometry.

        // Walk the rasterized cells of arc `a` tail-to-head, calling
        // f(x, y, dir, interiorQ) once per cell. `dir` (0=E,1=N,2=W,3=S) is
        // the direction of the axis-aligned segment the cell lies on;
        // `interiorQ` is false for polyline vertex cells (arc endpoints and
        // bends), where the perpendicular is ambiguous.
        template<typename F>
        void TraverseArcCells(Int a, F && f) const
        {
            static const Int step_dx[] = {1, 0, -1, 0};
            static const Int step_dy[] = {0, 1, 0, -1};

            const auto & A_V      = H.ArcVertices();
            const auto & V_coords = H.VertexCoordinates();

            auto sublist = A_V.Sublist(a);
            auto it  = sublist.begin();
            auto end = sublist.end();

            if (it == end) return;

            Int px = V_coords(*it, 0) + margin;
            Int py = V_coords(*it, 1) + margin;
            ++it;

            bool first = true;

            for (; it != end; ++it)
            {
                Int qx = V_coords(*it, 0) + margin;
                Int qy = V_coords(*it, 1) + margin;

                if (px == qx && py == qy) continue;

                int dir = (qx > px) ? 0 : (qy > py) ? 1 : (qx < px) ? 2 : 3;

                if (first) { f(px, py, dir, false); first = false; }

                Int steps = std::abs(qx - px) + std::abs(qy - py);

                for (Int s = 1; s < steps; ++s)
                {
                    f(px + s * step_dx[dir], py + s * step_dy[dir], dir, true);
                }

                // Segment far endpoint: an arc endpoint or a bend
                f(qx, qy, dir, false);

                px = qx;
                py = qy;
            }
        }

        void RasterizeArcs() const
        {
            Int a_count = H.MaxArcCount();

            for (Int a = 0; a < a_count; ++a)
            {
                if (!H.EdgeActiveQ(a)) continue;

                TraverseArcCells(a,
                    [this](Int x, Int y, int, bool) { MarkOccupied(x, y); });
            }
        }

        void MarkOccupied(Int x, Int y) const
        {
            if (x < 0 || x >= n_x || y < 0 || y >= n_y) return;
            Int idx = GridIndex(x, y);
            occupied[idx] = true;
            face_map[idx] = Int(-2);
        }

        //======================================================================
        // Find seed point for a face by stepping perpendicular from arc midpoint
        //======================================================================

        bool FindFaceSeed(Int face_id, Int & seed_x, Int & seed_y) const
        {
            static const Int step_dx[] = {1, 0, -1, 0};
            static const Int step_dy[] = {0, 1, 0, -1};

            const auto & F_dA     = H.FaceDarcs();
            const auto & A_V      = H.ArcVertices();
            const auto & V_coords = H.VertexCoordinates();

            auto darcs = F_dA[face_id];

            for (auto darc_it = darcs.begin(); darc_it != darcs.end(); ++darc_it)
            {
                Int da = *darc_it;
                Int a = da / Int(2);
                Int d = da % Int(2);  // PD_T convention: Tail = 0 (darc runs
                                      // against arc orientation), Head = 1
                                      // (along it); face lies LEFT of the darc

                if (!H.EdgeActiveQ(a)) continue;

                auto sublist = A_V.Sublist(a);
                Int point_count = static_cast<Int>(sublist.end() - sublist.begin());
                if (point_count < 2) continue;

                // Pick the middle segment of the vertex chain
                Int mid = point_count / 2;
                auto it0 = sublist.begin();
                auto it1 = sublist.begin();
                std::advance(it0, mid - 1);
                std::advance(it1, mid);

                Int x0 = V_coords(*it0, 0) + margin, y0 = V_coords(*it0, 1) + margin;
                Int x1 = V_coords(*it1, 0) + margin, y1 = V_coords(*it1, 1) + margin;

                // Segment midpoint
                Int mx = (x0 + x1) / 2;
                Int my = (y0 + y1) / 2;

                // Segment direction
                int seg_dir;
                if      (x1 > x0) seg_dir = 0; // East
                else if (y1 > y0) seg_dir = 1; // North
                else if (x1 < x0) seg_dir = 2; // West
                else               seg_dir = 3; // South

                // Step perpendicular toward face interior. seg_dir follows the
                // arc's stored (forward) orientation, and the face lies left
                // of the darc:
                // d=1 (Head, darc forward):  face left of forward  -> +1
                // d=0 (Tail, darc backward): face left of backward
                //                            = right of forward    -> +3
                int face_dir = (d == 0) ? (seg_dir + 3) % 4 : (seg_dir + 1) % 4;

                Int sx = mx + step_dx[face_dir];
                Int sy = my + step_dy[face_dir];

                // Verify it's a valid, unassigned cell
                if (sx >= 0 && sx < n_x - 1 && sy >= 0 && sy < n_y
                    && face_map[GridIndex(sx, sy)] == Int(-1))
                {
                    seed_x = sx;
                    seed_y = sy;
                    return true;
                }
            }

            return false;
        }

        //======================================================================
        // Flood-fill from seed to assign face membership
        //======================================================================

        void FloodFillFace(Int face_id, Int seed_x, Int seed_y) const
        {
            static const Int step_dx[] = {1, 0, -1, 0};
            static const Int step_dy[] = {0, 1, 0, -1};

            std::vector<std::pair<Int,Int>> stack;
            stack.push_back({seed_x, seed_y});
            face_map[GridIndex(seed_x, seed_y)] = face_id;

            while (!stack.empty())
            {
                auto [cx, cy] = stack.back();
                stack.pop_back();

                for (int dir = 0; dir < 4; ++dir)
                {
                    Int nx = cx + step_dx[dir];
                    Int ny = cy + step_dy[dir];

                    if (nx < 0 || nx >= n_x - 1 || ny < 0 || ny >= n_y) continue;

                    Int ni = GridIndex(nx, ny);
                    if (face_map[ni] == Int(-1))
                    {
                        face_map[ni] = face_id;
                        stack.push_back({nx, ny});
                    }
                }
            }
        }

        //======================================================================
        // Lazy face map computation
        //======================================================================

        void RequireFaceMap() const
        {
            if (face_map_ready) return;

            // Initialize arrays
            face_map = Tensor1<Int,Int>(n_x * n_y, Int(-1));
            occupied = Tensor1<bool,Int>(n_x * n_y, false);

            // Mark the newline column as occupied
            for (Int y = 0; y < n_y; ++y)
            {
                Int idx = GridIndex(n_x - 1, y);
                face_map[idx] = Int(-2);
                occupied[idx] = true;
            }

            // Rasterize arc polylines to mark occupied cells
            RasterizeArcs();

            // Seed and flood-fill each face
            const Int ext_f = H.ExteriorFace();

            for (Int f = 0; f < H.FaceCount(); ++f)
            {
                if (f == ext_f) continue;  // handled below

                Int seed_x = -1, seed_y = -1;
                if (FindFaceSeed(f, seed_x, seed_y))
                {
                    FloodFillFace(f, seed_x, seed_y);
                }
            }

            // The exterior face surrounds the drawing, so FindFaceSeed's
            // perpendicular step exits the grid for it. Instead: clipped to
            // the grid, every connected component of the exterior touches the
            // boundary ring (a pocket that didn't would be walled in, i.e., a
            // bounded face). So flood from every free ring cell. Column
            // n_x - 1 is the newline column, already marked occupied.
            for (Int y = 0; y < n_y; ++y)
            {
                for (Int x = 0; x < n_x - 1; ++x)
                {
                    bool on_ring = (x == 0) || (x == n_x - 2)
                                || (y == 0) || (y == n_y - 1);

                    if (on_ring && face_map[GridIndex(x, y)] == Int(-1))
                    {
                        FloodFillFace(ext_f, x, y);
                    }
                }
            }

            face_map_ready = true;
        }

        //======================================================================
        // Lazy per-face L∞ distance transform on stretched grid
        //======================================================================

        const DistanceField & RequireDistanceField(Int face_id) const
        {
            RequireFaceMap();

            auto it = dist_cache.find(face_id);
            if (it != dist_cache.end()) return it->second;

            // Find bounding box of face cells in original grid
            Int min_x = n_x, max_x = Int(-1);
            Int min_y = n_y, max_y = Int(-1);

            for (Int y = 0; y < n_y; ++y)
            {
                for (Int x = 0; x < n_x; ++x)
                {
                    if (face_map[GridIndex(x, y)] == face_id)
                    {
                        min_x = std::min(min_x, x);
                        max_x = std::max(max_x, x);
                        min_y = std::min(min_y, y);
                        max_y = std::max(max_y, y);
                    }
                }
            }

            // Expand bbox by 1 cell to include wall cells as distance-0 sources
            Int x0 = std::max(Int(0), min_x - Int(1));
            Int y0 = std::max(Int(0), min_y - Int(1));
            Int x1 = std::min(n_x - Int(1), max_x + Int(1));
            Int y1 = std::min(n_y - Int(1), max_y + Int(1));

            Int w = x1 - x0 + Int(1);
            Int h = y1 - y0 + Int(1);
            Int sw = w * stretch_x;
            Int sh = h * stretch_y;

            DistanceField df;
            df.x0 = x0;
            df.y0 = y0;
            df.w  = w;
            df.h  = h;
            df.sw = sw;
            df.sh = sh;
            df.max_dist = 0;
            df.dist.assign(static_cast<std::size_t>(sw * sh), Int(-1));

            // BFS queue: 8-connected for L∞
            std::queue<std::pair<Int,Int>> bfs;

            // Initialize: mark boundary/wall cells as distance 0 on the stretched grid
            for (Int oy = 0; oy < h; ++oy)
            {
                for (Int ox = 0; ox < w; ++ox)
                {
                    Int gx = x0 + ox;
                    Int gy = y0 + oy;

                    bool is_face = (face_map[GridIndex(gx, gy)] == face_id);

                    if (!is_face)
                    {
                        // This cell is a wall — mark all its stretched sub-cells as distance 0
                        for (Int sy = 0; sy < stretch_y; ++sy)
                        {
                            for (Int sx = 0; sx < stretch_x; ++sx)
                            {
                                Int ssx = ox * stretch_x + sx;
                                Int ssy = oy * stretch_y + sy;
                                Int si = StretchedIndex(ssx, ssy, sw);
                                df.dist[static_cast<std::size_t>(si)] = Int(0);
                                bfs.push({ssx, ssy});
                            }
                        }
                    }
                    // Face cells left at -1 (unvisited)
                }
            }

            // 8-connected BFS → L∞ distance field
            static const Int dx8[] = {1, 1, 0, -1, -1, -1, 0, 1};
            static const Int dy8[] = {0, 1, 1, 1, 0, -1, -1, -1};

            while (!bfs.empty())
            {
                auto [cx, cy] = bfs.front();
                bfs.pop();

                Int ci = StretchedIndex(cx, cy, sw);
                Int cd = df.dist[static_cast<std::size_t>(ci)];

                for (int dir = 0; dir < 8; ++dir)
                {
                    Int nx = cx + dx8[dir];
                    Int ny = cy + dy8[dir];

                    if (nx < 0 || nx >= sw || ny < 0 || ny >= sh) continue;

                    // Check that the original-grid cell belongs to this face
                    Int orig_x = x0 + nx / stretch_x;
                    Int orig_y = y0 + ny / stretch_y;
                    if (face_map[GridIndex(orig_x, orig_y)] != face_id) continue;

                    Int ni = StretchedIndex(nx, ny, sw);
                    if (df.dist[static_cast<std::size_t>(ni)] == Int(-1))
                    {
                        df.dist[static_cast<std::size_t>(ni)] = cd + Int(1);
                        df.max_dist = std::max(df.max_dist, cd + Int(1));
                        bfs.push({nx, ny});
                    }
                }
            }

            auto [ins_it, _] = dist_cache.emplace(face_id, std::move(df));
            return ins_it->second;
        }

        //======================================================================
        // Look up L∞ distance for an original-grid cell
        //======================================================================

        void RequireDarcFaces() const
        {
            if (darc_faces_ready) return;

            const auto & F_dA = H.FaceDarcs();

            dA_F = Tensor1<Int,Int>(Int(2) * H.MaxArcCount(), Int(-1));

            for (Int f = 0; f < H.FaceCount(); ++f)
            {
                for (auto da : F_dA[f])
                {
                    dA_F[da] = f;
                }
            }

            darc_faces_ready = true;
        }

        //======================================================================
        // Darc endpoint cells (arc-end vertex cells, oriented by the darc)
        //======================================================================

        Point_T ArcEndCell(Int a, bool headQ) const
        {
            const auto & A_V      = H.ArcVertices();
            const auto & V_coords = H.VertexCoordinates();

            auto sublist = A_V.Sublist(a);
            auto it = headQ ? sublist.end() - 1 : sublist.begin();

            return Point_T{ V_coords(*it, 0) + margin, V_coords(*it, 1) + margin };
        }

        // Tail end of a darc: where its traversal starts. A Head darc (d=1)
        // runs tail->head of the arc, a Tail darc (d=0) head->tail.
        Point_T DarcTailCell(Int da) const
        {
            return ArcEndCell(da / Int(2), (da % Int(2)) == Int(0));
        }

        Point_T DarcHeadCell(Int da) const
        {
            return ArcEndCell(da / Int(2), (da % Int(2)) == Int(1));
        }

        Int DistanceAt(const DistanceField & df, Int x, Int y) const
        {
            Int sx = (x - df.x0) * stretch_x;
            Int sy = (y - df.y0) * stretch_y;

            if (sx < 0 || sx >= df.sw || sy < 0 || sy >= df.sh) return Int(0);

            Int d = df.dist[static_cast<std::size_t>(StretchedIndex(sx, sy, df.sw))];
            return (d < 0) ? Int(0) : d;
        }

    public:

        //======================================================================
        // Constructor
        //======================================================================

        // `margin_` adds a ring of free cells around the drawing. The drawing
        // itself spans the whole unpadded grid, so the exterior face's
        // in-grid clip is generally DISCONNECTED corner pockets — any
        // corridor routed through the drawn exterior face then fails. With a
        // margin the exterior clip contains a connected ring and such
        // corridors route around the diagram, which is also how they look on
        // paper. All grid coordinates produced by this class (face map,
        // portals, routes, overlays) include the margin offset; renderers
        // pad their canvas by Margin() or subtract it.
        OrthoDecorate(OrthoDraw_T & H_, Int margin_ = Int(0))
        : H         { H_ }
        , margin    { std::max(Int(0), margin_) }
        , n_x       { H_.Width()  * H_.Settings().x_grid_size + Int(2)
                      + Int(2) * margin }
        , n_y       { H_.Height() * H_.Settings().y_grid_size + Int(1)
                      + Int(2) * margin }
        , x_grid_size { H_.Settings().x_grid_size }
        , y_grid_size { H_.Settings().y_grid_size }
        , stretch_x { static_cast<Int>(std::lcm(
                          static_cast<long long>(x_grid_size),
                          static_cast<long long>(y_grid_size))
                      ) / x_grid_size }
        , stretch_y { static_cast<Int>(std::lcm(
                          static_cast<long long>(x_grid_size),
                          static_cast<long long>(y_grid_size))
                      ) / y_grid_size }
        , face_map  { Int(0) }
        , occupied  { Int(0) }
        {}

        //======================================================================
        // Public API: Query face membership at a grid point
        //======================================================================

        Int FaceAt(Int x, Int y) const
        {
            RequireFaceMap();

            if (x < 0 || x >= n_x || y < 0 || y >= n_y) return Int(-1);
            return face_map[GridIndex(x, y)];
        }

        //======================================================================
        // Public API: Check if a grid point is occupied by an arc
        //======================================================================

        bool OccupiedQ(Int x, Int y) const
        {
            RequireFaceMap();

            if (x < 0 || x >= n_x || y < 0 || y >= n_y) return true;
            return occupied[GridIndex(x, y)];
        }

        //======================================================================
        // Public API: Route a path through a face between two boundary points
        //======================================================================

        Path_T RouteThroughFace(Int face_id, Point_T start, Point_T goal) const
        {
            return RouteThroughFaceImpl(face_id, start, goal, nullptr,
                                        Int(-1), Int(-1));
        }

    private:

        // The workhorse behind RouteThroughFace. `blocked` (optional) marks
        // cells that earlier routing has already drawn strokes through; once
        // something is drawn it is really there, so A* treats those cells as
        // walls. `avoid_a`/`avoid_b` are up to two additional grid indices to
        // keep out of (used for the multi-route's final goal and a leg's own
        // exit cell); -1 disables. The caller guarantees start and goal are
        // themselves admissible.
        Path_T RouteThroughFaceImpl(
            Int face_id, Point_T start, Point_T goal,
            const std::vector<char> * blocked, Int avoid_a, Int avoid_b
        ) const
        {
            RequireFaceMap();
            const DistanceField & df = RequireDistanceField(face_id);

            // Validate start and goal
            if (FaceAt(start[0], start[1]) != face_id) return {};
            if (FaceAt(goal[0], goal[1])   != face_id) return {};

            if (blocked)
            {
                if ((*blocked)[static_cast<std::size_t>(
                        GridIndex(start[0], start[1]))]) return {};
                if ((*blocked)[static_cast<std::size_t>(
                        GridIndex(goal[0], goal[1]))])   return {};
            }

            // Trivial case
            if (start[0] == goal[0] && start[1] == goal[1])
            {
                return { start };
            }

            // A* on the original grid (4-connected, axis-aligned moves)
            // Cost: max_dist - distance[cell]  (high clearance = low cost)
            // Heuristic: Manhattan distance (admissible for 4-connected)

            static const Int dx4[] = {1, 0, -1, 0};
            static const Int dy4[] = {0, 1, 0, -1};

            Int grid_size = n_x * n_y;

            // g-scores and parent pointers as flat arrays
            const Int INF = std::numeric_limits<Int>::max();
            std::vector<Int> g_score(static_cast<std::size_t>(grid_size), INF);
            std::vector<Int> parent(static_cast<std::size_t>(grid_size), Int(-1));

            // Priority queue: (f-score, grid-index)
            using PQEntry = std::pair<Int, Int>;
            std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> open;

            Int start_idx = GridIndex(start[0], start[1]);
            Int goal_idx  = GridIndex(goal[0], goal[1]);

            g_score[static_cast<std::size_t>(start_idx)] = Int(0);

            Int h_start = std::abs(goal[0] - start[0]) + std::abs(goal[1] - start[1]);
            open.push({h_start, start_idx});

            bool found = false;

            while (!open.empty())
            {
                auto [f_val, ci] = open.top();
                open.pop();

                if (ci == goal_idx) { found = true; break; }

                Int cg = g_score[static_cast<std::size_t>(ci)];

                // Recover (cx, cy) from grid index.
                // GridIndex(x, y) = x + n_x * (n_y - 1 - y)
                // So: row = ci / n_x,  col = ci % n_x,  y = n_y - 1 - row
                Int row = ci / n_x;
                Int cx  = ci % n_x;
                Int cy  = n_y - Int(1) - row;

                // Skip stale entries
                if (cg > g_score[static_cast<std::size_t>(ci)]) continue;

                for (int dir = 0; dir < 4; ++dir)
                {
                    Int nx = cx + dx4[dir];
                    Int ny = cy + dy4[dir];

                    if (nx < 0 || nx >= n_x - 1 || ny < 0 || ny >= n_y) continue;

                    Int ni = GridIndex(nx, ny);
                    if (face_map[ni] != face_id) continue;
                    if (blocked && (*blocked)[static_cast<std::size_t>(ni)]) continue;
                    if ((ni == avoid_a) || (ni == avoid_b)) continue;

                    // Edge cost: prefer cells far from the boundary
                    Int cell_dist = DistanceAt(df, nx, ny);
                    Int edge_cost = df.max_dist - cell_dist + Int(1);  // +1 base cost

                    Int tentative_g = cg + edge_cost;

                    if (tentative_g < g_score[static_cast<std::size_t>(ni)])
                    {
                        g_score[static_cast<std::size_t>(ni)] = tentative_g;
                        parent[static_cast<std::size_t>(ni)] = ci;

                        Int h = std::abs(goal[0] - nx) + std::abs(goal[1] - ny);
                        open.push({tentative_g + h, ni});
                    }
                }
            }

            if (!found) return {};

            // Reconstruct path
            Path_T path;
            Int ci = goal_idx;
            while (ci != Int(-1))
            {
                Int row = ci / n_x;
                Int x   = ci % n_x;
                Int y   = n_y - Int(1) - row;
                path.push_back({x, y});
                ci = parent[static_cast<std::size_t>(ci)];
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

    public:

        //======================================================================
        // Phase 2 — Public API: darc-level face lookup
        //
        // PD_T convention (see docs/move-descriptor.md): darc da = 2a + d,
        // Tail = 0, Head = 1; every face lies on the LEFT of its boundary
        // darcs. Face indices follow F_dA's numbering (= pd.FaceDarcs()).
        //======================================================================

        static constexpr Int ArcOf( Int da ) { return da / Int(2); }

        Int LeftFace(Int da) const
        {
            RequireDarcFaces();

            if (da < 0 || da >= dA_F.Dim(0)) return Int(-1);
            return dA_F[da];
        }

        Int RightFace(Int da) const
        {
            return LeftFace(da ^ Int(1));
        }

        //======================================================================
        // Phase 2 — Public API: portals
        //======================================================================

        struct PortalPoint_T
        {
            Point_T on_arc;  // cell of the crossed arc
            Point_T left;    // free perpendicular neighbor in LeftFace(da)
            Point_T right;   // free perpendicular neighbor in RightFace(da)
            Int     pos;     // index of on_arc in the arc's tail->head cell walk
                             // (orders portal points along the arc; the dot
                             // placement of RoutePassMove needs this to keep
                             // the two stubs of a one-arc strand disjoint)
        };

        // All grid positions where a route may cross the arc of darc `da`
        // from its left face into its right face: interior (non-bend,
        // non-endpoint) arc cells whose two perpendicular neighbors are free
        // and lie in L(da) / R(da) respectively. Crossings are perpendicular
        // to the arc by construction.
        std::vector<PortalPoint_T> Portal(Int da) const
        {
            RequireFaceMap();

            static const Int step_dx[] = {1, 0, -1, 0};
            static const Int step_dy[] = {0, 1, 0, -1};

            std::vector<PortalPoint_T> portal;

            const Int a = da / Int(2);
            const Int d = da % Int(2);

            if (a < 0 || a >= H.MaxArcCount() || !H.EdgeActiveQ(a))
            {
                return portal;
            }

            const Int fl = LeftFace(da);
            const Int fr = RightFace(da);

            Int walk_pos = -1;

            TraverseArcCells(a,
                [&](Int x, Int y, int dir, bool interiorQ)
                {
                    ++walk_pos;

                    if (!interiorQ) return;

                    // Left of the tail->head walk = dir rotated CCW. A Tail
                    // darc runs against the arc's orientation, so its left
                    // and right swap.
                    int l_dir = (dir + 1) % 4;
                    int r_dir = (dir + 3) % 4;
                    if (d == Int(0)) std::swap(l_dir, r_dir);

                    Point_T l { x + step_dx[l_dir], y + step_dy[l_dir] };
                    Point_T r { x + step_dx[r_dir], y + step_dy[r_dir] };

                    // FaceAt is -2 on occupied and -1 off-grid/unassigned,
                    // so these checks also reject non-free cells.
                    if (FaceAt(l[0], l[1]) != fl) return;
                    if (FaceAt(r[0], r[1]) != fr) return;

                    portal.push_back(PortalPoint_T{ {x, y}, l, r, walk_pos });
                });

            return portal;
        }

        //======================================================================
        // Phase 2 — Public API: multi-face routing
        //======================================================================

        struct MultiRoute_T
        {
            Path_T path;                        // full polyline incl. crossing cells
            std::vector<Int> crossing_indices;  // path[crossing_indices[i]] crosses darcs[i]
            bool validQ = false;
            std::string why;                    // failure detail when !validQ
        };

        // Route from `start` to `goal`, crossing exactly the arcs of `darcs`
        // in order, each from its left face into its right face (the
        // descriptor convention of docs/move-descriptor.md). The face
        // sequence is derived, never supplied: F_0 = face of `start`, which
        // must equal L(darcs[0]); thereafter L(darcs[i]) must equal
        // R(darcs[i-1]); and `goal` must lie in R(darcs[k-1]).
        //
        // The corridor is SIMPLE by construction: once a leg is routed its
        // cells are really there, and every later leg treats them (and the
        // final goal cell) as walls -- so legs cannot cross each other even
        // when the face sequence revisits a face. Per crossing, the portal
        // candidates are tried in score order (one-portal look-ahead: score =
        // distance from the previous exit + distance toward the next portal's
        // middle, final goal for the last) until one admits a leg; each leg
        // is clearance-maximizing A* via RouteThroughFaceImpl. Deterministic:
        // ties break to the first minimal portal point. There is no
        // backtracking across crossings; a crossing none of whose portal
        // points admits a leg fails loudly.
        MultiRoute_T RouteAcrossDarcs(
            Point_T start, Point_T goal, const std::vector<Int> & darcs
        ) const
        {
            RequireFaceMap();

            MultiRoute_T result;

            auto fail = [&](std::string msg) -> MultiRoute_T &
            {
                result.why = std::move(msg);
                return result;
            };

            const std::size_t k = darcs.size();

            // Derive and validate the face sequence.
            std::vector<Int> F(k + 1);

            F[0] = FaceAt(start[0], start[1]);
            if (F[0] < 0) return fail("start cell is not in a face");

            for (std::size_t i = 0; i < k; ++i)
            {
                if (LeftFace(darcs[i]) != F[i])
                {
                    return fail("chain break at cross[" + std::to_string(i)
                        + "]: L(" + std::to_string(darcs[i]) + ") != face "
                        + std::to_string(F[i]));
                }
                F[i + 1] = RightFace(darcs[i]);
                if (F[i + 1] < 0)
                {
                    return fail("darc " + std::to_string(darcs[i])
                        + " has no right face");
                }
            }

            if (FaceAt(goal[0], goal[1]) != F[k])
            {
                return fail("goal cell is not in the final face "
                    + std::to_string(F[k]));
            }

            // Compute all portals up front (needed for look-ahead).
            std::vector<std::vector<PortalPoint_T>> portals(k);
            for (std::size_t i = 0; i < k; ++i)
            {
                portals[i] = Portal(darcs[i]);
                if (portals[i].empty())
                {
                    return fail("empty portal for cross[" + std::to_string(i)
                        + "] (darc " + std::to_string(darcs[i]) + ", arc "
                        + std::to_string(darcs[i] / 2) + ")");
                }
            }

            auto dist = [](Point_T p, Point_T q) -> double
            {
                return std::hypot(
                    static_cast<double>(p[0] - q[0]),
                    static_cast<double>(p[1] - q[1]));
            };

            // Sequential routing with walls (see the doc comment above).
            std::vector<char> blocked(
                static_cast<std::size_t>(n_x * n_y), char(0));

            auto grid_idx = [this](Point_T p) -> Int
            {
                return GridIndex(p[0], p[1]);
            };

            const Int goal_idx = grid_idx(goal);

            auto commit_leg = [&](const Path_T & leg)
            {
                for (const Point_T & c : leg)
                {
                    blocked[static_cast<std::size_t>(grid_idx(c))] = char(1);
                    result.path.push_back(c);
                }
            };

            Point_T at = start;

            for (std::size_t i = 0; i < k; ++i)
            {
                Point_T target = (i + 1 < k)
                    ? portals[i + 1][portals[i + 1].size() / 2].on_arc
                    : goal;

                // Candidate order: by look-ahead score, ties to the earlier
                // portal point.
                std::vector<std::size_t> order(portals[i].size());
                std::iota(order.begin(), order.end(), std::size_t(0));
                std::stable_sort(order.begin(), order.end(),
                    [&](std::size_t a, std::size_t b)
                    {
                        const PortalPoint_T & pa = portals[i][a];
                        const PortalPoint_T & pb = portals[i][b];
                        return dist(at, pa.on_arc) + dist(pa.on_arc, target)
                             < dist(at, pb.on_arc) + dist(pb.on_arc, target);
                    });

                bool routed = false;

                for (std::size_t j : order)
                {
                    const PortalPoint_T & p = portals[i][j];

                    // A candidate whose cells the corridor has already used is
                    // gone. The goal cell is reserved for the LAST leg's
                    // arrival: no leg may end on it early (p.left == goal
                    // commits it before the last leg needs it), and no
                    // crossing but the final one may exit onto it (the exit
                    // becomes the next leg's start, so the goal would be
                    // visited twice). The final crossing exiting exactly onto
                    // the goal is fine -- the last leg is then trivial.
                    if (blocked[static_cast<std::size_t>(grid_idx(p.left))]
                     || blocked[static_cast<std::size_t>(grid_idx(p.right))]
                     || blocked[static_cast<std::size_t>(grid_idx(p.on_arc))])
                    {
                        continue;
                    }
                    if (grid_idx(p.left) == goal_idx)
                    {
                        continue;
                    }
                    if ((i + 1 < k) && (grid_idx(p.right) == goal_idx))
                    {
                        continue;
                    }

                    // The leg may not run through this candidate's own exit
                    // cell either -- the next leg starts there.
                    Path_T leg = RouteThroughFaceImpl(
                        F[i], at, p.left, &blocked,
                        goal_idx, grid_idx(p.right));

                    if (leg.empty()) continue;

                    commit_leg(leg);

                    result.crossing_indices.push_back(
                        static_cast<Int>(result.path.size()));
                    result.path.push_back(p.on_arc);
                    blocked[static_cast<std::size_t>(grid_idx(p.on_arc))] = char(1);

                    at = p.right;
                    routed = true;
                    break;
                }

                if (!routed)
                {
                    return fail("no portal point of cross["
                        + std::to_string(i) + "] (darc "
                        + std::to_string(darcs[i]) + ", arc "
                        + std::to_string(darcs[i] / 2) + ") admits a leg in"
                        " face " + std::to_string(F[i]) + " from ("
                        + std::to_string(at[0]) + "," + std::to_string(at[1])
                        + ") that avoids the already-routed corridor");
                }
            }

            Path_T last = RouteThroughFaceImpl(
                F[k], at, goal, &blocked, Int(-1), Int(-1));

            if (last.empty())
            {
                return fail("A* found no path in face "
                    + std::to_string(F[k]) + " from ("
                    + std::to_string(at[0]) + "," + std::to_string(at[1])
                    + ") to (" + std::to_string(goal[0]) + ","
                    + std::to_string(goal[1]) + ") that avoids the"
                    " already-routed corridor");
            }
            commit_leg(last);

            result.validQ = true;
            return result;
        }

        //======================================================================
        // Phase 3 — Public API: pass-move descriptors
        //
        // Mirrors the `pass` grammar of docs/move-descriptor.md:
        //   #move kind=pass strand=... depart=... cross=...:u|o,... land=...
        // All references are darcs against the PD snapshot this layout was
        // built from. RoutePassMove runs the spec's local validation checks
        // and realizes the corridor geometrically.
        //======================================================================

        /*!@brief The pass descriptor is a type of its own (src/PassDescriptor
         * .hpp), owned by `PlanarDiagram` rather than by the drawing: its
         * well-formedness is diagram combinatorics and has no business being
         * decided here. This alias is kept so callers can keep saying
         * `Deco_T::PassMove_T`.
         */
        using PassMove_T = PassDescriptor<Int>;

        struct PassRoute_T
        {
            MultiRoute_T      route;        // corridor incl. dot + crossing cells
            std::vector<bool> over;         // per crossing (copied from the move)
            Point_T           tail_anchor;  // grid cell of the tail anchor crossing
            Point_T           head_anchor;  // grid cell of the head anchor crossing
            Point_T           tail_dot;     // dot cell on the strand's first arc
                                            //   (== route.path.front())
            Point_T           head_dot;     // dot cell on the strand's last arc
                                            //   (== route.path.back())
            bool              validQ = false;
            std::string       why;          // human-readable reason when !validQ
        };

        // Validate a pass-move descriptor (the local checks of the spec) and
        // route its corridor. The corridor does not float beside the anchors:
        // it BRANCHES OFF the strand's own end arcs. Each endpoint is a DOT
        // at a portal point of that arc -- the same designated crossing sites
        // the corridor uses to cross any other arc -- chosen so the corridor
        // leaves the dot sideways into L(depart) (resp. arrives from
        // L(land)). The piece of the arc between the anchor and the dot is
        // SHARED by the two pictures of the two-deletions contract
        // (docs/move-descriptor.md): delete the corridor and it is the start
        // of W; delete W and it is the start of the rerouted strand, which
        // therefore attaches to the anchor through exactly the port W
        // vacated.
        /*!@brief Route a pass descriptor in THIS drawing (tier 2).
         *
         * `pd` is the diagram the descriptor is written against and the one
         * this drawing was built from. Tier 1 -- is the descriptor consistent
         * with that diagram at all -- is `PassDescriptor::WellFormedQ`, which
         * needs no drawing; everything below it here is geometry: can a
         * corridor for it actually be laid out on this grid.
         */
        PassRoute_T RoutePassMove(
            cref<PD_T> pd, const PassMove_T & mv
        ) const
        {
            PassRoute_T result;

            auto fail = [&](std::string msg) -> PassRoute_T &
            {
                result.why = std::move(msg);
                return result;
            };

            // -- Tier 1: is the descriptor consistent with the diagram at
            //    all? Pure combinatorics, so it lives on the descriptor and
            //    is equally available to Simplify, to unit tests, and to
            //    anyone else who has no drawing in hand. -------------------
            std::string why;
            if( !mv.WellFormedQ(pd,why) ) { return fail(std::move(why)); }

            result.tail_anchor = DarcTailCell(mv.strand.front());
            result.head_anchor = DarcHeadCell(mv.strand.back());

            // -- Tier 2 begins here: everything from this point on is about
            //    THIS drawing. Check 4 guarantees depart/land are darcs OF
            //    the strand's end arcs, so their portals are exactly the
            //    candidate dot sites; a well-formed descriptor can still fail
            //    here if the layout offers no portal point on the required
            //    flank, which is a fact about the drawing, not the move.
            const auto dep_portal  = Portal(mv.depart);
            const auto land_portal = Portal(mv.land);

            if (dep_portal.empty())
            {
                return fail("the strand's first arc has no portal point on"
                    " the depart flank in this drawing (the descriptor is"
                    " well-formed; the layout cannot carry it)");
            }
            if (land_portal.empty())
            {
                return fail("the strand's last arc has no portal point on"
                    " the land flank in this drawing (the descriptor is"
                    " well-formed; the layout cannot carry it)");
            }

            const std::size_t k = mv.cross.size();

            auto dist = [](Point_T p, Point_T q) -> double
            {
                return std::hypot(
                    static_cast<double>(p[0] - q[0]),
                    static_cast<double>(p[1] - q[1]));
            };

            std::size_t ti = 0, hi = 0;

            if (k == 0)
            {
                // Joint choice: the shortest hop. When the strand is a single
                // arc both dots live on it, and the tail dot must come FIRST
                // in the strand's own direction: the two shared stubs
                // (anchor -> dot) may not overlap, or neither deletion is a
                // diagram.
                const bool same_arcQ = (PassMove_T::ArcOf(mv.depart)
                                     == PassMove_T::ArcOf(mv.land));
                const bool fwdQ = PassMove_T::DirOf(mv.strand.front());

                bool found = false;
                double best = std::numeric_limits<double>::infinity();

                for (std::size_t i = 0; i < dep_portal.size(); ++i)
                {
                    for (std::size_t j = 0; j < land_portal.size(); ++j)
                    {
                        if (same_arcQ)
                        {
                            const Int pt = dep_portal[i].pos;
                            const Int ph = land_portal[j].pos;
                            const bool tail_firstQ =
                                fwdQ ? (pt < ph) : (pt > ph);
                            if (!tail_firstQ) continue;
                        }
                        const double s =
                            dist(dep_portal[i].left, land_portal[j].left);
                        if (s < best)
                        {
                            best = s; ti = i; hi = j; found = true;
                        }
                    }
                }

                if (!found)
                {
                    return fail("no dot pair on the strand's arc leaves room"
                        " for two disjoint stubs (the tail dot must precede"
                        " the head dot along the strand)");
                }
            }
            else
            {
                // Each dot leans toward the corridor's first / last crossing
                // (middle-portal proxy, as in RouteAcrossDarcs); ties fall to
                // the portal point nearest its anchor -- the shortest shared
                // stub.
                const auto c0 = Portal(mv.cross.front());
                const auto ck = Portal(mv.cross.back());

                if (c0.empty() || ck.empty())
                {
                    // RouteAcrossDarcs would say this too; saying it here
                    // keeps the message tied to the failing darc.
                    const Int bad = c0.empty() ? mv.cross.front()
                                               : mv.cross.back();
                    return fail("empty portal for a corridor crossing (darc "
                        + std::to_string(bad) + ", arc "
                        + std::to_string(bad / 2) + ")");
                }

                const Point_T t0 = c0[c0.size() / 2].on_arc;
                const Point_T tk = ck[ck.size() / 2].on_arc;

                const bool fwd0 = PassMove_T::DirOf(mv.strand.front());
                const bool fwdL = PassMove_T::DirOf(mv.strand.back());

                // Anchor proximity in strand order: the tail anchor sits at
                // the arc's tail iff the strand runs the arc forward; the
                // head anchor at the arc's head iff likewise.
                auto pick = [&](const std::vector<PortalPoint_T> & cand,
                                Point_T toward, bool anchor_at_tailQ)
                    -> std::size_t
                {
                    std::size_t best_i = 0;
                    double best_s = std::numeric_limits<double>::infinity();
                    Int    best_key = 0;

                    for (std::size_t i = 0; i < cand.size(); ++i)
                    {
                        const double s = dist(cand[i].left, toward);
                        const Int key = anchor_at_tailQ ?  cand[i].pos
                                                        : -cand[i].pos;
                        if ((s < best_s)
                         || ((s == best_s) && (key < best_key)))
                        {
                            best_s = s; best_key = key; best_i = i;
                        }
                    }
                    return best_i;
                };

                ti = pick(dep_portal,  t0,  fwd0);
                hi = pick(land_portal, tk, !fwdL);
            }

            const PortalPoint_T & tp = dep_portal[ti];
            const PortalPoint_T & hp = land_portal[hi];

            result.tail_dot = tp.on_arc;
            result.head_dot = hp.on_arc;

            // -- Route ------------------------------------------------------
            result.route = RouteAcrossDarcs(tp.left, hp.left, mv.cross);
            if (!result.route.validQ)
            {
                return fail("face chain validated but geometric routing"
                    " failed: " + result.route.why);
            }

            // The dots belong to the corridor polyline: the drawn corridor
            // runs dot -> sideways off the arc -> ... -> sideways back onto
            // the arc -> dot.
            result.route.path.insert(result.route.path.begin(), tp.on_arc);
            for (Int & ci : result.route.crossing_indices) { ci += Int(1); }
            result.route.path.push_back(hp.on_arc);

            result.over   = mv.over;
            result.validQ = true;
            return result;
        }

        //======================================================================
        // Phase 5 — "drawing minus W": the diagram the move should produce
        //
        // The pass picture is a superposition of two states, each one deletion
        // away. Delete the corridor and you have the diagram we were handed --
        // true by construction, since that is what was drawn. Delete the strand
        // W, smoothing the crossings it made, and what remains must be, up to
        // relabelling, the diagram the applier is supposed to return.
        //
        // `AfterDiagram` builds that second one from the descriptor alone,
        // without going near `PassSimplifier::Reroute`. That makes it an
        // independent oracle: if the applier disagrees with it, one of the two
        // is wrong, and the picture is where that shows up.
        //
        // Surviving crossings and arcs keep their indices -- what the move
        // deletes is left inactive in place, what it creates is appended -- so
        // the before/after correspondence is the identity on everything the
        // move promised not to touch, the two anchors included. That is exactly
        // the matching the picture needs in order to say where the move
        // attaches.
        //======================================================================

        PD_T AfterDiagram(
            cref<PD_T> pd, const PassMove_T & mv, mref<std::string> why
        ) const
        {
            auto fail = [&why]( std::string msg ) -> PD_T
            {
                why = std::move(msg);
                return PD_T();
            };

            if( !mv.WellFormedQ(pd,why) ) { return PD_T(); }

            const Int n_c = pd.MaxCrossingCount();
            const Int n_a = pd.MaxArcCount();
            const Int L   = static_cast<Int>(mv.strand.size());
            const Int k   = static_cast<Int>(mv.cross.size());

            const Int m_c = n_c + k;
            const Int m_a = Int(2) * m_c;   // PD_T requires exactly this

            std::vector<Int>             C ( static_cast<std::size_t>(Int(4)*m_c), PD_T::Uninitialized );
            std::vector<CrossingState_T> CS( static_cast<std::size_t>(m_c), CrossingState_T::Inactive );
            std::vector<Int>             A ( static_cast<std::size_t>(Int(2)*m_a), PD_T::Uninitialized );
            std::vector<ArcState_T>      AS( static_cast<std::size_t>(m_a), ArcState_T::Inactive );
            std::vector<Int>             AC( static_cast<std::size_t>(m_a), PD_T::Uninitialized );

            for( Int c = 0; c < n_c; ++c )
            {
                for( Int io = 0; io < 2; ++io )
                {
                    for( Int lr = 0; lr < 2; ++lr )
                    {
                        C[static_cast<std::size_t>(Int(4)*c + Int(2)*io + lr)]
                            = pd.Crossings()(c,io,lr);
                    }
                }
                CS[static_cast<std::size_t>(c)] = pd.CrossingStates()[c];
            }
            for( Int a = 0; a < n_a; ++a )
            {
                A [static_cast<std::size_t>(Int(2)*a    )] = pd.Arcs()(a,0);
                A [static_cast<std::size_t>(Int(2)*a + 1)] = pd.Arcs()(a,1);
                AS[static_cast<std::size_t>(a)] = pd.ArcStates()[a];
                AC[static_cast<std::size_t>(a)] = pd.ArcColors()[a];
            }

            auto Cx = [&C]( Int c, Int io, Int lr ) -> Int &
            { return C[static_cast<std::size_t>(Int(4)*c + Int(2)*io + lr)]; };
            auto Aend = [&A]( Int a, Int ht ) -> Int &
            { return A[static_cast<std::size_t>(Int(2)*a + ht)]; };

            constexpr Int In_ = Int(1), Out_ = Int(0);

            auto repoint = [&]( Int c, Int from, Int to ) -> bool
            {
                for( Int io = 0; io < 2; ++io )
                {
                    for( Int lr = 0; lr < 2; ++lr )
                    {
                        if( Cx(c,io,lr) == from ) { Cx(c,io,lr) = to; return true; }
                    }
                }
                return false;
            };

            std::vector<Int> w (static_cast<std::size_t>(L));
            for( Int i = 0; i < L; ++i )
            {
                w[static_cast<std::size_t>(i)]
                    = PassMove_T::ArcOf(mv.strand[static_cast<std::size_t>(i)]);
            }

            const Int T = PassMove_T::DarcTailCrossing(pd, mv.strand.front());
            const Int H = PassMove_T::DarcHeadCrossing(pd, mv.strand.back());

            // -- the transversal at each interior crossing, as a_0 -> a_1 ----
            std::vector<Int> heal_next(static_cast<std::size_t>(m_a), PD_T::Uninitialized);
            std::vector<Int> interior;

            for( Int i = 1; i < L; ++i )
            {
                const Int x = PassMove_T::DarcHeadCrossing(
                    pd, mv.strand[static_cast<std::size_t>(i-1)] );
                interior.push_back(x);

                Int a0 = PD_T::Uninitialized, a1 = PD_T::Uninitialized;
                const Int wp = w[static_cast<std::size_t>(i-1)];
                const Int wn = w[static_cast<std::size_t>(i)];
                for( Int lr = 0; lr < 2; ++lr )
                {
                    const Int in_a  = Cx(x,In_ ,lr);
                    const Int out_a = Cx(x,Out_,lr);
                    if( (in_a  != wp) && (in_a  != wn) ) { a0 = in_a;  }
                    if( (out_a != wp) && (out_a != wn) ) { a1 = out_a; }
                }
                if( (a0 == PD_T::Uninitialized) || (a1 == PD_T::Uninitialized) )
                {
                    return fail("could not identify the transversal at interior"
                        " crossing " + std::to_string(x));
                }
                heal_next[static_cast<std::size_t>(a0)] = a1;
            }

            // -- healed-arc representatives (a transversal can chain) --------
            std::vector<Int> rep_of(static_cast<std::size_t>(m_a), PD_T::Uninitialized);
            {
                std::vector<bool> is_second(static_cast<std::size_t>(m_a), false);
                for( Int a = 0; a < m_a; ++a )
                {
                    const Int nx = heal_next[static_cast<std::size_t>(a)];
                    if( nx != PD_T::Uninitialized )
                    {
                        is_second[static_cast<std::size_t>(nx)] = true;
                    }
                }
                for( Int a = 0; a < m_a; ++a )
                {
                    if( is_second[static_cast<std::size_t>(a)] ) { continue; }
                    Int cur = a;
                    while( cur != PD_T::Uninitialized )
                    {
                        rep_of[static_cast<std::size_t>(cur)] = a;
                        cur = heal_next[static_cast<std::size_t>(cur)];
                    }
                }
            }

            // -- heal: the chain start absorbs the rest of its chain ---------
            for( Int a = 0; a < n_a; ++a )
            {
                if( rep_of[static_cast<std::size_t>(a)] != a ) { continue; }

                Int last = a;
                while( heal_next[static_cast<std::size_t>(last)] != PD_T::Uninitialized )
                {
                    const Int nxt = heal_next[static_cast<std::size_t>(last)];
                    AS[static_cast<std::size_t>(nxt)] = ArcState_T::Inactive;
                    last = nxt;
                }
                if( last != a )
                {
                    const Int new_head = Aend(last,Int(1));
                    Aend(a,Int(1)) = new_head;
                    if( !repoint(new_head,last,a) )
                    {
                        return fail("healing: crossing " + std::to_string(new_head)
                            + " does not mention arc " + std::to_string(last));
                    }
                }
            }

            // -- retire W and the crossings it made --------------------------
            for( Int i = 0; i < L; ++i )
            {
                AS[static_cast<std::size_t>(w[static_cast<std::size_t>(i)])]
                    = ArcState_T::Inactive;
            }
            for( Int x : interior ) { CS[static_cast<std::size_t>(x)] = CrossingState_T::Inactive; }

            // -- the corridor -----------------------------------------------
            // The move frees W's arcs and every transversal half it healed
            // away; between those and the slots the array grew by there is
            // always room for the k+1 corridor arcs and the k split pieces.
            std::vector<Int> free_labels;
            for( Int a = 0; a < m_a; ++a )
            {
                if( AS[static_cast<std::size_t>(a)] == ArcState_T::Inactive )
                {
                    free_labels.push_back(a);
                }
            }
            if( static_cast<Int>(free_labels.size()) < Int(2)*k + Int(1) )
            {
                return fail("not enough arc slots for the corridor: need "
                    + std::to_string(Int(2)*k + Int(1)) + ", have "
                    + std::to_string(free_labels.size()));
            }

            std::size_t next_free = 0;
            std::vector<Int> p (static_cast<std::size_t>(k+1));
            for( Int j = 0; j <= k; ++j ) { p[static_cast<std::size_t>(j)] = free_labels[next_free++]; }
            std::vector<Int> q (static_cast<std::size_t>(k));
            for( Int j = 0; j < k; ++j ) { q[static_cast<std::size_t>(j)] = free_labels[next_free++]; }

            const Int color = pd.ArcColors()[w[0]];
            for( Int j = 0; j <= k; ++j )
            {
                AS[static_cast<std::size_t>(p[static_cast<std::size_t>(j)])] = ArcState_T::Active;
                AC[static_cast<std::size_t>(p[static_cast<std::size_t>(j)])] = color;
            }

            if( !repoint(T, w[0], p[0]) )
            {
                return fail("tail anchor " + std::to_string(T)
                    + " does not mention the strand's first arc");
            }
            if( !repoint(H, w[static_cast<std::size_t>(L-1)],
                            p[static_cast<std::size_t>(k)]) )
            {
                return fail("head anchor " + std::to_string(H)
                    + " does not mention the strand's last arc");
            }
            Aend(p[0],Int(0)) = T;
            Aend(p[static_cast<std::size_t>(k)],Int(1)) = H;

            std::vector<bool> split_seen(static_cast<std::size_t>(m_a), false);

            for( Int j = 0; j < k; ++j )
            {
                const Int y  = n_c + j;
                const Int b0 = PassMove_T::ArcOf(mv.cross[static_cast<std::size_t>(j)]);
                const Int b  = rep_of[static_cast<std::size_t>(b0)];

                if( split_seen[static_cast<std::size_t>(b)] )
                {
                    return fail("the corridor crosses one healed arc twice (arc "
                        + std::to_string(b) + "); the order of the two crossings"
                        " along it is not determined by the descriptor");
                }
                split_seen[static_cast<std::size_t>(b)] = true;

                const Int qj       = q[static_cast<std::size_t>(j)];
                const Int old_head = Aend(b,Int(1));

                AS[static_cast<std::size_t>(qj)] = ArcState_T::Active;
                AC[static_cast<std::size_t>(qj)] = AC[static_cast<std::size_t>(b)];
                Aend(qj,Int(0)) = y;
                Aend(qj,Int(1)) = old_head;
                if( !repoint(old_head,b,qj) )
                {
                    return fail("splitting: crossing " + std::to_string(old_head)
                        + " does not mention arc " + std::to_string(b));
                }
                Aend(b,Int(1)) = y;

                const Int a_in  = p[static_cast<std::size_t>(j)];
                const Int a_out = p[static_cast<std::size_t>(j+1)];
                Aend(a_in ,Int(1)) = y;
                Aend(a_out,Int(0)) = y;

                // Port layout and handedness follow the same formula the
                // applier uses (Reroute.hpp). That part is the geometry of the
                // crossing, exercised by every production reroute; the label
                // bookkeeping the aliasing bug lives in is above, and is ours.
                const bool l2rQ  = PassMove_T::DirOf(mv.cross[static_cast<std::size_t>(j)]);
                const bool overQ = static_cast<bool>(mv.over[static_cast<std::size_t>(j)]);

                CS[static_cast<std::size_t>(y)]
                    = BooleanToCrossingState(l2rQ ? overQ : !overQ);

                if( l2rQ )
                {
                    Cx(y,Out_,Int(0)) = qj;     Cx(y,Out_,Int(1)) = a_out;
                    Cx(y,In_ ,Int(0)) = a_in;   Cx(y,In_ ,Int(1)) = b;
                }
                else
                {
                    Cx(y,Out_,Int(0)) = a_out;  Cx(y,Out_,Int(1)) = qj;
                    Cx(y,In_ ,Int(0)) = b;      Cx(y,In_ ,Int(1)) = a_in;
                }
            }

            if( k == Int(0) ) { Aend(p[0],Int(1)) = H; }

            why.clear();

            return PD_T(
                m_c, C.data(), CS.data(), A.data(), AS.data(), AC.data(),
                pd.LastColorDeactivated(), false, false
            );
        }

        //======================================================================
        // Phase 4 — Public API: overlay generation
        //
        // Classify each cell of a routed pass move into renderer-agnostic
        // glyph kinds. Renderers (ASCII, Unicode box art, SVG, ...) map the
        // kinds to their own strokes; nothing here is terminal-specific.
        //======================================================================

        // Compass naming: a corner's two arms. N = +y (screen up), E = +x.
        enum class OverlayKind : std::uint8_t
        {
            Horizontal,   // corridor runs E-W through this cell
            Vertical,     // corridor runs N-S
            CornerNE,     // arms N+E   (box glyph:  up-and-right)
            CornerNW,     // arms N+W
            CornerSE,     // arms S+E
            CornerSW,     // arms S+W
            Dot,          // corridor endpoint: the dot ON the strand's end
                          // arc where the corridor branches off (the piece
                          // anchor->dot is shared by both deletion views)
            CrossOverH,   // corridor passes OVER the crossed arc, running E-W
            CrossOverV,   // corridor passes OVER the crossed arc, running N-S
            CrossUnder,   // corridor passes UNDER: the arc's own glyph stays
            Anchor,       // anchor crossing cell (color/emphasis only)
            ArrowN,       // straight cell carrying an orientation arrow, +y
            ArrowE,       // ... +x
            ArrowS,       // ... -y
            ArrowW        // ... -x
        };

        struct OverlayCell_T
        {
            Int x;
            Int y;
            OverlayKind kind;
        };

        std::vector<OverlayCell_T> RenderPassRoute(const PassRoute_T & pr) const
        {
            std::vector<OverlayCell_T> cells;

            if (!pr.validQ) return cells;

            const Path_T & p = pr.route.path;
            const auto & xi  = pr.route.crossing_indices;

            for (std::size_t i = 0; i < p.size(); ++i)
            {
                // Crossing cell?
                std::size_t j = 0;
                bool crossingQ = false;
                for (; j < xi.size(); ++j)
                {
                    if (xi[j] == static_cast<Int>(i)) { crossingQ = true; break; }
                }

                // Arms toward path neighbors (N = +y).
                bool n = false, e = false, s = false, w = false;
                auto arm = [&](std::size_t o)
                {
                    Int dx = p[o][0] - p[i][0];
                    Int dy = p[o][1] - p[i][1];
                    n |= (dy > 0); s |= (dy < 0);
                    e |= (dx > 0); w |= (dx < 0);
                };
                if (i > 0)            arm(i - 1);
                if (i + 1 < p.size()) arm(i + 1);

                OverlayKind kind;

                if (crossingQ)
                {
                    // Portal crossings are perpendicular, hence straight.
                    if (!pr.over[j]) { kind = OverlayKind::CrossUnder; }
                    else
                    {
                        kind = (e || w) ? OverlayKind::CrossOverH
                                        : OverlayKind::CrossOverV;
                    }
                }
                else if (i == 0 || i + 1 == p.size())
                {
                    kind = OverlayKind::Dot;
                }
                else if (e && w) { kind = OverlayKind::Horizontal; }
                else if (n && s) { kind = OverlayKind::Vertical; }
                else if (n && e) { kind = OverlayKind::CornerNE; }
                else if (n && w) { kind = OverlayKind::CornerNW; }
                else if (s && e) { kind = OverlayKind::CornerSE; }
                else             { kind = OverlayKind::CornerSW; }

                cells.push_back(OverlayCell_T{ p[i][0], p[i][1], kind });
            }

            // Orientation: one arrow per maximal straight run, at its midpoint.
            // `cells` is still index-aligned with `p` here (anchors are appended
            // below), so the travel direction is just the step at that cell.
            for (std::size_t i = 0; i < cells.size(); )
            {
                const OverlayKind k = cells[i].kind;

                if ((k != OverlayKind::Horizontal) && (k != OverlayKind::Vertical))
                {
                    ++i;
                    continue;
                }

                std::size_t j = i;
                while ((j + 1 < cells.size()) && (cells[j + 1].kind == k)) { ++j; }

                if (j > i)   // runs of one stay plain: an arrow there reads as a corner
                {
                    const std::size_t m = i + (j - i) / 2;
                    const std::size_t a = (m + 1 < p.size()) ? m     : m - 1;
                    const std::size_t b = (m + 1 < p.size()) ? m + 1 : m;
                    const Int dx = p[b][0] - p[a][0];
                    const Int dy = p[b][1] - p[a][1];

                    cells[m].kind = (k == OverlayKind::Horizontal)
                                  ? ((dx >= Int(0)) ? OverlayKind::ArrowE
                                                    : OverlayKind::ArrowW)
                                  : ((dy >= Int(0)) ? OverlayKind::ArrowN
                                                    : OverlayKind::ArrowS);
                }

                i = j + 1;
            }

            cells.push_back(OverlayCell_T{
                pr.tail_anchor[0], pr.tail_anchor[1], OverlayKind::Anchor });
            cells.push_back(OverlayCell_T{
                pr.head_anchor[0], pr.head_anchor[1], OverlayKind::Anchor });

            return cells;
        }

        //======================================================================
        // Public API: Grid dimensions
        //======================================================================

        Int GridWidth()  const { return n_x; }
        Int GridHeight() const { return n_y; }

        //======================================================================
        // Public API: Stretch factors (for rendering backends)
        //======================================================================

        Int StretchX() const { return stretch_x; }
        Int StretchY() const { return stretch_y; }

        // Free-ring width around the drawing; all coordinates this class
        // emits are offset by it (drawing coords = grid coords - Margin()).
        Int Margin() const { return margin; }

        //======================================================================
        // Public API: Access underlying OrthoDraw
        //======================================================================

        OrthoDraw_T & GetOrthoDraw() { return H; }
        const OrthoDraw_T & GetOrthoDraw() const { return H; }

    }; // class OrthoDecorate

} // namespace Knoodle
