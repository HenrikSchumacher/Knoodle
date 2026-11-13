#pragma once

// From a drawing back to a diagram: the PlanarDiagram a PlaneDrawing draws,
// with the drawing's own labels. Whatever surgery a move does on a drawing,
// this says what diagram it left, and that can be compared with the trace's
// next record. The includer has included Knoodle.hpp.

#include "PlaneDrawing.hpp"

#include <string>
#include <vector>

namespace Knoodle::Animate
{
    /*!@brief The diagram that D draws: crossing c and arc a of the result are
     * crossing c and arc a of D. Handedness and the left and right slots are
     * read off the geometry: with both strands leaving a crossing upward, the
     * out-left arc is the one counterclockwise from the other, and a crossing
     * is right-handed when the strand to out-right is the over strand. Returns
     * an invalid diagram, with a reason, if D has no crossing. */
    template<class PD_T>
    PD_T ToPlanarDiagram( const PlaneDrawing & D, std::string & why )
    {
        using Int = typename PD_T::Int;
        const Int n_c = static_cast<Int>(D.crossings.size()), n_a = static_cast<Int>(D.arcs.size());
        std::vector<Int>             C  (static_cast<std::size_t>(4 * n_c), PD_T::Uninitialized);
        std::vector<CrossingState_T> CS (static_cast<std::size_t>(n_c), CrossingState_T::Inactive);
        std::vector<Int>             A  (static_cast<std::size_t>(2 * n_a), PD_T::Uninitialized);
        std::vector<ArcState_T>      AS (static_cast<std::size_t>(n_a), ArcState_T::Inactive);
        std::vector<Int>             AC (static_cast<std::size_t>(n_a), PD_T::Uninitialized);
        Int active = 0;
        for( Idx c = 0; c < D.crossings.size(); ++c )
        {
            const PlaneDrawing::Crossing & X = D.crossings[c];
            if( X.over[1] == None ) { continue; }
            const Polyline & o = D.arcs[X.over[1]].pts, & u = D.arcs[X.under[1]].pts;
            if( o.size() < 2 || u.size() < 2 ) { why = "an arc at crossing " + std::to_string(c) + " has no direction"; return PD_T(); }
            const Real turn = (o[1][0] - o[0][0]) * (u[1][1] - u[0][1]) - (o[1][1] - o[0][1]) * (u[1][0] - u[0][0]);
            if( turn == 0 ) { why = "the strands at crossing " + std::to_string(c) + " leave it along one line"; return PD_T(); }
            const bool rightQ = (turn > 0);         // the under strand leaves on the left
            Int * slot = &C[4 * c];                 // [2 io + lr]: Out = 0, In = 1; Left = 0, Right = 1
            slot[0] = static_cast<Int>(rightQ ? X.under[1] : X.over[1]);
            slot[1] = static_cast<Int>(rightQ ? X.over[1]  : X.under[1]);
            slot[2] = static_cast<Int>(rightQ ? X.over[0]  : X.under[0]);
            slot[3] = static_cast<Int>(rightQ ? X.under[0] : X.over[0]);
            CS[c] = rightQ ? CrossingState_T::RightHanded : CrossingState_T::LeftHanded;
            ++active;
        }
        if( active == 0 ) { why = "the drawing has no crossing"; return PD_T(); }
        for( Idx a = 0; a < D.arcs.size(); ++a )
        {
            if( !D.ActiveQ(a) ) { continue; }
            A[2 * a] = static_cast<Int>(D.arcs[a].tail); A[2 * a + 1] = static_cast<Int>(D.arcs[a].head);
            AS[a] = ArcState_T::Active;
            AC[a] = static_cast<Int>(D.colors[D.arcs[a].component]);
        }
        return PD_T(n_c, C.data(), CS.data(), A.data(), AS.data(), AC.data(), Int(-1), false, false);
    }

} // namespace Knoodle::Animate
