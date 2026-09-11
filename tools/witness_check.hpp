#pragma once

/*
 * The verifier half of the middlepass feasibility witness.
 *
 * A record may carry `#feas side=<s> disk=<...>` and `#fvar <pieces>=<label>`
 * lines (see docs/move-descriptor.md and handoff
 * middlepass-descriptor-emission/): a claimed labelling of every piece on side
 * `s` of the move's sweep disk. The emitter checks V1/V2/V3/V5 against its own
 * reading of the disk. The two checks that need an INDEPENDENT reading of the
 * disk live here:
 *
 *   V0  the disk. Which crossings are interior on side s, and which pieces lie
 *       on side s -- rebuilt from the snapshot and the descriptor alone, then
 *       compared both ways against `disk=` and the pieces the classes name.
 *   V4  the classes. They must be exactly the unions forced by same-strand
 *       equalities at the disk's crossings: two pieces share a class iff a
 *       chain of such equalities joins them. A stray merge fails, and so does
 *       a split (which is V1 seen from the other side).
 *
 * The remaining checks are pure lookups once V0 has fixed the pieces, and they
 * are here too, so that a record's verdict never rests on the emitter's gate:
 *
 *   V2  at each interior crossing of W, the transversal's piece on the side is
 *       above if the transversal passes over W, below if under;
 *   V3  at each disk crossing, never under-strand above with over-strand
 *       below -- at an anchor, between whatever non-W pieces the strands have;
 *   V5  each `cross=` tag agrees with its arc's half on the side: `o` iff that
 *       half is below.
 *
 * `f` (free) is filled below throughout.
 *
 * Nothing here solves anything, and nothing reads the emitter's code.
 *
 * V0: the face-fragment flood
 * ---------------------------
 * The loop is W (between the two corridor endpoints) followed by the corridor.
 * The corridor enters its faces F_0..F_k through the middle of darcs -- the
 * strand's own end arcs at its two ends, and each crossed arc in between -- so
 * it cuts each F_i into two fragments. Every other face is one fragment.
 * Fragments are joined across every arc HALF the loop does not occupy:
 *
 *   - W's interior arcs are entirely loop;
 *   - on W's first and last arc only the half toward W's interior is loop --
 *     the corridor leaves from the middle of the arc, so the half toward the
 *     anchor (the stub) is passable;
 *   - every other arc, crossed arcs included, is passable on both halves.
 *
 * On a connected diagram this leaves exactly two regions (Jordan; asserted).
 * A piece never meets the loop, so it lies in one region. Every W arc is
 * NEUTRAL: it carries no piece.
 *
 * Which region is side 0? The side numbering is middlestrands' traversal
 * convention (`Feasibility::Partition`): walk W's component forward from the
 * arc after W's orientation-last arc; the first non-W piece reached is on
 * side 0 (a route arc is entered through its tail half).
 *
 * A crossing is interior on side s iff every incident piece is on side s, W
 * arcs being neutral (the arc-based rule, handoff §G.7). An anchor whose three
 * non-W pieces all lie on side s is therefore interior on side s -- which is
 * exactly what the stub makes true geometrically.
 *
 * Restrictions, each reported as a reason rather than guessed around: W of at
 * least two arcs, a connected diagram, a corridor that visits each face once.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace KnoodleWitness
{

/*!@brief The name of a piece as the witness grammar spells it. */
inline std::string PieceName( long long arc, int half )
{
    return std::to_string(arc) + (half < 0 ? "" : (half == 0 ? "t" : "h"));
}

/*!@brief Key for a piece: whole arc a -> 3a, tail half -> 3a+1, head -> 3a+2. */
template<typename Int>
constexpr Int PieceKey( Int arc, int half )
{
    return Int(3) * arc + Int(half + 1);
}

/*!@brief The two sides of the loop W + corridor, read off the diagram. */
template<class PD_T>
struct Sides_T
{
    using Int = typename PD_T::Int;

