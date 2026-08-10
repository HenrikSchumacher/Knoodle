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

        //======================================================================
        // Junction placement. The corridor attaches at an anchor crossing;
        // the descriptor's depart/land face says through which of the four
        // quadrant faces around that crossing it leaves/arrives. In the grid,
        // a crossing cell's 4-neighbors are the four incident arc stubs
        // (occupied), and its diagonal neighbors are the quadrant corner
        // cells — so the junction is the diagonal neighbor of the anchor
        // cell lying in face `f`. Fails iff `f` is not a free quadrant there
        // (the operational form of spec check 4). If the same face occupies
        // two quadrants, the first in fixed scan order (NE, NW, SW, SE) wins
        // — deterministic; flank disambiguation is an open spec question.
        //======================================================================

        bool JunctionCell(Point_T anchor, Int f, Point_T & out) const
        {
            RequireFaceMap();

            static const Int diag_dx[] = {1, -1, -1,  1};
            static const Int diag_dy[] = {1,  1, -1, -1};

            for (int q = 0; q < 4; ++q)
            {
                Point_T c { anchor[0] + diag_dx[q], anchor[1] + diag_dy[q] };

                if (FaceAt(c[0], c[1]) == f)
                {
                    out = c;
                    return true;
                }
            }

            return false;
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
            RequireFaceMap();
            const DistanceField & df = RequireDistanceField(face_id);

            // Validate start and goal
            if (FaceAt(start[0], start[1]) != face_id) return {};
            if (FaceAt(goal[0], goal[1])   != face_id) return {};

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

        //======================================================================
        // Phase 2 — Public API: darc-level face lookup
        //
        // PD_T convention (see docs/move-descriptor.md): darc da = 2a + d,
        // Tail = 0, Head = 1; every face lies on the LEFT of its boundary
        // darcs. Face indices follow F_dA's numbering (= pd.FaceDarcs()).
        //======================================================================

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

            TraverseArcCells(a,
                [&](Int x, Int y, int dir, bool interiorQ)
                {
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

                    portal.push_back(PortalPoint_T{ {x, y}, l, r });
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
        // Waypoints are chosen greedily with a one-portal look-ahead (score =
        // distance from previous exit + distance toward the next portal's
        // middle, final goal for the last); each leg is clearance-maximizing
        // A* via RouteThroughFace. Deterministic: ties break to the first
        // minimal portal point.
        //
        // Known limitation: if the face sequence revisits a face, legs are
        // still routed independently and may cross each other; callers that
        // need an embedded (simple) corridor must check for that.
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

            // Greedy waypoint selection with one-portal look-ahead.
            std::vector<PortalPoint_T> chosen(k);
            Point_T prev = start;

            for (std::size_t i = 0; i < k; ++i)
            {
                Point_T target = (i + 1 < k)
                    ? portals[i + 1][portals[i + 1].size() / 2].on_arc
                    : goal;

                std::size_t best = 0;
                double best_score = std::numeric_limits<double>::infinity();

                for (std::size_t j = 0; j < portals[i].size(); ++j)
                {
                    const PortalPoint_T & p = portals[i][j];
                    double score = dist(prev, p.on_arc) + dist(p.on_arc, target);
                    if (score < best_score)
                    {
                        best_score = score;
                        best = j;
                    }
                }

                chosen[i] = portals[i][best];
                prev = chosen[i].right;
            }

            // Route the legs and assemble.
            auto append_leg = [&](Int face, Point_T from, Point_T to) -> bool
            {
                Path_T leg = RouteThroughFace(face, from, to);
                if (leg.empty())
                {
                    result.why = "A* found no path in face "
                        + std::to_string(face) + " from ("
                        + std::to_string(from[0]) + ","
                        + std::to_string(from[1]) + ") to ("
                        + std::to_string(to[0]) + ","
                        + std::to_string(to[1]) + ")";
                    return false;
                }
                result.path.insert(result.path.end(), leg.begin(), leg.end());
                return true;
            };

            Point_T at = start;
            for (std::size_t i = 0; i < k; ++i)
            {
                if (!append_leg(F[i], at, chosen[i].left)) return result;

                result.crossing_indices.push_back(
                    static_cast<Int>(result.path.size()));
                result.path.push_back(chosen[i].on_arc);

                at = chosen[i].right;
            }

            if (!append_leg(F[k], at, goal)) return result;

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

        struct PassMove_T
        {
            std::vector<Int>  strand;  // darcs of the rerouted strand, traversal order
            Int               depart;  // darc: L(depart) = corridor's first face
            std::vector<Int>  cross;   // darcs crossed, each left -> right
            std::vector<bool> over;    // per crossing: new strand passes over?
            Int               land;    // darc: L(land) = corridor's last face

            // kind=middlepass (middlestrands' MiddleStrandSimplifier moves):
            // identical grammar and checks, except the over/under tags are
            // per-crossing (spec check 5 is dropped). Well-formedness only —
            // soundness of a middlepass move additionally needs the
            // quotient-simplicity / feasibility-witness checks, which are a
            // VERIFIER concern, not a rendering concern.
            bool middlepassQ = false;
        };

        struct PassRoute_T
        {
            MultiRoute_T      route;        // corridor incl. crossing cells
            std::vector<bool> over;         // per crossing (copied from the move)
            Point_T           tail_anchor;  // grid cell of the tail anchor crossing
            Point_T           head_anchor;  // grid cell of the head anchor crossing
            bool              validQ = false;
            std::string       why;          // human-readable reason when !validQ
        };

        // Validate a pass-move descriptor (the local checks of the spec) and
        // route its corridor. The corridor starts in the quadrant of face
        // L(depart) at the tail anchor crossing and ends in the quadrant of
        // face L(land) at the head anchor crossing — the new strand attaches
        // exactly where the old one did.
        PassRoute_T RoutePassMove(const PassMove_T & mv) const
        {
            PassRoute_T result;

            auto fail = [&](std::string msg) -> PassRoute_T &
            {
                result.why = std::move(msg);
                return result;
            };

            const std::size_t m = mv.strand.size();
            const std::size_t k = mv.cross.size();

            // -- Shape and tag checks (spec check 5: all tags equal) --------
            if (m == 0) return fail("empty strand");
            if (mv.over.size() != k)
            {
                return fail("cross and over lists differ in length");
            }
            if (!mv.middlepassQ)
            {
                for (std::size_t i = 1; i < k; ++i)
                {
                    if (mv.over[i] != mv.over[0])
                    {
                        return fail("over/under tags are not all equal"
                            " (check 5; use kind=middlepass for mixed tags)");
                    }
                }
            }

            // -- Arc activity; strand arcs distinct; cross arcs not in strand
            //    (spec check 1) ---------------------------------------------
            for (Int da : mv.strand)
            {
                if (LeftFace(da) < 0)
                {
                    return fail("strand darc " + std::to_string(da)
                        + " is inactive or out of range");
                }
            }
            for (std::size_t i = 0; i < m; ++i)
            {
                for (std::size_t j = i + 1; j < m; ++j)
                {
                    if (mv.strand[i] / 2 == mv.strand[j] / 2)
                    {
                        return fail("strand repeats arc "
                            + std::to_string(mv.strand[i] / 2) + " (check 1)");
                    }
                }
            }
            for (Int dc : mv.cross)
            {
                if (LeftFace(dc) < 0)
                {
                    return fail("crossed darc " + std::to_string(dc)
                        + " is inactive or out of range");
                }
                for (Int ds : mv.strand)
                {
                    if (dc / 2 == ds / 2)
                    {
                        return fail("crossed darc " + std::to_string(dc)
                            + " lies on the strand itself (check 1)");
                    }
                }
            }

            // -- Strand consecutiveness, checked geometrically: the head-end
            //    vertex cell of each darc must be the tail-end vertex cell of
            //    the next (vertex cells are unique per diagram crossing) -----
            for (std::size_t i = 0; i + 1 < m; ++i)
            {
                if (DarcHeadCell(mv.strand[i]) != DarcTailCell(mv.strand[i + 1]))
                {
                    return fail("strand darcs " + std::to_string(mv.strand[i])
                        + " and " + std::to_string(mv.strand[i + 1])
                        + " are not consecutive (check 1)");
                }
            }

            result.tail_anchor = DarcTailCell(mv.strand.front());
            result.head_anchor = DarcHeadCell(mv.strand.back());

            // -- Combinatorial face chain (spec checks 2 and 3; RouteAcross-
            //    Darcs re-derives this, but failing early is clearer) --------
            const Int F_dep  = LeftFace(mv.depart);
            const Int F_land = LeftFace(mv.land);
            if (F_dep < 0)  return fail("depart darc is inactive or out of range");
            if (F_land < 0) return fail("land darc is inactive or out of range");

            if (k > 0)
            {
                if (LeftFace(mv.cross.front()) != F_dep)
                {
                    return fail("L(cross[0]) != L(depart) (check 2)");
                }
                if (RightFace(mv.cross.back()) != F_land)
                {
                    return fail("L(land) != R(cross[last]) (check 3)");
                }
            }
            else if (F_dep != F_land)
            {
                return fail("no crossings but L(depart) != L(land) (check 3)");
            }

            // -- Junctions: the depart/land faces must be quadrants at the
            //    respective anchor crossings (spec check 4); the junction
            //    cell is the anchor's diagonal neighbor in that face. -------
            Point_T start, goal;
            if (!JunctionCell(result.tail_anchor, F_dep, start))
            {
                return fail("depart face is not a quadrant at the tail anchor"
                    " (check 4)");
            }
            if (!JunctionCell(result.head_anchor, F_land, goal))
            {
                return fail("land face is not a quadrant at the head anchor"
                    " (check 4)");
            }

            // -- Route ------------------------------------------------------
            result.route = RouteAcrossDarcs(start, goal, mv.cross);
            if (!result.route.validQ)
            {
                return fail("face chain validated but geometric routing"
                    " failed: " + result.route.why);
            }

            result.over   = mv.over;
            result.validQ = true;
            return result;
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
            Junction,     // corridor endpoint beside an anchor crossing
            CrossOverH,   // corridor passes OVER the crossed arc, running E-W
            CrossOverV,   // corridor passes OVER the crossed arc, running N-S
            CrossUnder,   // corridor passes UNDER: the arc's own glyph stays
            Anchor        // anchor crossing cell (color/emphasis only)
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
                    kind = OverlayKind::Junction;
                }
                else if (e && w) { kind = OverlayKind::Horizontal; }
                else if (n && s) { kind = OverlayKind::Vertical; }
                else if (n && e) { kind = OverlayKind::CornerNE; }
                else if (n && w) { kind = OverlayKind::CornerNW; }
                else if (s && e) { kind = OverlayKind::CornerSE; }
                else             { kind = OverlayKind::CornerSW; }

                cells.push_back(OverlayCell_T{ p[i][0], p[i][1], kind });
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
