/**
 * @file pass_view.hpp
 * @brief Shared pass-move view rendering: the canvas pipeline knoodledraw uses
 * to draw a `--move` descriptor's before/after/both views.
 *
 * These helpers were factored out of tools/knoodledraw.cpp verbatim so that
 * tests can build the exact same drawing canvas in-process instead of
 * re-implementing (and drifting from) the tool's pipeline.
 *
 * This header assumes `Knoodle.hpp` (and hence `src/OrthoDraw.hpp` /
 * `src/OrthoDecorate.hpp`) has already been included by the includer, which is
 * how the other headers in this repo behave.
 */

#pragma once

#include "diagram_agreement.hpp"
#include "drawing_extractor.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace KnoodlePassView
{

// Strand = the arcs of a --move descriptor's rerouted strand W. It is a
// highlight rather than an overlay: those arcs already exist in the diagram,
// we only need to tell them apart from the corridor that replaces them.
enum class HighlightType : uint8_t {
    None = 0, Arc = 1, Crossing = 2, Face = 3, Pass = 4, Strand = 5
};

/**
 * @brief Pad the drawing canvas (diagram string + parallel cell maps) by
 * `m` blank cells on every side, updating n_x/n_y.
 *
 * OrthoDecorate is given a margin so corridors through the drawn exterior
 * face can route AROUND the diagram (the unpadded exterior clip is
 * disconnected corner pockets); its route coordinates include that margin,
 * so the canvas must grow to match before stamping.
 */
template<class PD_T>
void PadCanvas(std::string& diagram, std::vector<HighlightType>& mask,
               std::vector<typename PD_T::Int>& comp_map,
               typename PD_T::Int& n_x, typename PD_T::Int& n_y,
               typename PD_T::Int m)
{
    using Int = typename PD_T::Int;

    const Int old_nx = n_x, old_ny = n_y;
    const Int new_nx = n_x + 2 * m, new_ny = n_y + 2 * m;

    std::vector<std::string> lines;
    {
        std::istringstream iss(diagram);
        std::string line;
        while (std::getline(iss, line)) lines.push_back(line);
    }
    lines.resize(static_cast<std::size_t>(old_ny));

    const std::string blank(static_cast<std::size_t>(new_nx - 1), ' ');
    std::string out;
    out.reserve(static_cast<std::size_t>(new_nx * new_ny));

    for (Int r = 0; r < m; ++r) { out += blank; out += '\n'; }
    for (Int r = 0; r < old_ny; ++r)
    {
        std::string& row = lines[static_cast<std::size_t>(r)];
        row.resize(static_cast<std::size_t>(old_nx - 1), ' ');
        out.append(static_cast<std::size_t>(m), ' ');
        out += row;
        out.append(static_cast<std::size_t>(m), ' ');
        out += '\n';
    }
    for (Int r = 0; r < m; ++r) { out += blank; out += '\n'; }

    auto pad_map = [&](auto& map, auto fill)
    {
        if (map.empty()) return;
        std::decay_t<decltype(map)> padded(
            static_cast<std::size_t>(new_nx * new_ny), fill);
        for (Int r = 0; r < old_ny; ++r)
        {
            for (Int c = 0; c < old_nx - 1; ++c)
            {
                padded[static_cast<std::size_t>((r + m) * new_nx + (c + m))] =
                    map[static_cast<std::size_t>(r * old_nx + c)];
            }
        }
        map = std::move(padded);
    };

    pad_map(mask, HighlightType::None);
    pad_map(comp_map, Int(-1));

    diagram = std::move(out);
    n_x = new_nx;
    n_y = new_ny;
}

/**
 * @brief Stamp a rendered pass route into the ASCII diagram + highlight mask.
 *
 * Overlay strokes use placeholder characters that UnicodeifyDiagram maps to
 * heavy box-drawing glyphs (=;{}[]* — in --ascii mode they are printed as
 * is). CrossUnder cells keep the crossed arc's own glyph and color (the
 * corridor visibly breaks); Anchor cells keep their glyph but get crossing
 * emphasis.
 */
template<class PD_T>
void StampPassOverlay(std::string& diagram, std::vector<HighlightType>& mask,
                      typename PD_T::Int n_x, typename PD_T::Int n_y,
                      const std::vector<
                          typename Knoodle::OrthoDecorate<PD_T>::OverlayCell_T
                      >& cells,
                      char marker = '\0')
{
    using Int    = typename PD_T::Int;
    using Deco_T = Knoodle::OrthoDecorate<PD_T>;

    if (mask.empty())
    {
        mask.assign(static_cast<std::size_t>(n_x * n_y), HighlightType::None);
    }

    // In --mono the corridor is labelled instead of coloured: drop a marker
    // every `marker_period` plain strokes, far enough apart to stay readable
    // and never on an arrow, corner or crossing cell. A corridor too short to
    // reach the period still gets one, at its middle plain cell -- otherwise a
    // short corridor would be indistinguishable from W, which is also heavy.
    constexpr int marker_period = 9;

    std::vector<std::size_t> plain_cells;
    if (marker)
    {
        for (std::size_t k = 0; k < cells.size(); ++k)
        {
            if ((cells[k].kind == Deco_T::OverlayKind::Horizontal)
             || (cells[k].kind == Deco_T::OverlayKind::Vertical))
            {
                plain_cells.push_back(k);
            }
        }
    }

    std::set<std::size_t> marked;
    if (plain_cells.size() >= static_cast<std::size_t>(marker_period))
    {
        for (std::size_t k = marker_period / 2; k < plain_cells.size();
             k += marker_period)
        {
            marked.insert(plain_cells[k]);
        }
    }
    else if (!plain_cells.empty())
    {
        marked.insert(plain_cells[plain_cells.size() / 2]);
    }

    std::size_t cell_index = 0;

    for (const auto& cell : cells)
    {
        const std::size_t this_cell = cell_index++;

        if (cell.x < 0 || cell.x >= n_x - 1 || cell.y < 0 || cell.y >= n_y)
            continue;

        auto idx = static_cast<std::size_t>(
            (n_y - Int(1) - cell.y) * n_x + cell.x);
        if (idx >= diagram.size() || idx >= mask.size()) continue;

        char ch = 0;
        switch (cell.kind)
        {
            case Deco_T::OverlayKind::Horizontal: ch = '='; break;
            case Deco_T::OverlayKind::Vertical:   ch = ';'; break;
            case Deco_T::OverlayKind::CornerNE:   ch = '{'; break;
            case Deco_T::OverlayKind::CornerNW:   ch = '}'; break;
            case Deco_T::OverlayKind::CornerSE:   ch = '['; break;
            case Deco_T::OverlayKind::CornerSW:   ch = ']'; break;
            case Deco_T::OverlayKind::Dot:        ch = '*'; break;
            case Deco_T::OverlayKind::CrossOverH: ch = '='; break;
            case Deco_T::OverlayKind::CrossOverV: ch = ';'; break;
            // Orientation arrows on the corridor. These reuse the diagram's own
            // arrow glyphs: unambiguous in context, since they only ever sit
            // inside a run of corridor strokes (and UnicodeifyDiagram already
            // maps them to the right arrows).
            case Deco_T::OverlayKind::ArrowN:     ch = '^'; break;
            case Deco_T::OverlayKind::ArrowE:     ch = '>'; break;
            case Deco_T::OverlayKind::ArrowS:     ch = 'v'; break;
            case Deco_T::OverlayKind::ArrowW:     ch = '<'; break;
            case Deco_T::OverlayKind::CrossUnder:
                continue;  // arc's glyph and color stay: corridor breaks
            case Deco_T::OverlayKind::Anchor:
                mask[idx] = HighlightType::Crossing;
                continue;  // emphasis only, glyph unchanged
        }

        if (marker && marked.count(this_cell)) { ch = marker; }

        diagram[idx] = ch;
        mask[idx]    = HighlightType::Pass;
    }
}

/**
 * @brief Turn the padded canvas into the AFTER view of the two-deletions
 * contract (docs/move-descriptor.md): delete the strand W.
 *
 * W's strokes are erased beyond the dots -- the shared stubs (anchor -> dot
 * on the strand's end arcs) survive, because they are the start of the
 * rerouted strand as much as they were the start of W. At each of W's
 * interior crossings the transversal was interrupted (if W ran over) or ran
 * through the crossing cell (if W ran under, in which case the cell held the
 * transversal's own glyph and erasing W's chain blanked it); either way the
 * strand runs STRAIGHT through a crossing in an orthogonal drawing, so the
 * healed transversal is the perpendicular stroke through that cell.
 *
 * Coordinates are padded-canvas coordinates: OrthoDraw vertex coordinates
 * plus `margin`, the same system the routed corridor uses. The corridor is
 * stamped separately (StampPassOverlay) after this runs.
 */
/*!@brief One drawn cell of an arc's gapless vertex-chain walk (tail -> head).
 *
 * `dir`: 0=E, 1=N, 2=W, 3=S, the direction of the segment the cell lies on.
 * `pos` numbers the cells along the walk, matching `OrthoDecorate`'s own
 * `TraverseArcCells` (and hence `PortalPoint_T::pos`), so a dot's `pos` can be
 * looked up here.
 */
template<class PD_T>
struct ArcCell_T
{
    typename PD_T::Int x, y, pos;
    int dir;
};

/*!@brief Walk arc `a`'s drawn cells, tail to head, in canvas coordinates.
 *
 * This is `ArcVertices()` + `VertexCoordinates()`, NOT `ArcLines()`: the
 * latter shortens an arc wherever it runs under a crossing (that is how the
 * drawing shows the interruption), and those gaps would put holes in a walk
 * that is supposed to be the arc's true, gapless geometry.
 */
template<class PD_T>
std::vector<ArcCell_T<PD_T>> ArcWalkCells(
    Knoodle::OrthoDraw<PD_T>& H, typename PD_T::Int a, typename PD_T::Int margin)
{
    using Int = typename PD_T::Int;

    static const Int sdx[] = {1, 0, -1, 0};
    static const Int sdy[] = {0, 1, 0, -1};

    const auto& A_V      = H.ArcVertices();
    const auto& V_coords = H.VertexCoordinates();

    std::vector<ArcCell_T<PD_T>> cells;

    auto verts = A_V[a];
    auto it  = verts.begin();
    auto end = verts.end();
    if (it == end) return cells;

    Int px = V_coords(*it, 0) + margin;
    Int py = V_coords(*it, 1) + margin;
    ++it;

    Int pos = 0;
    bool first = true;

    for (; it != end; ++it)
    {
        Int qx = V_coords(*it, 0) + margin;
        Int qy = V_coords(*it, 1) + margin;
        if (px == qx && py == qy) continue;

        int dir = (qx > px) ? 0 : (qy > py) ? 1 : (qx < px) ? 2 : 3;

        if (first) { cells.push_back({px, py, pos, dir}); first = false; }

        Int steps = std::abs(qx - px) + std::abs(qy - py);
        for (Int s = 1; s < steps; ++s)
        {
            ++pos;
            cells.push_back({px + s * sdx[dir], py + s * sdy[dir], pos, dir});
        }
        ++pos;
        cells.push_back({qx, qy, pos, dir});

        px = qx; py = qy;
    }
    return cells;
}

template<class PD_T>
void ApplyAfterView(Knoodle::OrthoDraw<PD_T>& H, std::string& diagram,
                    std::vector<HighlightType>& mask,
                    typename PD_T::Int n_x, typename PD_T::Int n_y,
                    typename PD_T::Int margin,
                    const typename Knoodle::OrthoDecorate<PD_T>::PassMove_T& move,
                    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T& pr)
{
    using Int    = typename PD_T::Int;
    using Deco_T = Knoodle::OrthoDecorate<PD_T>;

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };
    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return x >= 0 && x < n_x - 1 && y >= 0 && y < n_y;
    };

    using CellRec = ArcCell_T<PD_T>;

    auto collect = [&](Int a) -> std::vector<CellRec>
    {
        return ArcWalkCells<PD_T>(H, a, margin);
    };

    auto erase_cell = [&](Int x, Int y)
    {
        if (!in_bounds(x, y)) return;
        auto i = idx(x, y);
        if (i >= diagram.size()) return;
        diagram[i] = ' ';
        if (i < mask.size()) mask[i] = HighlightType::None;
    };

    const std::size_t L = move.strand.size();

    auto pos_of = [](const std::vector<CellRec>& cells,
                     const std::array<Int,2>& dot) -> Int
    {
        for (const auto& c : cells)
        {
            if (c.x == dot[0] && c.y == dot[1]) return c.pos;
        }
        return Int(-1);
    };

    for (std::size_t i = 0; i < L; ++i)
    {
        const Int  a   = Deco_T::PassMove_T::ArcOf(move.strand[i]);
        const bool fwd = Deco_T::PassMove_T::DirOf(move.strand[i]);
        const bool firstQ = (i == 0);
        const bool lastQ  = (i + 1 == L);

        if (!H.EdgeActiveQ(a)) continue;

        auto cells = collect(a);

        // Which walk positions get erased. Interior arcs vanish whole; end
        // arcs keep the shared stub between their anchor and their dot.
        Int lo = std::numeric_limits<Int>::min();
        Int hi = std::numeric_limits<Int>::max();   // erase pos in (lo, hi)

        if (firstQ && lastQ)
        {
            // One-arc strand: keep both stubs, erase strictly between the
            // dots (RoutePassMove guarantees the tail dot precedes the head
            // dot along the strand).
            const Int tp = pos_of(cells, pr.tail_dot);
            const Int hp = pos_of(cells, pr.head_dot);
            if (tp < 0 || hp < 0) continue;   // cannot happen for a routed move
            lo = std::min(tp, hp);
            hi = std::max(tp, hp);
        }
        else if (firstQ)
        {
            const Int tp = pos_of(cells, pr.tail_dot);
            if (tp < 0) continue;
            // Anchor sits at the walk's start iff the strand runs the arc
            // forward; the stub is the anchor side of the dot.
            if (fwd) { lo = tp; }
            else     { hi = tp; }
        }
        else if (lastQ)
        {
            const Int hp = pos_of(cells, pr.head_dot);
            if (hp < 0) continue;
            if (fwd) { hi = hp; }
            else     { lo = hp; }
        }

        for (const auto& c : cells)
        {
            if (c.pos > lo && c.pos < hi) erase_cell(c.x, c.y);
        }
    }

    // Heal the transversals. The interior crossing between strand arcs i-1
    // and i sits at the end of arc i-1's walk (its head end if the strand
    // runs it forward, its tail end otherwise); the endpoint record's `dir`
    // is W's direction through the crossing, and the transversal is the
    // perpendicular stroke.
    for (std::size_t i = 1; i < L; ++i)
    {
        const Int  a   = Deco_T::PassMove_T::ArcOf(move.strand[i - 1]);
        const bool fwd = Deco_T::PassMove_T::DirOf(move.strand[i - 1]);

        if (!H.EdgeActiveQ(a)) continue;

        auto cells = collect(a);
        if (cells.empty()) continue;

        const CellRec& x = fwd ? cells.back() : cells.front();

        if (!in_bounds(x.x, x.y)) continue;
        auto ci = idx(x.x, x.y);
        if (ci >= diagram.size()) continue;

        const bool w_horizontalQ = (x.dir % 2 == 0);
        diagram[ci] = w_horizontalQ ? '|' : '-';
        if (ci < mask.size()) mask[ci] = HighlightType::None;
    }
}

