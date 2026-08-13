#pragma once

// Do two PlanarDiagrams describe the same oriented diagram?
//
// Not "the same knot" and not "identical arrays": the same diagram under a
// SUPPLIED correspondence. Two implementations of one move will agree on the
// structure and disagree on every label, so a byte comparison is useless; and
// a knot-type comparison is far too weak -- it would miss a wrong splice that
// happens to preserve the invariant, and (for the MacLeod code we used to lean
// on) it does not exist for links at all.
//
// WHY A CORRESPONDENCE IS SUPPLIED RATHER THAN SEARCHED FOR. Testing whether
// two labelled diagrams are isomorphic sounds like graph isomorphism, and it
// is not. A knot diagram is a 4-regular plane graph carrying a rotation
// system, and an isomorphism of such an object is pinned down by where it
// sends a single flag: match one crossing to one crossing, and the four slots
// must map slot to slot (In/Out is fixed by the strand orientations,
// Left/Right by the crossing's handedness -- the only other candidates,
// rotations of the crossing by 90 or 180 degrees, all swap In with Out), which
// forces the four incident arcs, which forces the crossings at their far ends,
// and so on. So this is a propagation, not a search, and it is O(crossings).
//
// (If we ever wanted the unrooted question -- "are these two isomorphic at
// all?" -- the same fact is what makes that easy too: root at each of the 4n
// flags in turn and take the lexicographically least canonical traversal,
// which is Weinberg's O(n^2) planar-graph isomorphism test. General-purpose
// graph-isomorphism machinery is never needed here. We do not do that, because
// for our purposes a diagram that is isomorphic by SOME map but not by the map
// the geometry dictates is a failure, not a pass -- a corridor attached to the
// wrong port of an anchor can easily still be isomorphic to something.)
//
// The seeds therefore carry the claim being tested. When comparing an applier
// against an oracle, the seed is a crossing the move promised not to touch,
// which both diagrams still hold at its original index. When comparing a
// diagram parsed back out of a DRAWING (test/drawing_extractor.hpp), the seeds
// come from the grid: each parsed crossing is matched to the crossing whose
// cell it was read from. Seeding every crossing that way makes this check the
// strongest available -- it verifies not just that the drawing depicts a
// correct diagram, but that it depicts it in the expected place.
//
// Colors are deliberately NOT compared: they are labels of exactly the kind
// this routine exists to see past, and a parser assigns its own.

#include <array>
#include <string>
#include <vector>

/*!@brief Check that a supplied crossing correspondence extends to an
 * isomorphism of oriented diagrams.
 *
 * `seeds` is a list of `{c1, c2}` pairs asserting that crossing `c1` of `d1`
 * is crossing `c2` of `d2`. At least one seed is required per connected
 * component; supplying more is a stronger check, since every one of them must
 * survive propagation. On failure `why` names the crossing or arc where the
 * claim broke.
 */
