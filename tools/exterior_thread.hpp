/**
 * @file exterior_thread.hpp
 * @brief Tracking one face through a move trace, so that consecutive drawings
 * share an exterior face.
 *
 * Each record of a trace is drawn with a fresh layout. What keeps a sequence
 * of them from jumping is that they all put the SAME region of the plane at
 * infinity. `#view exterior=<da>` names that region in each record's own
 * labels; this header says what "the same region" means across a move, checks
 * a stream's claim that it kept it, and chooses a thread when asked to.
 * docs/move-descriptor.md, "Exterior faces across a trace", is the spec.
 *
 * The one fact everything rests on: a move never destroys a region, it only
 * merges and splits them. So every face of a record's diagram has an image in
 * the next record's -- a single face, or for a face the corridor cuts, one
 * face per side of the move -- and a thread never has to break.
 *
 * Across one move the correspondence is built in two stages:
 *
 *   before -> after   `AfterDiagram` (or `R1AfterDiagram`) keeps every
 *                     surviving label, so this is combinatorics on labels.
 *                     For a pass move the faces are rebuilt from the V0 flood's
 *                     FRAGMENTS (tools/witness_check.hpp): the corridor cuts
 *                     each face it visits in two, and deleting W joins the
 *                     fragments across W's arcs. Each resulting region is one
 *                     face of the after-diagram.
 *   after -> next     the next record's snapshot is the after-diagram only up
 *                     to relabelling (middlestrands compacts between records),
 *                     so the map is the isomorphism `DiagramsIsomorphicQ`
 *                     finds. It keeps In/Out and handedness, so it sends faces
 *                     to faces.
 *
 * Nothing here draws.
 *
 * This header assumes `Knoodle.hpp` has already been included by the includer.
 */

#pragma once