//==============================================================================
// Convenience entry point: build one view's plain canvas
//==============================================================================

enum class PassViewKind : std::uint8_t { Both, Before, After };

template<class PD_T>
struct Canvas_T
{
    std::string chars;                  // rows of length n_x, col n_x-1 is '\n'
    typename PD_T::Int n_x = 0;
    typename PD_T::Int n_y = 0;
};

/*!@brief Build the plain ASCII canvas for one view of a pass move.
 *
 * No labels, no colour, no mono markers -- just the strokes, so that a
 * parser can read the drawing back. This is the same pipeline
 * knoodledraw uses (DiagramString -> pad -> after-view deletion ->
 * overlay stamp), which is the point: the test parses the drawing the
 * tool actually makes, not a replica of it.
 */
template<class PD_T>
Canvas_T<PD_T> RenderPassView(
    Knoodle::OrthoDraw<PD_T> & H,
    const typename Knoodle::OrthoDecorate<PD_T>::PassMove_T & move,
    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T & pr,
    Knoodle::OrthoDecorate<PD_T> & deco,
    typename PD_T::Int margin,
    PassViewKind view )
{
    using Int    = typename PD_T::Int;
    using Deco_T = Knoodle::OrthoDecorate<PD_T>;

    std::string diagram = H.DiagramString();

    // Virtual edges are drawn as '.' by DiagramString(); they are not strokes.
    std::replace(diagram.begin(), diagram.end(), '.', ' ');

    Int n_x = H.Width()  * H.Settings().x_grid_size + 2;
    Int n_y = H.Height() * H.Settings().y_grid_size + 1;

    std::vector<HighlightType> mask;          // stays empty: no highlighting
    std::vector<Int>           component_map; // stays empty: no components

    PadCanvas<PD_T>(diagram, mask, component_map, n_x, n_y, margin);

    auto cells = deco.RenderPassRoute(pr);

    if (view == PassViewKind::After)
    {
        ApplyAfterView<PD_T>(H, diagram, mask, n_x, n_y, margin, move, pr);
    }
    else if (view == PassViewKind::Before)
    {
        std::erase_if(cells, [](const typename Deco_T::OverlayCell_T& c)
        {
            return (c.kind != Deco_T::OverlayKind::Dot)
                && (c.kind != Deco_T::OverlayKind::Anchor);
        });
    }

    StampPassOverlay<PD_T>(diagram, mask, n_x, n_y, cells, '\0');

    return Canvas_T<PD_T>{ std::move(diagram), n_x, n_y };
}

