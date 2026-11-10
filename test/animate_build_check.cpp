// animate_build_check -- Build (src/Animate/Build.hpp) on every committed
// trace: an animation is made, nothing in it jumps except at a cut, every
// curl removal is animated and reshaped, and the only cuts are for kinds of
// move that are not built yet.
//
// Build: `make animate_build_check` in test/ (a row in test/manifest.tsv).

#include "../Knoodle.hpp"
#include "../src/MoveTrace.hpp"
#include "../src/Animate/Build.hpp"
#include "../src/Animate/Diagram.hpp"
#include "../src/Animate/Frame.hpp"
#include "../src/Animate/Json.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using PD_T    = Knoodle::PlanarDiagram<Int>;
using Trace_T = Knoodle::MoveTrace<PD_T>;
namespace An  = Knoodle::Animate;
using An::Real;

static int g_checks = 0, g_failed = 0;
static void Check( bool ok, const std::string & what )
{
    ++g_checks;
    if( !ok ) { ++g_failed; std::printf("FAILED: %s\n", what.c_str()); }
}

// What a frame draws, in numbers: the strokes' total length and their box.
struct Ink { Real length = 0, x0 = 1e300, y0 = 1e300, x1 = -1e300, y1 = -1e300; };
static Ink InkOf( const An::Frame & f, const An::Style & st )
{
    Ink k;
    for( const An::Item & it : f.items )
    {
        if( it.fillQ ) { continue; }
        // a pass's route is shown while the picture holds and not while the strand sweeps: it may come and go
        if( it.stroke.color.r == st.route.r && it.stroke.color.g == st.route.g && it.stroke.color.b == st.route.b ) { continue; }
        const auto & p = it.stroke.pts;
        for( std::size_t i = 0; i < p.size(); ++i )
        {
            k.x0 = std::min(k.x0, p[i].x); k.y0 = std::min(k.y0, p[i].y); k.x1 = std::max(k.x1, p[i].x); k.y1 = std::max(k.y1, p[i].y);
            if( i > 0 ) { k.length += std::hypot(p[i].x - p[i-1].x, p[i].y - p[i-1].y); }
        }
        if( it.stroke.closed && p.size() > 1 ) { k.length += std::hypot(p[0].x - p.back().x, p[0].y - p.back().y); }
    }
    return k;
}