template<typename PD_T>
bool DiagramsAgreeQ(
    const PD_T & d1, const PD_T & d2,
    const std::vector<std::array<typename PD_T::Int,2>> & seeds,
    std::string & why )
{
    using Int = typename PD_T::Int;

    auto fail = [&why]( std::string msg ) -> bool
    {
        why = std::move(msg);
        return false;
    };

    if( d1.CrossingCount() != d2.CrossingCount() )
    {
        return fail("crossing counts differ: " + std::to_string(d1.CrossingCount())
            + " vs " + std::to_string(d2.CrossingCount()));
    }
    if( d1.ArcCount() != d2.ArcCount() )
    {
        return fail("arc counts differ: " + std::to_string(d1.ArcCount())
            + " vs " + std::to_string(d2.ArcCount()));
    }
    if( seeds.empty() )
    {
        return fail("no seed correspondence was supplied");
    }

    const Int n1 = d1.MaxCrossingCount(), n2 = d2.MaxCrossingCount();
    const Int m1 = d1.MaxArcCount(),      m2 = d2.MaxArcCount();

    constexpr Int None = Int(-1);
    std::vector<Int> cmap(static_cast<std::size_t>(n1), None);
    std::vector<Int> amap(static_cast<std::size_t>(m1), None);
    std::vector<char> ctaken(static_cast<std::size_t>(n2), 0);
    std::vector<char> ataken(static_cast<std::size_t>(m2), 0);

    std::vector<Int> queue;

    auto match_crossing = [&]( Int c1, Int c2 ) -> bool
    {
        if( (c1 < Int(0)) || (c1 >= n1) || (c2 < Int(0)) || (c2 >= n2) )
        {
            return fail("crossing index out of range while matching ("
                + std::to_string(c1) + " -> " + std::to_string(c2) + ")");
        }
        if( !d1.CrossingActiveQ(c1) || !d2.CrossingActiveQ(c2) )
        {
            return fail("crossing " + std::to_string(c1) + " -> "
                + std::to_string(c2) + ": one of the two is inactive");
        }
        if( cmap[static_cast<std::size_t>(c1)] != None )
        {
            if( cmap[static_cast<std::size_t>(c1)] != c2 )
            {
                return fail("crossing " + std::to_string(c1) + " would have to"
                    " match both " + std::to_string(cmap[static_cast<std::size_t>(c1)])
                    + " and " + std::to_string(c2));
            }
            return true;
        }
        if( ctaken[static_cast<std::size_t>(c2)] )
        {
            return fail("crossing " + std::to_string(c2) + " in the second"
                " diagram is claimed by two crossings of the first");
        }
        if( d1.CrossingStates()[c1] != d2.CrossingStates()[c2] )
        {
            return fail("crossings " + std::to_string(c1) + " and "
                + std::to_string(c2) + " have different handedness");
        }
        cmap[static_cast<std::size_t>(c1)] = c2;
        ctaken[static_cast<std::size_t>(c2)] = 1;
        queue.push_back(c1);
        return true;
    };

    for( const auto & s : seeds )
    {
        if( !match_crossing(s[0],s[1]) ) { return false; }
    }

    while( !queue.empty() )
    {
        const Int c1 = queue.back(); queue.pop_back();
        const Int c2 = cmap[static_cast<std::size_t>(c1)];

        for( Int io = 0; io < 2; ++io )
        {
            for( Int lr = 0; lr < 2; ++lr )
            {
                const Int a1 = d1.Crossings()(c1,io,lr);
                const Int a2 = d2.Crossings()(c2,io,lr);

                if( (a1 < Int(0)) || (a1 >= m1) || (a2 < Int(0)) || (a2 >= m2) )
                {
                    return fail("arc index out of range at crossing "
                        + std::to_string(c1));
                }

                if( amap[static_cast<std::size_t>(a1)] != None )
                {
                    if( amap[static_cast<std::size_t>(a1)] != a2 )
                    {
                        return fail("arc " + std::to_string(a1) + " would have"
                            " to match both "
                            + std::to_string(amap[static_cast<std::size_t>(a1)])
                            + " and " + std::to_string(a2)
                            + " (at crossing " + std::to_string(c1)
                            + ", port io=" + std::to_string(io)
                            + " lr=" + std::to_string(lr) + ")");
                    }
                    continue;
                }
                if( ataken[static_cast<std::size_t>(a2)] )
                {
                    return fail("arc " + std::to_string(a2) + " in the second"
                        " diagram is claimed by two arcs of the first (at"
                        " crossing " + std::to_string(c1) + ", port io="
                        + std::to_string(io) + " lr=" + std::to_string(lr) + ")");
                }

                amap[static_cast<std::size_t>(a1)] = a2;
                ataken[static_cast<std::size_t>(a2)] = 1;

                for( Int ht = 0; ht < 2; ++ht )
                {
                    if( !match_crossing(d1.Arcs()(a1,ht), d2.Arcs()(a2,ht)) )
                    {
                        return false;
                    }
                }
            }
        }
    }

    // Everything active must have been reached; an unreached crossing means the
    // diagrams have different component structure, or that the seeds did not
    // cover every connected component.
    for( Int c = 0; c < n1; ++c )
    {
        if( d1.CrossingActiveQ(c) && (cmap[static_cast<std::size_t>(c)] == None) )
        {
            return fail("crossing " + std::to_string(c) + " of the first diagram"
                " was never reached from the seeds (disconnected, structurally"
                " different, or a component with no seed)");
        }
    }

    why.clear();
    return true;
}

/*!@brief The common case: both diagrams still hold the seed crossing at the
 * same index, which is what an applier and an oracle agree on for every
 * crossing the move promised not to touch.
 */
template<typename PD_T>
bool DiagramsAgreeQ(
    const PD_T & d1, const PD_T & d2,
    typename PD_T::Int seed,
    std::string & why )
{
    using Int = typename PD_T::Int;
    return DiagramsAgreeQ(d1, d2, std::vector<std::array<Int,2>>{{seed,seed}}, why);
}