//==============================================================================
// The swept disk
//
// Between the two dots, the strand W and the corridor P are two paths with the
// same endpoints, so together they close up into a loop. When they do not
// cross each other that loop is a Jordan curve and the region it bounds is
// exactly the disk W sweeps out as it slides over to P -- the move's "before"
// and "after" positions are its two sides. When they do cross, the loop is not
// simple and the complement has several bounded pieces; we take the LARGEST,
// which is what the eye reads as the disk anyway.
//
// Note the disk is a region of the PLANE, not a face of the diagram: other
// strands run across it, and it is the union of every face they cut it into.
// So the flood below walls off only W and P and passes freely over everything
// else.
//==============================================================================

/*!@brief The cells of the closed curve formed by W (between the dots) and the
 * corridor, in canvas coordinates.
 */
template<class PD_T>
std::vector<std::array<typename PD_T::Int,2>> PassLoopCells(
    Knoodle::OrthoDraw<PD_T>& H,
    const typename Knoodle::OrthoDecorate<PD_T>::PassMove_T& move,
    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T& pr,
    typename PD_T::Int margin)
{
    using Int    = typename PD_T::Int;
    using Deco_T = Knoodle::OrthoDecorate<PD_T>;
    using Cell_T = std::array<Int,2>;

    std::vector<Cell_T> loop;

    // The corridor, dots included (they are its first and last cells).
    for (const auto& c : pr.route.path) { loop.push_back(Cell_T{c[0], c[1]}); }

    auto pos_of = [](const std::vector<ArcCell_T<PD_T>>& cells,
                     const std::array<Int,2>& dot) -> Int
    {
        for (const auto& c : cells)
        {
            if (c.x == dot[0] && c.y == dot[1]) return c.pos;
        }
        return Int(-1);
    };

    const std::size_t L = move.strand.size();

    for (std::size_t i = 0; i < L; ++i)
    {
        const Int  a   = Deco_T::PassMove_T::ArcOf(move.strand[i]);
        const bool fwd = Deco_T::PassMove_T::DirOf(move.strand[i]);
        const bool firstQ = (i == 0);
        const bool lastQ  = (i + 1 == L);

        if (!H.EdgeActiveQ(a)) continue;

        auto cells = ArcWalkCells<PD_T>(H, a, margin);

        // Same spans ApplyAfterView deletes, but INCLUSIVE of the dots: the
        // dots are where the loop closes.
        Int lo = std::numeric_limits<Int>::min();
        Int hi = std::numeric_limits<Int>::max();

        if (firstQ && lastQ)
        {
            const Int tp = pos_of(cells, pr.tail_dot);
            const Int hp = pos_of(cells, pr.head_dot);
            if (tp < 0 || hp < 0) continue;
            lo = std::min(tp, hp);
            hi = std::max(tp, hp);
        }
        else if (firstQ)
        {
            const Int tp = pos_of(cells, pr.tail_dot);
            if (tp < 0) continue;
            if (fwd) { lo = tp; } else { hi = tp; }
        }
        else if (lastQ)
        {
            const Int hp = pos_of(cells, pr.head_dot);
            if (hp < 0) continue;
            if (fwd) { hi = hp; } else { lo = hp; }
        }

        for (const auto& c : cells)
        {
            if (c.pos >= lo && c.pos <= hi) { loop.push_back(Cell_T{c.x, c.y}); }
        }
    }

    return loop;
}

