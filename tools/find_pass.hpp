#pragma once

// Ask Knoodle for a pass move, instead of writing one by hand.
//
// WHY THIS EXISTS. `PassSimplifier::Reroute` has exactly one stated
// precondition (Henrik, 2026-08-12): the corridor came from the shortest-path
// search and IS a shortest path. Not well-formedness, not strict shortening --
// optimality. A hand-written `--move=` descriptor satisfies none of that by
// accident, and a descriptor outside the contract can be perfectly well formed
// while the applier does something unrelated to it (that is exactly the PR #30
// reproducer: k = 8 against a minimum of 4).
//
// So the useful thing to name is not the corridor but its ENDPOINTS. Give the
// first and last arc of the strand and let `FindShortestRerouting` produce the
// corridor: the resulting move is inside `Reroute`'s contract BY
// CONSTRUCTION, and a drawing of it depicts a move production would actually
// accept and carry out -- which is the only kind worth calling a proof.
//
// `FindShortestRerouting` is Henrik's own public search, and its doc comment
// says it "is only meant for the visualization of a few paths" -- which is
// precisely what we are doing. It caps the corridor at `L-2`, so what it
// returns is both optimal and strictly simplifying. It does NOT reroute: it
// returns a `Path_T` and leaves the diagram alone.

#include <string>
#include <vector>

namespace KnoodleFindPass
{
    /*!@brief The arcs of the strand running from arc `a` to arc `b`, both
     * included, along the diagram's own orientation.
     *
     * Useful on its own: when no rerouting exists, the strand you ASKED about
     * is still the thing worth drawing, and this is what to highlight.
     */
    template<class PD_T>
    bool StrandArcs(
        const PD_T & pd, typename PD_T::Int a, typename PD_T::Int b,
        std::vector<typename PD_T::Int> & out, std::string & why )
    {
        using Int = typename PD_T::Int;

        out.clear();

        if( !pd.ArcActiveQ(a) || !pd.ArcActiveQ(b) )
        {
            why = "arc " + std::to_string(pd.ArcActiveQ(a) ? b : a)
                + " is not an active arc of this diagram";
            return false;
        }

        Int x = a, guard = 0;
        const Int lim = Int(2) * pd.MaxArcCount() + Int(2);

        out.push_back(x);
        while( (x != b) && (guard++ < lim) )
        {
            x = pd.NextArc(x, PD_T::Head);
            out.push_back(x);
        }

        if( x != b )
        {
            out.clear();
            why = "walking from arc " + std::to_string(a) + " along the diagram"
                  " never reaches arc " + std::to_string(b) + "; they are not"
                  " the ends of one strand (different components, or given"
                  " the wrong way round)";
            return false;
        }

        why.clear();
        return true;
    }

    /*!@brief Find the shortest rerouting of the strand running from arc `a` to
     * arc `b`, and return it as a pass descriptor.
     *
     * `a` and `b` are the strand's first and last arc, both included, in the
     * diagram's own orientation (the walk is `NextArc(.,Head)`). On failure
     * `why` says whether no rerouting exists or the descriptor could not be
     * built from the one found.
     *
     * The diagram is not modified. A copy goes into a `PlanarDiagramComplex`
     * because that is where the search lives; the complex is `Unlock()`ed
     * first, since its lock guard turns the operations the search needs into
     * silent no-ops.
     */
    template<class PD_T, class PDC_T, class PS_T, class Desc_T>
    bool FindPassDescriptor(
        const PD_T & pd, typename PD_T::Int a, typename PD_T::Int b,
        Desc_T & out, std::string & why )
    {
        using Int = typename PD_T::Int;

        if( !pd.ArcActiveQ(a) || !pd.ArcActiveQ(b) )
        {
            why = "arc " + std::to_string(pd.ArcActiveQ(a) ? b : a)
                + " is not an active arc of this diagram";
            return false;
        }
        if( a == b )
        {
            why = "the strand's first and last arc are the same, so it has no"
                  " interior crossing to reroute past";
            return false;
        }

        PDC_T pdc { PD_T(pd) };
        pdc.Unlock();

        auto found = pdc.FindShortestRerouting(
            Int(0), a, b, pd.MaxArcCount(),
            PDC_T::Dijkstra_T::Bidirectional );

        if( found.Size() < Int(2) )
        {
            why = "FindShortestRerouting found no rerouting of the strand from"
                  " arc " + std::to_string(a) + " to arc " + std::to_string(b)
                + " (it caps the corridor at L-2, so a strand with no strictly"
                  " shorter route through the faces has none to report)";
            return false;
        }

        // Rebuild the pass the finder was asked about. `overQ` is W's own
        // uniform role at its interior crossings; check 5 in `WellFormedQ` is
        // what decides it, so try both rather than restate the convention.
        for( bool overval : { false, true } )
        {
            typename PS_T::Pass_T pass;
            pass.first     = a;
            pass.last      = b;
            pass.overQ     = overval;
            pass.activeQ   = true;
            std::vector<Int> arcs;
            if( !StrandArcs(pd,a,b,arcs,why) ) { return false; }
            pass.arc_count = static_cast<Int>(arcs.size());

            Desc_T cand;
            if( Desc_T::FromPassAndPath(pd, pass, found, cand, why)
             && cand.WellFormedQ(pd, why) )
            {
                out = cand;
                why.clear();
                return true;
            }
        }

        why = "a rerouting was found but no well-formed descriptor could be"
              " built from it: " + why;
        return false;
    }

} // namespace KnoodleFindPass
