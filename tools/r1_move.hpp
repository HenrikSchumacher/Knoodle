/**
 * @file r1_move.hpp
 * @brief The R1 (curl removal) descriptor, its local checks, and the diagram
 * the move produces. No drawing.
 *
 * Split out of tools/r1_view.hpp so that a verifier can check an `r1` record
 * without compiling, or building, a layout -- the same division
 * tools/trace_verify.hpp draws for pass moves. r1_view.hpp includes this and
 * adds the rendering.
 *
 *   #move kind=r1 loop=<da>
 *
 * One field. `loop` is a darc `2a + d`; `a` is the loop arc, and by the
 * face-on-the-left convention **L(loop) is the monogon face that collapses** --
 * which is what makes the collapsing side unambiguous with no second field.
 *
 * This header assumes `Knoodle.hpp` has already been included by the includer.
 */

#pragma once

#include <array>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace KnoodleR1View
{

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
// The diagram the move produces
//==============================================================================

/*!@brief Remove the curl: the diagram an `r1` record's move produces, plus the
 * colours of any component that comes FREE of it.
 *
 * The r1 counterpart of `PassDescriptor::AfterDiagram`, and independent in the
 * same way: it rebuilds the arrays from `r` and the diagram, and never calls
 * `LoopRemover`. That is what lets it be used as an oracle against an applier
 * -- including Knoodle's own.
 *
 * It agrees with `LoopRemover` about which labels die, which is not a free
 * choice: `Reconnect(a_next,!d,a_prev)` keeps **a_next** alive and takes its
 * `!d` end over to where `a_prev`'s `!d` end was, then `a`, `a_prev` and the
 * crossing `c` are deactivated. A picture, an applier and a verifier that
 * disagreed about that would disagree about every label downstream.
 *
 * The spinoff case (`a_prev == a_next`) is the 8-shaped unlink: the curl's
 * component IS the loop, so removing the crossing leaves a crossingless
 * circle. A `PlanarDiagram` cannot hold one beside crossings, so its colour is
 * REPORTED in `freed`, exactly as a pass move's split-off loops are.
 *
 * That case only arises on a snapshot that is ALREADY SPLIT, or on one whose
 * every crossing is this curl. The loop's component has exactly one crossing,
 * so any other crossing belongs to a component it never meets. Hence: no
 * spinoff on a connected diagram of two or more crossings, and no drawing of
 * one either -- `OrthoDraw` lays out a connected picture, so the two-deletions
 * machinery cannot reach this path even in principle.
 */
template<class PD_T>
PD_T R1AfterDiagram(
    const PD_T & pd, const R1Resolved<PD_T> & r, std::string & why,
    std::vector<typename PD_T::Int> & freed )
{
    using Int = typename PD_T::Int;
    using CrossingState_T = Knoodle::CrossingState_T;
    using ArcState_T      = Knoodle::ArcState_T;

    freed.clear();
    why.clear();

    if( !r.validQ )
    {
        why = r.why;
        return PD_T();
    }

    const Int n_c = pd.MaxCrossingCount();
    const Int n_a = pd.MaxArcCount();

    std::vector<Int>             C ( static_cast<std::size_t>(Int(4)*n_c), PD_T::Uninitialized );
    std::vector<CrossingState_T> CS( static_cast<std::size_t>(n_c), CrossingState_T::Inactive );
    std::vector<Int>             A ( static_cast<std::size_t>(Int(2)*n_a), PD_T::Uninitialized );
    std::vector<ArcState_T>      AS( static_cast<std::size_t>(n_a), ArcState_T::Inactive );
    std::vector<Int>             AC( static_cast<std::size_t>(n_a), PD_T::Uninitialized );

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

    const Int ht = r.d ? Int(0) : Int(1);   // the `!d` end Reconnect works on

    if( r.spinoffQ )
    {
        // The whole component comes free; one colour for it, and its arcs
        // leave the diagram.
        freed.push_back(AC[static_cast<std::size_t>(r.a)]);
        AS[static_cast<std::size_t>(r.a)]      = ArcState_T::Inactive;
        AS[static_cast<std::size_t>(r.a_next)] = ArcState_T::Inactive;
    }
    else
    {
        const Int c_far = Aend(r.a_prev, ht);

        if( !pd.CrossingActiveQ(c_far) )
        {
            why = "the arc absorbed by the reconnection (" 
                + std::to_string(r.a_prev) + ") ends at crossing "
                + std::to_string(c_far) + ", which is not active";
            return PD_T();
        }

        Aend(r.a_next, ht) = c_far;

        bool repointedQ = false;
        for( Int io = 0; io < 2 && !repointedQ; ++io )
        {
            for( Int lr = 0; lr < 2; ++lr )
            {
                if( Cx(c_far,io,lr) == r.a_prev )
                {
                    Cx(c_far,io,lr) = r.a_next;
                    repointedQ = true;
                    break;
                }
            }
        }
        if( !repointedQ )
        {
            why = "crossing " + std::to_string(c_far)
                + " does not mention arc " + std::to_string(r.a_prev);
            return PD_T();
        }

        AS[static_cast<std::size_t>(r.a)]      = ArcState_T::Inactive;
        AS[static_cast<std::size_t>(r.a_prev)] = ArcState_T::Inactive;
    }

    CS[static_cast<std::size_t>(r.c)] = CrossingState_T::Inactive;

    return PD_T(
        n_c, C.data(), CS.data(), A.data(), AS.data(), AC.data(),
        pd.LastColorDeactivated(), false, false
    );
}

} // namespace KnoodleR1View