/*!@brief Which canvas cells lie in the swept disk.
 *
 * Returns a flat `n_x * n_y` flag array (canvas indexing, as everywhere else
 * here). Empty if the loop encloses nothing -- which is the honest answer when
 * W and P run alongside each other with no room between them.
 */
template<class PD_T>
std::vector<char> PassDiskCells(
    Knoodle::OrthoDraw<PD_T>& H,
    const typename Knoodle::OrthoDecorate<PD_T>::PassMove_T& move,
    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T& pr,
    typename PD_T::Int margin,
    typename PD_T::Int n_x, typename PD_T::Int n_y)
{
    using Int = typename PD_T::Int;

    const auto N = static_cast<std::size_t>(n_x * n_y);

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };
    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return x >= 0 && x < n_x - 1 && y >= 0 && y < n_y;
    };

    std::vector<char> wall(N, char(0));
    for (const auto& c : PassLoopCells<PD_T>(H, move, pr, margin))
    {
        if (in_bounds(c[0], c[1])) { wall[idx(c[0], c[1])] = char(1); }
    }

    // Flood the outside. 4-connected against a 4-connected loop: a closed
    // 4-connected curve does block a 4-connected flood, which is what makes
    // "inside" well defined on the grid at all.
    static const Int dx4[] = {1, 0, -1, 0};
    static const Int dy4[] = {0, 1, 0, -1};

    std::vector<char> outside(N, char(0));
    std::vector<std::array<Int,2>> stack;

    for (Int y = 0; y < n_y; ++y)
    {
        for (Int x = 0; x < n_x - 1; ++x)
        {
            const bool ringQ = (x == 0) || (x == n_x - 2)
                            || (y == 0) || (y == n_y - 1);
            if (!ringQ) continue;
            const auto i = idx(x,y);
            if (wall[i] || outside[i]) continue;
            outside[i] = char(1);
            stack.push_back({x,y});
        }
    }

    while (!stack.empty())
    {
        const auto p = stack.back(); stack.pop_back();
        for (int d = 0; d < 4; ++d)
        {
            const Int qx = p[0] + dx4[d], qy = p[1] + dy4[d];
            if (!in_bounds(qx,qy)) continue;
            const auto j = idx(qx,qy);
            if (wall[j] || outside[j]) continue;
            outside[j] = char(1);
            stack.push_back({qx,qy});
        }
    }

    // What is left, minus the loop itself, is the enclosed area -- possibly in
    // several pieces if W and P cross. Keep the biggest.
    std::vector<char> seen(N, char(0));
    std::vector<char> best;
    std::size_t best_size = 0;

    for (Int y = 0; y < n_y; ++y)
    for (Int x = 0; x < n_x - 1; ++x)
    {
        const auto i0 = idx(x,y);
        if (wall[i0] || outside[i0] || seen[i0]) continue;

        std::vector<char> comp(N, char(0));
        std::size_t size = 0;

        seen[i0] = char(1); comp[i0] = char(1); ++size;
        stack.push_back({x,y});

        while (!stack.empty())
        {
            const auto p = stack.back(); stack.pop_back();
            for (int d = 0; d < 4; ++d)
            {
                const Int qx = p[0] + dx4[d], qy = p[1] + dy4[d];
                if (!in_bounds(qx,qy)) continue;
                const auto j = idx(qx,qy);
                if (wall[j] || outside[j] || seen[j]) continue;
                seen[j] = char(1); comp[j] = char(1); ++size;
                stack.push_back({qx,qy});
            }
        }

        if (size > best_size) { best_size = size; best = std::move(comp); }
    }

    return best;
}

