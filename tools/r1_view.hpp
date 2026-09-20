/**
 * @file r1_view.hpp
 * @brief R1 (curl removal) descriptor and view rendering.
 *
 * The sibling of tools/pass_view.hpp, and factored out for the same reason:
 * tests build the very same canvas in-process rather than re-implementing the
 * tool's pipeline and drifting from it.
 *
 * An R1 is NOT a pass move, and the pass grammar refuses it by name -- check
 * 4's distinct-anchors clause exists precisely to exclude "an R_I curl at the
 * end of a strand, and its relatives" (docs/move-descriptor.md). So it gets its
 * own descriptor and its own, simpler, contract:
 *
 *   #move kind=r1 loop=<da>
 *
 * One field. `loop` is a darc `2a + d`; `a` is the loop arc, and by the
 * face-on-the-left convention **L(loop) is the monogon face that collapses** --
 * which is what makes the collapsing side unambiguous with no second field.
 *
 * TWO THINGS AN R1 DOES NOT NEED, both of which a pass move does:
 *
 *  - **No margin, no PadCanvas.** A pass move routes a corridor that may leave
 *    the drawing entirely, so OrthoDecorate is given a free ring to route in.
 *    An R1 adds nothing; every cell it touches is already on the canvas.
 *  - **No corner arithmetic.** The healed strand must turn a corner at the dead
 *    crossing (the loop's two ends flank the monogon, hence are rotationally
 *    adjacent, hence so are the survivors), and it turns away from the monogon.
 *    But we never compute which corner: erasing the loop and stamping a plain
 *    '+' leaves UnicodeifyDiagram to derive the glyph from which neighbours are
 *    still connected, which is exactly the forced answer.
 *
 * And one deletion, not two. A pass move superposes two states because it ADDS
 * a corridor; an R1 adds nothing, so the before view IS the input drawing (with
 * the loop marked and the monogon shaded) and there is a single checkable
 * claim: the after view, parsed back, is the diagram the move produces.
 */

#pragma once

#include "pass_view.hpp"
#include "r1_move.hpp"

#include <array>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace KnoodleR1View
{

using KnoodlePassView::HighlightType;

//==============================================================================
// Drawing
//==============================================================================

/*!@brief The canvas cell of crossing `c`. No margin: an R1 never routes.*/
template<class PD_T>
std::array<typename PD_T::Int,2> CrossingCell(
    Knoodle::OrthoDraw<PD_T>& H, typename PD_T::Int c)
{
    const auto& V = H.VertexCoordinates();
    return { V(c, typename PD_T::Int(0)), V(c, typename PD_T::Int(1)) };
}

/*!@brief Mark the loop arc and the dying crossing for the BEFORE view.
 *
 * The monogon itself is NOT shaded here: it is a real face of the diagram, so
 * the caller shades it through the ordinary face-highlight path (the same one
 * --checkerboard-coloring uses) by adding `r.monogon` to the highlight set.
 * That is the whole reason an R1 disk is cheaper than a pass move's swept disk,
 * which is not a face and has to be computed.
 */
template<class PD_T>
void MarkR1(Knoodle::OrthoDraw<PD_T>& H, std::string& diagram,
            typename PD_T::Int n_x, typename PD_T::Int n_y,
            std::vector<HighlightType>& mask,
            const R1Resolved<PD_T>& r, char marker = '\0')
{
    using Int = typename PD_T::Int;

    if (!r.validQ) { return; }

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };
    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return (x >= Int(0)) && (x < n_x - Int(1)) && (y >= Int(0)) && (y < n_y);
    };

    if (mask.empty())
    {
        mask.assign(static_cast<std::size_t>(n_x * n_y), HighlightType::None);
    }

    if (!H.EdgeActiveQ(r.a)) { return; }

    for (const auto& cell : KnoodlePassView::ArcWalkCells<PD_T>(H, r.a, Int(0)))
    {
        if (!in_bounds(cell.x, cell.y)) { continue; }
        const auto i = idx(cell.x, cell.y);
        if (i >= mask.size()) { continue; }

        mask[i] = HighlightType::Strand;

        if ((marker != '\0') && (i < diagram.size()) && (diagram[i] != ' '))
        {
            diagram[i] = marker;
        }
    }

    // The crossing that dies. A pass move's anchors SURVIVE; this one does not,
    // so it is marked apart from them.
    const auto cc = CrossingCell<PD_T>(H, r.c);
    if (in_bounds(cc[0], cc[1]))
    {
        const auto i = idx(cc[0], cc[1]);
        if (i < mask.size()) { mask[i] = HighlightType::Crossing; }
    }
}

/*!@brief The AFTER view: delete the curl and heal the crossing.
 *
 * Erase every drawn cell of the loop arc except the crossing's own cell, then
 * stamp '+' there. What remains connected at that cell is exactly a_prev and
 * a_next, which are rotationally adjacent, so UnicodeifyDiagram resolves the
 * '+' to the corner facing away from the collapsed monogon. We never name that
 * corner; the drawing derives it.
 */
