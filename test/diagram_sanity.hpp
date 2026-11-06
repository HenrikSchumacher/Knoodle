// diagram_sanity.hpp -- a THOROUGH sanity check for PlanarDiagram and
// PlanarDiagramComplex, shared by the tests in this directory.
//
// WHY THIS LIVES IN test/ AND NOT IN src/. The library's own `CheckAll` is
// deliberately lightweight: it runs frequently inside performance-critical
// code, so it performs only LOCAL checks (indices in bounds, arcs on active
// crossings are active, `A_cross` and `C_arcs` agree, degrees and colors are
// right) and its own comment warns that a `true` result does not certify a
// valid planar diagram. Henrik wants it to stay that way. The checks here are
// the expensive complement: they recompute cached global data from scratch on
// a copy, so they cost a full face/component computation per call and must
// never be pushed into the library's hot paths. Our test code, by contrast,
// is happy to pay that price on every iteration to catch otherwise-silent
// diagram corruption early.
//
// WHAT THE GLOBAL CHECKS CATCH THAT `CheckAll` CANNOT. A 4-valent map can
// satisfy every local check and still fail to be a planar diagram: exchanging
// the two `In` entries of a single crossing leaves `A_cross(a,tailtip) == c`
// true for every arc -- which is all `CheckCrossing` verifies -- but changes
// the rotation system at that vertex. On a trefoil, one such swap turns the
// sphere into a torus (V-E+F: 2 -> 0) and `CheckAll` still passes. That is
// not hypothetical: handed a corridor outside its documented contract,
// `PassSimplifier::Reroute` once produced a 73-crossing genus-two object that
// passed `CheckAll`, was carried downstream, and had knot invariants computed
// from it -- two readings of the same object returned two different
// determinants. (See handoff/reroute-arc-label-aliasing/.)
//
// THE EULER CONVENTION, measured on the current face computation (see
// test/diagram_sanity_check.cpp, which pins it): `FaceCount()` counts faces
// by their BOUNDARY components, so a split diagram counts the outer region
// once per diagram component, and the invariant is
//
//     V - E + F == 2 * DiagramComponentCount()
//
// (a connected trefoil gives 3 - 6 + 5 = 2; two disjoint trefoils in one
// diagram give 6 - 12 + 10 = 4). This matches the formula in the library's
// own (commented-out) `EulerCharacteristicValidQ`.
//
// FRESHNESS. All the global quantities are cached on the diagram and can go
// stale if a modification forgets to clear the cache. Every global check here
// therefore runs on a COPY whose cache has been cleared -- the copy's cache
// is independent of the original's, so the original is not disturbed -- and,
// when the original HAS a cached value, the fresh recomputation is compared
// against it. A mismatch means some earlier operation mutated the diagram
// without invalidating its cache.
//
// Usage:
//
//     #include "diagram_sanity.hpp"
//     if( !KnoodleTest::DiagramSanityQ(pd) )   { /* corrupted */ }
//     if( !KnoodleTest::ComplexSanityQ(pdc) )  { /* corrupted */ }
//
// Both return true iff every check passes. On failure they describe each
// violation: appended to `*why` when a string is supplied, printed to stderr
// otherwise. They never modify the object they are handed.