    // Indexed by PieceKey: -1 if there is no such piece (a W arc, an inactive
    // arc, a half of an uncrossed arc, the whole of a crossed one), else 0/1.
    std::vector<signed char> side_of;

    std::vector<char> in_W;       // per arc
    std::vector<char> in_route;   // per arc

    std::vector<Int> disk [2];    // interior crossings per side, ascending
};

/*!@brief Rebuild both sides of the move's loop. False, with a reason, when the
 * reconstruction does not apply (see the restrictions above).
 */
template<class PD_T>
bool ReconstructSides(
    const PD_T &                                          pd,
    const Knoodle::PassDescriptor<typename PD_T::Int> &  mv,
    Sides_T<PD_T> &                                       out,
    std::string &                                         why )
{
    using Int    = typename PD_T::Int;
    using Desc_T = Knoodle::PassDescriptor<Int>;

    auto Z = []( Int i ) { return static_cast<std::size_t>(i); };

    const std::size_t L = mv.strand.size();
    const std::size_t k = mv.cross.size();

    if( L < 2 )
    {
        why = "W is a single arc; the reconstruction needs W's first and last"
              " arcs to differ";
        return false;
    }
    if( pd.DiagramComponentCount() != Int(1) )
    {
        why = "the diagram has " + std::to_string(pd.DiagramComponentCount())
            + " diagram components; a loop in one says nothing about the others";
        return false;
    }

    const Int n_a = pd.MaxArcCount();

    out.in_W.assign(Z(n_a), char(0));
    out.in_route.assign(Z(n_a), char(0));
    for( Int da : mv.strand ) { out.in_W[Z(Desc_T::ArcOf(da))] = char(1); }
    for( Int da : mv.cross  ) { out.in_route[Z(Desc_T::ArcOf(da))] = char(1); }

    const auto & in_W     = out.in_W;
    const auto & in_route = out.in_route;

    // ---- every darc's face and position on that face's boundary cycle ------
    const auto & F_dA = pd.FaceDarcs();
    const Int    n_f  = F_dA.SublistCount();

    std::vector<Int> face_of (Z(Int(2) * n_a), Int(-1));
    std::vector<Int> pos_of  (Z(Int(2) * n_a), Int(-1));

    for( Int f = 0; f < n_f; ++f )
    {
        auto darcs = F_dA[f];
        Int p = 0;
        for( auto it = darcs.begin(); it != darcs.end(); ++it, ++p )
        {
            face_of[Z(*it)] = f;
            pos_of [Z(*it)] = p;
        }
    }

    // ---- the corridor's faces and where it cuts them ------------------------
    std::vector<Int> F (k + 1);
    F[0] = Desc_T::LeftFace(pd, mv.depart);
    for( std::size_t i = 0; i < k; ++i )
    {
        if( Desc_T::LeftFace(pd, mv.cross[i]) != F[i] )
        {
            why = "the corridor's face chain breaks at cross[" + std::to_string(i) + "]";
            return false;
        }
        F[i+1] = Desc_T::RightFace(pd, mv.cross[i]);
    }
    if( Desc_T::LeftFace(pd, mv.land) != F[k] )
    {
        why = "the corridor's last face is not the face left of land";
        return false;
    }
    {
        std::vector<Int> sorted = F;
        std::sort(sorted.begin(), sorted.end());
        const auto dup = std::adjacent_find(sorted.begin(), sorted.end());
        if( dup != sorted.end() )
        {
            why = "the corridor visits face " + std::to_string(*dup)
                + " twice; the reconstruction handles a face-simple corridor only";
            return false;
        }
    }

    // Fragment id of each darc HALF (index 2*da + h; h = 0 the half the darc
    // starts on, h = 1 the half it ends on). Uncut faces: the face index. A cut
    // face F_i keeps its index for the halves strictly between the two cuts
    // (in cycle order) and takes n_f + i for the rest.
    std::vector<Int> frag (Z(Int(4) * n_a), Int(-1));
    for( Int da = 0; da < Int(2) * n_a; ++da )
    {
        if( face_of[Z(da)] >= Int(0) )
        {
            frag[Z(Int(2)*da)] = frag[Z(Int(2)*da + Int(1))] = face_of[Z(da)];
        }
    }

    for( std::size_t i = 0; i <= k; ++i )
    {
        const Int c1 = (i == 0) ? mv.depart : (mv.cross[i-1] ^ Int(1));
        const Int c2 = (i == k) ? mv.land   : mv.cross[i];

        if( (face_of[Z(c1)] != F[i]) || (face_of[Z(c2)] != F[i]) )
        {
            why = "internal: ArcFaces and FaceDarcs disagree about face "
                + std::to_string(F[i]);
            return false;
        }
        if( c1 == c2 )
        {
            why = "the corridor enters and leaves face " + std::to_string(F[i])
                + " through the same darc";
            return false;
        }

        Int p = pos_of[Z(c1)];
        Int q = pos_of[Z(c2)];
        if( p > q ) { std::swap(p,q); }

        auto darcs = F_dA[F[i]];
        Int pos = 0;
        for( auto it = darcs.begin(); it != darcs.end(); ++it, ++pos )
        {
            for( Int h = 0; h < Int(2); ++h )
            {
                const Int idx = Int(2) * pos + h;
                const bool betweenQ = (idx >= Int(2) * p + Int(1)) && (idx <= Int(2) * q);
                if( !betweenQ ) { frag[Z(Int(2) * (*it) + h)] = n_f + Int(i); }
            }
        }
    }

    // ---- union-find over fragments -----------------------------------------
    const Int n_frag = n_f + Int(k) + Int(1);
    std::vector<Int> parent (Z(n_frag));
    for( Int i = 0; i < n_frag; ++i ) { parent[Z(i)] = i; }

    auto find = [&parent,&Z]( Int x )
    {
        while( parent[Z(x)] != x )
        {
            parent[Z(x)] = parent[Z(parent[Z(x)])];
            x = parent[Z(x)];
        }
        return x;
    };

    const Int s_first = mv.strand.front();
    const Int s_last  = mv.strand.back();

    // Is this half of arc a part of the loop? half: 0 tail, 1 head.
    auto loopQ = [&]( Int a, int half ) -> bool
    {
        if( !in_W[Z(a)] ) { return false; }
        // First arc: the loop half is the one at the head of darc s_first.
        if( a == Desc_T::ArcOf(s_first) )
        {
            return half == (Desc_T::DirOf(s_first) ? 1 : 0);
        }
        // Last arc: the loop half is the one at the tail of darc s_last.
        if( a == Desc_T::ArcOf(s_last) )
        {
            return half == (Desc_T::DirOf(s_last) ? 0 : 1);
        }
        return true;
    };

    // The fragment beside arc half (a, half) on the side of darc 2a+d. The
    // forward darc 2a+1 starts at a's tail; the backward darc 2a ends there.
    auto frag_of_half = [&]( Int a, int half, int d ) -> Int
    {
        const Int da = Int(2) * a + Int(d);
        const Int h  = (d == 1) ? Int(half) : Int(1 - half);
        return frag[Z(Int(2) * da + h)];
    };

    for( Int a = 0; a < n_a; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; }

        for( int half = 0; half < 2; ++half )
        {
            if( loopQ(a,half) ) { continue; }

            const Int f1 = frag_of_half(a,half,1);
            const Int f2 = frag_of_half(a,half,0);
            if( (f1 < Int(0)) || (f2 < Int(0)) )
            {
                why = "internal: arc " + std::to_string(a) + " has no face";
                return false;
            }
            const Int r1 = find(f1);
            const Int r2 = find(f2);
            if( r1 != r2 ) { parent[Z(r1)] = r2; }
        }
    }

    std::vector<Int> roots;
    for( Int i = 0; i < n_frag; ++i ) { roots.push_back(find(i)); }
    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());

    if( roots.size() != 2 )
    {
        why = "the loop W + corridor leaves " + std::to_string(roots.size())
            + " regions, not two";
        return false;
    }

    auto region = [&]( Int a, int half ) -> Int
    {
        return find(frag_of_half(a, (half < 0) ? 0 : half, 1));
    };

    // ---- side 0: the first non-W piece past W's orientation-last arc --------
    const Int a_last = Desc_T::DirOf(s_first) ? Desc_T::ArcOf(s_last)
                                              : Desc_T::ArcOf(s_first);
    Int cur = pd.NextArc(a_last, PD_T::Head);
    for( Int guard = 0; in_W[Z(cur)]; ++guard )
    {
        if( guard > n_a )
        {
            why = "W is its whole component, so there is no piece to fix side 0";
            return false;
        }
        cur = pd.NextArc(cur, PD_T::Head);
    }
    const Int root0 = region(cur, in_route[Z(cur)] ? 0 : -1);

    out.side_of.assign(Z(Int(3) * n_a), static_cast<signed char>(-1));
    for( Int a = 0; a < n_a; ++a )
    {
        if( !pd.ArcActiveQ(a) || in_W[Z(a)] ) { continue; }

        if( in_route[Z(a)] )
        {
            for( int half = 0; half < 2; ++half )
            {
                out.side_of[Z(PieceKey(a,half))] = (region(a,half) == root0) ? 0 : 1;
            }
        }
        else
        {
            out.side_of[Z(PieceKey(a,-1))] = (region(a,-1) == root0) ? 0 : 1;
        }
    }

    // ---- interior crossings: every incident piece on one side ---------------
    const auto & C_arcs = pd.Crossings();
    out.disk[0].clear();
    out.disk[1].clear();

    for( Int c = 0; c < pd.MaxCrossingCount(); ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }

        int seen = 0;   // bit s set: some incident piece lies on side s
        for( bool io : { PD_T::Out, PD_T::In } )
        {
            for( bool lr : { PD_T::Left, PD_T::Right } )
            {
                const Int a = C_arcs(c,io,lr);
                if( in_W[Z(a)] ) { continue; }
                // An arc leaves through an Out port, so that end is its tail.
                const int half = in_route[Z(a)] ? ((io == PD_T::Out) ? 0 : 1) : -1;
                seen |= (out.side_of[Z(PieceKey(a,half))] == 0) ? 1 : 2;
            }
        }
        if( !(seen & 2) ) { out.disk[0].push_back(c); }
        if( !(seen & 1) ) { out.disk[1].push_back(c); }
    }

    return true;
}

