// move_trace_check -- the move-trace record carrier, both versions.
//
// `src/MoveTrace.hpp` is a contract between two programs: a simplifier writes
// a trace, `knoodledraw --trace` reads it, and the two are usually not even
// the same executable. The failure this file exists to prevent is the one the
// middlepass shakedown actually hit -- two implementations of one grammar
// quietly disagreeing -- so the checks here are about the CARRIER, not about
// pass moves. What a record means is `orthodecorate_check`'s and
// `pass_view_check`'s business.
//
// The property everything else rests on is in section 1: a v1 `#state` block
// round-trips with the IDENTITY on every crossing. That is the whole reason
// v1 exists. A v0 record carries a PD code, which renumbers, so the strongest
// question a reader could ask about a v0 snapshot was "is it the same knot";
// with labels preserved, a disagreement can name a crossing and a port.
//
// The state bytes here are never hand-written -- they come from
// `MoveTrace::WriteStateBlock`, which is the same writer an emitter would
// call, and the line count in the header is computed from the very bytes it
// introduces.
//
// Build: `make move_trace_check` in tools/.

#include "../Knoodle.hpp"

#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include "../tools/diagram_agreement.hpp"

using Int     = std::int64_t;
using PD_T    = Knoodle::PlanarDiagram<Int>;
using Trace_T = Knoodle::MoveTrace<PD_T>;

static bool ok = true;

static void check( bool passedQ, const char * what )
{
    std::printf("  %-62s %s\n", what, passedQ ? "OK" : "FAILED");
    if( !passedQ ) { ok = false; }
}

