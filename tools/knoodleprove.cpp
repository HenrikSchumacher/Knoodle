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
#include "exterior_thread.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

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
        "  r1        a curl removal's local checks (for r1 these ARE soundness)\n"
        "  redraw    the rotation is one of the two permitted lattice rotations\n"
        "  projection  a redraw's lattice curve E projects (exactly) to this\n"
        "            snapshot, colours kept; 'rotated' says what R*E projects to\n"
        "            (connected, its freed components named by '#spinoffs'),\n"
        "            and 'trace' compares that to the next snapshot\n"
        "  trace     what a move produces is the NEXT record's snapshot\n"
        "  split     crossingless components the move frees (reported)\n"
        "  spinoffs  the '#spinoffs' count agrees with the surgery\n"
        "  result    '#result' agrees port-by-port with the descriptor's surgery\n"
        "  colors    ... and agrees about which COMPONENT each arc belongs to\n"
        "  V0/V4/V2-V5  the '#feas' feasibility witness\n"
        "  unlink    (one line for the whole stream, only when it ends at the\n"
        "            empty diagram) the input is an unlink: every applied move\n"
        "            sound, every link between records VERIFIED, and each input\n"
        "            colour freed exactly once\n"
        "\n"
        "Each is VERIFIED, MISMATCH or UNCHECKED. Move kinds checked: pass,\n"
        "middlepass, r1. A move kind with no checker is\n"
        "reported UNCHECKED, never skipped silently. The drawing claims (the two\n"
        "deletions) belong to 'knoodledraw --trace --verify'.\n"
        "\n"
        "Exits 1 on any MISMATCH, a malformed stream, or a library error.\n"
        "\n"
        "Exterior faces (docs/move-descriptor.md, \"Exterior faces across a trace\"):\n"
        "  --check-exterior   also check the '#view' claims: 'exterior' says each\n"
        "                 record's exterior face is the image of the previous one's\n"
        "                 across its move, with outside=/behind as the rule requires\n"
        "  --thread-exterior  do not check; REWRITE the stream to stdout with one\n"
        "                 exterior thread chosen through it (fewest 'behind' moves,\n"
        "                 then largest exteriors), replacing every '#view' line.\n"
        "                 Seams and records left without a view are noted on stderr.\n"
        "                 The new views are self-checked as --check-exterior would;\n"
        "                 if any fails, nothing is written and the exit status is 1.\n"
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

bool Prove( std::istream & input, const char * source, bool check_exteriorQ )
{
    // Kept only for --check-exterior, which needs neighbouring records.
    std::vector<typename Trace_T::Record> kept;
    std::vector<std::size_t>              kept_steps;

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
        // reports on one stream line up step for step. A record with no
        // diagram is a crossingless summand -- still a state, so a pending
        // claim is answered against it rather than carried over it.
        if( check_exteriorQ )
        {
            kept.push_back(rec);
            kept_steps.push_back(step);
        }

        if( !rec.state )
        {
            verifier.NoteRecord(rec, nullptr, step);
            verifier.BeginRecordWithoutDiagram();
            ++step;
            continue;
        }

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

        verifier.NoteRecord(rec, &dia, step);
        verifier.BeginRecord(dia, !rec.state_from_pd);

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
            else if( kind == "r1" )
            {
                auto m = verifier.BeginR1(rec, dia, *rec.move, step);

                if( !m.parsedQ )
                {
                    std::cout << "#verify step " << step
                              << " descriptor: MISMATCH -- " << m.why << "\n";
                    verifier.Fail();
                }

                verifier.EndR1(rec, m, step);
            }
            else if( kind == "redraw" )
            {
                verifier.CheckRedraw(rec, dia, *rec.move, step);
            }
            else
            {
                // No checker for this kind yet. Say so: a certificate checker
                // that passes over a step it cannot check has not checked the
                // certificate.
                std::cout << "#verify step " << step << " move: UNCHECKED"
                             " (no checker for kind=" << kind << " yet)\n";
                if( !rec.candidateQ )
                {
                    verifier.Gap("step " + std::to_string(step) + " is a kind="
                                 + kind + " move, which nothing checks");
                }
            }
        }

        ++step;
    }

    verifier.Finish("nothing follows it in the stream");
    verifier.ReportStream();

    if( check_exteriorQ
        && !KnoodleExterior::CheckExteriorThread<PD_T>(kept, kept_steps, std::cout) )
    {
        verifier.Fail();
    }

    if( step == 0 )
    {
        std::cerr << "knoodleprove: warning: " << source
                  << ": trace stream contained no records\n";
    }

    return !verifier.FailedQ();
}