/*!@brief What V0 and V4 made of one witness. */
struct WitnessReport_T
{
    bool        v0_checkedQ = false;
    bool        v0_okQ      = false;
    std::string v0_why;

    bool        v4_checkedQ = false;
    bool        v4_okQ      = false;
    std::string v4_why;

    bool        labels_checkedQ = false;   // V2, V3 and V5 together
    bool        labels_okQ      = false;
    std::string labels_why;                // every failing check, first instance each

    std::size_t disk_size   = 0;
    std::size_t piece_count = 0;
    std::size_t class_count = 0;
    std::size_t germ_count  = 0;
    std::size_t order_count = 0;
    std::size_t tag_count   = 0;
};

/*!@brief The pieces at crossing `c` as keys, in the order under-strand in,
 * under-strand out, over-strand in, over-strand out; -1 where the arc is W's.
 */
template<class PD_T>
std::array<typename PD_T::Int,4> StrandKeysAt(
    const PD_T &          pd,
    const Sides_T<PD_T> & S,
    typename PD_T::Int    c )
{
    using Int = typename PD_T::Int;

    const auto & C_arcs = pd.Crossings();

    // The under-strand enters at (In,Right) at a right-handed crossing and at
    // (In,Left) at a left-handed one, and leaves straight through.
    const bool side = pd.CrossingRightHandedQ(c) ? PD_T::Right : PD_T::Left;

    const Int arcs [4] = {
        C_arcs(c, PD_T::In,  side), C_arcs(c, PD_T::Out, !side),
        C_arcs(c, PD_T::In, !side), C_arcs(c, PD_T::Out,  side)
    };

    std::array<Int,4> keys;
    for( int i = 0; i < 4; ++i )
    {
        const auto z = static_cast<std::size_t>(arcs[i]);
        if( S.in_W[z] ) { keys[std::size_t(i)] = Int(-1); continue; }

        // An arc arrives at an In port with its head, leaves an Out port with
        // its tail.
        const int half = S.in_route[z] ? ((i % 2 == 0) ? 1 : 0) : -1;
        keys[std::size_t(i)] = PieceKey(arcs[i], half);
    }
    return keys;
}

