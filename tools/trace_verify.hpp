/**
 * @file trace_verify.hpp
 * @brief The picture-independent claims of a move-trace stream: the checks
 * `knoodleprove` makes, and that `knoodledraw --verify` makes alongside its
 * drawing check.
 *
 * docs/move-descriptor.md splits conformance into two tiers, and this header is
 * the boundary between them. The drawing check (the two deletions,
 * `KnoodlePassView::CheckBothDeletions`) is a claim ABOUT A PICTURE: it renders
 * each view and parses it back, so it belongs to the renderer and stays there.
 * Everything here is a claim about the COMBINATORICS and needs no layout:
 *
 *   pd        a record's `#pd` annotation is its `#state`, up to relabelling;
 *   trace     what a move produces is the NEXT record's snapshot (isomorphism);
 *   split     crossingless components the surgery frees (reported, not judged);
 *   spinoffs  the emitter's `#spinoffs` count agrees with the surgery's;
 *   result    the emitter's `#result` agrees port-by-port with `AfterDiagram`;
 *   V0/V4/V2-V5  the `#feas` witness (tools/witness_check.hpp);
 *   redraw    a re-projection's whole witness: the rotation is one of the two
 *             permitted lattice rotations, and projecting the lattice curve E
 *             and R*E exactly gives this and the next snapshot, colours kept;
 *   r1        a curl removal's local checks, which for `r1` are the whole of
 *             soundness -- the spec's "well-formed implies sound" (there is no
 *             witness to demand), so `ResolveR1` passing IS the verdict.
 *
 * The report lines are the ones `knoodledraw --verify` has always printed,
 * byte for byte, so a consumer grepping for `#verify ... VERIFIED` sees no
 * difference between the two tools.
 *
 * A record is checked in phases, because knoodledraw interleaves its drawing
 * check between them and must keep its output order:
 *
 *   NoteRecord   -> the whole-stream ledger (input colours, summands, links
 *                   a per-record check cannot see: after a #candidate, after
 *                   a record with no move)
 *   BeginRecord  -> trace (against the previous move's claim)
 *   BeginMove    -> after-diagram, split, spinoffs, result
 *   [the caller's drawing check, if any]
 *   CheckWitness -> V0, V4, V2/V3/V5
 *   EndMove      -> carry this move's claim to the next record
 *   Finish       -> the last move's trace claim: VERIFIED if it is the empty
 *                   diagram, else UNCHECKED
 *   ReportStream -> `stream unlink:`, when the stream ends at the empty diagram
 *
 * Nothing here touches a drawing. This header does not include
 * `OrthoDecorate.hpp` (the pass-overlay drawing code), and nothing below
 * builds a layout: `knoodleprove` never constructs an `OrthoDraw`, which is
 * the point -- middlepass's diagrams run to 30,000 crossings. (`Knoodle.hpp`
 * still pulls in `OrthoDraw.hpp` for its own reasons; what matters here is
 * that no layout is ever computed.)
 *
 * This header assumes `Knoodle.hpp` has already been included by the includer.
 */

#pragma once

#include "../src/PassDescriptor.hpp"
#include "../src/MoveTrace.hpp"
#include "r1_move.hpp"
#include "redraw_move.hpp"
#include "diagram_agreement.hpp"
#include "witness_check.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <ostream>
#include <string>
#include <vector>