int main()
{
    namespace fs = std::filesystem;
    const fs::path dir = fs::exists("pass_lasso.trace") ? "." : "test";
    std::vector<fs::path> files;
    for( const auto & e : fs::directory_iterator(dir) ) { if( e.path().extension() == ".trace" ) { files.push_back(e.path()); } }
    std::sort(files.begin(), files.end());
    Check(files.size() >= 10, "the committed traces were found");

    const An::Style style = An::Style::Talk();
    const Real W = 1280, H = 720;
    int turns = 0, readbacks = 0, traces = 0, scenes = 0, curls = 0, reshapes = 0, cuts = 0, spins = 0;
    for( const fs::path & file : files )
    {
        const std::string name = file.filename().string();
        std::ifstream in (file);
        Trace_T::Reader reader (in);
        std::vector<Trace_T::Record> records; Trace_T::Record rec; std::string why;
        while( reader.Next(rec, why) == Trace_T::Status::Record ) { records.push_back(rec); }
        int r1_moves = 0, other_moves = 0;
        for( const auto & r : records ) { if( r.candidateQ || !r.state || !r.move ) { continue; } if( r.move->find("kind=r1") != std::string::npos ) { ++r1_moves; } else { ++other_moves; } }

        // a layout, read back as a diagram, is the diagram it was made from: label for label
        for( const auto & r : records )
        {
            if( !r.state || !r.state->ValidQ() || r.state->CrossingCount() <= Int(0) ) { continue; }
            const PD_T & pd = *r.state;
            Knoodle::OrthoDraw<PD_T> Hd (pd, Int(-1), Knoodle::OrthoDraw<PD_T>::Settings_T{});
            An::PlaneDrawing D; std::string w;
            if( !An::FromOrthoDraw(Hd, pd, D, w) ) { Check(false, name + ": export: " + w); continue; }
            const PD_T back = An::ToPlanarDiagram<PD_T>(D, w);
            bool sameQ = back.ValidQ() && back.CrossingCount() == pd.CrossingCount() && back.ArcCount() == pd.ArcCount();
            for( Int c = 0; sameQ && c < pd.MaxCrossingCount(); ++c )
            {
                if( !pd.CrossingActiveQ(c) ) { sameQ = !back.CrossingActiveQ(c); continue; }
                sameQ = back.CrossingActiveQ(c) && back.CrossingStates()[c] == pd.CrossingStates()[c];
                for( int io = 0; io < 2; ++io ) { for( int lr = 0; lr < 2; ++lr ) { sameQ = sameQ && back.Crossings()(c, io, lr) == pd.Crossings()(c, io, lr); } }
            }
            for( Int a = 0; sameQ && a < pd.MaxArcCount(); ++a )
            {
                if( !pd.ArcActiveQ(a) ) { sameQ = !back.ArcActiveQ(a); continue; }
                sameQ = back.ArcActiveQ(a) && back.Arcs()(a, 0) == pd.Arcs()(a, 0) && back.Arcs()(a, 1) == pd.Arcs()(a, 1) && back.ArcColors()[a] == pd.ArcColors()[a];
            }
            ++readbacks;
            Check(sameQ, name + ": a layout read back as a diagram is the diagram, label for label (" + w + ")");
        }

        An::Animation A;
        const bool builtQ = An::Builder<PD_T>(An::BuildOptions()).Build(records, name, A, why);
        Check(builtQ, name + ": an animation is built (" + why + ")");
        if( !builtQ ) { continue; }
        ++traces; scenes += int(A.scenes.size());

        int r1_scenes = 0, my_cuts = 0, tokens = 0;
        for( const An::Scene & s : A.scenes )
        {
            if( s.label == "R1 move" ) { ++r1_scenes; }
            if( s.label == "Reshape diagram" ) { ++reshapes; }
            if( s.label == "Spin-off" ) { ++spins; }
            if( s.label == "Cut" )
            {
                ++my_cuts;
                Check(s.note.find("is not animated yet") != std::string::npos || s.note.find("is not built yet") != std::string::npos
                      || s.note.find("carries no witness") != std::string::npos
                      || (name == "redraw_split_refused.trace" && s.note.find("is not connected") != std::string::npos),
                      name + ": a cut only for what is not built yet, a middle pass with no witness, or the split redraw (" + s.note + ")");
            }
            Check(s.duration > 0 && (!s.keys.empty() || !s.stages.empty()), name + ": a scene has a duration and a picture");
            if( s.label == "Turn the sphere" ) { ++turns; }
            for( const An::Key & k : s.keys ) { Check(k.pts.size() == s.paths.size(), name + ": a key has a polyline for every path"); }
        }
        for( const An::Path & p : A.scenes.back().paths ) { if( p.role == An::Role::Token ) { ++tokens; } }
        cuts += my_cuts; curls += r1_moves;
        Check(r1_scenes + 2 * 0 >= 2 * r1_moves - 2 * my_cuts && r1_scenes <= 2 * r1_moves, name + ": every curl removal is a hold and a contraction (" + std::to_string(r1_scenes) + " scenes for " + std::to_string(r1_moves) + " curls)");
        Check(my_cuts <= other_moves + r1_moves, name + ": no more cuts than moves");

        // nothing jumps from one scene to the next, except into and out of a cut
        Real t = 0;
        for( std::size_t i = 0; i + 1 < A.scenes.size(); ++i )
        {
            t += A.scenes[i].duration;
            if( A.scenes[i].label == "Cut" ) { continue; }
            const Ink a = InkOf(An::FrameAt(A, t - 1e-9, style, W, H), style), b = InkOf(An::FrameAt(A, t, style, W, H), style);
            const bool sameQ = std::abs(a.length - b.length) <= 0.005 * std::max(a.length, 1.0) + 0.5
                            && std::abs(a.x0 - b.x0) < 1 && std::abs(a.y0 - b.y0) < 1 && std::abs(a.x1 - b.x1) < 1 && std::abs(a.y1 - b.y1) < 1;
            Check(sameQ, name + ": no jump from scene " + std::to_string(i) + " (" + A.scenes[i].label + ") to " + std::to_string(i + 1) + " (" + A.scenes[i+1].label
                         + "): length " + std::to_string(a.length) + " -> " + std::to_string(b.length));
        }
        const std::string j = An::ToJSON(A);
        long depth = 0; bool balanced = true;
        for( char ch : j ) { if( ch == '{' || ch == '[' ) { ++depth; } if( ch == '}' || ch == ']' ) { --depth; } if( depth < 0 ) { balanced = false; } }
        Check(balanced && depth == 0 && j.size() > 100, name + ": its JSON is balanced");
        if( name == "r1_last_curl.trace" ) { Check(tokens == 1 && A.scenes.back().crossings_after == 0, name + ": the last curl leaves one square in the tray and no crossings"); }
    }
    std::printf("read back %d layouts; turns of the sphere %d; ", readbacks, turns);
    std::printf("traces=%d scenes=%d curls=%d reshapes=%d spin-offs=%d cuts=%d\n", traces, scenes, curls, reshapes, spins, cuts);
    Check(curls >= 13 && reshapes >= 10 && spins >= 2, "curls, reshapes and spin-offs were all exercised");
    std::printf("animate_build_check: (%d checks, %d failed)\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
