#pragma once
// Staging implementation of the KLUT identify protocol (docs/klut-identify-design.md).
// Lives in test/ (ours) for now; to be ported into src/ by Henrik, where the
// std::vector<PD_T> worklist becomes the PDC-as-worklist using PDC::Pop() (his
// impl) to avoid the boundary copies marked "// copy (Pop() will move)" below.
//
// The protocol: decompose the input into prime candidates (pass-only Simplify,
// canonicalize OFF — the hot-path optimization, verified safe), look each up; if
// not found, escalate Reapr (which also splits any under-decomposed composite).
// Reapr is reused across calls. Lookups are reentrant (own buffer) so the routine
// is thread-safe given a per-thread Reapr.
#include "../Knoodle.hpp"
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace klut_identify {

using Int     = std::int64_t;
using Real    = double;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, float>;
using Klut    = Knoodle::Klut;
using Code    = Klut::CodeInt;
using Size_T  = Knoodle::Size_T;

struct IdentifyParams
{
    Size_T n0     = 1;    // initial embedding_trials for the Reapr escalation
    Size_T cap    = 2;    // max escalation rounds per candidate (tuned via klut_bench: a
                          // high cap only burns time on genuinely irreducible diagrams)
    Size_T rot    = 5;    // rotation_trials (reprojections) per embedding during escalation
                          // (tuned via klut_bench: ~all of rot=1's speed, 5x the margin)
    Int    max_cx = static_cast<Int>(Klut::max_crossing_count);  // 13

    // Experimental seed-Simplify knobs, kept for benchmarking (klut_bench's
    // --seed-local-opt / --seed-reroute). Defaults reproduce the tuned hot path
    // exactly, so leaving them here does NOT change Identify's behavior.
    //
    // Finding (2026-07, klut_bench): a local-only seed (seed_reroute=false,
    // seed_local_opt=4) beats the reroute seed only on trivially small diagrams
    // (~<=5 crossings: +15..32% at 3-4 crossings), and loses above that as local
    // moves stall and force Reapr escalation. Not worth a size-adaptive branch --
    // the reroute default (seed_local_opt=0, seed_reroute=true) stays.
    Size_T seed_local_opt = 0;  // local_opt_level for the seed: 0=off (default), 1=R1,
                                // 2=R1+R2, 4=all local patterns.
    bool   seed_reroute = true; // rerouteQ for the seed. false + seed_local_opt>0 = a
                                // local-only seed (no Dijkstra reroute).
};

struct Summand
{
    enum class Kind { Identified, Unidentified, Error };
    Kind             kind = Kind::Error;
    Klut::ID_T       id   = Klut::not_found;   // Identified: table id
    Int              crossings = 0;            // Unidentified / Error
    std::vector<Int> pd_code;                  // Unidentified / Error: flattened signed PD code
                                               // (5 cols/crossing) -- standard, lossless, for
                                               // offline analysis of what we could not identify
};

struct IdentifyResult
{
    enum class Status { Knot, LinkOutOfScope };
    Status               status = Status::Knot;
    std::vector<Summand> summands;        // status==Knot && empty  =>  the unknot
    bool                 component_error = false;  // Simplify changed component count (a bug)
    Size_T               reapr_calls = 0;          // escalation Simplify invocations (diagnostic)
};

namespace detail
{
    inline bool Found(Klut::ID_T id)
    { return id != Klut::not_found && id != Klut::error && id != Klut::invalid; }

    // Reentrant lookup: write D's (canonical-by-construction) MacLeod code into a
    // local buffer and probe the table. {crossings, id}; not_found unless D is a
    // single-component knot with 3..max_cx crossings whose code is a key.
    inline std::pair<Int, Klut::ID_T>
    Lookup(Klut& table, const PD_T& D, Int max_cx)
    {
        if( !D.ValidQ() || D.LinkComponentCount() != Int(1) ) { return {Int(0), Klut::not_found}; }
        const Int c = D.CrossingCount();
        if( c < Int(3) || c > max_cx ) { return {c, Klut::not_found}; }
        std::array<Code, Klut::max_crossing_count> buf{};
        D.template WriteMacLeodCode<Code>(buf.data());
        auto [cc, id] = table.FindID(buf.data(), c);
        (void)cc;
        return {c, id};
    }
}

// Reusing core: the caller owns the scratch PDCs `work` (which holds the input
// diagram(s) on entry) and `temp`, plus the result `R`; all three are reset here
// and their heap capacity is reused across calls. This is the firehose path --
// it amortizes the scratch/result allocations over many identifications. Diagrams
// move between `work` and the one-candidate scratch `temp` via Pop()/Push() (no
// copies); `temp` holds exactly one candidate at a time and is Simplified in
// place during escalation. On return `work` and `temp` are empty (ready to reuse).
inline void
IdentifyInto(Klut& table, PDC_T& work, PDC_T& temp, Reapr_T& reapr,
             IdentifyResult& R, IdentifyParams q = {})
{
    using detail::Found; using detail::Lookup;

    R.summands.clear();                            // keep the vector's capacity
    R.status          = IdentifyResult::Status::Knot;
    R.component_error = false;
    R.reapr_calls     = Size_T(0);
    temp.Clear();

    // ColorCount()==0 means work arrived with no diagrams at all (e.g. a bare
    // 's' unknot summand, which never reaches `work` in the first place --
    // see knoodleidentify.cpp's ReadKnot/unknot_colors split) -- there is
    // nothing to identify, but that is the unknot, not a link. Leave
    // R.status at its default (Knot) and R.summands empty so the caller's
    // own "no summands -> unknot" handling applies, instead of misreporting
    // it as LinkOutOfScope alongside genuine multi-component (>=2 colors)
    // input.
    if( work.ColorCount() == Int(0) ) { work.Clear(); return; }

    if( work.ColorCount() != Int(1) )              // a link (or multi-component) -> out of scope
    { R.status = IdentifyResult::Status::LinkOutOfScope; work.Clear(); return; }

    // Seed: pass-only decomposition, canonicalize OFF (hot path).
    {
        PDC_T::Simplify_Args_T a{};
        a.embedding_trials = Size_T(0);
        a.canonicalizeQ    = false;
        a.local_opt_level  = static_cast<Knoodle::UInt8>(q.seed_local_opt);  // default 0: no-op
        a.rerouteQ         = q.seed_reroute;                                 // default true: no-op
        work.Simplify(reapr, a);
        if( work.ColorCount() != Int(1) ) { R.component_error = true; }  // pass-reduce must preserve components
    }

    auto record_terminal = [&R, &q](const PD_T& D) {   // cap exhausted -> Unidentified / Error
        Summand s;
        s.crossings = D.CrossingCount();
        if( D.ValidQ() && D.LinkComponentCount() == Int(1) )
        {
            PD_T pd(D);   // PDCode is non-const
            auto code = pd.template PDCode<Int, {.signQ = true, .colorQ = false}>();
            const Int c = pd.CrossingCount();
            s.pd_code.reserve(static_cast<std::size_t>(5 * c));
            for( Int i = 0; i < c; ++i )
                for( int j = 0; j < 5; ++j ) { s.pd_code.push_back(code(i,j)); }
        }
        s.kind = (s.crossings > q.max_cx) ? Summand::Kind::Unidentified : Summand::Kind::Error;
        R.summands.push_back(std::move(s));
    };

    while( work.DiagramCount() > Int(0) )
    {
        temp.Push( work.Pop() );                   // MOVE one candidate into temp
        bool done = false;

        // (1) cheap path: look up the pass-reduced (un-canonicalized) candidate.
        {
            const PD_T& D = temp.Diagram(0);
            if( !D.ValidQ() || D.CrossingCount() == Int(0) ) { temp.Clear(); continue; }  // unknot/invalid -> drop
            auto [c, id] = Lookup(table, D, q.max_cx);
            if( Found(id) ) { R.summands.push_back(Summand{Summand::Kind::Identified, id, c, {}}); temp.Clear(); continue; }
        }

        // (2) escalate Reapr in place on temp (also splits an under-decomposed composite).
        Size_T n = q.n0;
        for( Size_T att = 0; att < q.cap && !done; ++att )
        {
            PDC_T::Simplify_Args_T a{};
            a.embedding_trials = n;                // escalation: canonicalize default (immaterial; Reapr swamps)
            a.rotation_trials  = q.rot;            // reprojections per embedding (tunable)
            ++R.reapr_calls;
            temp.Simplify(reapr, a);

            if( temp.ColorCount() != Int(1) )      // Simplify changed component count -> bug
            { R.component_error = true; done = true; break; }

            if( temp.DiagramCount() > Int(1) )     // composite revealed -> requeue the pieces (MOVE)
            { while( temp.DiagramCount() > Int(0) ) { work.Push( temp.Pop() ); } done = true; break; }

            const PD_T& D = temp.Diagram(0);
            if( D.CrossingCount() == Int(0) ) { done = true; break; }   // reduced away -> drop
            auto [c, id] = Lookup(table, D, q.max_cx);
            if( Found(id) ) { R.summands.push_back(Summand{Summand::Kind::Identified, id, c, {}}); done = true; break; }
            if( n < Size_T(64) ) { n *= Size_T(2); }  // escalate (schedule to be tuned via klut_bench)
        }
        if( done ) { temp.Clear(); continue; }

        // (3) cap exhausted.
        record_terminal( temp.Diagram(0) );
        temp.Clear();
    }
}

// Signature 1: caller supplies (and tunes) the Reapr. Owning convenience wrapper
// over IdentifyInto -- allocates fresh scratch per call. Use IdentifyInto in a
// firehose loop to reuse scratch across calls.
inline IdentifyResult
Identify(Klut& table, PDC_T P, Reapr_T& reapr, IdentifyParams q = {})
{
    IdentifyResult R;
    PDC_T work = std::move(P);
    PDC_T temp;
    IdentifyInto(table, work, temp, reapr, R, q);
    return R;
}

// Signature 2: build a Reapr with our tuned defaults (settled via klut_bench).
inline IdentifyResult
Identify(Klut& table, PDC_T P, IdentifyParams q = {})
{
    Reapr_T reapr{};   // TODO: plug in tuned Reapr settings once the klut_bench sweep is done
    return Identify(table, std::move(P), reapr, q);
}

} // namespace klut_identify