namespace KnoodleTraceVerify
{

/**
 * @brief Does a record's `#pd` annotation describe its `#state` snapshot?
 *
 * Up to relabelling: a PD code renumbers on the way out, so the two cannot be
 * compared byte for byte even when both are right. Returns true (and leaves
 * `why` empty) when there is nothing to reconcile.
 */
template<class PD_T>
bool AnnotationAgreesQ(
    const typename Knoodle::MoveTrace<PD_T>::Record & rec,
    const PD_T & dia,
    std::string & why )
{
    using Int = typename PD_T::Int;

    why.clear();
    if( rec.pd_rows.empty() || rec.state_from_pd ) { return true; }

    const Int rows = static_cast<Int>(rec.pd_rows.size()) / Int(5);
    PD_T annotated = PD_T::FromSignedPDCode(rec.pd_rows.data(), rows);

    if( annotated.CrossingCount() <= Int(0) )
    {
        why = "the annotation did not parse into a valid diagram";
        return false;
    }
    if( DiagramsIsomorphicQ(dia, annotated, why) )
    {
        why.clear();
        return true;
    }
    return false;
}

/**
 * @brief Seed the correspondence between two claimed results of one move.
 *
 * Both sides keep every crossing the move promised not to touch at its
 * original index -- `AfterDiagram` because it rebuilds in place from the
 * before-diagram's arrays, an applier because it edits in place -- so on
 * exactly those crossings the correspondence is the IDENTITY, and propagation
 * forces everything else. Crossings created by the corridor are deliberately
 * left unseeded: neither side owes the other a numbering for them.
 *
 * This is the whole point of carrying the snapshot as internal state rather
 * than a PD code. A PD code renumbers, so the strongest question one could
 * ask of a cross-process result was "is it the same knot"; here a
 * disagreement names a crossing and a port.
 *
 * `touchedQ[c]` marks the crossings the move is allowed to dispose of -- a
 * pass move's interior crossings of W, an r1's dying crossing. Everything
 * else must survive on both sides or neither.
 */
template<class PD_T>
bool BuildSurvivorSeeds(
    const PD_T & before,
    const std::vector<char> & touchedQ,
    const PD_T & d1,
    const PD_T & d2,
    std::vector<std::array<typename PD_T::Int,2>> & seeds,
    std::string & why )
{
    using Int = typename PD_T::Int;

    seeds.clear();

    const Int n_c = before.MaxCrossingCount();

    for (Int c = 0; c < n_c; ++c)
    {
        if (!before.CrossingActiveQ(c)) continue;
        if (touchedQ[static_cast<std::size_t>(c)]) continue;

        const bool in1 = (c < d1.MaxCrossingCount()) && d1.CrossingActiveQ(c);
        const bool in2 = (c < d2.MaxCrossingCount()) && d2.CrossingActiveQ(c);

        if (in1 != in2)
        {
            why = "crossing " + std::to_string(c) + " is untouched by the move"
                  " but survives in only one of the two results (ours: "
                + std::string(in1 ? "active" : "gone") + ", theirs: "
                + std::string(in2 ? "active" : "gone") + ")";
            return false;
        }
        if (in1) { seeds.push_back({c,c}); }
    }

    if (!seeds.empty()) { return true; }

    // A move that disposes of every crossing leaves nothing to seed, and
    // nothing to need seeding if both sides agree the result is empty.
    const Int n1 = d1.CrossingCount();
    const Int n2 = d2.CrossingCount();

    if ((n1 == Int(0)) && (n2 == Int(0))) { return true; }

    why = (n1 != n2)
        ? "crossing counts differ: " + std::to_string(n1) + " (ours) vs "
          + std::to_string(n2) + " (theirs)"
        : std::string("the move touches every crossing, so there is no"
                      " untouched crossing to seed the correspondence with");
    return false;
}

/*!@brief `BuildSurvivorSeeds` for a pass move: W's interior crossings are the
 * ones it may dispose of.
 */
template<class PD_T>
bool BuildSurvivorSeeds(
    const PD_T & before,
    const Knoodle::PassDescriptor<typename PD_T::Int> & mv,
    const PD_T & d1,
    const PD_T & d2,
    std::vector<std::array<typename PD_T::Int,2>> & seeds,
    std::string & why )
{
    using Int    = typename PD_T::Int;
    using Move_T = Knoodle::PassDescriptor<Int>;

    const Int n_c = before.MaxCrossingCount();
    std::vector<char> touchedQ(static_cast<std::size_t>(n_c), char(0));

    const Int L = static_cast<Int>(mv.strand.size());
    for (Int i = 1; i < L; ++i)
    {
        const Int x = Move_T::DarcHeadCrossing(
            before, mv.strand[static_cast<std::size_t>(i-1)]);
        if ((x >= Int(0)) && (x < n_c))
        {
            touchedQ[static_cast<std::size_t>(x)] = char(1);
        }
    }

    return BuildSurvivorSeeds(before, touchedQ, d1, d2, seeds, why);
}


/*!@brief Do two results agree about which component every arc belongs to?
 *
 * Structure is not the whole claim a move makes. Both diagrams descend from
 * the same snapshot, and a component carries its color across a move: survivors
 * keep theirs, the new corridor arcs take W's, and each arc split by a corridor
 * crossing keeps the color of the arc it was split from. So under the
 * correspondence the port-by-port check already built, matched arcs must carry
 * EQUAL colors -- not merely a consistent renaming. An applier that renumbers
 * components has changed the link's labelling where the pictures agree, and on
 * a link that is a real error: it is what tells the two components apart.
 *
 * This matters exactly when there is more than one component to confuse; on a
 * knot every arc is color 0 and the check is free.
 */
template<class PD_T>
bool ColorsAgreeQ(
    const PD_T & ours, const PD_T & theirs,
    const DiagramMatch_T<typename PD_T::Int> & M,
    std::string & why )
{
    using Int = typename PD_T::Int;

    for( Int a = 0; a < ours.MaxArcCount(); ++a )
    {
        if( !ours.ArcActiveQ(a) ) { continue; }

        const Int b = M.amap[static_cast<std::size_t>(a)];
        if( b == DiagramMatch_T<Int>::None ) { continue; }
        if( (b < Int(0)) || (b >= theirs.MaxArcCount()) ) { continue; }

        const Int c1 = ours.ArcColors()[a];
        const Int c2 = theirs.ArcColors()[b];

        if( c1 != c2 )
        {
            why = "arc " + std::to_string(a) + " is colour "
                + std::to_string(c1) + " in the diagram the descriptor"
                  " produces, but the applier's arc " + std::to_string(b)
                + " (the same arc) is colour " + std::to_string(c2)
                + ": the two disagree about which component it belongs to";
            return false;
        }
    }
    return true;
}

/*!@brief What projecting a lattice witness down z produced. */
template<class PD_T>
struct LatticeProjection
{
    using Int = typename PD_T::Int;