#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace KnoodleTest
{

namespace Detail
{
    inline void SanityComplain(
        std::string * why, const std::string & msg
    )
    {
        if( why != nullptr )
        {
            *why += msg;
            *why += '\n';
        }
        else
        {
            std::fprintf(stderr, "%s\n", msg.c_str());
        }
    }
}

/*!@brief Thorough validity check for a single `PlanarDiagram`: the library's
 * local `CheckAll`, then Euler's formula and face-structure coherence
 * recomputed from scratch on a cache-cleared copy, then a stale-cache
 * comparison against whatever the original had cached. Expensive by design;
 * test code only.
 */
template<typename PD_T>
bool DiagramSanityQ( const PD_T & pd, std::string * why = nullptr )
{
    using Int = typename PD_T::Int;

    bool okQ = true;

    auto complain = [why,&okQ]( const std::string & msg )
    {
        Detail::SanityComplain(why, "DiagramSanityQ: " + msg);
        okQ = false;
    };

    if( !pd.ValidQ() )
    {
        complain("diagram is not ValidQ()");
        return false;
    }

    // The library's own local checks (they print details via eprint).
    if( !pd.CheckAll() )
    {
        complain("CheckAll() failed (local consistency)");
    }

    // A 0-crossing diagram (an unknot placeholder) has no arcs and no faces;
    // there is nothing global left to check.
    if( pd.CrossingCount() == Int(0) )
    {
        return okQ;
    }

    // Recompute every global quantity from scratch. The copy's cache is
    // independent of the original's, so this does not disturb `pd`.
    PD_T fresh ( pd );
    fresh.ClearCache();

    const Int V  = fresh.CrossingCount();
    const Int E  = fresh.ArcCount();
    const Int F  = fresh.FaceCount();
    const Int dc = fresh.DiagramComponentCount();
    const Int lc = fresh.LinkComponentCount();

    // Euler's formula, with the outer region counted once per diagram
    // component (see the header comment).
    if( V - E + F != Int(2) * dc )
    {
        complain(
            "Euler characteristic V-E+F = " + std::to_string((long long)(V - E + F))
            + " != 2 * DiagramComponentCount() = " + std::to_string((long long)(Int(2) * dc))
            + " (V=" + std::to_string((long long)V)
            + " E=" + std::to_string((long long)E)
            + " F=" + std::to_string((long long)F)
            + "): the stored embedding is not planar"
        );
    }

    // Face-structure coherence: every active darc lies in exactly one face
    // boundary, no inactive darc lies in any, and `ArcFaces` -- the face to
    // the LEFT of each darc -- agrees with which face's boundary lists it.
    {
        const auto & F_dA   = fresh.FaceDarcs();
        const Int  * ptr    = F_dA.Pointers().data();
        const Int  * idx    = F_dA.Elements().data();
        const auto & A_F    = fresh.ArcFaces();
        const Int    da_max = Int(2) * fresh.MaxArcCount();

        std::vector<Int> seen ( (std::size_t)da_max, Int(0) );

        for( Int f = 0; f < F; ++f )
        {
            for( Int i = ptr[f]; i < ptr[f+1]; ++i )
            {
                const Int da = idx[i];

                if( (da < Int(0)) || (da >= da_max) )
                {
                    complain(
                        "FaceDarcs lists out-of-bounds darc "
                        + std::to_string((long long)da)
                        + " in face " + std::to_string((long long)f)
                    );
                    continue;
                }

                ++seen[(std::size_t)da];

                const Int  a = da / Int(2);
                const bool d = ( (da % Int(2)) != Int(0) );

                if( A_F(a,d) != f )
                {
                    complain(
                        "ArcFaces(" + std::to_string((long long)a)
                        + "," + std::to_string((int)d) + ") = "
                        + std::to_string((long long)A_F(a,d))
                        + " but FaceDarcs lists that darc in face "
                        + std::to_string((long long)f)
                    );
                }
            }
        }

        Int total = 0;

        for( Int a = 0; a < fresh.MaxArcCount(); ++a )
        {
            const bool activeQ = fresh.ArcActiveQ(a);

            for( Int d = 0; d < Int(2); ++d )
            {
                const Int n = seen[(std::size_t)(Int(2)*a + d)];
                total += n;

                if( activeQ ? (n != Int(1)) : (n != Int(0)) )
                {
                    complain(
                        (activeQ
                            ? std::string("active darc ")
                            : std::string("inactive darc "))
                        + std::to_string((long long)(Int(2)*a + d))
                        + " appears " + std::to_string((long long)n)
                        + " times in FaceDarcs (want "
                        + (activeQ ? "1" : "0") + ")"
                    );
                }
            }
        }

        if( total != Int(2) * E )
        {
            complain(
                "FaceDarcs lists " + std::to_string((long long)total)
                + " darcs != 2 * ArcCount() = "
                + std::to_string((long long)(Int(2) * E))
            );
        }
    }

    // Stale-cache detection: whatever the original has ALREADY cached must
    // match the fresh recomputation. (An uncached quantity is not queried, so
    // this block leaves an unpopulated cache unpopulated in spirit -- though
    // querying pd would populate it, we only touch tags that are present.)
    if( pd.InCacheQ("FaceDarcs") && (pd.FaceCount() != F) )
    {
        complain(
            "cached FaceCount() = " + std::to_string((long long)pd.FaceCount())
            + " != fresh recomputation " + std::to_string((long long)F)
            + ": some operation mutated the diagram without clearing its cache"
        );
    }

    if( pd.InCacheQ("LinkComponentCount") && (pd.LinkComponentCount() != lc) )
    {
        complain(
            "cached LinkComponentCount() = "
            + std::to_string((long long)pd.LinkComponentCount())
            + " != fresh recomputation " + std::to_string((long long)lc)
            + ": some operation mutated the diagram without clearing its cache"
        );
    }

    return okQ;
}

/*!@brief Thorough validity check for a `PlanarDiagramComplex`: the complex's
 * own `CheckAll` (validity of every held diagram plus their local checks),
 * then `DiagramSanityQ` on each diagram in `Diagrams()`. Non-const because
 * `PlanarDiagramComplex::CheckAll` is; the complex is not modified.
 */
template<typename PDC_T>
bool ComplexSanityQ( PDC_T & pdc, std::string * why = nullptr )
{
    bool okQ = true;

    if( !pdc.CheckAll() )
    {
        Detail::SanityComplain(why,
            "ComplexSanityQ: PlanarDiagramComplex::CheckAll() failed");
        okQ = false;
    }

    using Int = typename PDC_T::Int;

    const Int n = pdc.DiagramCount();

    for( Int i = 0; i < n; ++i )
    {
        std::string sub;

        if( !DiagramSanityQ( pdc.Diagram(i), &sub ) )
        {
            Detail::SanityComplain(why,
                "ComplexSanityQ: diagram " + std::to_string((long long)i)
                + " of " + std::to_string((long long)n) + " failed:");
            Detail::SanityComplain(why, sub);
            okQ = false;
        }
    }

    return okQ;
}

} // namespace KnoodleTest
