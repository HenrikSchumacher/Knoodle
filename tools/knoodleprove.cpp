// knoodleprove -- check the picture-independent claims of a move-trace stream.
//
// A move trace (docs/move-descriptor.md) makes two kinds of claim. Some are
// about a PICTURE: "delete the corridor from this drawing and you have the
// diagram; delete the strand and you have what the move produces". Those are
// the renderer's, and `knoodledraw --trace --verify` checks them by drawing
// each view and parsing it back.
//
// The rest are about COMBINATORICS and need no layout at all: that a record's
// `#pd` annotation is its snapshot, that the emitter's `#result` is what the
// descriptor says the move produces, that each move's result is the next
// record's snapshot, that the `#spinoffs` count and the `#feas` witness
// (V0-V5) hold up. Those are this tool's. They are the claims an unknotting
// certificate rests on, and they should not need a drawing tool to check them,
// or pay for a layout per record: middlepass's diagrams run to 30,000
// crossings.
//
// The checks themselves live in tools/trace_verify.hpp, shared with
// knoodledraw, so the two tools print the same `#verify` lines for the same
// claims. What knoodleprove adds is honesty about coverage: a move it has no
// checker for is reported UNCHECKED rather than passed over in silence.
//
// Usage: knoodleprove [FILE]   (reads stdin when FILE is absent or "-")
// Exit:  0 if every claim checked came back VERIFIED; 1 on any MISMATCH, a
//        malformed stream, or a library error.

#include "../Knoodle.hpp"
#include "knoodle_io.hpp"
#include "../src/MoveTrace.hpp"
#include "trace_verify.hpp"

#include <fstream>
#include <iostream>
#include <string>

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

using Int     = std::int64_t;
using PD_T    = Knoodle::PlanarDiagram<Int>;
using Trace_T = Knoodle::MoveTrace<PD_T>;