template<class PD_T>
void ApplyR1AfterView(Knoodle::OrthoDraw<PD_T>& H, std::string& diagram,
                      std::vector<HighlightType>& mask,
                      typename PD_T::Int n_x, typename PD_T::Int n_y,
                      const R1Resolved<PD_T>& r)
{
    using Int = typename PD_T::Int;

    if (!r.validQ) { return; }
    if (!H.EdgeActiveQ(r.a)) { return; }

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };
    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return (x >= Int(0)) && (x < n_x - Int(1)) && (y >= Int(0)) && (y < n_y);
    };

    const auto cc = CrossingCell<PD_T>(H, r.c);

    for (const auto& cell : KnoodlePassView::ArcWalkCells<PD_T>(H, r.a, Int(0)))
    {
        if ((cell.x == cc[0]) && (cell.y == cc[1])) { continue; }  // heal, below
        if (!in_bounds(cell.x, cell.y)) { continue; }

        const auto i = idx(cell.x, cell.y);
        if (i < diagram.size()) { diagram[i] = ' '; }
        if (i < mask.size())    { mask[i] = HighlightType::None; }
    }

    if (in_bounds(cc[0], cc[1]))
    {
        const auto i = idx(cc[0], cc[1]);
        if (i < diagram.size()) { diagram[i] = '+'; }
        if (i < mask.size())    { mask[i] = HighlightType::None; }
    }
}

//==============================================================================
// The healed corner, named
//==============================================================================

template<class Int_>
struct R1Corner_T
{
    using Int = Int_;

    Int         x = Int(-1);
    Int         y = Int(-1);
    std::string kind;        // CornerNE | CornerNW | CornerSE | CornerSW
    bool        validQ = false;
};

/*!@brief Which corner the healed strand turns at the dead crossing.
 *
 * The ASCII backend never needs this: it stamps '+' and UnicodeifyDiagram
 * resolves the glyph from which neighbours are still connected. A geometry
 * consumer has no such renderer, so the answer has to be computed -- and it is
 * computed the same way, by asking which neighbours survive.
 *
 * The loop's two ends leave `c` in two rotationally ADJACENT directions (that
 * is what makes L(loop) a monogon), so the two that remain are adjacent too and
 * name a corner rather than a straight run. Opposite survivors would mean the
 * strand runs straight through, which a monogon makes impossible; if it is ever
 * seen the result is returned invalid rather than guessed at.
 *
 * Directions follow RenderPassRoute: N = +y, E = +x. The name lists the
 * vertical arm first, matching OverlayKind (CornerNE = arms N+E).
 */
template<class PD_T>
R1Corner_T<typename PD_T::Int> R1Corner(
    Knoodle::OrthoDraw<PD_T>& H, const R1Resolved<PD_T>& r)
{
    using Int = typename PD_T::Int;

    R1Corner_T<Int> out;
    if (!r.validQ || !H.EdgeActiveQ(r.a)) { return out; }

    const auto cc = CrossingCell<PD_T>(H, r.c);
    out.x = cc[0];
    out.y = cc[1];

    static const Int  dx[] = { Int(0), Int(1), Int(0), Int(-1) };
    static const Int  dy[] = { Int(1), Int(0), Int(-1), Int(0) };
    static const char nm[] = { 'N', 'E', 'S', 'W' };

    bool loop_dirQ[4] = { false, false, false, false };

    // ArcWalkCells is gapless (ArcVertices + VertexCoordinates, never
    // ArcLines), so the cell one step from the crossing along each of the
    // loop's two ends really is part of the walk.
    for (const auto& cell : KnoodlePassView::ArcWalkCells<PD_T>(H, r.a, Int(0)))
    {
        for (int k = 0; k < 4; ++k)
        {
            if ((cell.x == cc[0] + dx[k]) && (cell.y == cc[1] + dy[k]))
            {
                loop_dirQ[k] = true;
            }
        }
    }

    int surv[4] = {-1,-1,-1,-1};
    int n_surv = 0;
    for (int k = 0; k < 4; ++k)
    {
        if (!loop_dirQ[k] && (n_surv < 4)) { surv[n_surv++] = k; }
    }
    if (n_surv != 2) { return out; }

    const int p = surv[0];
    const int q = surv[1];
    if (((p + 2) % 4) == q) { return out; }   // opposite: not a corner

    const bool p_verticalQ = ((p == 0) || (p == 2));

    out.kind = std::string("Corner");
    out.kind += p_verticalQ ? nm[p] : nm[q];
    out.kind += p_verticalQ ? nm[q] : nm[p];
    out.validQ = true;

    return out;
}

} // namespace KnoodleR1View