//==============================================================================
// The two-deletions check (docs/move-descriptor.md)
//
// Render a deletion view, read it back with the drawing parser, and compare it
// port-by-port to the diagram that view is supposed to be. The correspondence
// is GEOMETRIC -- each parsed crossing is matched to the crossing whose grid
// cell it was read from -- which makes this stronger than asking whether the
// two are isomorphic. A corridor attached to the wrong port of an anchor draws
// a perfectly legal diagram, often of the right knot; only insisting on the
// map the geometry dictates catches it.
//==============================================================================

/*!@brief Match parsed crossings to crossings of the diagram that was drawn.
 *
 * `pr` may be null, in which case only crossings of `pd` are candidates (the
 * BEFORE view). With it, a parsed crossing that is not one of `pd`'s may
 * instead be one of the corridor's, identified by its position along the
 * corridor -- which is exactly the order `AfterDiagram` appends them in.
 */
template<class PD_T>
bool PassViewSeeds(
    const typename KnoodleDrawIO::DrawingExtractor<PD_T>::Result_T & R,
    const PD_T & pd,
    const Knoodle::OrthoDraw<PD_T> & H,
    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T * pr,
    typename PD_T::Int margin,
    std::vector<std::array<typename PD_T::Int,2>> & seeds,
    std::string & why )
{
    using Int = typename PD_T::Int;

    const auto & V   = H.VertexCoordinates();
    const Int    n_c = pd.MaxCrossingCount();

    seeds.clear();

    for( std::size_t i = 0; i < R.crossing_cell.size(); ++i )
    {
        const Int x = R.crossing_cell[i][0], y = R.crossing_cell[i][1];

        Int match = Int(-1);

        for( Int c = 0; c < n_c; ++c )
        {
            if( !pd.CrossingActiveQ(c) ) { continue; }
            if( (V(c,0) + margin == x) && (V(c,1) + margin == y) )
            {
                match = c;
                break;
            }
        }

        if( (match < 0) && (pr != nullptr) )
        {
            for( std::size_t j = 0; j < pr->route.crossing_indices.size(); ++j )
            {
                const auto & cell = pr->route.path[
                    static_cast<std::size_t>(pr->route.crossing_indices[j])];
                if( (cell[0] == x) && (cell[1] == y) )
                {
                    match = n_c + static_cast<Int>(j);
                    break;
                }
            }
        }

        if( match < 0 )
        {
            why = "the drawing has a crossing at (" + std::to_string(x) + ","
                + std::to_string(y) + ") that is neither a crossing of the"
                " diagram nor a crossing of the corridor";
            return false;
        }

        seeds.push_back({ static_cast<Int>(i), match });
    }

    return true;
}