    bool             okQ = false;
    std::string      why;
    PD_T             pd;          // meaningful only when crossings > 0
    Int              crossings = 0;
    std::vector<Int> anelli;      // colours of the crossingless components
};

/**
 * @brief Project a lattice link down z, EXACTLY, with the components' colours
 * supplied rather than invented.
 *
 * `LinkEmbedding_Int` + `Prosector` with an integer coordinate type: no
 * scaling, no rounding, and simulation of simplicity resolving every
 * degenerate projection (a lattice curve seen down an axis is maximally
 * degenerate) the same way every time. This is the ONLY projector the redraw
 * checks trust; docs/move-descriptor.md cites it as normative for the
 * projection convention (larger z is the over-strand).
 *
 * Passing the colours IN is what pins components through a rotation: the
 * diagram inherits each component's colour from the curve it came from, so
 * two interchangeable components of a symmetric link cannot be swapped.
 */
template<class PD_T>
LatticeProjection<PD_T> ProjectLattice(
    const std::vector<std::int64_t> & xyz,
    const std::vector<typename PD_T::Int> & ptr,
    const std::vector<typename PD_T::Int> & colors )
{
    using Int = typename PD_T::Int;
    using L_T = Knoodle::LinkEmbedding4<std::int64_t,Int>;

    LatticeProjection<PD_T> out;

    using T_T = Tensors::Tensor1<Int,Int>;

    L_T L ( T_T(ptr.data(),    static_cast<Int>(ptr.size())),
            T_T(colors.data(), static_cast<Int>(colors.size())) );
    L.ReadVertexCoordinates(xyz.data());

    auto [pd, anelli] = PD_T::FromLinkEmbedding(L);

    for( Int i = 0; i < anelli.Size(); ++i ) { out.anelli.push_back(anelli[i]); }

    if( pd.InvalidQ() )
    {
        // Every component crossingless is a legitimate outcome (no diagram);
        // anything else is the projector refusing -- which for an integer
        // curve means two edges meet in 3-space: not an embedding.
        if( static_cast<std::size_t>(anelli.Size()) + 1 == ptr.size() )
        {
            out.okQ = true;
            return out;
        }
        out.why = "the curve is not an embedding (the exact projector found"
                  " edges meeting in 3-space)";
        return out;
    }

    out.crossings = pd.CrossingCount();
    out.pd        = std::move(pd);
    out.okQ       = true;
    return out;
}

/*!@brief The colours present on a diagram's active arcs, sorted. */
template<class PD_T>
std::vector<typename PD_T::Int> DiagramColors( const PD_T & dia )
{
    using Int = typename PD_T::Int;
    std::set<Int> s;
    for( Int a = 0; a < dia.MaxArcCount(); ++a )
    {
        if( dia.ArcActiveQ(a) ) { s.insert(dia.ArcColors()[a]); }
    }
    return std::vector<Int>(s.begin(), s.end());
}

template<class Int>
std::string ColorList( const std::vector<Int> & v )
{
    std::string s;
    for( std::size_t i = 0; i < v.size(); ++i )
    {
        if( i ) { s += ","; }
        s += std::to_string(v[i]);
    }
    return s.empty() ? std::string("none") : s;
}

/**
 * @brief Checks one trace stream's picture-independent claims, record by
 * record, writing one `#verify` line per claim to `out`.
 *
 * Carries the pending trace claim forward rather than buffering the stream.
 */
template<class PD_T>
class TraceVerifier
{
public:

    using Int      = typename PD_T::Int;
    using Move_T   = Knoodle::PassDescriptor<Int>;
    using Record_T = typename Knoodle::MoveTrace<PD_T>::Record;

    /// What BeginMove learned, for the caller's drawing check and for EndMove.
    struct Move
    {
        bool             parsedQ = false;  // a well-formed `pass` descriptor
        std::string      parse_why;        // the refusal, if not
        Move_T           mv;
        bool             afterQ  = false;  // AfterDiagram built the result
        PD_T             after;
        std::vector<Int> freed;            // colours of freed components
        std::string      why;              // AfterDiagram's refusal, if any
    };

    explicit TraceVerifier( std::ostream & out ) : out_(out) {}

    /// True once any claim has come back MISMATCH.
    bool FailedQ() const { return failedQ_; }

    /// The `pd` line for a record whose annotation has been reconciled.
    void ReportAnnotation( std::size_t step )
    {
        out_ << "#verify step " << step
             << " pd: VERIFIED (the annotation is this snapshot,"
                " up to relabelling)\n";
    }

    /// The previous move's claim, checked against this record's snapshot.
    /// `coloredQ` is false when this snapshot was built from bare PD rows,
    /// whose colours are only the order the parser met the components in.
    void BeginRecord( const PD_T & dia, bool coloredQ = true )
    {
        if( !pending_after_ ) { return; }

        PD_T expected = std::move(*pending_after_);
        pending_after_.reset();

        // Colours are lifelong labels, so the isomorphism must keep every
        // arc's colour. A redraw's claim carries colours pinned by its
        // embedding (check 4), so it is always compared that way. For pass,
        // middlepass and r1 the claim's colours are inherited from the
        // snapshot the move was made on, so they are a claim exactly when
        // that snapshot and this one both carry real colours (v1 `#state`).
        const bool colorsQ = pending_colorsQ_ && (pending_redrawQ_ || coloredQ);
        pending_colorsQ_ = false;
        pending_redrawQ_ = false;

        std::string vwhy;
        const bool okQ = DiagramsIsomorphicQ(expected, dia, vwhy,
                                             nullptr, colorsQ);

        out_ << "#verify " << pending_label_ << " trace: "
             << (okQ ? "VERIFIED" : "MISMATCH")
             << " (" << expected.CrossingCount() << " crossings expected, "
             << dia.CrossingCount() << " found"
             << (colorsQ ? ", colours kept" : "") << ")";
        if( !okQ ) { out_ << " -- " << vwhy; }
        out_ << "\n";

        if( !okQ ) { failedQ_ = true; }
    }

    /**
     * @brief A record that carries no diagram: the summand is crossingless.
     *
     * It is still the next state, so a pending claim has to be answered HERE
     * rather than carried across it. Carried across, a move claiming three
     * crossings would be compared to whatever record came next and could be
     * reported VERIFIED against a state two steps downstream.
     */
    void BeginRecordWithoutDiagram()
    {
        emptyQ_ = true;

        if( !pending_after_ ) { return; }

        PD_T expected = std::move(*pending_after_);
        pending_after_.reset();
        pending_colorsQ_ = false;
        pending_redrawQ_ = false;

        const bool okQ = (expected.CrossingCount() == Int(0));

        out_ << "#verify " << pending_label_ << " trace: "
             << (okQ ? "VERIFIED" : "MISMATCH")
             << " (" << expected.CrossingCount()
             << " crossings expected, and the next record carries no diagram)\n";

        if( !okQ ) { failedQ_ = true; }
    }