/*!@brief V2, V3 and V5: the labels satisfy the clauses the snapshot forces.
 * Needs V0's pieces to be right (`class_of` maps every piece key on the side to
 * its class).
 */
template<class PD_T>
void CheckLabels(
    const PD_T &                                              pd,
    const Knoodle::PassDescriptor<typename PD_T::Int> &      mv,
    const typename Knoodle::MoveTrace<PD_T>::Feasibility &   w,
    const Sides_T<PD_T> &                                     S,
    const std::vector<int> &                                  class_of,
    WitnessReport_T &                                         r )
{
    using Int    = typename PD_T::Int;
    using Desc_T = Knoodle::PassDescriptor<Int>;

    auto Z = []( Int i ) { return static_cast<std::size_t>(i); };

    r.labels_checkedQ = true;

    const int s = w.side;

    auto label_of = [&]( Int key )
    {
        return w.classes[std::size_t(class_of[Z(key)])].label;
    };
    auto belowQ  = []( char lab ) { return lab != 'a'; };   // free fills below
    auto name_of = []( Int key )
    {
        return PieceName(static_cast<long long>(key / Int(3)), int(key % Int(3)) - 1);
    };

    std::string v2, v3, v5;

    // ---- V2: the germs at W's interior crossings ----------------------------
    std::vector<char> W_interior (Z(pd.MaxCrossingCount()), char(0));
    for( std::size_t i = 1; i < mv.strand.size(); ++i )
    {
        W_interior[Z(Desc_T::DarcHeadCrossing(pd, mv.strand[i-1]))] = char(1);
    }

    for( std::size_t i = 1; i < mv.strand.size(); ++i )
    {
        const Int  x = Desc_T::DarcHeadCrossing(pd, mv.strand[i-1]);
        const auto k = StrandKeysAt(pd, S, x);

        const bool W_underQ = (k[0] < Int(0)) || (k[1] < Int(0));
        const bool W_overQ  = (k[2] < Int(0)) || (k[3] < Int(0));
        if( W_underQ == W_overQ ) { continue; }   // W crosses itself here

        // A transversal passing over W is forced above; under W, below.
        const char want = W_underQ ? 'a' : 'b';

        for( Int key : { W_underQ ? k[2] : k[0], W_underQ ? k[3] : k[1] } )
        {
            // A crossed arc running between two interior crossings of W is a
            // chord: one physical arc, so a germ at either end forces both of
            // its halves (middlestrands' Feasibility::ForcedPieces).
            const Int  a      = key / Int(3);
            const bool chordQ = S.in_route[Z(a)]
                             && W_interior[Z(pd.Arcs()(a, PD_T::Tail))]
                             && W_interior[Z(pd.Arcs()(a, PD_T::Head))];

            std::vector<Int> forced;
            if( chordQ ) { forced = { PieceKey(a,0), PieceKey(a,1) }; }
            else         { forced = { key }; }

            for( Int f : forced )
            {
                if( S.side_of[Z(f)] != s ) { continue; }   // the other side's germ

                ++r.germ_count;
                const char lab = label_of(f);
                if( ((want == 'a') ? (lab == 'a') : belowQ(lab)) || !v2.empty() ) { continue; }

                v2 = "at crossing " + std::to_string(x) + " of W the transversal passes "
                   + (want == 'a' ? "over" : "under") + " W, so " + name_of(f)
                   + (chordQ ? " (a chord of W)" : "") + " must be "
                   + (want == 'a' ? "above" : "below") + ", but its class is ="
                   + lab + " (V2)";
            }
        }
    }

    // ---- V3: never under-strand above with over-strand below ----------------
    for( Int c : S.disk[s] )
    {
        const auto k = StrandKeysAt(pd, S, c);

        // At an anchor one strand runs along W's end arc, which carries no
        // piece. The ordering still binds whatever pieces the two strands DO
        // have there: middlestrands' FeasibleSide imposes these edges, and a
        // witness that breaks one can prescribe a move that changes the knot
        // (found by a synthetic search: determinant 1 -> 29 on an unknot).
        bool countedQ = false;
        for( Int ku : { k[0], k[1] } )
        {
            for( Int ko : { k[2], k[3] } )
            {
                if( (ku < Int(0)) || (ko < Int(0)) ) { continue; }
                if( !countedQ ) { ++r.order_count; countedQ = true; }

                const char lu = label_of(ku);
                const char lo = label_of(ko);
                if( (lu != 'a') || !belowQ(lo) || !v3.empty() ) { continue; }

                const bool anchorQ = (k[0] < Int(0)) || (k[1] < Int(0))
                                  || (k[2] < Int(0)) || (k[3] < Int(0));
                v3 = "at crossing " + std::to_string(c) + (anchorQ ? " (an anchor)" : "")
                   + " the under-strand's " + name_of(ku) + " is above (=a) but the"
                     " over-strand's " + name_of(ko) + " is below (=" + lo + ") (V3)";
            }
        }
    }

    // ---- V5: each route tag agrees with its arc's half on the side ---------
    for( std::size_t i = 0; i < mv.cross.size(); ++i )
    {
        const Int a   = Desc_T::ArcOf(mv.cross[i]);
        const Int key = (S.side_of[Z(PieceKey(a,0))] == s) ? PieceKey(a,0) : PieceKey(a,1);
        if( S.side_of[Z(key)] != s ) { continue; }

        ++r.tag_count;
        const char lab = label_of(key);
        if( (bool(mv.over[i]) == belowQ(lab)) || !v5.empty() ) { continue; }

        v5 = "cross[" + std::to_string(i) + "] is " + std::to_string(mv.cross[i]) + ":"
           + (mv.over[i] ? "o" : "u") + ", but " + name_of(key) + ", its half on side "
           + std::to_string(s) + ", is =" + lab + ", which calls for :"
           + (belowQ(lab) ? "o" : "u") + " (V5)";
    }

    for( const std::string * m : { &v2, &v3, &v5 } )
    {
        if( m->empty() ) { continue; }
        if( !r.labels_why.empty() ) { r.labels_why += "; "; }
        r.labels_why += *m;
    }
    r.labels_okQ = r.labels_why.empty();
}