/*!@brief Check one deletion view against the diagram it should depict.
 *
 * `view` selects the deletion; `truth` is what that view is supposed to be
 * (the input diagram for Before, `AfterDiagram`'s result for After). Returns
 * false with `why` set if the drawing does not parse, if a parsed crossing has
 * no counterpart, or if the structures disagree.
 */
template<class PD_T>
bool CheckPassView(
    Knoodle::OrthoDraw<PD_T> & H,
    Knoodle::OrthoDecorate<PD_T> & deco,
    const PD_T & pd,
    const typename Knoodle::OrthoDecorate<PD_T>::PassMove_T & move,
    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T & pr,
    const PD_T & truth,
    typename PD_T::Int margin,
    PassViewKind view,
    std::string & why,
    typename PD_T::Int expect_free_loops = 0 )
{
    using Int       = typename PD_T::Int;
    using Extract_T = KnoodleDrawIO::DrawingExtractor<PD_T>;

    if( view == PassViewKind::Both )
    {
        why = "the superposition is not a deletion: at a dot the strand and the"
              " corridor branch, so there is no single diagram to compare to";
        return false;
    }

    auto canvas = RenderPassView<PD_T>(H, move, pr, deco, margin, view);

    auto R = Extract_T::Extract(canvas.chars, canvas.n_x, canvas.n_y);
    if( !R.okQ )
    {
        why = "the drawing does not parse: " + R.why;
        return false;
    }

    std::vector<std::array<Int,2>> seeds;
    const auto * prp = (view == PassViewKind::After) ? &pr : nullptr;

    if( !PassViewSeeds<PD_T>(R, pd, H, prp, margin, seeds, why) )
    {
        return false;
    }

    if( !DiagramsAgreeQ(R.pd, truth, seeds, why) )
    {
        return false;
    }

    // Crossingless closed curves. A pass move can genuinely produce these --
    // when a transversal closes up through nothing but interior crossings of
    // W, deleting W leaves it with no crossings at all -- so the count is
    // CHECKED against what AfterDiagram reported splitting off, not merely
    // required to be zero. The drawing and the surgery have to agree about how
    // many components came free, which is a real claim about both.
    if( R.free_loops != expect_free_loops )
    {
        why = "the drawing has " + std::to_string(R.free_loops)
            + " crossing-free closed curve(s) but the move accounts for "
            + std::to_string(expect_free_loops);
        return false;
    }

    why.clear();
    return true;
}