    /**
     * @brief Build what the move produces and check the claims about it.
     *
     * `AfterDiagram` works from the descriptor alone, never calling the
     * applier, so everything here is independent of whatever produced the
     * trace. A descriptor that does not parse as a `pass` move, or parses
     * but is not well-formed against this snapshot, returns with `parsedQ`
     * false, the reason in `parse_why`, and nothing reported: the caller
     * reports it as a `descriptor:` MISMATCH.
     */
    Move BeginMove( const Record_T & rec, const PD_T & dia,
                    const std::string & spec, std::size_t step )
    {
        Move m;

        if( !Move_T::Parse(spec, m.mv, m.parse_why) ) { return m; }

        // Well-formedness is a fault in the RECORD, not a limit of our surgery,
        // so it must not fall into the UNCHECKED branch below: a crossing
        // change wearing a pass move's tags would then exit 0 (ROUND-22 §2).
        // `AfterDiagram` runs the same checks, so everything it refuses after
        // this is genuinely something it cannot build.
        std::string wwhy;
        if( !m.mv.WellFormedQ(dia, wwhy) )
        {
            m.parse_why = "not well-formed against this snapshot: " + wwhy;
            return m;
        }
        m.parsedQ = true;

        // The reporting overload: a pass move can split a crossingless
        // component off, and that is an outcome, not an error.
        m.after  = m.mv.AfterDiagram(dia, m.why, m.freed);
        m.afterQ = m.why.empty();

        // Our surgery may not carry out every well-formed move. That is a
        // limit of the checker, not a fault in the record: the claims that
        // need the after-diagram go UNCHECKED, and the witness is still
        // checked.
        if( !m.afterQ )
        {
            out_ << "#verify step " << step
                 << " result/drawing/trace: UNCHECKED (AfterDiagram"
                    " cannot build what the move produces: "
                 << m.why << ")\n";
        }
        if( !m.freed.empty() )
        {
            out_ << "#verify step " << step
                 << " split: " << m.freed.size()
                 << " crossingless component(s) came free"
                    " (colours";
            for( Int c : m.freed ) { out_ << " " << c; }
            out_ << ")\n";
        }

        // A `#spinoffs` header is the emitter's account of the same thing.
        // Neither side can hold a crossingless component beside crossings, so
        // both report rather than represent -- and the two reports have to
        // agree.
        if( m.afterQ ) { ReportSpinoffs(rec, m.freed, step); }

        // The applier's own result, compared port by port.
        if( m.afterQ && rec.result )
        {
            std::vector<std::array<Int,2>> seeds;
            std::string swhy;
            DiagramMatch_T<Int> M( dia.MaxCrossingCount(), dia.MaxArcCount(),
                                   dia.MaxCrossingCount(), dia.MaxArcCount() );

            bool okQ = BuildSurvivorSeeds<PD_T>(dia, m.mv, m.after,
                                                *rec.result, seeds, swhy);
            if( okQ )
            {
                okQ = DiagramsAgreeQ(m.after, *rec.result, seeds, swhy, &M);
            }

            ReportResult(dia, m.after, *rec.result, M, okQ, swhy, step);
        }

        return m;
    }

    /// What BeginR1 learned.
    struct R1Move
    {
        bool             parsedQ = false;
        bool             validQ  = false;   // the local checks passed
        KnoodleR1View::R1Resolved<PD_T> r;
        PD_T             after;
        std::vector<Int> freed;
        std::string      why;
    };

    /**
     * @brief A curl removal: local checks, the diagram it produces, and the
     * claims about that diagram.
     *
     * For `r1` the two conformance tiers coincide (docs/move-descriptor.md,
     * "Kinds for which well-formed implies sound"): there is no witness to
     * demand, so the local checks ARE the verdict, and `ResolveR1` is where
     * they live. It checks that the named darc is a loop arc whose crossing is
     * active and that the face to its left is a monogon -- which is what pins
     * down which side collapses.
     *
     * `R1AfterDiagram` then performs the surgery from the descriptor alone,
     * never calling `LoopRemover`, so it can serve as an oracle against an
     * applier -- Knoodle's own included.
     */
    R1Move BeginR1( const Record_T & rec, const PD_T & dia,
                    const std::string & spec, std::size_t step )
    {
        using R1_T = KnoodleR1View::R1Descriptor<Int>;

        R1Move m;

        R1_T desc;
        std::string perr;
        if( !R1_T::Parse(spec, desc, perr) )
        {
            m.why = perr;
            return m;
        }
        m.parsedQ = true;

        m.r      = KnoodleR1View::ResolveR1<PD_T>(dia, desc.loop);
        m.validQ = m.r.validQ;

        out_ << "#verify step " << step << " r1: ";
        if( !m.validQ )
        {
            out_ << "MISMATCH -- " << m.r.why << "\n";
            failedQ_ = true;
            return m;
        }
        out_ << "VERIFIED (loop arc " << m.r.a << " at crossing " << m.r.c
             << "; face " << m.r.monogon << " is the monogon"
             << (m.r.spinoffQ ? "; the component comes free" : "") << ")\n";

        m.after = KnoodleR1View::R1AfterDiagram<PD_T>(dia, m.r, m.why, m.freed);

        if( !m.why.empty() )
        {
            out_ << "#verify step " << step
                 << " result/trace: UNCHECKED (the surgery cannot build what"
                    " the move produces: " << m.why << ")\n";
            m.validQ = false;
            return m;
        }

        if( !m.freed.empty() )
        {
            out_ << "#verify step " << step
                 << " split: " << m.freed.size()
                 << " crossingless component(s) came free (colours";
            for( Int c : m.freed ) { out_ << " " << c; }
            out_ << ")\n";
        }

        ReportSpinoffs(rec, m.freed, step);

        if( rec.result )
        {
            std::vector<char> touchedQ(
                static_cast<std::size_t>(dia.MaxCrossingCount()), char(0));
            if( (m.r.c >= Int(0)) && (m.r.c < dia.MaxCrossingCount()) )
            {
                touchedQ[static_cast<std::size_t>(m.r.c)] = char(1);
            }

            std::vector<std::array<Int,2>> seeds;
            std::string swhy;
            DiagramMatch_T<Int> M( dia.MaxCrossingCount(), dia.MaxArcCount(),
                                   dia.MaxCrossingCount(), dia.MaxArcCount() );

            bool okQ = BuildSurvivorSeeds<PD_T>(dia, touchedQ, m.after,
                                                *rec.result, seeds, swhy);
            if( okQ )
            {
                okQ = DiagramsAgreeQ(m.after, *rec.result, seeds, swhy, &M);
            }

            ReportResult(dia, m.after, *rec.result, M, okQ, swhy, step);
        }

        return m;
    }

