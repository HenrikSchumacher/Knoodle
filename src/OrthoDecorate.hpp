#pragma once

#include <queue>
#include <numeric>   // std::lcm
#include <algorithm> // std::min, std::max
#include <limits>
#include <unordered_map>

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

        Int n_x;            // grid width:  Width()  * x_grid_size + 2
        Int n_y;            // grid height: Height() * y_grid_size + 1
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

        void RasterizeArcs() const
        {
            auto & A_lines = const_cast<OrthoDraw_T &>(H).ArcLines();

            Int a_count = H.MaxArcCount();

            for (Int a = 0; a < a_count; ++a)
            {
                if (!H.EdgeActiveQ(a)) continue;

                auto sublist = A_lines[a];
                auto it = sublist.begin();
                auto end = sublist.end();

                if (it == end) continue;

                Int px = (*it)[0];
                Int py = (*it)[1];
                MarkOccupied(px, py);
                ++it;

                for (; it != end; ++it)
                {
                    Int qx = (*it)[0];
                    Int qy = (*it)[1];

                    // Walk axis-aligned segment from (px,py) to (qx,qy)
                    if (px == qx)
                    {
                        // Vertical segment
                        Int lo = std::min(py, qy);
                        Int hi = std::max(py, qy);
                        for (Int y = lo; y <= hi; ++y)
                        {
                            MarkOccupied(px, y);
                        }
                    }
                    else
                    {
                        // Horizontal segment
                        Int lo = std::min(px, qx);
                        Int hi = std::max(px, qx);
                        for (Int x = lo; x <= hi; ++x)
                        {
                            MarkOccupied(x, py);
                        }
                    }

                    px = qx;
                    py = qy;
                }
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

            const auto & F_dA = H.FaceDarcs();
            auto & A_lines = const_cast<OrthoDraw_T &>(H).ArcLines();

            auto darcs = F_dA[face_id];

            for (auto darc_it = darcs.begin(); darc_it != darcs.end(); ++darc_it)
            {
                Int da = *darc_it;
                Int a = da / Int(2);
                Int d = da % Int(2);  // 0 = Tail (forward), 1 = Head (backward)

                if (!H.EdgeActiveQ(a)) continue;

                auto sublist = A_lines[a];
                Int point_count = static_cast<Int>(sublist.end() - sublist.begin());
                if (point_count < 2) continue;

                // Pick the middle segment of the polyline
                Int mid = point_count / 2;
                auto it0 = sublist.begin();
                auto it1 = sublist.begin();
                std::advance(it0, mid - 1);
                std::advance(it1, mid);

                Int x0 = (*it0)[0], y0 = (*it0)[1];
                Int x1 = (*it1)[0], y1 = (*it1)[1];

                // Segment midpoint
                Int mx = (x0 + x1) / 2;
                Int my = (y0 + y1) / 2;

                // Segment direction
                int seg_dir;
                if      (x1 > x0) seg_dir = 0; // East
                else if (y1 > y0) seg_dir = 1; // North
                else if (x1 < x0) seg_dir = 2; // West
                else               seg_dir = 3; // South

                // Step perpendicular toward face interior
                // d=0 (face is RIGHT of forward): step (seg_dir + 3) % 4
                // d=1 (face is LEFT  of forward): step (seg_dir + 1) % 4
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
            for (Int f = 0; f < H.FaceCount(); ++f)
            {
                Int seed_x = -1, seed_y = -1;
                if (FindFaceSeed(f, seed_x, seed_y))
                {
                    FloodFillFace(f, seed_x, seed_y);
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

        OrthoDecorate(OrthoDraw_T & H_)
        : H         { H_ }
        , n_x       { H_.Width()  * H_.Settings().x_grid_size + Int(2) }
        , n_y       { H_.Height() * H_.Settings().y_grid_size + Int(1) }
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

            Int start_cost = df.max_dist - DistanceAt(df, start[0], start[1]);
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
        // Public API: Grid dimensions
        //======================================================================

        Int GridWidth()  const { return n_x; }
        Int GridHeight() const { return n_y; }

        //======================================================================
        // Public API: Stretch factors (for rendering backends)
        //======================================================================

        Int StretchX() const { return stretch_x; }
        Int StretchY() const { return stretch_y; }

        //======================================================================
        // Public API: Access underlying OrthoDraw
        //======================================================================

        OrthoDraw_T & GetOrthoDraw() { return H; }
        const OrthoDraw_T & GetOrthoDraw() const { return H; }

    }; // class OrthoDecorate

} // namespace Knoodle