/// --thread-exterior: the stream again, byte for byte, except that every
/// record's `#view` line is replaced by the threader's (or dropped where it
/// has none). The new line goes where the grammar puts `#view`: before the
/// record's first `#move`, `#faces`, `#feas`, `#state` or `#pd` line, or its
/// first data row (a v0 record has no `#state` line).
bool Thread( std::istream & input, const char * source )
{
    std::stringstream buf;
    buf << input.rdbuf();
    const std::string text = buf.str();

    std::vector<std::string> lines;
    {
        std::istringstream ls(text);
        std::string l;
        while( std::getline(ls, l) ) { lines.push_back(l); }
    }

    std::vector<typename Trace_T::Record> recs;
    {
        std::istringstream in(text);
        typename Trace_T::Reader reader (in);
        typename Trace_T::Record rec;
        std::string why;
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
            recs.push_back(rec);
        }
    }

    auto views = KnoodleExterior::ThreadExterior<PD_T>(recs);

    // TEST HOOK, for test/exterior_thread_check.py only: corrupt one view
    // after threading, so the self-check below can be seen to refuse.
    // KNOODLEPROVE_TEST_CORRUPT_VIEW=<record index>:outside flips that
    // record's outside=; <record index>:behind toggles its `behind`.
    if( const char * spec = std::getenv("KNOODLEPROVE_TEST_CORRUPT_VIEW") )
    {
        const std::string sp (spec);
        const auto colon = sp.find(':');
        std::size_t i = 0;
        bool okQ = (colon != std::string::npos) && (colon > 0);
        try { if( okQ ) { i = std::stoul(sp.substr(0, colon)); } } catch( ... ) { okQ = false; }
        const std::string how = okQ ? sp.substr(colon + 1) : std::string();
        okQ = okQ && (i < views.size()) && views[i].view
            && ((how == "behind") || ((how == "outside") && (views[i].view->outside >= 0)));
        if( !okQ )
        {
            std::cerr << "knoodleprove: KNOODLEPROVE_TEST_CORRUPT_VIEW='" << sp
                      << "' names no view it can corrupt\n";
            return false;
        }
        auto & v = *views[i].view;
        if( how == "outside" ) { v.outside = 1 - v.outside; }
        else                   { v.behindQ = !v.behindQ; }
    }

    // Self-check: the threader's choices must pass the checker that
    // --check-exterior runs, or nothing is emitted. A false claim here (say
    // outside= naming a side the exterior does not lie on) is a bug in the
    // threader, and a stream carrying it would make the renderer refuse.
    {
        auto checked = recs;
        std::vector<std::size_t> steps (recs.size());
        for( std::size_t i = 0; i < checked.size(); ++i )
        {
            steps[i] = i;
            auto & h = checked[i].headers;
            h.erase(std::remove_if(h.begin(), h.end(),
                        []( const std::string & x ) { return x.rfind("#view ", 0) == 0; }),
                    h.end());
            if( views[i].view ) { h.push_back(KnoodleExterior::FormatView(*views[i].view)); }
        }

        std::ostringstream report;
        if( !KnoodleExterior::CheckExteriorThread<PD_T>(checked, steps, report) )
        {
            std::cerr << "knoodleprove: " << source << ": internal error: the"
                         " exterior thread fails its own check; nothing"
                         " emitted:\n";
            std::istringstream rs(report.str());
            for( std::string l; std::getline(rs, l); )
            {
                if( l.find("MISMATCH") != std::string::npos ) { std::cerr << "  " << l << "\n"; }
            }
            return false;
        }
    }

    // Record i spans lines [recs[i].line, recs[i+1].line), 1-based.
    std::size_t next_rec = 0;
    std::size_t behind = 0, seams = 0;
    std::size_t emitted_for = std::size_t(-1);
    for( std::size_t ln = 1; ln <= lines.size(); ++ln )
    {
        const std::string & l = lines[ln-1];
        if( (next_rec < recs.size()) && (ln == recs[next_rec].line) ) { ++next_rec; }
        const std::size_t r = (next_rec > 0) ? next_rec - 1 : std::size_t(-1);

        if( l.rfind("#view ", 0) == 0 ) { continue; }

        if( r != std::size_t(-1) && views[r].view )
        {
            const bool slotQ = (l.rfind("#move ", 0) == 0) || (l.rfind("#faces", 0) == 0)
                || (l.rfind("#feas", 0) == 0) || (l.rfind("#state", 0) == 0)
                || (l.rfind("#pd", 0) == 0)
                || (!l.empty() && (l[0] != '#'));   // a v0 record's bare PD rows
            // Emit once per record, at the first slot line.
            if( slotQ && (emitted_for != r) )
            {
                std::cout << KnoodleExterior::FormatView(*views[r].view) << "\n";
                emitted_for = r;
                if( views[r].view->behindQ ) { ++behind; }
                if( views[r].view->seamQ   ) { ++seams; }
            }
        }
        std::cout << l << "\n";
    }

    for( std::size_t i = 0; i < recs.size(); ++i )
    {
        if( !views[i].note.empty() )
        {
            std::cerr << "knoodleprove: record at line " << recs[i].line << ": "
                      << views[i].note << "\n";
        }
    }
    std::cerr << "knoodleprove: threaded " << recs.size() << " records; "
              << behind << " behind, " << seams << " seam(s)\n";
    return true;
}

} // namespace

int main( int argc, char * argv[] )
{
    std::string path;
    bool check_exteriorQ  = false;
    bool thread_exteriorQ = false;

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
        if( arg == "--check-exterior"  ) { check_exteriorQ  = true; continue; }
        if( arg == "--thread-exterior" ) { thread_exteriorQ = true; continue; }
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
        okQ = thread_exteriorQ ? Thread(std::cin, "<stdin>")
                               : Prove(std::cin, "<stdin>", check_exteriorQ);
    }
    else
    {
        std::ifstream in(path);
        if( !in )
        {
            std::cerr << "knoodleprove: cannot open '" << path << "'\n";
            return 1;
        }
        okQ = thread_exteriorQ ? Thread(in, path.c_str())
                               : Prove(in, path.c_str(), check_exteriorQ);
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