/*!@brief Run V0-V5 on a witness against its record's snapshot and move. */
template<class PD_T>
WitnessReport_T CheckWitness(
    const PD_T &                                                      pd,
    const Knoodle::PassDescriptor<typename PD_T::Int> &              mv,
    const typename Knoodle::MoveTrace<PD_T>::Feasibility &           w )
{
    using Int = typename PD_T::Int;

    auto Z = []( Int i ) { return static_cast<std::size_t>(i); };

    WitnessReport_T r;
    r.class_count = w.classes.size();

    Sides_T<PD_T> S;
    std::string why;
    if( !ReconstructSides(pd, mv, S, why) )
    {
        r.v0_why = why;
        r.v4_why = "V0 could not rebuild the disk";
        return r;
    }
    r.v0_checkedQ = true;

    const int s   = w.side;
    const Int n_a = pd.MaxArcCount();

    // ---- V0 (a): the interior crossings -------------------------------------
    std::vector<Int> claimed = w.disk;
    std::sort(claimed.begin(), claimed.end());
    {
        const auto dup = std::adjacent_find(claimed.begin(), claimed.end());
        if( dup != claimed.end() )
        {
            r.v0_why = "disk= lists crossing " + std::to_string(*dup) + " twice";
            return r;
        }
    }

    const auto & ours = S.disk[s];
    r.disk_size = ours.size();

    std::vector<Int> missing, extra;
    std::set_difference(ours.begin(), ours.end(), claimed.begin(), claimed.end(),
                        std::back_inserter(missing));
    std::set_difference(claimed.begin(), claimed.end(), ours.begin(), ours.end(),
                        std::back_inserter(extra));

    std::string disk_why;
    if( !missing.empty() )
    {
        disk_why = "crossing " + std::to_string(missing.front()) + " is interior on"
                   " side " + std::to_string(s) + " but disk= omits it";
        if( missing.size() > 1 )
        {
            disk_why += " (and " + std::to_string(missing.size() - 1) + " more:";
            for( std::size_t i = 1; i < missing.size() && i < 9; ++i )
            {
                disk_why += " " + std::to_string(missing[i]);
            }
            disk_why += (missing.size() > 9) ? " ...)" : ")";
        }
    }
    else if( !extra.empty() )
    {
        const Int c = extra.front();
        disk_why = "disk= lists crossing " + std::to_string(c) + ", which is "
                 + ((c >= Int(0) && c < pd.MaxCrossingCount() && pd.CrossingActiveQ(c))
                    ? "not interior on side " + std::to_string(s)
                    : std::string("not an active crossing"));
    }

    // ---- V0 (b): the pieces, both ways --------------------------------------
    std::vector<int> class_of (Z(Int(3) * n_a), -1);
    std::string piece_why;

    for( std::size_t ci = 0; ci < w.classes.size() && piece_why.empty(); ++ci )
    {
        for( const auto & p : w.classes[ci].pieces )
        {
            const std::string name = PieceName(static_cast<long long>(p.arc), p.half);

            if( p.arc >= n_a || !pd.ArcActiveQ(p.arc) )
            {
                piece_why = "piece " + name + " is not an active arc";
                break;
            }
            if( S.in_W[Z(p.arc)] )
            {
                piece_why = "piece " + name + " is an arc of W, which carries no piece";
                break;
            }
            if( S.in_route[Z(p.arc)] != char(p.half >= 0) )
            {
                piece_why = "piece " + name + ": arc " + std::to_string(p.arc)
                          + (S.in_route[Z(p.arc)]
                             ? " is crossed by the corridor, so its pieces are "
                               + std::to_string(p.arc) + "t and " + std::to_string(p.arc) + "h"
                             : " is not crossed by the corridor, so it is one whole piece");
                break;
            }
            const signed char sd = S.side_of[Z(PieceKey(p.arc,p.half))];
            if( sd != s )
            {
                piece_why = "piece " + name + " lies on side " + std::to_string(int(sd))
                          + ", not on the witness's side " + std::to_string(s);
                break;
            }
            class_of[Z(PieceKey(p.arc,p.half))] = int(ci);
            ++r.piece_count;
        }
    }

    if( piece_why.empty() )
    {
        for( Int key = 0; key < Int(3) * n_a; ++key )
        {
            if( (S.side_of[Z(key)] == s) && (class_of[Z(key)] < 0) )
            {
                piece_why = "piece " + PieceName(static_cast<long long>(key / Int(3)),
                                                 int(key % Int(3)) - 1)
                          + " lies on side " + std::to_string(s)
                          + " but is in no #fvar class";
                break;
            }
        }
    }

    if( disk_why.empty() && piece_why.empty() )
    {
        r.v0_okQ = true;
    }
    else
    {
        r.v0_why = disk_why.empty() ? piece_why
                 : (piece_why.empty() ? disk_why : disk_why + "; also " + piece_why);
    }

    if( !piece_why.empty() )
    {
        r.v4_why     = "the witness's pieces are not the side's pieces (V0)";
        r.labels_why = r.v4_why;
        return r;
    }

    // Before V4, whose mismatches return early: a bad class must not hide a
    // bad label.
    CheckLabels<PD_T>(pd, mv, w, S, class_of, r);

    // ---- V4: classes are exactly the same-strand unions at disk crossings ---
    r.v4_checkedQ = true;

    const Int n_key = Int(3) * n_a;
    std::vector<Int> up (Z(n_key));
    for( Int i = 0; i < n_key; ++i ) { up[Z(i)] = i; }

    auto find = [&up,&Z]( Int x )
    {
        while( up[Z(x)] != x ) { up[Z(x)] = up[Z(up[Z(x)])]; x = up[Z(x)]; }
        return x;
    };

    const auto & C_arcs = pd.Crossings();

    for( Int c : ours )
    {
        // A strand goes straight through: in at (In,lr), out at (Out,!lr).
        for( bool lr : { PD_T::Left, PD_T::Right } )
        {
            const Int a_in  = C_arcs(c, PD_T::In,  lr);
            const Int a_out = C_arcs(c, PD_T::Out, !lr);
            if( S.in_W[Z(a_in)] || S.in_W[Z(a_out)] ) { continue; }   // anchor pair

            const Int k_in  = PieceKey(a_in,  S.in_route[Z(a_in)]  ? 1 : -1);
            const Int k_out = PieceKey(a_out, S.in_route[Z(a_out)] ? 0 : -1);
            const Int r1 = find(k_in), r2 = find(k_out);
            if( r1 != r2 ) { up[Z(r1)] = r2; }
        }
    }

    auto name_of = [&]( Int key )
    {
        return PieceName(static_cast<long long>(key / Int(3)), int(key % Int(3)) - 1);
    };

    std::vector<Int> class_root  (w.classes.size(), Int(-1));
    std::vector<Int> class_first (w.classes.size(), Int(-1));
    std::vector<int> root_class  (Z(n_key), -1);
    std::vector<Int> root_first  (Z(n_key), Int(-1));

    for( std::size_t ci = 0; ci < w.classes.size(); ++ci )
    {
        for( const auto & p : w.classes[ci].pieces )
        {
            const Int key  = PieceKey(p.arc,p.half);
            const Int root = find(key);

            if( class_root[ci] < Int(0) )
            {
                class_root[ci]  = root;
                class_first[ci] = key;
            }
            else if( class_root[ci] != root )
            {
                r.v4_why = "class " + std::to_string(ci) + " (=" + w.classes[ci].label
                         + ") joins " + name_of(class_first[ci]) + " and " + name_of(key)
                         + ", but no chain of same-strand equalities at the disk's"
                           " crossings connects them -- a stray merge (V4)";
                return r;
            }

            if( root_class[Z(root)] < 0 )
            {
                root_class[Z(root)] = int(ci);
                root_first[Z(root)] = key;
            }
            else if( root_class[Z(root)] != int(ci) )
            {
                r.v4_why = name_of(root_first[Z(root)]) + " (class "
                         + std::to_string(root_class[Z(root)]) + ") and " + name_of(key)
                         + " (class " + std::to_string(ci) + ") are joined through the"
                           " disk's crossings, so they must share a class (V1)";
                return r;
            }
        }
    }

    r.v4_okQ = true;
    return r;
}

} // namespace KnoodleWitness