/*!@brief Both deletions at once: the full contract for one pass move.*/
template<class PD_T>
bool CheckBothDeletions(
    Knoodle::OrthoDraw<PD_T> & H,
    Knoodle::OrthoDecorate<PD_T> & deco,
    const PD_T & pd,
    const typename Knoodle::OrthoDecorate<PD_T>::PassMove_T & move,
    const typename Knoodle::OrthoDecorate<PD_T>::PassRoute_T & pr,
    const PD_T & after,
    typename PD_T::Int margin,
    std::string & why,
    typename PD_T::Int freed_components = 0 )
{
    // Deleting the corridor restores the diagram exactly as handed to us, so
    // nothing has come free in that view whatever the move goes on to do.
    if( !CheckPassView<PD_T>(H, deco, pd, move, pr, pd, margin,
                             PassViewKind::Before, why, 0) )
    {
        why = "deleting the corridor does not leave the diagram we were handed: "
            + why;
        return false;
    }

    // Deleting the strand is where a component can come free.
    if( !CheckPassView<PD_T>(H, deco, pd, move, pr, after, margin,
                             PassViewKind::After, why, freed_components) )
    {
        why = "deleting the strand does not leave the diagram the move produces: "
            + why;
        return false;
    }

    why.clear();
    return true;
}

} // namespace KnoodlePassView
