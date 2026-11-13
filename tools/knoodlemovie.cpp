// knoodlemovie -- a move trace as a movie.
//
// Reads a trace (the stream knoodleprove checks), builds its animation with
// src/Animate/, and writes it in one of three forms:
//
//   knoodlemovie TRACE --json=FILE         the animation, for the web player (players/)
//   knoodlemovie TRACE --frame=SECONDS     one frame as a PPM picture on standard output
//   knoodlemovie TRACE                     every frame as raw rgb24 on standard output:
//
//       knoodlemovie TRACE --size=1920x1080 --fps=30 \
//         | ffmpeg -f rawvideo -pix_fmt rgb24 -s 1920x1080 -r 30 -i - -pix_fmt yuv420p out.mp4
//
// Options: --size=WxH (1280x720), --fps=N (30), --style=talk|light|dark (talk).
// A summary goes to standard error. Movies carry no caption text.

#include "../Knoodle.hpp"
#include "../src/MoveTrace.hpp"
#include "../src/Animate/Build.hpp"
#include "../src/Animate/Frame.hpp"
#include "../src/Animate/Json.hpp"
#include "../src/Animate/Raster.hpp"
#include "knoodle_io.hpp"   // VersionLine, VersionText

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using PD_T    = Knoodle::PlanarDiagram<Int>;
using Trace_T = Knoodle::MoveTrace<PD_T>;
namespace An  = Knoodle::Animate;

static int Fail( const std::string & why )
{
    std::fprintf(stderr, "knoodlemovie: %s\n", why.c_str());
    return 1;
}

int main( int argc, char ** argv )
{
    std::string path, json, style_name = "talk";
    int W = 1280, H = 720, fps = 30;
    double frame_at = -1;
    for( int i = 1; i < argc; ++i )
    {
        const std::string a = argv[i];
        auto val = [&]( const char * key ) { const std::string k = key; return (a.rfind(k, 0) == 0) ? a.substr(k.size()) : std::string(); };
        try
        {
            if( a == "--help" || a == "-h" )
            {
                std::printf("%s\n\n", VersionLine("knoodlemovie").c_str());
                std::printf("Usage: knoodlemovie TRACE [--json=FILE | --frame=SECONDS] [--size=WxH] [--fps=N] [--style=talk|light|dark]\n\n"
                            "Reads a move trace (the stream knoodleprove checks) and writes its animation:\n"
                            "  --json=FILE       the animation, for the web player (players/knoodle-player.html)\n"
                            "  --frame=SECONDS   one frame, as a PPM picture, on standard output\n"
                            "  (neither)         every frame as raw rgb24 on standard output, for ffmpeg:\n"
                            "      knoodlemovie TRACE --size=1280x720 --fps=30 | ffmpeg -f rawvideo -pix_fmt rgb24 \\\n"
                            "        -s 1280x720 -r 30 -i - -pix_fmt yuv420p out.mp4\n\n"
                            "  --size=WxH        the picture's size (1280x720)\n"
                            "  --fps=N           frames a second (30)\n"
                            "  --style=NAME      talk, light or dark (talk)\n"
                            "  -h, --help        show this message\n"
                            "  --version         show the version and exit\n\n"
                            "A summary goes to standard error. examples/traces has traces to try.\n");
                return 0;
            }
            else if( a == "--version" )         { std::fputs(VersionText("knoodlemovie").c_str(), stdout); return 0; }
            else if( !val("--json=").empty() )  { json = val("--json="); }
            else if( !val("--size=").empty() )  { const std::string v = val("--size="); const auto x = v.find('x'); W = std::stoi(v.substr(0, x)); H = std::stoi(v.substr(x + 1)); }
            else if( !val("--fps=").empty() )   { fps = std::stoi(val("--fps=")); }
            else if( !val("--style=").empty() ) { style_name = val("--style="); }
            else if( !val("--frame=").empty() ) { frame_at = std::stod(val("--frame=")); }
            else if( a.rfind("--", 0) == 0 )    { return Fail("unknown option " + a); }
            else if( path.empty() )             { path = a; }
            else                                { return Fail("more than one trace named"); }
        }
        catch( const std::exception & ) { return Fail("bad value in " + a); }
    }
    if( path.empty() ) { return Fail("usage: knoodlemovie TRACE [--json=FILE | --frame=SECONDS] [--size=WxH] [--fps=N] [--style=talk|light|dark]"); }
    if( W < 16 || H < 16 || W > 7680 || H > 4320 || fps < 1 || fps > 120 ) { return Fail("--size must be 16x16 to 7680x4320 and --fps 1 to 120"); }
    An::Style style;
    if( style_name == "talk" ) { style = An::Style::Talk(); } else if( style_name == "light" ) { style = An::Style::Light(); }
    else if( style_name == "dark" ) { style = An::Style::Dark(); } else { return Fail("--style must be talk, light or dark"); }

    std::ifstream in (path);
    if( !in ) { return Fail("cannot read " + path); }
    Trace_T::Reader reader (in);
    std::vector<Trace_T::Record> records;
    Trace_T::Record rec; std::string why;
    Trace_T::Status st;
    while( (st = reader.Next(rec, why)) == Trace_T::Status::Record ) { records.push_back(rec); }
    if( st == Trace_T::Status::Error ) { return Fail(path + ": " + why); }

    An::BuildOptions options;
    options.tray.aspect = double(W) / double(H);
    An::Animation A;
    if( !An::Builder<PD_T>(options).Build(records, path, A, why) ) { return Fail(why); }
    int cuts = 0;
    for( const An::Scene & s : A.scenes ) { if( s.label == "Cut" ) { ++cuts; std::fprintf(stderr, "knoodlemovie: cut: %s\n", s.note.c_str()); } }
    std::fprintf(stderr, "knoodlemovie: %zu scenes, %.1f s, %d cuts\n", A.scenes.size(), A.Duration(), cuts);

    if( !json.empty() )
    {
        std::ofstream o (json);
        if( !(o << An::ToJSON(A)) ) { return Fail("cannot write " + json); }
        return 0;
    }
    std::ios::sync_with_stdio(false);
    if( frame_at >= 0 )
    {
        const An::Image img = An::Draw(An::FrameAt(A, frame_at, style, W, H));
        const auto bytes = img.Bytes();
        std::cout << "P6\n" << W << " " << H << "\n255\n";
        std::cout.write(reinterpret_cast<const char *>(bytes.data()), std::streamsize(bytes.size()));
        return std::cout ? 0 : Fail("cannot write the frame");
    }
    const long n = long(A.Duration() * fps) + 1;
    for( long k = 0; k < n; ++k )
    {
        const auto bytes = An::Draw(An::FrameAt(A, double(k) / fps, style, W, H)).Bytes();
        if( !std::cout.write(reinterpret_cast<const char *>(bytes.data()), std::streamsize(bytes.size())) ) { return Fail("the frames' reader went away"); }
    }
    std::fprintf(stderr, "knoodlemovie: %ld frames of %dx%d rgb24 at %d fps\n", n, W, H, fps);
    return 0;
}
