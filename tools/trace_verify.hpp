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
 *   BeginRecord  -> trace (against the previous move's claim)
 *   BeginMove    -> after-diagram, split, spinoffs, result
 *   [the caller's drawing check, if any]
 *   CheckWitness -> V0, V4, V2/V3/V5
 *   EndMove      -> carry this move's claim to the next record
 *   Finish       -> the last move's trace claim goes UNCHECKED
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
#include "diagram_agreement.hpp"
#include "witness_check.hpp"

#include <array>
#include <optional>
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

    if (seeds.empty())
    {
        why = "the move touches every crossing, so there is no untouched"
              " crossing to seed the correspondence with";
        return false;
    }
    return true;
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
        bool             parsedQ = false;  // a `pass` descriptor at all
        std::string      parse_why;        // the parser's refusal, if not
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
    void BeginRecord( const PD_T & dia )
    {
        if( !pending_after_ ) { return; }

        PD_T expected = std::move(*pending_after_);
        pending_after_.reset();

        std::string vwhy;
        const bool okQ = DiagramsIsomorphicQ(expected, dia, vwhy);

        out_ << "#verify " << pending_label_ << " trace: "
             << (okQ ? "VERIFIED" : "MISMATCH")
             << " (" << expected.CrossingCount() << " crossings expected, "
             << dia.CrossingCount() << " found)";
        if( !okQ ) { out_ << " -- " << vwhy; }
        out_ << "\n";

        if( !okQ ) { failedQ_ = true; }
    }

    /**
     * @brief Build what the move produces and check the claims about it.
     *
     * `AfterDiagram` works from the descriptor alone, never calling the
     * applier, so everything here is independent of whatever produced the
     * trace. A descriptor that does not parse as a `pass` move returns with
     * `parsedQ` false and nothing reported: that is the renderer's to refuse.
     */
    Move BeginMove( const Record_T & rec, const PD_T & dia,
                    const std::string & spec, std::size_t step )
    {
        Move m;

        if( !Move_T::Parse(spec, m.mv, m.parse_why) ) { return m; }
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
        if( m.afterQ && rec.spinoffs )
        {
            const Int mine   = static_cast<Int>(m.freed.size());
            const bool sameQ = (*rec.spinoffs == mine);

            out_ << "#verify step " << step
                 << " spinoffs: " << (sameQ ? "VERIFIED" : "MISMATCH")
                 << " (" << *rec.spinoffs << " reported, "
                 << mine << " from the surgery)\n";

            if( !sameQ ) { failedQ_ = true; }
        }

        // The applier's own result, compared port by port.
        if( m.afterQ && rec.result )
        {
            std::vector<std::array<Int,2>> seeds;
            std::string swhy;

            bool okQ = BuildSurvivorSeeds<PD_T>(dia, m.mv, m.after,
                                                *rec.result, seeds, swhy);
            if( okQ )
            {
                okQ = DiagramsAgreeQ(m.after, *rec.result, seeds, swhy);
            }

            out_ << "#verify step " << step
                 << " result: "
                 << (okQ ? "VERIFIED (port-by-port against the applier)"
                         : "MISMATCH");
            if( !okQ ) { out_ << " -- " << swhy; }
            out_ << "\n";

            if( !okQ ) { failedQ_ = true; }
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

        if( rec.spinoffs )
        {
            const Int mine   = static_cast<Int>(m.freed.size());
            const bool sameQ = (*rec.spinoffs == mine);

            out_ << "#verify step " << step
                 << " spinoffs: " << (sameQ ? "VERIFIED" : "MISMATCH")
                 << " (" << *rec.spinoffs << " reported, "
                 << mine << " from the surgery)\n";

            if( !sameQ ) { failedQ_ = true; }
        }

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

            bool okQ = BuildSurvivorSeeds<PD_T>(dia, touchedQ, m.after,
                                                *rec.result, seeds, swhy);
            if( okQ )
            {
                okQ = DiagramsAgreeQ(m.after, *rec.result, seeds, swhy);
            }

            out_ << "#verify step " << step << " result: "
                 << (okQ ? "VERIFIED (port-by-port against the applier)"
                         : "MISMATCH");
            if( !okQ ) { out_ << " -- " << swhy; }
            out_ << "\n";

            if( !okQ ) { failedQ_ = true; }
        }

        return m;
    }

    /// Carry an r1's claim forward, as EndMove does for a pass.
    void EndR1( const Record_T & rec, R1Move & m, std::size_t step )
    {
        if( m.validQ && !rec.candidateQ )
        {
            pending_after_ = std::move(m.after);
            pending_label_ = "step " + std::to_string(step);
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
        if( !m.parsedQ || !rec.feas ) { return; }

        const auto wr = KnoodleWitness::CheckWitness<PD_T>(dia, m.mv, *rec.feas);

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
        if( m.afterQ && !rec.candidateQ )
        {
            pending_after_ = std::move(m.after);
            pending_label_ = "step " + std::to_string(step);
        }
    }

    /// Nothing follows the last move, so its trace claim cannot be checked.
    void Finish( const char * still_checked = "its drawing was still checked" )
    {
        if( !pending_after_ ) { return; }

        out_ << "#verify " << pending_label_
             << " trace: UNCHECKED (no following record to compare"
                " against; " << still_checked << ")\n";
        pending_after_.reset();
    }

    /// Mark a failure found by a check that lives outside this class.
    void Fail() { failedQ_ = true; }

private:

    std::ostream &      out_;
    bool                failedQ_ = false;
    std::optional<PD_T> pending_after_;
    std::string         pending_label_;
};

} // namespace KnoodleTraceVerify
