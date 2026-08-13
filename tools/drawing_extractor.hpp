#pragma once

// Read a knoodledraw drawing back into a PlanarDiagram.
//
// This is the oracle for the two-deletions contract of
// docs/move-descriptor.md. A pass-move picture claims two things at once:
// delete the corridor and what remains is an embedding of the diagram we were
// handed; delete the strand W and what remains is an embedding of the diagram
// the move produces. Both claims are about the DRAWING, so the only honest way
// to check them is to read the drawing back and compare the result to the
// diagram it is supposed to be.
//
// STRUCTURE COMES FROM THE PIXELS, NOT FROM THE DIAGRAM. Nothing here consults
// the PlanarDiagram the drawing was made from -- if it did, it could not catch
// a corridor attached to the wrong port, which is exactly the failure the
// picture exists to expose. Crossings, arcs, over/under and handedness are all
// read off the characters. (The caller may afterwards use the grid coordinates
// this reports to say WHICH crossing disagrees; that is localization, not
// structure, and it happens outside this file.)
//
// The grammar it reads is small, because knoodledraw's is:
//
//   strokes      - < > =   run east-west;   | ^ v ;   run north-south
//   corners      + (base, arms inferred), { } [ ] (corridor: NE NW SE SW)
//   dots         * (corridor branch point; arms inferred)
//   crossings    ANY stroke cell whose two PERPENDICULAR neighbours both
//                point into it. The cell's own glyph is the over-strand
//                (drawn straight through); the under-strand is the one
//                interrupted by exactly this one cell.
//
// That last rule is the whole reason this works: `OrthoDraw::DiagramString`
// draws the over-strand's glyph at the crossing vertex and leaves the
// under-strand a one-cell gap there, and `StampPassOverlay` does exactly the
// same for corridor crossings (an over-crossing writes = or ; into the crossed
// arc's cell; an under-crossing leaves the arc's glyph and breaks the
// corridor). One rule reads both, so the corridor is not a special case.
//
// NO VIEW AWARENESS. It might look as though the parser would have to know
// about pass moves -- to "detour" at a dot onto W or onto the corridor
// depending on which deletion is being drawn. It does not: the deletion is
// already materialized in the pixels, so in either single-deletion view a dot
// is an ordinary degree-2 cell (one arm to the anchor stub, one to whichever
// of the two continuations survives). Only `--pass-view=both` leaves a dot
// with degree 3, and that view is refused rather than guessed at.
//
// FAIL LOUD. Every cell's arms must agree with its neighbours' (if I claim an
// arm north, the cell north of me must claim one south), every cell must have
// degree 2 or be a degree-4 crossing, and every closed curve must carry at
// least one orientation arrow, with all its arrows agreeing. A drawing that
// breaks any of these is reported with the offending grid cell rather than
// parsed into a plausible-looking wrong diagram.

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace KnoodleDrawIO
{
    // Compass directions on the drawing grid: N = +y = up the page, matching
    // OrthoDraw's vertex coordinates and OrthoDecorate's overlay naming.
    inline constexpr int DirE = 0;
    inline constexpr int DirN = 1;
    inline constexpr int DirW = 2;
    inline constexpr int DirS = 3;

    inline constexpr int DirOpp( int d ) { return (d + 2) % 4; }

    inline constexpr int DirDX[4] = {  1,  0, -1,  0 };
    inline constexpr int DirDY[4] = {  0,  1,  0, -1 };

    template<class PD_T>
    class DrawingExtractor
    {
    public:

        using Int    = typename PD_T::Int;
        using Cell_T = std::array<Int,2>;

        struct Result_T
        {
            bool        okQ = false;
            std::string why;            // the failing cell, when !okQ

            PD_T        pd;             // everything with crossings in it
            Int         free_loops = 0; // closed curves carrying no crossing

            // Geometry, so a caller can say WHICH crossing disagrees: the grid
            // cell each parsed crossing was read from, and each parsed arc's
            // cell walk in its own tail -> head order.
            std::vector<Cell_T>              crossing_cell;
            std::vector<std::vector<Cell_T>> arc_cells;
        };

        /*!@brief Parse one drawing canvas.
         *
         * `canvas` is a row-major character grid of `n_y` rows of `n_x`
         * characters, column `n_x - 1` of each row being the newline --
         * exactly what `OrthoDraw::DiagramString` produces and what
         * `KnoodlePassView::RenderPassView` hands back. Row 0 of the string is
         * the TOP of the picture, so cell (x,y) lives at
         * `x + n_x * (n_y - 1 - y)`.
         */
        static Result_T Extract( const std::string & canvas, Int n_x, Int n_y )
        {
            Result_T R;

            auto coord = []( Int x, Int y ) -> std::string
            {
                return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
            };

            auto fail = [&R]( std::string msg ) -> Result_T
            {
                R.okQ = false;
                R.why = std::move(msg);
                return R;
            };

            if( (n_x < Int(2)) || (n_y < Int(1)) )
            {
                return fail("degenerate canvas");
            }

            const Int  wide = n_x - Int(1);   // drawable columns are [0,wide)
            const auto NC   = static_cast<std::size_t>(n_x)
                            * static_cast<std::size_t>(n_y);

            auto idx = [n_x,n_y]( Int x, Int y ) -> std::size_t
            {
                return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
            };

            auto inQ = [wide,n_y]( Int x, Int y ) -> bool
            {
                return (x >= Int(0)) && (x < wide)
                    && (y >= Int(0)) && (y < n_y);
            };

            auto ch = [&]( Int x, Int y ) -> char
            {
                if( !inQ(x,y) ) { return ' '; }
                const std::size_t i = idx(x,y);
                return (i < canvas.size()) ? canvas[i] : ' ';
            };

            //==================================================================
            // 1. Arms
            //==================================================================

            std::vector<std::uint8_t> arms  (NC, std::uint8_t(0));
            std::vector<char>         fullQ (NC, char(0));  // non-blank cell?
            std::vector<char>         inferQ(NC, char(0));  // + or * : arms inferred
            std::vector<char>         crossQ(NC, char(0));
            std::vector<char>         horizQ(NC, char(0));  // own glyph runs E-W
            std::vector<char>         strokeQ(NC, char(0)); // a straight stroke
                                                            // (not a corner, not
                                                            //  an inference cell)

            auto setarm = [&arms]( std::size_t i, int d )
            {
                arms[i] = static_cast<std::uint8_t>(
                    arms[i] | static_cast<std::uint8_t>(1u << d) );
            };
            auto hasarm = [&arms]( std::size_t i, int d ) -> bool
            {
                return ((arms[i] >> d) & 1u) != 0u;
            };
            auto degree = [&arms]( std::size_t i ) -> int
            {
                int n = 0;
                for( int d = 0; d < 4; ++d ) { n += (arms[i] >> d) & 1u; }
                return n;
            };

            for( Int y = 0; y < n_y; ++y )
            {
                for( Int x = 0; x < wide; ++x )
                {
                    const char c = ch(x,y);
                    if( c == ' ' ) { continue; }

                    const std::size_t i = idx(x,y);
                    fullQ[i] = char(1);

                    switch( c )
                    {
                        case '-': case '<': case '>': case '=':
                            horizQ[i] = char(1); strokeQ[i] = char(1);
                            setarm(i,DirE); setarm(i,DirW);
                            break;

                        case '|': case '^': case 'v': case ';':
                            strokeQ[i] = char(1);
                            setarm(i,DirN); setarm(i,DirS);
                            break;

                        case '{': setarm(i,DirN); setarm(i,DirE); break;
                        case '}': setarm(i,DirN); setarm(i,DirW); break;
                        case '[': setarm(i,DirS); setarm(i,DirE); break;
                        case ']': setarm(i,DirS); setarm(i,DirW); break;

                        case '+': case '*': inferQ[i] = char(1); break;

                        default:
                            return fail("cell " + coord(x,y) + " holds '"
                                + std::string(1,c) + "', which is not a stroke"
                                " glyph -- the canvas must be a plain drawing"
                                " (no labels, no --mono markers, no ANSI)");
                    }
                }
            }

            //==================================================================
            // 2. Inferred corners/dots and crossing detection, to a fixpoint.
            //
            // The two feed each other, and on a big diagram they can deadlock:
            // a dot whose own arc runs UNDER at the crossing beside it needs
            // that crossing's arms to know it connects there, while the
            // crossing needs the dot's arm to be recognized as a crossing at
            // all. Neither moves, and the dot is left with one arm.
            //
            // What breaks it is that one of the two directions is forced. An
            // inference cell must end up with degree 2 -- it is a cell of some
            // strand, and strands do not end. So if it still has fewer than
            // two arms, and a neighbour is a straight stroke lying ACROSS the
            // direction in question, then that neighbour can only be the
            // interrupted side of a crossing: nothing else lets a strand
            // continue through it that way. Promoting it is a deduction, not a
            // guess, and it only ever fires for a cell that demonstrably needs
            // another arm.
            //
            // Everything here is monotone (arms only ever appear), so the
            // iteration settles; the consistency check below is what makes a
            // wrong fixpoint impossible to mistake for a right one.
            //==================================================================

            for( int round = 0; round < 6; ++round )
            {
                for( Int y = 0; y < n_y; ++y )
                for( Int x = 0; x < wide; ++x )
                {
                    const std::size_t i = idx(x,y);
                    if( !inferQ[i] ) { continue; }

                    arms[i] = std::uint8_t(0);
                    for( int d = 0; d < 4; ++d )
                    {
                        const Int nx = x + DirDX[d], ny = y + DirDY[d];
                        if( !inQ(nx,ny) ) { continue; }
                        const std::size_t j = idx(nx,ny);
                        if( fullQ[j] && hasarm(j,DirOpp(d)) ) { setarm(i,d); }
                    }
                }

                for( Int y = 0; y < n_y; ++y )
                for( Int x = 0; x < wide; ++x )
                {
                    const std::size_t i = idx(x,y);
                    if( !fullQ[i] || inferQ[i] || crossQ[i] ) { continue; }

                    // Perpendicular to this cell's own glyph.
                    const int p0 = horizQ[i] ? DirN : DirE;
                    const int p1 = horizQ[i] ? DirS : DirW;

                    const Int ax = x + DirDX[p0], ay = y + DirDY[p0];
                    const Int bx = x + DirDX[p1], by = y + DirDY[p1];
                    if( !inQ(ax,ay) || !inQ(bx,by) ) { continue; }

                    const std::size_t ja = idx(ax,ay), jb = idx(bx,by);
                    if( fullQ[ja] && hasarm(ja,DirOpp(p0))
                     && fullQ[jb] && hasarm(jb,DirOpp(p1)) )
                    {
                        crossQ[i] = char(1);
                        for( int d = 0; d < 4; ++d ) { setarm(i,d); }
                    }
                }

                // An inference cell still short of degree 2 forces a crossing
                // on any neighbour it can only reach through one.
                for( Int y = 0; y < n_y; ++y )
                for( Int x = 0; x < wide; ++x )
                {
                    const std::size_t i = idx(x,y);
                    if( !inferQ[i] ) { continue; }

                    int deg = 0;
                    for( int d = 0; d < 4; ++d ) { deg += hasarm(i,d) ? 1 : 0; }
                    if( deg >= 2 ) { continue; }

                    for( int d = 0; d < 4; ++d )
                    {
                        if( hasarm(i,d) ) { continue; }

                        const Int nx = x + DirDX[d], ny = y + DirDY[d];
                        if( !inQ(nx,ny) ) { continue; }

                        const std::size_t j = idx(nx,ny);
                        if( !fullQ[j] || !strokeQ[j] || crossQ[j] ) { continue; }

                        // Does the neighbour's own glyph already run the way
                        // we need? Then it is an ordinary continuation and
                        // will have been picked up above. Only a stroke lying
                        // ACROSS us has to be a crossing.
                        const bool alongQ = horizQ[j] ? ((d == DirE) || (d == DirW))
                                                      : ((d == DirN) || (d == DirS));
                        if( alongQ ) { continue; }

                        crossQ[j] = char(1);
                        for( int e = 0; e < 4; ++e ) { setarm(j,e); }
                    }
                }
            }

            //==================================================================
            // 3. Consistency and degree
            //==================================================================

            for( Int y = 0; y < n_y; ++y )
            for( Int x = 0; x < wide; ++x )
            {
                const std::size_t i = idx(x,y);
                if( !fullQ[i] ) { continue; }

                for( int d = 0; d < 4; ++d )
                {
                    if( !hasarm(i,d) ) { continue; }

                    const Int nx = x + DirDX[d], ny = y + DirDY[d];
                    if( !inQ(nx,ny) )
                    {
                        return fail("the stroke at " + coord(x,y)
                            + " runs off the edge of the canvas");
                    }
                    const std::size_t j = idx(nx,ny);
                    if( !fullQ[j] || !hasarm(j,DirOpp(d)) )
                    {
                        return fail("stroke mismatch: " + coord(x,y) + " '"
                            + std::string(1,ch(x,y)) + "' connects toward "
                            + coord(nx,ny) + " '" + std::string(1,ch(nx,ny))
                            + "', which does not connect back");
                    }
                }

                const int deg = degree(i);

                if( crossQ[i] )
                {
                    if( deg != 4 )
                    {
                        return fail("crossing at " + coord(x,y)
                            + " has degree " + std::to_string(deg));
                    }
                }
                else if( deg != 2 )
                {
                    return fail("cell " + coord(x,y) + " '"
                        + std::string(1,ch(x,y)) + "' has degree "
                        + std::to_string(deg) + "; every cell of a drawing is"
                        " degree 2, or degree 4 at a crossing. (Degree 3 at a"
                        " '*' is what --pass-view=both looks like: the dot is a"
                        " branch point there, and only the single-deletion"
                        " views are parseable.)");
                }
            }

            //==================================================================
            // 4. Crossings
            //==================================================================

            std::vector<Int> cell_cx(NC, Int(-1));

            for( Int y = n_y - Int(1); y >= Int(0); --y )
            for( Int x = Int(0); x < wide; ++x )
            {
                const std::size_t i = idx(x,y);
                if( !crossQ[i] ) { continue; }
                cell_cx[i] = static_cast<Int>(R.crossing_cell.size());
                R.crossing_cell.push_back(Cell_T{x,y});
            }

            const Int n_c = static_cast<Int>(R.crossing_cell.size());

            if( n_c == Int(0) )
            {
                return fail("the drawing has no crossings");
            }

            //==================================================================
            // 5. Arcs: walk out of every crossing port until the next crossing
            //==================================================================

            struct ArcRec
            {
                Int c0 = -1; int d0 = -1;    // traced start (crossing, port)
                Int c1 = -1; int d1 = -1;    // traced end
                std::vector<Cell_T> cells;   // interior cells, in traced order
                std::vector<int>    travel;  // direction of travel into each
            };

            std::vector<ArcRec> arc(0);
            std::vector<char>   used(static_cast<std::size_t>(Int(4)*n_c), char(0));
            std::vector<char>   seen(NC, char(0));

            for( const Cell_T & cc : R.crossing_cell ) { seen[idx(cc[0],cc[1])] = char(1); }

            for( Int c = 0; c < n_c; ++c )
            {
                for( int d = 0; d < 4; ++d )
                {
                    if( used[static_cast<std::size_t>(Int(4)*c) + std::size_t(d)] )
                    {
                        continue;
                    }

                    ArcRec ar;
                    ar.c0 = c; ar.d0 = d;

                    Int cx = R.crossing_cell[static_cast<std::size_t>(c)][0] + DirDX[d];
                    Int cy = R.crossing_cell[static_cast<std::size_t>(c)][1] + DirDY[d];
                    int from = DirOpp(d);

                    std::size_t guard = 0;

                    while( true )
                    {
                        if( ++guard > NC + std::size_t(8) )
                        {
                            return fail("an arc walk out of crossing "
                                + std::to_string(c) + " did not terminate");
                        }

                        const std::size_t i = idx(cx,cy);

                        if( cell_cx[i] >= Int(0) )
                        {
                            ar.c1 = cell_cx[i];
                            ar.d1 = from;
                            break;
                        }

                        seen[i] = char(1);
                        ar.cells.push_back(Cell_T{cx,cy});
                        ar.travel.push_back(DirOpp(from));

                        int nd = -1;
                        for( int e = 0; e < 4; ++e )
                        {
                            if( (e != from) && hasarm(i,e) ) { nd = e; break; }
                        }
                        if( nd < 0 )
                        {
                            return fail("dead end at " + coord(cx,cy));
                        }

                        cx += DirDX[nd];
                        cy += DirDY[nd];
                        from = DirOpp(nd);
                    }

                    used[static_cast<std::size_t>(Int(4)*c)      + std::size_t(d)     ] = char(1);
                    used[static_cast<std::size_t>(Int(4)*ar.c1)  + std::size_t(ar.d1) ] = char(1);

                    arc.push_back(std::move(ar));
                }
            }

            const Int n_a = static_cast<Int>(arc.size());

            if( n_a != Int(2) * n_c )
            {
                return fail("read " + std::to_string(n_a) + " arcs from "
                    + std::to_string(n_c) + " crossings; a diagram has exactly"
                    " twice as many arcs as crossings");
            }

            //==================================================================
            // 6. Free loops: closed curves that meet no crossing
            //==================================================================

            for( Int y = 0; y < n_y; ++y )
            for( Int x = 0; x < wide; ++x )
            {
                const std::size_t i = idx(x,y);
                if( !fullQ[i] || seen[i] ) { continue; }

                ++R.free_loops;

                std::vector<Cell_T> stack{ Cell_T{x,y} };
                seen[i] = char(1);

                while( !stack.empty() )
                {
                    const Cell_T p = stack.back(); stack.pop_back();
                    const std::size_t pi = idx(p[0],p[1]);

                    for( int d = 0; d < 4; ++d )
                    {
                        if( !hasarm(pi,d) ) { continue; }
                        const Int qx = p[0] + DirDX[d], qy = p[1] + DirDY[d];
                        const std::size_t qi = idx(qx,qy);
                        if( seen[qi] ) { continue; }
                        seen[qi] = char(1);
                        stack.push_back(Cell_T{qx,qy});
                    }
                }
            }

            //==================================================================
            // 7. Orientation
            //
            // Each arrow glyph fixes its own arc; the rest follows, because a
            // strand runs straight through a crossing (in at port d, out at
            // port opposite d), so the arcs of one component form a cycle.
            //==================================================================

            std::vector<char> known(static_cast<std::size_t>(n_a), char(0));
            std::vector<char> flip (static_cast<std::size_t>(n_a), char(0));

            for( Int a = 0; a < n_a; ++a )
            {
                const ArcRec & ar = arc[static_cast<std::size_t>(a)];

                for( std::size_t k = 0; k < ar.cells.size(); ++k )
                {
                    const char c = ch(ar.cells[k][0], ar.cells[k][1]);

                    int glyph = -1;
                    if     ( c == '>' ) { glyph = DirE; }
                    else if( c == '<' ) { glyph = DirW; }
                    else if( c == '^' ) { glyph = DirN; }
                    else if( c == 'v' ) { glyph = DirS; }
                    else { continue; }

                    const int t = ar.travel[k];

                    if( (glyph != t) && (glyph != DirOpp(t)) )
                    {
                        return fail("the arrow at "
                            + coord(ar.cells[k][0],ar.cells[k][1])
                            + " points across its own stroke");
                    }

                    const char f = (glyph == DirOpp(t)) ? char(1) : char(0);

                    if( known[static_cast<std::size_t>(a)]
                     && (flip[static_cast<std::size_t>(a)] != f) )
                    {
                        return fail("the arc through "
                            + coord(ar.cells[k][0],ar.cells[k][1])
                            + " carries arrows pointing both ways");
                    }

                    known[static_cast<std::size_t>(a)] = char(1);
                    flip [static_cast<std::size_t>(a)] = f;
                }
            }

            // port -> (arc, which traced end)
            std::vector<Int> port_arc(static_cast<std::size_t>(Int(4)*n_c), Int(-1));
            std::vector<Int> port_end(static_cast<std::size_t>(Int(4)*n_c), Int(-1));

            for( Int a = 0; a < n_a; ++a )
            {
                const ArcRec & ar = arc[static_cast<std::size_t>(a)];

                const auto s0 = static_cast<std::size_t>(Int(4)*ar.c0) + std::size_t(ar.d0);
                const auto s1 = static_cast<std::size_t>(Int(4)*ar.c1) + std::size_t(ar.d1);

                port_arc[s0] = a; port_end[s0] = Int(0);
                port_arc[s1] = a; port_end[s1] = Int(1);
            }

            for( std::size_t s = 0; s < port_arc.size(); ++s )
            {
                if( port_arc[s] < Int(0) )
                {
                    return fail("crossing " + std::to_string(s / 4) + " port "
                        + std::to_string(s % 4) + " has no arc");
                }
            }

            // The traced end of `a` that is its true tail (0 or 1).
            auto tail_end = [&flip]( Int a ) -> Int
            {
                return flip[static_cast<std::size_t>(a)] ? Int(1) : Int(0);
            };

            auto end_port = []( const ArcRec & ar, Int e ) -> std::pair<Int,int>
            {
                return (e == Int(0)) ? std::pair<Int,int>{ar.c0, ar.d0}
                                     : std::pair<Int,int>{ar.c1, ar.d1};
            };

            {
                std::vector<Int> work;
                for( Int a = 0; a < n_a; ++a )
                {
                    if( known[static_cast<std::size_t>(a)] ) { work.push_back(a); }
                }

                if( work.empty() )
                {
                    return fail("no arc in the drawing carries an orientation"
                        " arrow, so the diagram cannot be oriented");
                }

                while( !work.empty() )
                {
                    const Int a = work.back(); work.pop_back();
                    const ArcRec & ar = arc[static_cast<std::size_t>(a)];

                    const Int te = tail_end(a);
                    const Int he = Int(1) - te;

                    // Forward: the strand leaves this crossing through the
                    // opposite port, and that arc's TAIL is there.
                    {
                        const auto [hc,hd] = end_port(ar,he);
                        const auto s = static_cast<std::size_t>(Int(4)*hc)
                                     + std::size_t(DirOpp(hd));
                        const Int  b  = port_arc[s];
                        const char fb = (port_end[s] == Int(1)) ? char(1) : char(0);

                        if( known[static_cast<std::size_t>(b)] )
                        {
                            if( flip[static_cast<std::size_t>(b)] != fb )
                            {
                                return fail("orientations disagree where the"
                                    " strand runs through crossing "
                                    + std::to_string(hc));
                            }
                        }
                        else
                        {
                            known[static_cast<std::size_t>(b)] = char(1);
                            flip [static_cast<std::size_t>(b)] = fb;
                            work.push_back(b);
                        }
                    }

                    // Backward: the arc arriving at this crossing has its HEAD
                    // at the opposite port.
                    {
                        const auto [tc,td] = end_port(ar,te);
                        const auto s = static_cast<std::size_t>(Int(4)*tc)
                                     + std::size_t(DirOpp(td));
                        const Int  p  = port_arc[s];
                        const char fp = (port_end[s] == Int(0)) ? char(1) : char(0);

                        if( known[static_cast<std::size_t>(p)] )
                        {
                            if( flip[static_cast<std::size_t>(p)] != fp )
                            {
                                return fail("orientations disagree where the"
                                    " strand runs through crossing "
                                    + std::to_string(tc));
                            }
                        }
                        else
                        {
                            known[static_cast<std::size_t>(p)] = char(1);
                            flip [static_cast<std::size_t>(p)] = fp;
                            work.push_back(p);
                        }
                    }
                }

                for( Int a = 0; a < n_a; ++a )
                {
                    if( !known[static_cast<std::size_t>(a)] )
                    {
                        return fail("a closed curve in the drawing carries no"
                            " orientation arrow anywhere along it");
                    }
                }
            }

            //==================================================================
            // 8. Assemble the diagram
            //==================================================================

            std::vector<Int> C( static_cast<std::size_t>(Int(4)*n_c), PD_T::Uninitialized );
            std::vector<Knoodle::CrossingState_T> CS(
                static_cast<std::size_t>(n_c), Knoodle::CrossingState_T::Inactive );
            std::vector<Int> A( static_cast<std::size_t>(Int(2)*n_a), PD_T::Uninitialized );
            std::vector<Knoodle::ArcState_T> AS(
                static_cast<std::size_t>(n_a), Knoodle::ArcState_T::Active );
            std::vector<Int> AC( static_cast<std::size_t>(n_a), Int(0) );

            constexpr Int Out_ = Int(0), In_ = Int(1);
            constexpr Int Left_ = Int(0), Right_ = Int(1);

            auto Cx = [&C]( Int c, Int io, Int lr ) -> Int &
            {
                return C[static_cast<std::size_t>(Int(4)*c + Int(2)*io + lr)];
            };

            // Does the arc at port (c,d) LEAVE c (i.e. is its tail here)?
            auto leavesQ = [&]( Int c, int d ) -> bool
            {
                const auto s = static_cast<std::size_t>(Int(4)*c) + std::size_t(d);
                return port_end[s] == tail_end(port_arc[s]);
            };

            for( Int c = 0; c < n_c; ++c )
            {
                const Cell_T & cc = R.crossing_cell[static_cast<std::size_t>(c)];
                const std::size_t i = idx(cc[0],cc[1]);

                // The cell's own glyph is the strand drawn through it: the
                // over-strand. The other axis is the interrupted one.
                const int o0 = horizQ[i] ? DirE : DirN;
                const int o1 = horizQ[i] ? DirW : DirS;
                const int u0 = horizQ[i] ? DirN : DirE;
                const int u1 = horizQ[i] ? DirS : DirW;

                auto outgoing = [&]( int a0, int a1, const char * which ) -> int
                {
                    const bool q0 = leavesQ(c,a0);
                    const bool q1 = leavesQ(c,a1);
                    if( q0 == q1 ) { return -1; }
                    (void)which;
                    return q0 ? a0 : a1;
                };

                const int o = outgoing(o0,o1,"over");
                const int u = outgoing(u0,u1,"under");

                if( (o < 0) || (u < 0) )
                {
                    return fail("at the crossing at " + coord(cc[0],cc[1])
                        + " a strand does not enter on one side and leave on"
                          " the other");
                }

                // Handedness. In Knoodle's picture (src/PlanarDiagram/
                // Crossings.hpp, and confirmed at PassDescriptor.hpp's
                // under-enters-at-X[0] note) the over-strand of a RIGHT-handed
                // crossing runs In,Left -> Out,Right with both strands drawn
                // upward. Rotating that picture 45 degrees counter-clockwise
                // onto the grid axes sends the over-strand to N and the
                // under-strand to W, so right-handed means det[o,u] > 0 -- a
                // statement invariant under rotation, hence usable as is.
                const Int ox = DirDX[o], oy = DirDY[o];
                const Int ux = DirDX[u], uy = DirDY[u];
                const bool rightQ = ((ox * uy - oy * ux) > Int(0));

                const auto at = [&]( int d ) -> Int
                {
                    return port_arc[static_cast<std::size_t>(Int(4)*c) + std::size_t(d)];
                };

                const Int over_in   = at(DirOpp(o));
                const Int over_out  = at(o);
                const Int under_in  = at(DirOpp(u));
                const Int under_out = at(u);

                CS[static_cast<std::size_t>(c)]
                    = Knoodle::BooleanToCrossingState(rightQ);

                if( rightQ )
                {
                    Cx(c,In_ ,Left_ ) = over_in;
                    Cx(c,Out_,Right_) = over_out;
                    Cx(c,In_ ,Right_) = under_in;
                    Cx(c,Out_,Left_ ) = under_out;
                }
                else
                {
                    Cx(c,In_ ,Right_) = over_in;
                    Cx(c,Out_,Left_ ) = over_out;
                    Cx(c,In_ ,Left_ ) = under_in;
                    Cx(c,Out_,Right_) = under_out;
                }
            }

            for( Int a = 0; a < n_a; ++a )
            {
                const ArcRec & ar = arc[static_cast<std::size_t>(a)];

                const Int te = tail_end(a);
                const auto [tc,td] = end_port(ar,te);
                const auto [hc,hd] = end_port(ar,Int(1) - te);
                (void)td; (void)hd;

                A[static_cast<std::size_t>(Int(2)*a + Int(0))] = tc;  // Tail
                A[static_cast<std::size_t>(Int(2)*a + Int(1))] = hc;  // Head

                std::vector<Cell_T> cells = ar.cells;
                if( flip[static_cast<std::size_t>(a)] )
                {
                    std::reverse(cells.begin(), cells.end());
                }
                R.arc_cells.push_back(std::move(cells));
            }

            // Colors track link components: follow each strand straight
            // through its crossings and give the whole cycle one color.
            {
                Int color = Int(0);
                std::vector<char> done(static_cast<std::size_t>(n_a), char(0));

                for( Int a0 = 0; a0 < n_a; ++a0 )
                {
                    if( done[static_cast<std::size_t>(a0)] ) { continue; }

                    Int a = a0;
                    std::size_t guard = 0;

                    while( !done[static_cast<std::size_t>(a)] )
                    {
                        if( ++guard > static_cast<std::size_t>(n_a) + 2 ) { break; }

                        done[static_cast<std::size_t>(a)] = char(1);
                        AC[static_cast<std::size_t>(a)]   = color;

                        const ArcRec & ar = arc[static_cast<std::size_t>(a)];
                        const auto [hc,hd] = end_port(ar, Int(1) - tail_end(a));
                        a = port_arc[static_cast<std::size_t>(Int(4)*hc)
                                   + std::size_t(DirOpp(hd))];
                    }

                    ++color;
                }
            }

            R.pd = PD_T(
                n_c, C.data(), CS.data(), A.data(), AS.data(), AC.data(),
                PD_T::Uninitialized, false, false
            );

            R.okQ = true;
            R.why.clear();
            return R;
        }

    }; // class DrawingExtractor

} // namespace KnoodleDrawIO
