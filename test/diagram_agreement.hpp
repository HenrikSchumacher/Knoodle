#pragma once

// Do two PlanarDiagrams describe the same oriented diagram?
//
// Not "the same knot" and not "identical arrays": the same diagram up to
// relabelling. Two implementations of one move will agree on the structure and
// disagree on every label, so a byte comparison is useless and a knot-type
// comparison is far too weak -- it would miss a wrong splice that happens to
// preserve the determinant.
//
// The correspondence is not searched for. Both diagrams contain the crossings
// the move promised not to touch, at their original indices, so seeding at one
// of those (an anchor) forces everything else: matched crossings match their
// ports slot by slot, matched ports match their arcs, and matched arcs match
// the crossings at their far ends. An orientation- and handedness-preserving
// isomorphism must map slot to slot, since In/Out and Left/Right are fixed by
// the strand orientations and the crossing sign. So this is a propagation, not
// a search, and it is O(crossings).

#include <string>
#include <vector>

template<typename PD_T>
bool DiagramsAgreeQ(
    const PD_T & d1, const PD_T & d2,
    typename PD_T::Int seed,
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
    if( !d1.CrossingActiveQ(seed) || !d2.CrossingActiveQ(seed) )
    {
        return fail("seed crossing " + std::to_string(seed)
            + " is not active in both diagrams");
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
            return fail("crossing index out of range while matching");
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

    if( !match_crossing(seed,seed) ) { return false; }

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
                            + " (at crossing " + std::to_string(c1) + ")");
                    }
                    continue;
                }
                if( ataken[static_cast<std::size_t>(a2)] )
                {
                    return fail("arc " + std::to_string(a2) + " in the second"
                        " diagram is claimed by two arcs of the first");
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
    // diagrams have different component structure, not merely different labels.
    for( Int c = 0; c < n1; ++c )
    {
        if( d1.CrossingActiveQ(c) && (cmap[static_cast<std::size_t>(c)] == None) )
        {
            return fail("crossing " + std::to_string(c) + " of the first diagram"
                " was never reached from the seed (disconnected or structurally"
                " different)");
        }
    }

    why.clear();
    return true;
}