    /// Carry an r1's claim forward, as EndMove does for a pass.
    void EndR1( const Record_T & rec, R1Move & m, std::size_t step )
    {
        if( rec.candidateQ ) { return; }

        if( !m.validQ )
        {
            Gap("step " + std::to_string(step) + "'s curl removal is not"
                " established");
            return;
        }
        freed_colors_.insert(freed_colors_.end(), m.freed.begin(), m.freed.end());
        pending_after_   = std::move(m.after);
        pending_label_   = "step " + std::to_string(step);
        pending_colorsQ_ = !rec.state_from_pd;
        pending_redrawQ_ = false;
    }

    /**
     * @brief A re-projection: the full `redraw` contract of
     * docs/move-descriptor.md, checks 1-5.
     *
     *   redraw:     `R` is one of the two permitted rotations (equality).
     *   projection: `E`'s colours are exactly the snapshot's, and `project(E)`
     *               is isomorphic to the snapshot KEEPING every arc's colour
     *               (check 1 and the before half of check 4).
     *   spinoffs:   the components crossingless in `project(R*E)` are exactly
     *               the record's `#spinoffs colors=` (after half of check 4).
     *   rotated:    `project(R*E)` is connected once those are set aside
     *               (check 5).
     *   trace:      on the NEXT record, `project(R*E)` is isomorphic to its
     *               snapshot, colours kept (checks 2 and 4).
     *
     * Everything is exact: integer coordinates, a permutation matrix, and
     * `LinkEmbedding_Int` + `Prosector` to project. Colours are pinned by the
     * embedding, never read off an isomorphism (which may swap
     * interchangeable components).
     */
    void CheckRedraw( const Record_T & rec, const PD_T & dia,
                      const std::string & spec, std::size_t step )
    {
        using Redraw_T = KnoodleRedraw::RedrawDescriptor<Int>;

        Redraw_T desc;
        std::string err;

        auto fail = [&]( const char * what, const std::string & msg )
        {
            out_ << "#verify step " << step << " " << what << ": MISMATCH -- "
                 << msg << "\n";
            failedQ_ = true;
        };

        out_ << "#verify step " << step << " redraw: ";
        if( !Redraw_T::Parse(spec, desc, err) )
        {
            out_ << "MISMATCH -- " << err << "\n";
            failedQ_ = true;
            return;
        }
        out_ << "VERIFIED (rotation " << desc.CycleName() << ")\n";

        // The witness itself. The spec makes it part of the record: without
        // it a redraw is a bare assertion that the diagram changed.
        if( rec.embedding.empty() )
        {
            fail("projection", "the record carries no '#embedding' block, so"
                 " there is no witness to project");
            return;
        }

        // Check 4, before half: the embedding names exactly the snapshot's
        // components. (Distinct colours: the reader enforced that.)
        {
            std::vector<Int> ecol = rec.embedding_colors;
            std::sort(ecol.begin(), ecol.end());
            const std::vector<Int> dcol = DiagramColors(dia);
            if( ecol != dcol )
            {
                fail("projection", "the embedding's components are colours "
                     + ColorList(ecol) + " but the snapshot's are "
                     + ColorList(dcol));
                return;
            }
        }

        // Check 1.
        const auto before = ProjectLattice<PD_T>(
            rec.embedding, rec.embedding_ptr, rec.embedding_colors);
        if( !before.okQ )
        {
            fail("projection", "E: " + before.why);
            return;
        }
        if( !before.anelli.empty() )
        {
            fail("projection", "component(s) " + ColorList(before.anelli)
                 + " of E have no crossings in project(E); the snapshot"
                   " cannot hold them, so they should already have been spun"
                   " off");
            return;
        }
        {
            std::string iwhy;
            if( !DiagramsIsomorphicQ(before.pd, dia, iwhy, nullptr, true) )
            {
                fail("projection", "project(E) is not this record's snapshot"
                     " (colours kept): " + iwhy);
                return;
            }
        }
        out_ << "#verify step " << step << " projection: VERIFIED (E has "
             << rec.embedding_colors.size() << " component(s), "
             << rec.embedding.size() / 3 << " lattice vertices; project(E) is"
                " the snapshot, " << before.crossings << " crossings, colours"
                " kept)\n";

        // Check 4, after half: what the rotation frees must be declared, BY
        // COLOUR -- the bare count cannot say which component came free.
        const auto after = ProjectLattice<PD_T>(
            desc.Apply(rec.embedding), rec.embedding_ptr, rec.embedding_colors);
        if( !after.okQ )
        {
            fail("rotated", "R*E: " + after.why);
            return;
        }

        std::vector<Int> freed = after.anelli;
        std::sort(freed.begin(), freed.end());

        if( !freed.empty() || rec.spinoffs )
        {
            std::vector<Int> declared = rec.spinoff_colors;
            std::sort(declared.begin(), declared.end());

            if( rec.spinoffs && declared.empty() && (*rec.spinoffs > Int(0)) )
            {
                fail("spinoffs", "a redraw must name what it frees"
                     " ('#spinoffs colors=<list>'); the bare count cannot say"
                     " which component came free");
                return;
            }
            const bool sameQ = (declared == freed);
            out_ << "#verify step " << step << " spinoffs: "
                 << (sameQ ? "VERIFIED" : "MISMATCH")
                 << " (colours " << ColorList(declared) << " declared, "
                 << ColorList(freed) << " crossingless in project(R*E))\n";
            if( !sameQ ) { failedQ_ = true; return; }
        }

        // Check 5.
        if( (after.crossings > Int(0))
         && (after.pd.DiagramComponentCount() != Int(1)) )
        {
            fail("rotated", "project(R*E) falls apart into "
                 + std::to_string(after.pd.DiagramComponentCount())
                 + " diagram components; a split projection is refused until"
                   " the spec settles 'split' (check 5)");
            return;
        }

        out_ << "#verify step " << step << " rotated: VERIFIED (project(R*E)"
                " has " << after.crossings << " crossings"
             << ((after.crossings > Int(0)) ? ", connected" : "")
             << "; checked against the next snapshot)\n";

        // Check 2 is the ordinary trace claim, made colour-strict.
        if( !rec.candidateQ )
        {
            freed_colors_.insert(freed_colors_.end(), freed.begin(), freed.end());
            pending_after_   = (after.crossings > Int(0))
                             ? after.pd : PD_T::InvalidDiagram();
            pending_label_   = "step " + std::to_string(step);
            pending_colorsQ_ = true;
            pending_redrawQ_ = true;
        }
    }