static PD_T Trefoil()
{
    Int code[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
    return PD_T::FromSignedPDCode(&code[0],Int(3),false,false);
}

static PD_T FigureEight()
{
    return PD_T::FigureEightKnot(Int(0));
}

/*!@brief Read one record out of `text`. */
static Trace_T::Status ReadOne( const std::string &  text,
                                Trace_T::Record &    rec,
                                std::string &        why )
{
    std::istringstream in (text);
    Trace_T::Reader reader (in);
    return reader.Next(rec,why);
}

/*!@brief Every active crossing paired with itself. */
static std::vector<std::array<Int,2>> IdentitySeeds( const PD_T & pd )
{
    std::vector<std::array<Int,2>> seeds;
    for( Int c = 0; c < pd.MaxCrossingCount(); ++c )
    {
        if( pd.CrossingActiveQ(c) ) { seeds.push_back({c,c}); }
    }
    return seeds;
}

int main()
{
    // ---- 1. the carrier property: labels survive ------------------------
    {
        std::printf("=== a v1 #state block round-trips with the identity ===\n");

        PD_T pd = Trefoil();

        std::string block;
        check(Trace_T::WriteStateBlock(pd,"#state",block),
              "WriteStateBlock produced a block");

        // The header must count the bytes it introduces, not a constant.
        std::size_t lines = 0;
        for( std::size_t i = block.find('\n') + 1; i < block.size(); ++i )
        {
            lines += (block[i] == '\n');
        }
        check(block.starts_with("#state lines=" + std::to_string(lines) + "\n"),
              "the header's line count matches the block it introduces");

        const std::string text =
            "#trace v=1\n#knoodle version=test\n\n#step n=0 summand=0\n"
          + block + "\n";

        Trace_T::Record rec;
        std::string why;
        check(ReadOne(text,rec,why) == Trace_T::Status::Record,
              "the record reads back");
        check(rec.state.has_value(), "the record carries a state");

        if( rec.state )
        {
            std::string awhy;
            check(DiagramsAgreeQ(pd,*rec.state,IdentitySeeds(pd),awhy),
                  "every crossing came back at its own index");
            check(!rec.state_from_pd,
                  "the state came from #state, not from an annotation");
        }
    }

    // ---- 2. the record's parts ------------------------------------------
    {
        std::printf("=== the parts of a record ===\n");

        std::string state, result;
        Trace_T::WriteStateBlock(Trefoil(),    "#state", state);
        Trace_T::WriteStateBlock(FigureEight(),"#result",result);

        const std::string text =
            "#trace v=1\n"
            "#knoodle version=test\n"
            "\n"
            "#step n=7 summand=2\n"
            "#candidate\n"
            "#comment tier=cnp reason=feasible_side_not_taken\n"
            "#view exterior=4\n"
            "#move kind=middlepass strand=1,3 depart=1 cross=6:u land=3\n"
          + state + result + "\n";

        Trace_T::Record rec;
        std::string why;
        check(ReadOne(text,rec,why) == Trace_T::Status::Record,
              "the record reads");
        check(rec.candidateQ, "#candidate is surfaced");
        check(rec.exterior_da.has_value() && (*rec.exterior_da == Int(4)),
              "#view exterior=4 is surfaced");
        check(rec.move.has_value()
              && rec.move->starts_with("kind=middlepass strand=1,3"),
              "#move's payload is the header minus its token");
        check(rec.result.has_value()
              && (rec.result->CrossingCount() == Int(4)),
              "#result parses to the applier's diagram, not the snapshot's");
        check(rec.state.has_value() && (rec.state->CrossingCount() == Int(3)),
              "#state is still the before diagram");

        // Headers are echoed by the renderer as captions, so they must arrive
        // verbatim and in order. The two block headers are NOT among them:
        // "#state lines=12" says nothing about what the record means, and the
        // lines it counts must not leak into the captions either.
        check(rec.headers.size() == std::size_t(5),
              "the five caption headers were kept, and neither block header");
        check((rec.headers[0] == "#step n=7 summand=2")
              && (rec.headers[1] == "#candidate"),
              "headers arrive verbatim and in order");
        check(rec.line == std::size_t(4),
              "the record's start line is reported for diagnostics");
    }

    // ---- 3. #spinoffs, in both spellings --------------------------------
    {
        std::printf("=== #spinoffs: two spellings, one meaning ===\n");

        std::string state;
        Trace_T::WriteStateBlock(Trefoil(),"#state",state);

        for( const char * h : { "#spinoffs n=2\n", "#spinoffs colors=0,3\n" } )
        {
            const std::string text = "#trace v=1\n\n#step n=0\n"
                                   + std::string(h) + state + "\n";
            Trace_T::Record rec;
            std::string why;
            check(ReadOne(text,rec,why) == Trace_T::Status::Record
                  && rec.spinoffs.has_value() && (*rec.spinoffs == Int(2)),
                  h[10] == 'n' ? "#spinoffs n=2 gives a count of 2"
                               : "#spinoffs colors=0,3 gives a count of 2");
        }

        const std::string text = "#trace v=1\n\n#step n=0\n#spinoffs colors=0,3\n"
                               + state + "\n";
        Trace_T::Record rec;
        std::string why;
        ReadOne(text,rec,why);
        check(rec.spinoff_colors == std::vector<Int>{Int(0),Int(3)},
              "the colours themselves are kept");
    }

    // ---- 4. v0 still reads ----------------------------------------------
    {
        std::printf("=== v0: the PD-code carrier ===\n");

        const std::string text =
            "#trace v=0\n"
            "#step n=0 summand=0\n"
            "#move kind=pass strand=1,3 depart=1 cross=6:u land=3\n"
            "0\t4\t1\t3\t1\n"
            "2\t0\t3\t5\t1\n"
            "4\t2\t5\t1\t1\n"
            "\n";

        Trace_T::Record rec;
        std::string why;
        check(ReadOne(text,rec,why) == Trace_T::Status::Record,
              "a v0 record reads");
        check(rec.state.has_value() && (rec.state->CrossingCount() == Int(3)),
              "bare PD rows become the snapshot");
        check(rec.state_from_pd,
              "and the record says so, since the labels are then a PD code's");
        check(rec.pd_rows.size() == std::size_t(15),
              "the rows are kept as well");
    }

    // ---- 5. a v1 record may carry both carriers --------------------------
    {
        std::printf("=== #pd beside #state: an annotation, not a source ===\n");

        std::string state;
        Trace_T::WriteStateBlock(Trefoil(),"#state",state);

        const std::string text =
            "#trace v=1\n\n#step n=0\n" + state
          + "#pd rows=3\n0\t4\t1\t3\t1\n2\t0\t3\t5\t1\n4\t2\t5\t1\t1\n\n";

        Trace_T::Record rec;
        std::string why;
        check(ReadOne(text,rec,why) == Trace_T::Status::Record,
              "the record reads with both carriers present");
        check(!rec.state_from_pd,
              "the state came from #state; the annotation is not a source");
        check(rec.pd_rows.size() == std::size_t(15),
              "the annotation is handed on for the consumer to reconcile");

        // The reader does NOT compare them: a PD code renumbers on the way out,
        // so `PDCode(state)` need not be these bytes even when both are right,
        // and the comparison can only be made up to relabelling -- which is a
        // verifier's job, not a parser's. (This annotation is the trefoil's
        // input code; recomputing it from the state gives different labels.)
    }

    // ---- 6. two records, and the stream headers make no record ----------
    {
        std::printf("=== record separation ===\n");

        std::string a, b;
        Trace_T::WriteStateBlock(Trefoil(),    "#state",a);
        Trace_T::WriteStateBlock(FigureEight(),"#state",b);

        const std::string text = "#trace v=1\n#knoodle version=test\n\n\n"
                               + std::string("#step n=0\n") + a + "\n"
                               + "#step n=1\n" + b + "\n";

        std::istringstream in (text);
        Trace_T::Reader reader (in);

        Trace_T::Record rec;
        std::string why;
        int n = 0;
        std::vector<Int> counts;

        while( reader.Next(rec,why) == Trace_T::Status::Record )
        {
            ++n;
            counts.push_back(rec.state ? rec.state->CrossingCount() : Int(-1));
            if( n > 8 ) { break; }
        }

        check(n == 2, "two records, and the stream preamble is not one of them");
        check(counts == std::vector<Int>{Int(3),Int(4)},
              "they arrive in order with their own snapshots");
        check(reader.Version() == Int(1) && (reader.KnoodleVersion() == "test"),
              "the stream headers were captured");
    }

    // ---- 7. fail loud ---------------------------------------------------
    {
        std::printf("=== fail loud, never fall back ===\n");

        std::string state;
        Trace_T::WriteStateBlock(Trefoil(),"#state",state);

        struct Case { const char * what; std::string text; };

        const std::string pd_ann =
            "#pd rows=3\n0\t4\t1\t3\t1\n2\t0\t3\t5\t1\n4\t2\t5\t1\t1\n";

        std::vector<Case> cases = {
            { "a version this reader cannot parse is refused",
              "#trace v=2\n\n#step n=0\n" + state + "\n" },
            { "a lines= count past the end of the stream",
              "#trace v=1\n\n#step n=0\n"
                + std::string("#state lines=99\n") + state.substr(state.find('\n')+1) + "\n" },
            { "a #state block that is not internal state",
              "#trace v=1\n\n#step n=0\n#state lines=2\nnot a diagram\nat all\n\n" },
            { "an unparseable #state never falls back to a good #pd",
              "#trace v=1\n\n#step n=0\n#state lines=2\nnot a diagram\nat all\n"
                + pd_ann + "\n" },
            { "a v1 record with no snapshot at all",
              "#trace v=1\n\n#step n=0\n#comment nothing here\n\n" },
            { "two #state blocks in one record",
              "#trace v=1\n\n#step n=0\n" + state + state + "\n" },
            { "a PD row with the wrong number of columns",
              "#trace v=0\n#step n=0\n0\t4\t1\t3\n\n" },
        };

        for( const Case & c : cases )
        {
            Trace_T::Record rec;
            std::string why;
            const bool refusedQ = (ReadOne(c.text,rec,why) == Trace_T::Status::Error)
                               && !why.empty();
            check(refusedQ,c.what);
            if( refusedQ ) { std::printf("      %s\n",why.c_str()); }
        }
    }

    std::printf("\n%s\n", ok ? "MOVE TRACE CHECK OK" : "MOVE TRACE CHECK FAILED");
    return ok ? 0 : 1;
}
