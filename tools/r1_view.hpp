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
// Descriptor
//==============================================================================

template<class Int_>
struct R1Descriptor
{
    using Int = Int_;

    Int loop = Int(-1);   // darc 2a+d; L(loop) is the monogon

    std::string ToString() const
    {
        return "kind=r1 loop=" + std::to_string(loop);
    }

    /*!@brief Parse the payload of a `#move kind=r1` line.
     *
     * Tokenisation matches PassDescriptor::Parse deliberately: whitespace
     * separated `key=value`, a leading `#move` token tolerated, so the same
     * string works as a `--move=` argument and as a trace header payload.
     * `kind=r1` is REQUIRED -- without it a descriptor is a pass descriptor,
     * and silently guessing between the two grammars would be worse than
     * asking.
     */
    static bool Parse(const std::string& spec, R1Descriptor& out, std::string& err)
    {
        out = R1Descriptor{};

        bool have_kindQ = false, have_loopQ = false;
        std::size_t pos = 0;

        while (pos < spec.size())
        {
            while ((pos < spec.size())
                && std::isspace(static_cast<unsigned char>(spec[pos]))) { ++pos; }
            if (pos >= spec.size()) { break; }

            std::size_t end = pos;
            while ((end < spec.size())
                && !std::isspace(static_cast<unsigned char>(spec[end]))) { ++end; }

            const std::string_view tok(spec.data() + pos, end - pos);
            pos = end;

            if (tok == "#move") { continue; }

            const auto eq = tok.find('=');
            if (eq == std::string_view::npos)
            {
                err = "expected key=value, got '" + std::string(tok) + "'";
                return false;
            }

            const std::string_view key = tok.substr(0, eq);
            const std::string_view val = tok.substr(eq + 1);

            if (key == "kind")
            {
                if (val != "r1")
                {
                    err = "unsupported kind=" + std::string(val) + " (want r1)";
                    return false;
                }
                have_kindQ = true;
            }
            else if (key == "loop")
            {
                Int v = Int(0);
                if (!ParseInt(val, v))
                {
                    err = "bad loop darc '" + std::string(val) + "'";
                    return false;
                }
                out.loop = v;
                have_loopQ = true;
            }
            else
            {
                err = "unknown key '" + std::string(key) + "' in an r1 descriptor";
                return false;
            }
        }

        if (!have_kindQ) { err = "an r1 descriptor must say kind=r1"; return false; }
        if (!have_loopQ) { err = "missing loop=<darc>"; return false; }
        if (out.loop < Int(0))
        {
            err = "loop darc must be non-negative, got "
                + std::to_string(out.loop);
            return false;
        }
        return true;
    }

private:

    static bool ParseInt(std::string_view s, Int& out)
    {
        if (s.empty()) { return false; }

        bool negQ = false;
        std::size_t i = 0;
        if (s[0] == '-') { negQ = true; i = 1; }
        if (i >= s.size()) { return false; }

        Int v = Int(0);
        for (; i < s.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) { return false; }
            v = v * Int(10) + Int(s[i] - '0');
        }
        out = negQ ? -v : v;
        return true;
    }
};

//==============================================================================
// Resolution against a diagram (the local validation of the spec)
//==============================================================================

template<class PD_T>
struct R1Resolved
{
    using Int = typename PD_T::Int;

    Int  loop    = Int(-1);
    Int  a       = Int(-1);   // the loop arc
    Int  c       = Int(-1);   // the crossing that dies
    Int  a_prev  = Int(-1);   // absorbed by the Reconnect
    Int  a_next  = Int(-1);   // SURVIVES the Reconnect
    Int  monogon = Int(-1);   // L(loop): the face that collapses
    bool d       = false;
    bool spinoffQ = false;    // a_prev == a_next: the 8-shaped unlink
    bool validQ   = false;
    std::string why;
};

/*!@brief Run the r1 local checks and derive everything the drawing needs.
 *
 * Derivation follows LoopRemover (src/PlanarDiagramComplex/LoopRemover.hpp)
 * exactly, so a picture and an applier cannot disagree about which labels die:
 * `Reconnect(a_next,!d,a_prev)` keeps a_next alive, and a, a_prev, c are
 * deactivated.
 */
template<class PD_T>
R1Resolved<PD_T> ResolveR1(const PD_T& pd, typename PD_T::Int loop)
{
    using Int = typename PD_T::Int;

    R1Resolved<PD_T> r;
    r.loop = loop;

    auto fail = [&r](std::string msg) -> R1Resolved<PD_T>&
    {
        r.why = std::move(msg);
        return r;
    };

    if (loop < Int(0) || loop >= Int(2) * pd.MaxArcCount())
    {
        return fail("loop darc " + std::to_string(loop) + " is out of range");
    }

    r.a = loop / Int(2);
    r.d = ((loop % Int(2)) != Int(0));

    if (!pd.ArcActiveQ(r.a))
    {
        return fail("arc " + std::to_string(r.a) + " is not active");
    }

    // Check 1: it really is a loop arc, and its crossing is active.
    const Int c_head = pd.Arcs()(r.a, PD_T::Head);
    const Int c_tail = pd.Arcs()(r.a, PD_T::Tail);

    if (c_head != c_tail)
    {
        return fail("arc " + std::to_string(r.a) + " is not a loop arc: its head"
                    " is at crossing " + std::to_string(c_head) + " and its tail"
                    " at " + std::to_string(c_tail));
    }
    r.c = c_head;

    if (!pd.CrossingActiveQ(r.c))
    {
        return fail("crossing " + std::to_string(r.c) + " is not active");
    }

    // Check 2: L(loop) is a MONOGON. This is what pins which side collapses,
    // and it is the reason `loop` is a darc rather than an arc.
    r.monogon = pd.ArcFaces()(r.a, r.d ? Int(1) : Int(0));
    if (r.monogon < Int(0) || r.monogon >= pd.FaceCount())
    {
        return fail("the face left of darc " + std::to_string(loop)
                    + " is not a face of this diagram");
    }

    {
        auto s = pd.FaceDarcs()[r.monogon];
        const auto n = s.end() - s.begin();
        if (n != 1)
        {
            return fail("the face left of darc " + std::to_string(loop)
                        + " (face " + std::to_string(r.monogon) + ") is bounded by "
                        + std::to_string(static_cast<long long>(n))
                        + " darcs, so it is not a monogon; name the other darc of"
                          " arc " + std::to_string(r.a) + " to collapse the other"
                          " side");
        }
        if (*(s.begin()) != loop)
        {
            return fail("face " + std::to_string(r.monogon) + " is a monogon but"
                        " is bounded by darc " + std::to_string(*(s.begin()))
                        + ", not " + std::to_string(loop));
        }
    }

    // The surviving ends, derived the way LoopRemover does.
    r.a_prev = pd.NextArc(r.a, !r.d);
    r.a_next = pd.NextArc(r.a,  r.d);
    r.spinoffQ = (r.a_prev == r.a_next);

    r.validQ = true;
    return r;
}

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

} // namespace KnoodleR1View