    /**
     * @brief The feasibility witness, when the record carries one.
     *
     * V0 rebuilds the disk and the pieces on the witness's side from the
     * snapshot and the descriptor alone; V4 demands the classes be exactly the
     * same-strand unions at the disk's crossings; V2/V3/V5 check the labels.
     */
    void CheckWitness( const Record_T & rec, const PD_T & dia,
                       const Move & m, std::size_t step )
    {
        if( !m.parsedQ ) { return; }

        // A uniform pass is sound by Proposition C′; a middlepass only by its
        // witness, so for the whole-stream verdict an applied one needs all
        // three witness checks VERIFIED.
        const bool needQ = m.mv.middlepassQ && !rec.candidateQ;

        if( !rec.feas )
        {
            if( needQ )
            {
                Gap("step " + std::to_string(step) + " is a middlepass with no"
                    " '#feas' witness");
            }
            return;
        }

        const auto wr = KnoodleWitness::CheckWitness<PD_T>(dia, m.mv, *rec.feas);

        if( needQ && !(wr.v0_checkedQ && wr.v4_checkedQ && wr.labels_checkedQ) )
        {
            Gap("step " + std::to_string(step) + "'s middlepass witness is not"
                " fully checked");
        }

        out_ << "#verify step " << step << " disk (V0): ";
        if( !wr.v0_checkedQ )
        {
            out_ << "UNCHECKED (" << wr.v0_why << ")\n";
        }
        else if( wr.v0_okQ )
        {
            out_ << "VERIFIED (side " << rec.feas->side << ": "
                 << wr.disk_size << " interior crossings, "
                 << wr.piece_count << " pieces)\n";
        }
        else
        {
            out_ << "MISMATCH -- " << wr.v0_why << "\n";
            failedQ_ = true;
        }

        out_ << "#verify step " << step << " classes (V4): ";
        if( !wr.v4_checkedQ )
        {
            out_ << "UNCHECKED (" << wr.v4_why << ")\n";
        }
        else if( wr.v4_okQ )
        {
            out_ << "VERIFIED (" << wr.class_count
                 << " classes, exactly the same-strand unions)\n";
        }
        else
        {
            out_ << "MISMATCH -- " << wr.v4_why << "\n";
            failedQ_ = true;
        }

        out_ << "#verify step " << step << " labels (V2/V3/V5): ";
        if( !wr.labels_checkedQ )
        {
            out_ << "UNCHECKED (" << wr.labels_why << ")\n";
        }
        else if( wr.labels_okQ )
        {
            out_ << "VERIFIED (" << wr.germ_count << " germs, "
                 << wr.order_count << " orderings, "
                 << wr.tag_count << " tags)\n";
        }
        else
        {
            out_ << "MISMATCH -- " << wr.labels_why << "\n";
            failedQ_ = true;
        }
    }

    /**
     * @brief Carry this move's claim forward to the next record.
     *
     * A `#candidate` was evaluated and NOT applied, so the stream's diagram
     * does not advance across it and its claim must not be carried into the
     * next record's trace check.
     */
    void EndMove( const Record_T & rec, Move & m, std::size_t step )
    {
        if( rec.candidateQ ) { return; }

        if( !m.parsedQ )
        {
            Gap("step " + std::to_string(step) + "'s descriptor is not well formed");
        }
        else if( !m.afterQ )
        {
            Gap("step " + std::to_string(step) + ": AfterDiagram cannot build"
                " what the move produces");
        }
        else
        {
            freed_colors_.insert(freed_colors_.end(), m.freed.begin(), m.freed.end());
            pending_after_   = std::move(m.after);
            pending_label_   = "step " + std::to_string(step);
            pending_colorsQ_ = !rec.state_from_pd;
            pending_redrawQ_ = false;
        }
    }