#include "../src/PassDescriptor.hpp"
#include "../src/MoveTrace.hpp"
#include "r1_move.hpp"
#include "diagram_agreement.hpp"
#include "witness_check.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace KnoodleExterior
{

//==============================================================================
// The `#view` line
//==============================================================================

/*!@brief `#view exterior=<da> [outside=<0|1>] [behind] [seam]`. */
struct View
{
    long long exterior = -1;   // darc; L(exterior) is laid out as unbounded
    int       outside  = -1;   // side of the move's loop that holds infinity
    bool      behindQ  = false;
    bool      seamQ    = false;
};

/*!@brief Parse a `#view` line strictly: unknown tokens are errors, so a
 * misspelt `behind` cannot silently turn into "no special animation".
 */
inline bool ParseView( const std::string & line, View & v, std::string & why )
{
    v = View();

    bool have_exteriorQ = false;
    std::size_t pos = 0;
    bool firstQ = true;

    while( pos < line.size() )
    {
        while( (pos < line.size()) && (line[pos] == ' ' || line[pos] == '\t') ) { ++pos; }
        if( pos >= line.size() ) { break; }
        std::size_t end = pos;
        while( (end < line.size()) && (line[end] != ' ') && (line[end] != '\t') ) { ++end; }
        const std::string tok = line.substr(pos, end - pos);
        pos = end;

        if( firstQ )
        {
            firstQ = false;
            if( tok == "#view" ) { continue; }
        }

        auto int_of = [&]( const std::string & s, long long & out ) -> bool
        {
            if( s.empty() ) { return false; }
            std::size_t used = 0;
            try { out = std::stoll(s, &used); } catch( ... ) { return false; }
            return used == s.size();
        };

        if( tok.rfind("exterior=", 0) == 0 )
        {
            if( !int_of(tok.substr(9), v.exterior) || (v.exterior < 0) )
            {
                why = "bad exterior darc '" + tok + "'";
                return false;
            }
            have_exteriorQ = true;
        }
        else if( tok.rfind("outside=", 0) == 0 )
        {
            const std::string s = tok.substr(8);
            if( s == "0" ) { v.outside = 0; }
            else if( s == "1" ) { v.outside = 1; }
            else
            {
                why = "outside= must be 0 or 1, not '" + s + "'";
                return false;
            }
        }
        else if( tok == "behind" ) { v.behindQ = true; }
        else if( tok == "seam"   ) { v.seamQ   = true; }
        else
        {
            why = "unknown '#view' token '" + tok + "'";
            return false;
        }
    }

    if( !have_exteriorQ )
    {
        why = "'#view' without exterior=";
        return false;
    }
    return true;
}

inline std::string FormatView( const View & v )
{
    std::string s = "#view exterior=" + std::to_string(v.exterior);
    if( v.outside >= 0 ) { s += " outside=" + std::to_string(v.outside); }
    if( v.behindQ ) { s += " behind"; }
    if( v.seamQ   ) { s += " seam"; }
    return s;
}

/*!@brief The `#view` line among a record's headers, if any. */
inline std::optional<std::string> ViewLine( const std::vector<std::string> & headers )
{
    for( const auto & h : headers )
    {
        if( h.rfind("#view ", 0) == 0 ) { return h; }
    }
    return std::nullopt;
}

//==============================================================================
// Faces
//==============================================================================

template<class PD_T>
typename PD_T::Int LeftFace( const PD_T & pd, typename PD_T::Int da )
{
    return Knoodle::PassDescriptor<typename PD_T::Int>::LeftFace(pd, da);
}

/*!@brief The canonical name of a face: its minimal boundary darc. */
template<class PD_T>
typename PD_T::Int MinimalDarc( const PD_T & pd, typename PD_T::Int f )
{
    using Int = typename PD_T::Int;
    auto darcs = pd.FaceDarcs()[f];
    Int best = Int(-1);
    for( auto it = darcs.begin(); it != darcs.end(); ++it )
    {
        if( (best < Int(0)) || (*it < best) ) { best = *it; }
    }
    return best;
}

template<class PD_T>
typename PD_T::Int FaceSize( const PD_T & pd, typename PD_T::Int f )
{
    auto darcs = pd.FaceDarcs()[f];
    return static_cast<typename PD_T::Int>(darcs.end() - darcs.begin());
}

/*!@brief Faces of `d1` to faces of `d2` under an isomorphism of the two.
 *
 * Tries the colour-keeping isomorphism first -- that is the one the trace
 * check uses, and a symmetric link's components should not trade places --
 * and falls back to any isomorphism. Every darc of a face must land on one
 * face, and the map must be a bijection; either failing is reported, not
 * papered over.
 */
template<class PD_T>
bool FaceMapByIsomorphism(
    const PD_T & d1, const PD_T & d2,
    std::vector<typename PD_T::Int> & fmap, std::string & why )
{
    using Int = typename PD_T::Int;

    DiagramMatch_T<Int> M (d1.MaxCrossingCount(), d1.MaxArcCount(),
                           d2.MaxCrossingCount(), d2.MaxArcCount());
    std::string iwhy;
    if( !DiagramsIsomorphicQ(d1, d2, iwhy, &M, true) )
    {
        DiagramMatch_T<Int> M2 (d1.MaxCrossingCount(), d1.MaxArcCount(),
                                d2.MaxCrossingCount(), d2.MaxArcCount());
        if( !DiagramsIsomorphicQ(d1, d2, iwhy, &M2, false) )
        {
            why = "the two diagrams are not isomorphic: " + iwhy;
            return false;
        }
        M = std::move(M2);
    }

    const Int n1 = d1.FaceCount();
    const Int n2 = d2.FaceCount();
    if( n1 != n2 )
    {
        why = "face counts differ: " + std::to_string(n1) + " vs " + std::to_string(n2);
        return false;
    }

    fmap.assign(static_cast<std::size_t>(n1), Int(-1));
    std::vector<char> hit (static_cast<std::size_t>(n2), char(0));

    for( Int f = 0; f < n1; ++f )
    {
        auto darcs = d1.FaceDarcs()[f];
        Int g = Int(-1);
        for( auto it = darcs.begin(); it != darcs.end(); ++it )
        {
            const Int a  = *it / Int(2);
            const Int a2 = M.amap[static_cast<std::size_t>(a)];
            if( a2 < Int(0) )
            {
                why = "the isomorphism does not cover arc " + std::to_string(a);
                return false;
            }
            const Int g_here = LeftFace(d2, Int(2) * a2 + (*it % Int(2)));
            if( g < Int(0) ) { g = g_here; }
            else if( g != g_here )
            {
                why = "the isomorphism sends the darcs of face " + std::to_string(f)
                    + " to two different faces (" + std::to_string(g) + ", "
                    + std::to_string(g_here) + ")";
                return false;
            }
        }
        if( (g < Int(0)) || hit[static_cast<std::size_t>(g)] )
        {
            why = "the isomorphism's face map is not a bijection at face "
                + std::to_string(f);
            return false;
        }
        hit[static_cast<std::size_t>(g)] = char(1);
        fmap[static_cast<std::size_t>(f)] = g;
    }
    return true;
}

//==============================================================================
// One move's face correspondence
//==============================================================================

/*!@brief Where each face of a record's diagram goes across its move.
 *
 * For a pass or middlepass the loop W + corridor has two sides, and a face the
 * corridor visits has a part on each: `image[f][s]` is the face its side-`s`
 * part lands in (-1 when f has no part on side s). For an r1 there are no
 * sides, and both entries are the same face.
 *
 * `after` images are in the after-diagram's labels (what a frozen-layout
 * drawing of the move shows); `next` images are in the NEXT record's labels,
 * and are filled only when a next snapshot was supplied.
 */
template<class PD_T>
struct MoveFaceMap
{
    using Int = typename PD_T::Int;

    bool        okQ = false;
    std::string why;

    bool sidedQ      = false;       // pass/middlepass: the loop has two sides
    bool middlepassQ = false;
    int  feas_side   = -1;          // `#feas side=`, -1 if absent
    std::array<Int,2> side_pieces {Int(0),Int(0)};   // pieces per side

    std::vector<std::array<Int,2>> after_image;
    std::vector<std::array<Int,2>> next_image;
    PD_T after;
};

namespace detail
{

template<typename Int>
struct UnionFind
{
    std::vector<Int> parent;
    explicit UnionFind( Int n ) : parent(static_cast<std::size_t>(n))
    {
        for( Int i = 0; i < n; ++i ) { parent[static_cast<std::size_t>(i)] = i; }
    }
    Int Find( Int x )
    {
        while( parent[static_cast<std::size_t>(x)] != x )
        {
            parent[static_cast<std::size_t>(x)]
                = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
            x = parent[static_cast<std::size_t>(x)];
        }
        return x;
    }
    void Join( Int a, Int b )
    {
        a = Find(a); b = Find(b);
        if( a != b ) { parent[static_cast<std::size_t>(a)] = b; }
    }
};

/*!@brief Pass / middlepass: before faces -> after faces, per side. */
template<class PD_T>
bool PassAfterImage(
    const PD_T & pd, const Knoodle::PassDescriptor<typename PD_T::Int> & mv,
    MoveFaceMap<PD_T> & out )
{
    using Int    = typename PD_T::Int;
    using Desc_T = Knoodle::PassDescriptor<Int>;
    auto Z = []( Int i ) { return static_cast<std::size_t>(i); };

    // A corridor that crosses W (Proposition C', uniform pass only) makes
    // W + corridor a non-simple loop, which has no two sides. Say so before
    // the flood does, less helpfully.
    for( Int da : mv.cross )
    {
        for( Int ws : mv.strand )
        {
            if( Desc_T::ArcOf(da) == Desc_T::ArcOf(ws) )
            {
                out.why = "the corridor crosses W itself (Proposition C'), so"
                          " the move has no two sides; face tracking does not"
                          " cover that yet";
                return false;
            }
        }
    }

    KnoodleWitness::Sides_T<PD_T> sides;
    if( !KnoodleWitness::ReconstructSides(pd, mv, sides, out.why) )
    {
        out.why = "the move's two sides cannot be rebuilt: " + out.why;
        return false;
    }

    std::vector<Int> freed;
    out.after = mv.AfterDiagram(pd, out.why, freed);
    if( !out.why.empty() )
    {
        out.why = "no after-diagram: " + out.why;
        return false;
    }
    if( !freed.empty() )
    {
        out.why = "the move frees a crossingless loop; face tracking does not"
                  " cover that yet";
        return false;
    }

    for( auto s : sides.side_of )
    {
        if( s == 0 ) { ++out.side_pieces[0]; }
        if( s == 1 ) { ++out.side_pieces[1]; }
    }

    const Int n_a    = pd.MaxArcCount();
    const Int n_f    = pd.FaceCount();
    const Int k      = static_cast<Int>(mv.cross.size());
    const Int n_frag = n_f + k + Int(1);

    // Deleting W joins the fragments on either side of every arc half the
    // loop runs along. The stubs of W's end arcs stay walls: the new strand
    // starts along them.
    UnionFind<Int> uf (n_frag);
    for( Int a = 0; a < n_a; ++a )
    {
        if( !pd.ArcActiveQ(a) || !sides.in_W[Z(a)] ) { continue; }
        for( int half = 0; half < 2; ++half )
        {
            if( !sides.loop_half[Z(Int(2) * a + Int(half))] ) { continue; }
            // The two darcs of arc a; darc 2a+1 starts at a's tail.
            const Int f1 = sides.frag[Z(Int(2) * (Int(2) * a + Int(1)) + Int(half))];
            const Int f0 = sides.frag[Z(Int(2) * (Int(2) * a) + Int(1 - half))];
            uf.Join(f0, f1);
        }
    }

    // Arcs healed away: the transversal's outgoing arc at each interior
    // crossing of W. Their labels die (the chain start absorbs them), so they
    // cannot name an after face.
    std::vector<char> healed (Z(n_a), char(0));
    for( std::size_t i = 0; i + 1 < mv.strand.size(); ++i )
    {
        const Int x = Desc_T::DarcHeadCrossing(pd, mv.strand[i]);
        for( bool lr : { PD_T::Left, PD_T::Right } )
        {
            const Int a = pd.Crossings()(x, PD_T::Out, lr);
            if( !sides.in_W[Z(a)] ) { healed[Z(a)] = char(1); }
        }
    }

    // Each after face claims the region its surviving labels sit in. A
    // surviving arc keeps its TAIL half under the move (a corridor crossing
    // splits off the head end; healing extends the head end), so the tail
    // half's fragment is where that darc's side of the after face lies.
    const auto & A = out.after;
    const Int n_g = A.FaceCount();
    std::vector<Int> region_face (Z(n_frag), Int(-1));
    std::vector<Int> face_region (Z(n_g),    Int(-1));

    for( Int g = 0; g < n_g; ++g )
    {
        auto darcs = A.FaceDarcs()[g];
        for( auto it = darcs.begin(); it != darcs.end(); ++it )
        {
            const Int da = *it;
            const Int a  = da / Int(2);
            if( (a >= n_a) || !pd.ArcActiveQ(a) || sides.in_W[Z(a)]
                || healed[Z(a)] || !A.ArcActiveQ(a) )
            {
                continue;
            }
            const Int h = ((da % Int(2)) != Int(0)) ? Int(0) : Int(1);
            const Int r = uf.Find(sides.frag[Z(Int(2) * da + h)]);

            if( face_region[Z(g)] < Int(0) ) { face_region[Z(g)] = r; }
            else if( face_region[Z(g)] != r )
            {
                out.why = "after face " + std::to_string(g) + " touches two"
                          " regions of the rebuilt plane";
                return false;
            }
        }
        const Int r = face_region[Z(g)];
        if( r < Int(0) ) { continue; }
        if( (region_face[Z(r)] >= Int(0)) && (region_face[Z(r)] != g) )
        {
            out.why = "one region of the rebuilt plane holds two after faces ("
                + std::to_string(region_face[Z(r)]) + ", " + std::to_string(g) + ")";
            return false;
        }
        region_face[Z(r)] = g;
    }

    // An after face bounded only by new labels (corridor arcs, split-off head
    // pieces) claims nothing. If exactly one face and one region are left,
    // they are each other's; anything more is reported, not guessed.
    {
        std::vector<Int> free_faces, free_regions;
        for( Int g = 0; g < n_g; ++g ) { if( face_region[Z(g)] < Int(0) ) { free_faces.push_back(g); } }
        for( Int i = 0; i < n_frag; ++i )
        {
            const Int r = uf.Find(i);
            if( (r == i) && (region_face[Z(r)] < Int(0)) ) { free_regions.push_back(r); }
        }
        if( free_faces.size() != free_regions.size() )
        {
            out.why = "the rebuilt plane has " + std::to_string(free_regions.size())
                + " unclaimed regions but the after-diagram has "
                + std::to_string(free_faces.size()) + " unclaimed faces";
            return false;
        }
        if( free_faces.size() > 1 )
        {
            out.why = std::to_string(free_faces.size()) + " after faces carry no"
                      " surviving label; cannot tell them apart";
            return false;
        }
        if( free_faces.size() == 1 )
        {
            region_face[Z(free_regions[0])] = free_faces[0];
        }
    }

    // Before faces, by fragment. Fragment f < n_f is (part of) face f; the
    // i-th corridor face's other part is fragment n_f + i.
    out.after_image.assign(Z(n_f), {Int(-1), Int(-1)});
    auto put = [&]( Int f, Int frag ) -> bool
    {
        const int s = sides.frag_side[Z(frag)];
        const Int g = region_face[Z(uf.Find(frag))];
        if( (s < 0) || (g < Int(0)) ) { return false; }
        out.after_image[Z(f)][Z(s)] = g;
        return true;
    };
    for( Int f = 0; f < n_f; ++f )
    {
        if( !put(f, f) )
        {
            out.why = "face " + std::to_string(f) + " has no image";
            return false;
        }
    }
    for( Int i = 0; i <= k; ++i )
    {
        const Int f = sides.corridor_faces[Z(i)];
        if( !put(f, n_f + i) )
        {
            out.why = "the far part of corridor face " + std::to_string(f)
                    + " has no image";
            return false;
        }
        if( (out.after_image[Z(f)][0] < Int(0)) || (out.after_image[Z(f)][1] < Int(0)) )
        {
            out.why = "corridor face " + std::to_string(f)
                    + " does not have a part on each side";
            return false;
        }
    }
    // Self-check of the claim the spec rests on: a face the move leaves alone
    // -- not bordered by W, not visited by the corridor -- comes out of the
    // after-diagram with the SAME boundary darc cycle. Every corner at an
    // interior crossing of W touches W, so no such face meets a healed arc or
    // a dying crossing; and a split arc has corridor faces on both sides.
    {
        std::vector<char> touched (Z(n_f), char(0));
        for( Int da : mv.strand )
        {
            touched[Z(Desc_T::LeftFace(pd, da))]  = char(1);
            touched[Z(Desc_T::RightFace(pd, da))] = char(1);
        }
        for( Int f : sides.corridor_faces ) { touched[Z(f)] = char(1); }

        for( Int f = 0; f < n_f; ++f )
        {
            if( touched[Z(f)] ) { continue; }
            const Int g = out.after_image[Z(f)][0] >= Int(0)
                        ? out.after_image[Z(f)][0] : out.after_image[Z(f)][1];
            std::vector<Int> before_c, after_c;
            for( auto d : pd.FaceDarcs()[f] ) { before_c.push_back(d); }
            for( auto d : A.FaceDarcs()[g]  ) { after_c.push_back(d); }
            std::sort(before_c.begin(), before_c.end());
            std::sort(after_c.begin(),  after_c.end());
            if( before_c != after_c )
            {
                out.why = "internal: untouched face " + std::to_string(f)
                    + " changed its boundary across the move (after face "
                    + std::to_string(g) + ")";
                return false;
            }
        }
    }

    return true;
}

/*!@brief r1: before faces -> after faces. The monogon goes where its
 * surrounding face goes.
 */
template<class PD_T>
bool R1AfterImage( const PD_T & pd, typename PD_T::Int loop, MoveFaceMap<PD_T> & out )
{
    using Int = typename PD_T::Int;
    auto Z = []( Int i ) { return static_cast<std::size_t>(i); };

    auto r = KnoodleR1View::ResolveR1(pd, loop);
    if( !r.validQ ) { out.why = r.why; return false; }

    std::vector<Int> freed;
    out.after = KnoodleR1View::R1AfterDiagram(pd, r, out.why, freed);
    if( !out.why.empty() ) { out.why = "no after-diagram: " + out.why; return false; }
    if( !freed.empty() )
    {
        out.why = "the curl's whole component comes free";
        return false;
    }

    const Int n_f = pd.FaceCount();
    std::vector<Int> img (Z(n_f), Int(-1));
    const auto & A = out.after;

    // Every after arc is a before arc (a_next absorbs a_prev, same way round),
    // and its darcs border the same faces they did.
    for( Int g = 0; g < A.FaceCount(); ++g )
    {
        auto darcs = A.FaceDarcs()[g];
        for( auto it = darcs.begin(); it != darcs.end(); ++it )
        {
            const Int f = LeftFace(pd, *it);
            if( f < Int(0) ) { out.why = "after darc with no before face"; return false; }
            if( (img[Z(f)] >= Int(0)) && (img[Z(f)] != g) )
            {
                out.why = "before face " + std::to_string(f) + " lands in two after faces";
                return false;
            }
            img[Z(f)] = g;
        }
    }
    const Int around = LeftFace(pd, loop ^ Int(1));
    img[Z(r.monogon)] = img[Z(around)];

    out.after_image.assign(Z(n_f), {Int(-1), Int(-1)});
    for( Int f = 0; f < n_f; ++f )
    {
        if( img[Z(f)] < Int(0) )
        {
            out.why = "before face " + std::to_string(f) + " has no image";
            return false;
        }
        out.after_image[Z(f)] = { img[Z(f)], img[Z(f)] };
    }
    return true;
}

inline std::string MoveKind( const std::string & payload )
{
    const auto k = payload.find("kind=");
    if( k == std::string::npos ) { return "pass"; }
    const auto b = k + 5;
    const auto e = payload.find_first_of(" \t", b);
    return payload.substr(b, (e == std::string::npos) ? std::string::npos : e - b);
}

} // namespace detail

/*!@brief Build the face correspondence of one record's move.
 *
 * `next` is the next record's snapshot, or null (the after images are still
 * built; the next images are not).
 */
template<class PD_T>
MoveFaceMap<PD_T> BuildMoveFaceMap(
    const typename Knoodle::MoveTrace<PD_T>::Record & rec,
    const PD_T & pd,
    const PD_T * next )
{
    using Int = typename PD_T::Int;
    MoveFaceMap<PD_T> out;

    if( !rec.move ) { out.why = "the record carries no move"; return out; }

    const std::string kind = detail::MoveKind(*rec.move);

    if( (kind == "pass") || (kind == "middlepass") )
    {
        Knoodle::PassDescriptor<Int> mv;
        if( !Knoodle::PassDescriptor<Int>::Parse(*rec.move, mv, out.why) ) { return out; }
        out.sidedQ      = true;
        out.middlepassQ = mv.middlepassQ;
        if( rec.feas ) { out.feas_side = rec.feas->side; }
        if( !detail::PassAfterImage(pd, mv, out) ) { return out; }
    }
    else if( kind == "r1" )
    {
        KnoodleR1View::R1Descriptor<Int> d;
        if( !KnoodleR1View::R1Descriptor<Int>::Parse(*rec.move, d, out.why) ) { return out; }
        if( !detail::R1AfterImage(pd, d.loop, out) ) { return out; }
    }
    else
    {
        out.why = "no face correspondence for kind=" + kind;
        return out;
    }

    if( next )
    {
        std::vector<Int> fmap;
        if( !FaceMapByIsomorphism(out.after, *next, fmap, out.why) )
        {
            out.why = "after-diagram vs next snapshot: " + out.why;
            return out;
        }
        out.next_image = out.after_image;
        for( auto & im : out.next_image )
        {
            for( auto & g : im )
            {
                if( g >= Int(0) ) { g = fmap[static_cast<std::size_t>(g)]; }
            }
        }
    }

    out.okQ = true;
    return out;
}

//==============================================================================
// The rule: which part of a cut exterior keeps infinity
//==============================================================================

/*!@brief Decide `outside` and `behind` for exterior face `f` across a sided
 * move.
 *
 *   f on one side only   infinity is there. For a middlepass that is `behind`
 *                        exactly when it is the witness's side s -- the swept
 *                        disk holds infinity, and the strand has to go round
 *                        the back of the sphere. A uniform pass is never
 *                        behind: it is an isotopy across either side, so its
 *                        disk is by definition the side infinity is not on.
 *   f cut by the corridor  a middlepass keeps infinity on 1-s (never behind
 *                        when there is a choice); a pass keeps it on the side
 *                        with MORE pieces, so the smaller disk is the one
 *                        drawn bounded (ties: side 0). A middlepass with no
 *                        witness falls back to the pass rule.
 */
template<class PD_T>
void ChooseOutside( const MoveFaceMap<PD_T> & m, typename PD_T::Int f,
                    int & outside, bool & behindQ )
{
    using Int = typename PD_T::Int;
    const auto & im = m.after_image[static_cast<std::size_t>(f)];
    const bool on0 = im[0] >= Int(0);
    const bool on1 = im[1] >= Int(0);
    const bool witnessQ = m.middlepassQ && (m.feas_side >= 0);

    behindQ = false;
    if( on0 != on1 )
    {
        outside = on0 ? 0 : 1;
        behindQ = witnessQ && (outside == m.feas_side);
        return;
    }
    if( witnessQ ) { outside = 1 - m.feas_side; return; }
    outside = (m.side_pieces[1] > m.side_pieces[0]) ? 1 : 0;
}

//==============================================================================
// Threading a whole trace
//==============================================================================

/*!@brief What the threader decided for one record. */
struct RecordView
{
    std::optional<View> view;   // nullopt: no `#view` (no diagram to name a face in)
    std::string         note;   // why a seam, or why nothing, for stderr
};

/*!@brief Choose one exterior thread through a whole trace.
 *
 * Only the first record of a run has a free choice: after that the rule in
 * `ChooseOutside` decides every step. So the threader tries every face of a
 * run's first diagram as the start, and keeps the thread with the fewest
 * `behind` moves, then the largest total exterior (a big outer face reads
 * better), then the smallest start face index.
 *
 * A run ends where no face correspondence exists: after a `redraw` (the
 * redraw animation carries that transition, so no seam is declared), or where
 * a move's correspondence could not be built (declared `seam`, with the
 * reason in `note`). Candidate records do not advance the diagram; each takes
 * the thread's face from an applied record of the same diagram, found by
 * isomorphism.
 */
template<class PD_T>
std::vector<RecordView> ThreadExterior(
    const std::vector<typename Knoodle::MoveTrace<PD_T>::Record> & recs )
{
    using Int = typename PD_T::Int;

    const std::size_t n = recs.size();
    std::vector<RecordView> out (n);

    auto has_diagramQ = [&]( std::size_t i )
    {
        return recs[i].state && (recs[i].state->CrossingCount() > Int(0));
    };

    // Applied records in order; a crossingless record ends a run.
    std::vector<std::size_t> A;
    for( std::size_t i = 0; i < n; ++i )
    {
        if( !recs[i].candidateQ ) { A.push_back(i); }
    }

    // The map across each applied transition, and whether a run continues.
    std::vector<MoveFaceMap<PD_T>> maps (A.size());
    std::vector<char> links (A.size(), char(0));   // A[t] -> A[t+1] continues
    std::vector<char> seam  (A.size(), char(0));   // A[t] starts with a seam

    for( std::size_t t = 0; t < A.size(); ++t )
    {
        const auto & r = recs[A[t]];
        if( !has_diagramQ(A[t]) ) { continue; }

        const bool nextQ = (t + 1 < A.size()) && has_diagramQ(A[t+1]);
        const PD_T * next = nextQ ? &*recs[A[t+1]].state : nullptr;

        if( !r.move ) { continue; }
        const std::string kind = detail::MoveKind(*r.move);
        if( kind == "redraw" ) { continue; }

        maps[t] = BuildMoveFaceMap<PD_T>(r, *r.state, next);
        if( maps[t].okQ && nextQ )
        {
            links[t] = char(1);
        }
        else if( nextQ )
        {
            seam[t+1] = char(1);
            out[A[t+1]].note = "seam: " + maps[t].why;
            // Keep the sides for this record's own outside=, if they exist.
            if( maps[t].after_image.empty() ) { maps[t] = BuildMoveFaceMap<PD_T>(r, *r.state, nullptr); }
        }
        else if( !maps[t].okQ )
        {
            maps[t] = BuildMoveFaceMap<PD_T>(r, *r.state, nullptr);
        }
    }

    // Follow a thread from face f at A[s]; fills faces[t] for the run.
    struct Walk { Int behind = 0; Int size = 0; std::vector<Int> faces; };

    auto walk = [&]( std::size_t s, Int f ) -> Walk
    {
        Walk w;
        std::size_t t = s;
        for(;;)
        {
            w.faces.push_back(f);
            w.size += FaceSize(*recs[A[t]].state, f);
            const auto & m = maps[t];
            if( m.sidedQ && !m.after_image.empty() )
            {
                int o; bool b;
                ChooseOutside(m, f, o, b);
                if( b ) { ++w.behind; }
                if( !links[t] ) { break; }
                f = m.next_image[static_cast<std::size_t>(f)][static_cast<std::size_t>(o)];
            }
            else
            {
                if( !links[t] ) { break; }
                f = m.next_image[static_cast<std::size_t>(f)][0];
            }
            ++t;
        }
        return w;
    };

    std::vector<Int> face_at (A.size(), Int(-1));

    for( std::size_t s = 0; s < A.size(); )
    {
        if( !has_diagramQ(A[s]) ) { ++s; continue; }

        const Int n_f = recs[A[s]].state->FaceCount();
        Walk best; bool haveQ = false;
        for( Int f = 0; f < n_f; ++f )
        {
            Walk w = walk(s, f);
            const bool betterQ = !haveQ
                || (w.behind < best.behind)
                || ((w.behind == best.behind) && (w.size > best.size));
            if( betterQ ) { best = std::move(w); haveQ = true; }
        }
        for( std::size_t j = 0; j < best.faces.size(); ++j )
        {
            face_at[s + j] = best.faces[j];
        }
        s += best.faces.size();
    }

    auto view_for = [&]( const PD_T & pd, Int f, const MoveFaceMap<PD_T> * m ) -> View
    {
        View v;
        v.exterior = static_cast<long long>(MinimalDarc(pd, f));
        if( m && m->sidedQ && !m->after_image.empty() )
        {
            ChooseOutside(*m, f, v.outside, v.behindQ);
        }
        return v;
    };

    for( std::size_t t = 0; t < A.size(); ++t )
    {
        if( face_at[t] < Int(0) ) { continue; }
        View v = view_for(*recs[A[t]].state, face_at[t], &maps[t]);
        v.seamQ = (seam[t] != 0);
        out[A[t]].view = v;
    }

    // Candidates: borrow the face of an applied record showing the same
    // diagram -- the next one first (the emitter writes a candidate just before
    // the state it was evaluated on is acted upon), else the previous one.
    for( std::size_t i = 0; i < n; ++i )
    {
        if( !recs[i].candidateQ || !has_diagramQ(i) ) { continue; }

        std::vector<std::size_t> tries;
        for( std::size_t t = 0; t < A.size(); ++t ) { if( A[t] > i ) { tries.push_back(t); break; } }
        for( std::size_t t = A.size(); t-- > 0; ) { if( A[t] < i ) { tries.push_back(t); break; } }

        for( std::size_t t : tries )
        {
            if( face_at[t] < Int(0) ) { continue; }
            std::vector<Int> fmap; std::string why;
            if( !FaceMapByIsomorphism(*recs[A[t]].state, *recs[i].state, fmap, why) ) { continue; }
            auto m = BuildMoveFaceMap<PD_T>(recs[i], *recs[i].state, nullptr);
            out[i].view = view_for(*recs[i].state, fmap[static_cast<std::size_t>(face_at[t])],
                                   m.after_image.empty() ? nullptr : &m);
            break;
        }
        if( !out[i].view ) { out[i].note = "candidate: no applied record shows the same diagram"; }
    }

    return out;
}

//==============================================================================
// Checking a stream's claim
//==============================================================================

/*!@brief Check the `#view` claims of a whole trace; one `#verify` line per
 * move that has something to say. Returns false on any MISMATCH.
 *
 * Claims checked, per record with a move and a `#view`:
 *
 *   outside=   required when the corridor cuts the exterior face (the drawing
 *              cannot know which way round to route without it); when the
 *              face lies on one side, it must name that side if present;
 *   behind     present exactly when a witnessed middlepass keeps infinity on
 *              its own side `s`; never on a uniform pass;
 *   continuity the next applied record's exterior is the image of this one's
 *              (its `outside` part), unless the next record declares `seam`.
 *              Not claimed across a `redraw`.
 */
template<class PD_T>
bool CheckExteriorThread(
    const std::vector<typename Knoodle::MoveTrace<PD_T>::Record> & recs,
    const std::vector<std::size_t> & steps,   // step number of each record
    std::ostream & os )
{
    using Int = typename PD_T::Int;
    bool okQ = true;

    auto has_diagramQ = [&]( std::size_t i )
    {
        return recs[i].state && (recs[i].state->CrossingCount() > Int(0));
    };

    for( std::size_t i = 0; i < recs.size(); ++i )
    {
        const auto & r = recs[i];
        if( !has_diagramQ(i) || !r.move ) { continue; }
        const std::string kind = detail::MoveKind(*r.move);
        if( kind == "redraw" ) { continue; }

        const std::string label = "#verify step " + std::to_string(steps[i]) + " exterior: ";

        const auto line = ViewLine(r.headers);
        if( !line ) { os << label << "UNCHECKED (no '#view')\n"; continue; }
        View v; std::string why;
        if( !ParseView(*line, v, why) )
        {
            os << label << "MISMATCH -- " << why << "\n";
            okQ = false; continue;
        }

        const PD_T & pd = *r.state;
        const Int e = LeftFace(pd, static_cast<Int>(v.exterior));
        if( e < Int(0) )
        {
            os << label << "MISMATCH -- exterior darc " << v.exterior
               << " is not a darc of this snapshot\n";
            okQ = false; continue;
        }

        // The next applied record, when this one advances the diagram.
        std::optional<std::size_t> nx;
        if( !r.candidateQ )
        {
            for( std::size_t j = i + 1; j < recs.size(); ++j )
            {
                if( !recs[j].candidateQ ) { nx = j; break; }
            }
            if( nx && !has_diagramQ(*nx) ) { nx.reset(); }
        }

        const auto m = BuildMoveFaceMap<PD_T>(r, pd, nx ? &*recs[*nx].state : nullptr);
        if( !m.okQ )
        {
            os << label << "UNCHECKED (" << m.why << ")\n";
            continue;
        }

        int sigma = 0;
        bool cutQ = false;
        if( m.sidedQ )
        {
            const auto & im = m.after_image[static_cast<std::size_t>(e)];
            const bool on0 = im[0] >= Int(0), on1 = im[1] >= Int(0);
            if( on0 && on1 )
            {
                if( v.outside < 0 )
                {
                    os << label << "MISMATCH -- the corridor cuts the exterior face,"
                          " so '#view' must say outside=0|1\n";
                    okQ = false; continue;
                }
                sigma = v.outside;
                cutQ  = true;
            }
            else
            {
                sigma = on0 ? 0 : 1;
                if( (v.outside >= 0) && (v.outside != sigma) )
                {
                    os << label << "MISMATCH -- outside=" << v.outside
                       << " but the exterior face lies wholly on side " << sigma << "\n";
                    okQ = false; continue;
                }
            }
            const bool witnessQ = m.middlepassQ && (m.feas_side >= 0);
            const bool expect_behindQ = witnessQ && (sigma == m.feas_side);
            if( v.behindQ != expect_behindQ )
            {
                os << label << "MISMATCH -- " << (v.behindQ ? "'behind' is set" : "'behind' is missing")
                   << ": infinity is on side " << sigma << " and the swept side is "
                   << (witnessQ ? std::to_string(m.feas_side) : std::string("not fixed (uniform pass)"))
                   << "\n";
                okQ = false; continue;
            }
        }
        else if( (v.outside >= 0) || v.behindQ )
        {
            os << label << "MISMATCH -- outside=/behind mean nothing for kind=" << kind << "\n";
            okQ = false; continue;
        }

        if( !nx )
        {
            os << label << "VERIFIED (" << (r.candidateQ ? "candidate; " : "")
               << "no following record to continue into)\n";
            continue;
        }

        const auto nline = ViewLine(recs[*nx].headers);
        View nv;
        if( !nline || !ParseView(*nline, nv, why) )
        {
            os << label << "UNCHECKED (the next record has no usable '#view')\n";
            continue;
        }
        if( nv.seamQ )
        {
            os << label << "UNCHECKED (the next record declares a seam)\n";
            continue;
        }
        const Int got  = LeftFace(*recs[*nx].state, static_cast<Int>(nv.exterior));
        const Int want = m.next_image[static_cast<std::size_t>(e)][static_cast<std::size_t>(sigma)];
        if( got == want )
        {
            os << label << "VERIFIED (face " << e << " -> face " << want
               << (m.sidedQ ? ", outside=" + std::to_string(sigma) : std::string())
               << (cutQ ? ", corridor cuts it" : "")
               << (v.behindQ ? ", behind" : "") << ")\n";
        }
        else
        {
            os << label << "MISMATCH -- the next record's exterior is face " << got
               << " (darc " << nv.exterior << "), but face " << e
               << (m.sidedQ ? " on side " + std::to_string(sigma) : std::string())
               << " goes to face " << want << "\n";
            okQ = false;
        }
    }
    return okQ;
}

} // namespace KnoodleExterior