namespace
{

void PrintUsage()
{
    std::cerr <<
        "Usage: knoodleprove [OPTIONS] [FILE]\n"
        "\n"
        "Checks the claims of a move-trace stream (docs/move-descriptor.md) that\n"
        "do not depend on a drawing. Reads FILE, or stdin if FILE is absent or '-'.\n"
        "Writes one '#verify' line per claim to stdout:\n"
        "\n"
        "  pd        the '#pd' annotation is the '#state' snapshot, up to relabelling\n"
        "  trace     what a move produces is the NEXT record's snapshot\n"
        "  split     crossingless components the move frees (reported)\n"
        "  spinoffs  the '#spinoffs' count agrees with the surgery\n"
        "  result    '#result' agrees port-by-port with the descriptor's surgery\n"
        "  V0/V4/V2-V5  the '#feas' feasibility witness\n"
        "\n"
        "Each is VERIFIED, MISMATCH or UNCHECKED. A move kind with no checker is\n"
        "reported UNCHECKED, never skipped silently. The drawing claims (the two\n"
        "deletions) belong to 'knoodledraw --trace --verify'.\n"
        "\n"
        "Exits 1 on any MISMATCH, a malformed stream, or a library error.\n"
        "\n"
        "Options:\n"
        "  -h, --help     Show this message\n"
        "  --version      Show the version\n";
}

/// The move kind a `#move` payload names; `pass` when it names none (the
/// grammar's default).
std::string MoveKind( const std::string & payload )
{
    const auto k = payload.find("kind=");
    if( k == std::string::npos ) { return "pass"; }
    const auto b = k + 5;
    const auto e = payload.find_first_of(" \t", b);
    return payload.substr(b, (e == std::string::npos) ? std::string::npos
                                                       : e - b);
}

bool Prove( std::istream & input, const char * source )
{
    typename Trace_T::Reader reader (input);
    KnoodleTraceVerify::TraceVerifier<PD_T> verifier(std::cout);

    typename Trace_T::Record rec;
    std::string why;
    std::size_t step = 0;

    for(;;)
    {
        const auto status = reader.Next(rec, why);

        if( status == Trace_T::Status::Eof ) { break; }

        if( status == Trace_T::Status::Error )
        {
            std::cerr << "knoodleprove: " << source << ": trace line "
                      << reader.LineNo() << ": " << why << "\n";
            return false;
        }

        // Numbered exactly as knoodledraw numbers them, so the two tools'
        // reports on one stream line up step for step.
        if( !rec.state ) { ++step; continue; }

        PD_T dia = std::move(*rec.state);

        if( dia.CrossingCount() <= Int(0) )
        {
            std::cerr << "knoodleprove: " << source << ": trace line "
                      << rec.line << ": snapshot did not parse into a valid"
                         " diagram\n";
            return false;
        }

        {
            std::string awhy;
            if( !KnoodleTraceVerify::AnnotationAgreesQ<PD_T>(rec, dia, awhy) )
            {
                std::cout << "#verify step " << step
                          << " pd: MISMATCH -- the '#pd' annotation and the"
                             " '#state' block describe different diagrams: "
                          << awhy << "\n";
                verifier.Fail();
            }
            else if( !rec.pd_rows.empty() && !rec.state_from_pd )
            {
                verifier.ReportAnnotation(step);
            }
        }

        verifier.BeginRecord(dia);

        if( rec.move )
        {
            const std::string kind = MoveKind(*rec.move);

            if( (kind == "pass") || (kind == "middlepass") )
            {
                auto m = verifier.BeginMove(rec, dia, *rec.move, step);

                if( !m.parsedQ )
                {
                    std::cout << "#verify step " << step
                              << " descriptor: MISMATCH -- " << m.parse_why
                              << "\n";
                    verifier.Fail();
                }

                verifier.CheckWitness(rec, dia, m, step);
                verifier.EndMove(rec, m, step);
            }
            else
            {
                // No checker for this kind yet (r1 is the next one due). Say
                // so: a certificate checker that passes over a step it cannot
                // check has not checked the certificate.
                std::cout << "#verify step " << step << " move: UNCHECKED"
                             " (no checker for kind=" << kind << " yet)\n";
            }
        }

        ++step;
    }

    verifier.Finish("nothing follows it in the stream");

    if( step == 0 )
    {
        std::cerr << "knoodleprove: warning: " << source
                  << ": trace stream contained no records\n";
    }

    return !verifier.FailedQ();
}

} // namespace

int main( int argc, char * argv[] )
{
    std::string path;

    for( int i = 1; i < argc; ++i )
    {
        const std::string arg = argv[i];

        if( (arg == "-h") || (arg == "--help") )
        {
            PrintUsage();
            return 0;
        }
        if( arg == "--version" )
        {
            std::cout << "knoodleprove " << GIT_VERSION << "\n";
            return 0;
        }
        if( (arg.size() > 1) && (arg[0] == '-') )
        {
            std::cerr << "knoodleprove: unknown option '" << arg << "'\n";
            PrintUsage();
            return 1;
        }
        if( !path.empty() )
        {
            std::cerr << "knoodleprove: one input file at most\n";
            return 1;
        }
        path = arg;
    }

    // Library errors print to std::cerr and are otherwise swallowed; a proof
    // checker that carried on past one would be vouching for a diagram the
    // library has already flagged as invalid.
    CerrErrorTap cerr_tap;

    bool okQ;
    if( path.empty() || (path == "-") )
    {
        okQ = Prove(std::cin, "<stdin>");
    }
    else
    {
        std::ifstream in(path);
        if( !in )
        {
            std::cerr << "knoodleprove: cannot open '" << path << "'\n";
            return 1;
        }
        okQ = Prove(in, path.c_str());
    }

    if( cerr_tap.Count() > 0 )
    {
        std::cerr << "knoodleprove: " << cerr_tap.Count()
                  << " library error(s) during this run -- the report above"
                     " is UNRELIABLE\n";
        return 1;
    }

    return okQ ? 0 : 1;
}