    /// Nothing follows the last move, so its trace claim cannot be checked --
    /// unless the claim is that nothing is left. A crossingless diagram has no
    /// next record to carry it, so a stream that stops there has continued
    /// exactly as claimed.
    void Finish( const char * still_checked = "its drawing was still checked" )
    {
        if( !pending_after_ ) { return; }

        if( pending_after_->CrossingCount() == Int(0) )
        {
            out_ << "#verify " << pending_label_
                 << " trace: VERIFIED (0 crossings expected, and the stream"
                    " ends)\n";
            pending_after_.reset();
            pending_colorsQ_ = false;
            pending_redrawQ_ = false;
            emptyQ_ = true;
            return;
        }

        out_ << "#verify " << pending_label_
             << " trace: UNCHECKED (no following record to compare"
                " against; " << still_checked << ")\n";
        pending_after_.reset();
        pending_colorsQ_ = false;
        pending_redrawQ_ = false;
    }

    /// Mark a failure found by a check that lives outside this class.
    void Fail() { failedQ_ = true; }

    //==========================================================================
    // The whole stream (docs/move-descriptor.md, "The whole stream: unlink").
    //
    // A stream that ends at the EMPTY diagram claims something about its
    // input: it is an unlink, each component coming free exactly once. That
    // needs every applied move to be sound, every link from one record to the
    // next to be VERIFIED, and the freed colours to be exactly the input's.
    // The per-record checks establish the pieces; this ledger notes where one
    // is missing (a "gap") and adds up the colours.
    //==========================================================================

    /// Called at the start of every record, before BeginRecord (or
    /// BeginRecordWithoutDiagram). `dia` is null for a record with no diagram.
    void NoteRecord( const Record_T & rec, const PD_T * dia, std::size_t step )
    {
        for( const std::string & h : rec.headers )
        {
            if( h.rfind("#step", 0) != 0 ) { continue; }
            const auto s = h.find("summand=");
            if( s != std::string::npos )
            {
                summands_.insert(h.substr(s + 8, h.find_first_of(" \t", s) - (s + 8)));
            }
        }

        if( (dia != nullptr) && !input_colors_ ) { input_colors_ = DiagramColors(*dia); }

        // A snapshot built from bare PD rows numbers its components in parse
        // order, so from then on a colour is not a lifelong label.
        if( (dia != nullptr) && rec.state_from_pd ) { colors_realQ_ = false; }

        if( emptyQ_ && (dia != nullptr) )
        {
            Gap("step " + std::to_string(step) + " carries a diagram after the"
                " diagram went empty");
        }

        // A record with a diagram and no move is a state nothing leads out of.
        if( open_step_ )
        {
            Gap("step " + std::to_string(*open_step_) + " carries no move, so"
                " nothing connects it to step " + std::to_string(step));
            open_step_.reset();
        }

        // A #candidate does not advance the diagram: the next record must carry
        // the very snapshot it was evaluated on.
        if( candidate_state_ )
        {
            std::string iwhy;
            const bool sameQ = (dia != nullptr)
                && DiagramsIsomorphicQ(*candidate_state_, *dia, iwhy, nullptr, true);
            if( !sameQ )
            {
                out_ << "#verify step " << candidate_step_ << " trace: MISMATCH"
                        " (a #candidate does not advance the diagram, but step "
                     << step << " is not its snapshot)"
                     << (iwhy.empty() ? std::string() : " -- " + iwhy) << "\n";
                failedQ_ = true;
            }
            candidate_state_.reset();
        }

        if( dia != nullptr )
        {
            if( !rec.move )           { open_step_ = step; }
            else if( rec.candidateQ ) { candidate_state_ = *dia; candidate_step_ = step; }
        }
        if( rec.move && !rec.candidateQ ) { ++applied_; }
    }

    /// A load-bearing claim the per-record checks could not establish.
    void Gap( std::string why ) { gaps_.push_back(std::move(why)); }

    /// The `#verify stream unlink:` verdict, after Finish. Silent unless the
    /// stream ends at the empty diagram: any other stream makes no such claim.
    void ReportStream()
    {
        if( !emptyQ_ ) { return; }

        out_ << "#verify stream unlink: ";

        if( summands_.size() > 1 )
        {
            out_ << "UNCHECKED (the stream has " << summands_.size()
                 << " summands; the unlink verdict reads one-summand streams)\n";
            return;
        }
        if( failedQ_ )
        {
            out_ << "UNCHECKED (the chain is broken: a claim along it came back"
                    " MISMATCH)\n";
            return;
        }
        if( !gaps_.empty() )
        {
            out_ << "UNCHECKED (" << gaps_.front();
            if( gaps_.size() > 1 ) { out_ << "; and " << gaps_.size() - 1 << " more"; }
            out_ << ")\n";
            return;
        }

        std::vector<Int> freed = freed_colors_;
        std::sort(freed.begin(), freed.end());
        const auto twice = std::adjacent_find(freed.begin(), freed.end());
        const std::vector<Int> input = input_colors_ ? *input_colors_ : std::vector<Int>();

        // Without real colours only the count can balance.
        if( !colors_realQ_ )
        {
            if( freed.size() != input.size() )
            {
                out_ << "MISMATCH -- the input has " << input.size()
                     << " component(s) but " << freed.size() << " came free\n";
                failedQ_ = true;
                return;
            }
            out_ << "VERIFIED (the input's " << input.size() << " component"
                 << ((input.size() == 1) ? "" : "s") << " came free, counted:"
                    " PD-row snapshots carry no colours; " << applied_ << " move"
                 << ((applied_ == 1) ? "" : "s") << ", every one sound and every"
                    " link VERIFIED)\n";
            return;
        }

        // With colour-kept links every step of the way this balance cannot
        // fail on a stream whose links all VERIFIED; it stays as a backstop.

        if( twice != freed.end() )
        {
            out_ << "MISMATCH -- colour " << *twice << " came free twice\n";
            failedQ_ = true;
            return;
        }
        if( freed != input )
        {
            out_ << "MISMATCH -- the input's colours are " << ColorList(input)
                 << " but the colours that came free are " << ColorList(freed) << "\n";
            failedQ_ = true;
            return;
        }

        out_ << "VERIFIED (the input's " << input.size() << " component"
             << ((input.size() == 1) ? "" : "s") << ", colour"
             << ((input.size() == 1) ? " " : "s ") << ColorList(input)
             << ", each came free exactly once; " << applied_ << " move"
             << ((applied_ == 1) ? "" : "s") << ", every one sound and every"
                " link VERIFIED)\n";
    }

private:

    /*!@brief The `spinoffs:` verdict.
     *
     * `#spinoffs` comes in two spellings, `n=<count>` and `colors=<list>`, and
     * they are not equally strong. A count says how many components came free;
     * the list says WHICH, and on a link that is the part worth checking --
     * freeing the right number of components while naming the wrong one is
     * exactly the confusion colours exist to prevent. When the emitter gives
     * the list, compare the list.
     */
    void ReportSpinoffs( const Record_T & rec,
                         const std::vector<Int> & freed, std::size_t step )
    {
        if( !rec.spinoffs ) { return; }

        const Int mine = static_cast<Int>(freed.size());

        if( !rec.spinoff_colors.empty() )
        {
            std::vector<Int> theirs = rec.spinoff_colors;
            std::vector<Int> ours   = freed;
            std::sort(theirs.begin(), theirs.end());
            std::sort(ours.begin(),   ours.end());

            const bool sameQ = (theirs == ours);

            auto list = []( const std::vector<Int> & v ) -> std::string
            {
                std::string s;
                for( std::size_t i = 0; i < v.size(); ++i )
                {
                    if( i ) { s += ","; }
                    s += std::to_string(v[i]);
                }
                return s.empty() ? std::string("none") : s;
            };

            out_ << "#verify step " << step
                 << " spinoffs: " << (sameQ ? "VERIFIED" : "MISMATCH")
                 << " (colours " << list(theirs) << " reported, "
                 << list(ours) << " from the surgery)\n";

            if( !sameQ ) { failedQ_ = true; }
            return;
        }

        const bool sameQ = (*rec.spinoffs == mine);

        out_ << "#verify step " << step
             << " spinoffs: " << (sameQ ? "VERIFIED" : "MISMATCH")
             << " (" << *rec.spinoffs << " reported, "
             << mine << " from the surgery)\n";

        if( !sameQ ) { failedQ_ = true; }
    }

    /*!@brief The `colors:` verdict: do we and the applier agree about which
     * component each arc belongs to?
     *
     * Reported only when a `#result` was compared, because it is a claim about
     * the applier's output. `n_components` is named so that a reader can see
     * at a glance whether the check had anything to distinguish: on a knot
     * there is one colour and the verdict is free.
     */
    /// The `result:` verdict, and on success the `colors:` one. When both
    /// results are empty the colours have nowhere to live; where they went is
    /// the `spinoffs:` check's business.
    void ReportResult( const PD_T & before, const PD_T & ours,
                       const PD_T & theirs,
                       const DiagramMatch_T<Int> & M,
                       bool okQ, const std::string & why, std::size_t step )
    {
        const bool emptyQ = okQ && (ours.CrossingCount() == Int(0));

        out_ << "#verify step " << step << " result: ";
        if( !okQ )       { out_ << "MISMATCH -- " << why; }
        else if( emptyQ ) { out_ << "VERIFIED (both results are empty)"; }
        else             { out_ << "VERIFIED (port-by-port against the applier)"; }
        out_ << "\n";

        if( !okQ )       { failedQ_ = true; }
        else if( !emptyQ ) { ReportColors(before, ours, theirs, M, step); }
    }

    void ReportColors( const PD_T & before, const PD_T & ours,
                       const PD_T & theirs,
                       const DiagramMatch_T<Int> & M, std::size_t step )
    {
        std::string cwhy;
        const bool okQ = ColorsAgreeQ(ours, theirs, M, cwhy);

        std::set<Int> colors;
        for( Int a = 0; a < before.MaxArcCount(); ++a )
        {
            if( before.ArcActiveQ(a) ) { colors.insert(before.ArcColors()[a]); }
        }

        out_ << "#verify step " << step << " colors: "
             << (okQ ? "VERIFIED" : "MISMATCH");
        if( okQ )
        {
            out_ << " (" << colors.size() << " component colour"
                 << ((colors.size() == std::size_t(1)) ? "" : "s")
                 << " carried through the move)";
        }
        else
        {
            out_ << " -- " << cwhy;
        }
        out_ << "\n";

        if( !okQ ) { failedQ_ = true; }
    }

public:

private:

    std::ostream &      out_;
    bool                failedQ_ = false;
    std::optional<PD_T> pending_after_;
    std::string         pending_label_;
    bool                pending_colorsQ_ = false;  // the claim's colours are real
    bool                pending_redrawQ_ = false;  // ... pinned by a redraw's embedding

    // The whole-stream ledger (ReportStream).
    std::optional<std::vector<Int>> input_colors_;
    std::vector<Int>                freed_colors_;
    std::vector<std::string>        gaps_;
    std::set<std::string>           summands_;
    std::optional<std::size_t>      open_step_;       // a state with no move out
    std::optional<PD_T>             candidate_state_; // must be the next snapshot
    std::size_t                     candidate_step_ = 0;
    std::size_t                     applied_ = 0;
    bool                            emptyQ_ = false;  // the diagram went empty
    bool                            colors_realQ_ = true;  // no PD-row snapshot yet
};

} // namespace KnoodleTraceVerify
