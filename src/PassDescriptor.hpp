#pragma once

namespace Knoodle
{
    /*!@brief A pass move, named relative to a `PlanarDiagram`.
     *
     * This is the combinatorial descriptor specified in
     * `docs/move-descriptor.md`: the strand W to reroute, the face the new
     * corridor leaves through, the arcs it crosses in order with over/under
     * tags, and the face it arrives through.
     *
     * It is *syntactically* self-contained -- a list of darcs and tags -- but
     * *semantically* relative: those darcs are labels in one particular
     * diagram, and the checks need that diagram's face structure. So every
     * operation here takes the diagram as a parameter, and a serialized
     * descriptor is meaningless unless it travels with the diagram it was
     * written against. (`Pass_T`/`Path_T` are relative in exactly the same
     * way; what distinguishes the descriptor is that it can be *checked*.)
     *
     * Three tiers apply to a descriptor, and only the first is decided here:
     *
     *   well-formed  the descriptor is consistent with the diagram
     *                (`WellFormedQ`) -- needs `PlanarDiagram` and nothing else,
     *                which is why this lives outside the drawing code;
     *   routable     a corridor for it exists *in a given drawing*
     *                (`OrthoDecorate::RoutePassMove`) -- genuinely drawing-
     *                specific: a well-formed descriptor can be unroutable in
     *                one layout and fine in another;
     *   sound        the move preserves the link type -- needs a feasibility
     *                witness and is nobody's business here.
     */

    template<typename Int_>
    struct PassDescriptor
    {
        using Int  = Int_;
        using PD_T = PlanarDiagram<Int>;

        static constexpr bool Head = PD_T::Head;
        static constexpr bool Tail = PD_T::Tail;

        std::vector<Int>  strand;        // darcs of W, in traversal order
        Int               depart = -1;   // darc; L(depart) = corridor's 1st face
        std::vector<Int>  cross;         // darcs crossed, in order, left -> right
        std::vector<bool> over;          // per crossing: does W pass over?
        Int               land   = -1;   // darc; L(land) = corridor's last face

        // kind=middlepass: identical grammar, per-crossing tags allowed
        // (check 5 dropped). Well-formedness only; soundness of a middlepass
        // needs the quotient-simplicity witness, which is not checked here.
        bool middlepassQ = false;

        //======================================================================
        // Darc helpers. Convention (docs/move-descriptor.md): da = 2a + d with
        // Tail = 0, Head = 1, and every face lies to the LEFT of its boundary
        // darcs.
        //======================================================================

        static constexpr Int ArcOf( Int da ) { return da / Int(2); }
        static constexpr bool DirOf( Int da ) { return (da % Int(2)) != Int(0); }

        /*!@brief The face to the left of darc `da`, in `pd.FaceDarcs()`
         * numbering -- which is the numbering `OrthoDecorate` uses, so face
         * indices agree with the drawing code.
         */
        static Int LeftFace( cref<PD_T> pd, Int da )
        {
            if( (da < Int(0)) || (da >= Int(2) * pd.MaxArcCount()) )
            {
                return Int(-1);
            }
            const Int a = ArcOf(da);
            if( !pd.ArcActiveQ(a) ) { return Int(-1); }

            // `ArcFaces()(a,d)` is the face to the LEFT of `ToDarc(a,d)`, so
            // the darc's direction bit indexes it directly. (Faces.hpp says so
            // upstream since PR #29; the older comment there called `(a,1)` the
            // *right* face, which is what this branch still carries until it
            // merges main -- do not follow it.) Cross-checked against a map
            // built from `pd.FaceDarcs()` on all 282 arcs of a trefoil and the
            // two handoff reproducers.
            return pd.ArcFaces()(a, DirOf(da) ? Int(1) : Int(0));
        }

        static Int RightFace( cref<PD_T> pd, Int da )
        {
            return LeftFace(pd, da ^ Int(1));
        }

        /*!@brief The crossing a darc runs from / to. A Head-directed darc runs
         * along its arc's orientation; a Tail-directed one runs against it.
         */
        static Int DarcTailCrossing( cref<PD_T> pd, Int da )
        {
            return pd.Arcs()(ArcOf(da), (DirOf(da) == Head) ? Tail : Head);
        }

        static Int DarcHeadCrossing( cref<PD_T> pd, Int da )
        {
            return pd.Arcs()(ArcOf(da), (DirOf(da) == Head) ? Head : Tail);
        }

        //======================================================================
        // Tier 1: well-formedness
        //======================================================================

        /*!@brief Check the descriptor against `pd`. On failure `why` says which
         * check failed and why. The checks are those of
         * `docs/move-descriptor.md`, all of them pure diagram combinatorics.
         */
        bool WellFormedQ( cref<PD_T> pd, mref<std::string> why ) const
        {
            auto fail = [&why]( std::string msg ) -> bool
            {
                why = std::move(msg);
                return false;
            };

            const std::size_t m = strand.size();
            const std::size_t k = cross.size();

            if( m == 0 ) { return fail("empty strand"); }

            if( over.size() != k )
            {
                return fail("cross and over lists differ in length");
            }

            // -- check 5: uniform tags unless this is a middlepass ----------
            if( !middlepassQ )
            {
                for( std::size_t i = 1; i < k; ++i )
                {
                    if( over[i] != over[0] )
                    {
                        return fail("over/under tags are not all equal"
                            " (check 5; use kind=middlepass for mixed tags)");
                    }
                }
            }

            // -- check 1: arcs active, strand arcs distinct, crossed arcs not
            //    on the strand ---------------------------------------------
            for( Int da : strand )
            {
                if( LeftFace(pd,da) < Int(0) )
                {
                    return fail("strand darc " + Tools::ToString(da)
                        + " is inactive or out of range");
                }
            }
            for( Int da : cross )
            {
                if( LeftFace(pd,da) < Int(0) )
                {
                    return fail("crossed darc " + Tools::ToString(da)
                        + " is inactive or out of range");
                }
            }
            for( std::size_t i = 0; i < m; ++i )
            {
                for( std::size_t j = i + 1; j < m; ++j )
                {
                    if( ArcOf(strand[i]) == ArcOf(strand[j]) )
                    {
                        return fail("strand repeats arc "
                            + Tools::ToString(ArcOf(strand[i])) + " (check 1)");
                    }
                }
            }
            for( std::size_t i = 0; i < k; ++i )
            {
                for( std::size_t j = 0; j < m; ++j )
                {
                    if( ArcOf(cross[i]) == ArcOf(strand[j]) )
                    {
                        return fail("corridor crosses its own strand at arc "
                            + Tools::ToString(ArcOf(cross[i])) + " (check 1)");
                    }
                }
            }

            // -- check 1 (cont.): the corridor is arc-disjoint. Crossing one
            //    arc twice is not something an applier can do: after the first
            //    crossing splits it, the label denotes only the piece up to
            //    that new crossing, so a later step naming it again operates on
            //    a changed extent -- the same aliasing hazard the transversals
            //    have. `FindShortestPath` cannot emit such a path anyway (it
            //    keeps a visited set on arcs and only expands unvisited ones),
            //    so nothing legitimate is being excluded; this makes the
            //    precondition explicit instead of accidental.
            for( std::size_t i = 0; i < k; ++i )
            {
                for( std::size_t j = i + 1; j < k; ++j )
                {
                    if( ArcOf(cross[i]) == ArcOf(cross[j]) )
                    {
                        return fail("corridor crosses arc "
                            + Tools::ToString(ArcOf(cross[i]))
                            + " twice (steps " + Tools::ToString(i)
                            + " and " + Tools::ToString(j)
                            + "); the corridor must be arc-disjoint (check 1)");
                    }
                }
            }

            // -- check 1 (cont.): the strand is a consecutive run. Compared by
            //    crossing index; the drawing is not involved. --------------
            for( std::size_t i = 0; i + 1 < m; ++i )
            {
                if( DarcHeadCrossing(pd,strand[i])
                    != DarcTailCrossing(pd,strand[i+1]) )
                {
                    return fail("strand darcs " + Tools::ToString(strand[i]) + " and "
                        + Tools::ToString(strand[i+1])
                        + " are not consecutive (check 1)");
                }
            }

            const Int tail_anchor = DarcTailCrossing(pd,strand.front());
            const Int head_anchor = DarcHeadCrossing(pd,strand.back());

            if( tail_anchor == head_anchor )
            {
                return fail("tail and head anchors are the same crossing"
                    " (strand closes on itself; R_I curls and friends are not"
                    " supported)");
            }

            // -- check 6: the corridor may not be longer than the strand.
            //    An applier rebuilds the strand in place out of the labels the
            //    move frees -- W's own arcs and the transversal halves it heals
            //    away -- so a corridor with more crossings than W had simply
            //    has nowhere to live; the diagram would have to grow, which
            //    `Reroute` cannot do. It does not refuse such input either: its
            //    loop walks path positions while the strand pointer runs off
            //    the end of W, and it returns a diagram unrelated to the move
            //    (on a trefoil, 2 crossings out of 3). A lengthening pass is a
            //    perfectly good isotopy; it is just not expressible here.
            if( k + Int(1) > static_cast<Int>(m) )
            {
                return fail("the corridor has " + Tools::ToString(k)
                    + " crossings but the strand has only "
                    + Tools::ToString(static_cast<Int>(m) - Int(1))
                    + "; a pass move cannot lengthen the strand, there is no"
                      " room in the diagram for the extra crossings (check 6)");
            }

            // -- The tags must describe the strand we actually have. A pass
            //    move slides W; it cannot turn an over-strand into an under-
            //    strand. So for a classical pass W must be uniformly over or
            //    uniformly under at its interior crossings, and the corridor's
            //    tags must agree with that. Without this a descriptor can ask
            //    for a crossing change wearing a pass move's clothes -- and be
            //    faithfully carried out, silently changing the knot.
            //
            //    (kind=middlepass is exempt: its tags are per-crossing by
            //    definition, and what makes such a move legitimate is the
            //    feasibility witness, not this.)
            if( !middlepassQ && (m > std::size_t(1)) )
            {
                bool w_underQ = false;

                for( std::size_t i = 1; i < m; ++i )
                {
                    const Int x = DarcHeadCrossing(pd,strand[i-1]);
                    const Int a_in = ArcOf(strand[i-1]);

                    const bool rightQ =
                        (pd.CrossingStates()[x] == CrossingState_T::RightHanded);

                    // The under-strand enters at X[0]: (In,Right) for a right-
                    // handed crossing, (In,Left) for a left-handed one.
                    const Int under_in = pd.Crossings()(
                        x, PD_T::In, rightQ ? PD_T::Right : PD_T::Left );

                    const bool here_underQ = (under_in == a_in);

                    if( i == 1 ) { w_underQ = here_underQ; }
                    else if( here_underQ != w_underQ )
                    {
                        return fail("the strand passes over at some of its"
                            " interior crossings and under at others, so it is"
                            " not a pass move at all (use kind=middlepass)");
                    }
                }

                if( (k > 0) && (static_cast<bool>(over[0]) == w_underQ) )
                {
                    return fail(std::string("the corridor is tagged ")
                        + (over[0] ? "over" : "under")
                        + " but the strand runs "
                        + (w_underQ ? "under" : "over")
                        + " at its interior crossings; a pass move cannot swap"
                          " the two (that is a crossing change)");
                }
            }

            const Int F_dep  = LeftFace(pd,depart);
            const Int F_land = LeftFace(pd,land);

            if( F_dep  < Int(0) ) { return fail("depart darc is inactive or out of range"); }
            if( F_land < Int(0) ) { return fail("land darc is inactive or out of range"); }

            // -- checks 2 and 3: the face chain ----------------------------
            if( k > 0 )
            {
                if( LeftFace(pd,cross.front()) != F_dep )
                {
                    return fail("L(cross[0]) != L(depart) (check 2)");
                }
                for( std::size_t i = 0; i + 1 < k; ++i )
                {
                    if( RightFace(pd,cross[i]) != LeftFace(pd,cross[i+1]) )
                    {
                        return fail("R(cross[" + Tools::ToString(i) + "]) != L(cross["
                            + Tools::ToString(i+1) + "]) (check 2)");
                    }
                }
                if( RightFace(pd,cross.back()) != F_land )
                {
                    return fail("L(land) != R(cross[last]) (check 3)");
                }
            }
            else if( F_dep != F_land )
            {
                return fail("no crossings but L(depart) != L(land) (check 3)");
            }

            // -- check 4: `depart` must be a darc OF W's first arc, and `land`
            //    a darc of its last.
            //
            //    The requirement it enforces is about ports: a crossing has
            //    four quadrant faces, the anchors stay put across a pass, so
            //    the rerouted strand leaves (reaches) an anchor through the
            //    very port W's end arc occupies, and only the two quadrants
            //    flanking that port are reachable. Naming a darc of that arc
            //    says exactly that -- its two darcs have precisely those two
            //    faces on their left -- so this subsumes the face test.
            //
            //    It is also the reason the descriptor has a normal form. Any
            //    number of darcs can name one face, so "some darc whose left
            //    face is F" would leave `depart`/`land` free to vary while the
            //    move stayed the same, and no round trip through
            //    `Pass_T`/`Path_T` could be exact. Since both ends of this
            //    format are ours, we require the normal form rather than
            //    accepting the others and normalizing.
            if( ArcOf(depart) != ArcOf(strand.front()) )
            {
                return fail("depart darc " + Tools::ToString(depart)
                    + " names arc " + Tools::ToString(ArcOf(depart))
                    + ", but must name the strand's first arc "
                    + Tools::ToString(ArcOf(strand.front()))
                    + " -- otherwise the rerouted strand leaves the tail anchor"
                      " through a port that is not the one it vacated (check 4)");
            }
            if( ArcOf(land) != ArcOf(strand.back()) )
            {
                return fail("land darc " + Tools::ToString(land)
                    + " names arc " + Tools::ToString(ArcOf(land))
                    + ", but must name the strand's last arc "
                    + Tools::ToString(ArcOf(strand.back()))
                    + " -- otherwise the rerouted strand reaches the head anchor"
                      " through a port that is not the one it vacated (check 4)");
            }

            why.clear();
            return true;
        }

        //======================================================================
        // Serialization. The grammar is the `pass` payload of
        // docs/move-descriptor.md, i.e. what knoodledraw's --move accepts.
        //======================================================================

        std::string ToString() const
        {
            std::string s = middlepassQ ? "kind=middlepass " : "kind=pass ";

            s += "strand=";
            for( std::size_t i = 0; i < strand.size(); ++i )
            {
                if( i > 0 ) { s += ","; }
                s += Tools::ToString(strand[i]);
            }

            s += " depart=" + Tools::ToString(depart);

            if( !cross.empty() )
            {
                s += " cross=";
                for( std::size_t i = 0; i < cross.size(); ++i )
                {
                    if( i > 0 ) { s += ","; }
                    s += Tools::ToString(cross[i]);
                    s += over[i] ? ":o" : ":u";
                }
            }

            s += " land=" + Tools::ToString(land);

            return s;
        }

        friend std::string ToString( cref<PassDescriptor> d )
        {
            return d.ToString();
        }

        /*!@brief Parse the `pass` payload grammar. The `#move` token and
         * `kind=` are optional; `kind` defaults to `pass`.
         */
        static bool Parse(
            std::string_view spec, mref<PassDescriptor> out, mref<std::string> err
        )
        {
            out = PassDescriptor();

            bool have_strand = false, have_depart = false, have_land = false;

            auto parse_int = []( std::string_view tok, mref<Int> v ) -> bool
            {
                std::int64_t x = 0;
                auto [p,ec] = std::from_chars(
                    tok.data(), tok.data() + tok.size(), x );
                if( (ec != std::errc{}) || (p != tok.data() + tok.size()) )
                {
                    return false;
                }
                v = static_cast<Int>(x);
                return true;
            };

            auto for_each_item = []( std::string_view list, auto && f ) -> bool
            {
                while( !list.empty() )
                {
                    const auto comma = list.find(',');
                    if( !f(list.substr(0,comma)) ) { return false; }
                    if( comma == std::string_view::npos ) { break; }
                    list.remove_prefix(comma + 1);
                }
                return true;
            };

            std::size_t pos = 0;
            while( pos < spec.size() )
            {
                while( (pos < spec.size()) && std::isspace(static_cast<unsigned char>(spec[pos])) )
                {
                    ++pos;
                }
                if( pos >= spec.size() ) { break; }

                std::size_t end = pos;
                while( (end < spec.size()) && !std::isspace(static_cast<unsigned char>(spec[end])) )
                {
                    ++end;
                }

                const std::string_view tok = spec.substr(pos, end - pos);
                pos = end;

                if( tok == "#move" ) { continue; }

                const auto eq = tok.find('=');
                if( eq == std::string_view::npos )
                {
                    err = "expected key=value, got '" + std::string(tok) + "'";
                    return false;
                }

                const std::string_view key = tok.substr(0,eq);
                const std::string_view val = tok.substr(eq+1);

                if( key == "kind" )
                {
                    if     ( val == "pass"       ) { out.middlepassQ = false; }
                    else if( val == "middlepass" ) { out.middlepassQ = true;  }
                    else
                    {
                        err = "unsupported kind=" + std::string(val)
                            + " (want pass or middlepass)";
                        return false;
                    }
                }
                else if( key == "strand" )
                {
                    have_strand = true;
                    if( !for_each_item(val, [&](std::string_view item)
                        {
                            Int da;
                            if( !parse_int(item,da) ) { return false; }
                            out.strand.push_back(da);
                            return true;
                        }) )
                    {
                        err = "bad strand darc list '" + std::string(val) + "'";
                        return false;
                    }
                }
                else if( key == "depart" )
                {
                    have_depart = true;
                    if( !parse_int(val,out.depart) )
                    {
                        err = "bad depart darc '" + std::string(val) + "'";
                        return false;
                    }
                }
                else if( key == "land" )
                {
                    have_land = true;
                    if( !parse_int(val,out.land) )
                    {
                        err = "bad land darc '" + std::string(val) + "'";
                        return false;
                    }
                }
                else if( key == "cross" )
                {
                    if( !for_each_item(val, [&](std::string_view item)
                        {
                            const auto colon = item.find(':');
                            if( colon == std::string_view::npos ) { return false; }
                            Int da;
                            if( !parse_int(item.substr(0,colon),da) ) { return false; }
                            const std::string_view tag = item.substr(colon+1);
                            if     ( tag == "u" ) { out.over.push_back(false); }
                            else if( tag == "o" ) { out.over.push_back(true);  }
                            else { return false; }
                            out.cross.push_back(da);
                            return true;
                        }) )
                    {
                        err = "bad cross list '" + std::string(val)
                            + "' (want DARC:u or DARC:o, comma separated)";
                        return false;
                    }
                }
                else
                {
                    err = "unknown key '" + std::string(key) + "'";
                    return false;
                }
            }

            if( !have_strand ) { err = "missing strand=";  return false; }
            if( !have_depart ) { err = "missing depart=";  return false; }
            if( !have_land   ) { err = "missing land=";    return false; }

            err.clear();
            return true;
        }

        //======================================================================
        // Conversion to and from the arguments `PassSimplifier::Reroute` takes.
        //
        // Templated on `Pass_T`/`Path_T` so that this header depends on
        // `PlanarDiagram` alone and not on `PlanarDiagramComplex`.
        //
        // The low bit: SETTLED (2026-08-13). `Reroute` reads `Path_T`'s low
        // bit as `left_to_rightQ`, a side flag, while in `cross` it is the
        // darc's Head/Tail bit. This used to be recorded here as an
        // unproven coincidence. It is not a coincidence; the two are the same
        // proposition, and it is worth writing down why, because a prescribed
        // (non-shortest) path is exactly where an accidental agreement would
        // stop being safe.
        //
        // It is literally the same bit: `Reroute` takes it via
        // `FromDarc(path[p])`, whose second component is `da % 2`, and
        // `ToPassAndPath` below copies `cross[i]` into `path[i+1]` untouched.
        // So only the MEANINGS need to agree, and they do:
        //
        //   - Here, `cross = da` means the corridor steps from `L(da)` to
        //     `R(da)`. For `d = Head`, `L(2b+Head)` is the face to the left of
        //     `b`'s forward direction, so the corridor crosses `b` from `b`'s
        //     left to `b`'s right. For `d = Tail` it is the other way.
        //
        //   - In `Reroute` (PassSimplifier/Reroute.hpp), the
        //     `left_to_rightQ == true` diagram draws the rerouted strand
        //     running EAST across a `b` that runs NORTH -- from `b`'s left to
        //     `b`'s right -- and the `false` branch draws it running WEST.
        //
        // Same statement, so `left_to_rightQ == DirOf(da)`. The handedness
        // formula that consumes it agrees too: `Reroute` sets the new
        // crossing to `overQ` when left-to-right and `!overQ` otherwise, which
        // is exactly what `OrthoDecorate::AfterDiagram` computes, and the two
        // are checked against each other end-to-end by
        // `test/oracle_vs_reroute` (up to a 4-crossing corridor found by
        // Knoodle's own search).
        //
        // CAUTION that remains, and it is the live one: `Pass_T::overQ` is a
        // SINGLE bool, so these conversions can only express a move whose
        // corridor is uniformly over or uniformly under. A `kind=middlepass`
        // with mixed tags has no `Pass_T` to convert to. Check 5 does NOT
        // catch that -- middlepass exists to drop check 5 -- so
        // `ToPassAndPath` guards it explicitly; without that guard it read
        // `over[0]` and discarded the rest, handing the applier a different
        // move that happened to typecheck.
        //======================================================================

        template<typename Pass_T, typename Path_T>
        bool ToPassAndPath(
            cref<PD_T> pd, mref<Pass_T> pass, mref<Path_T> path,
            mref<std::string> why
        ) const
        {
            std::string ignored;
            if( !WellFormedQ(pd,why) ) { return false; }

            // `Pass_T` carries ONE `overQ` for the whole move, so a corridor
            // with mixed tags has nothing to convert to. Check 5 would have
            // caught this for a classical pass, but `kind=middlepass` exists
            // precisely to drop check 5, and the assignment below reads only
            // `over[0]` -- so without this guard a middlepass would be handed
            // to the applier with every tag after the first silently
            // discarded, i.e. as a DIFFERENT MOVE that happens to typecheck.
            //
            // Per-crossing over/under needs an applier that accepts it, which
            // `Reroute` is not. Refusing here is the honest answer until one
            // exists.
            for( std::size_t i = 1; i < over.size(); ++i )
            {
                if( over[i] != over[0] )
                {
                    why = "this corridor is over at some crossings and under at"
                          " others, and Pass_T has a single overQ for the whole"
                          " move, so it cannot be expressed as (Pass_T,Path_T)."
                          " A per-crossing applier is needed for it.";
                    return false;
                }
            }

            // `Reroute` walks the strand with `NextArc(a,Head)`, i.e. along the
            // arcs' own orientation, so a descriptor whose strand darcs run
            // against it describes the same strand traversed backwards.
            const bool forwardQ = (DirOf(strand.front()) == Head);

            for( Int da : strand )
            {
                if( (DirOf(da) == Head) != forwardQ )
                {
                    why = "strand darcs do not all run the same way along the"
                          " component, so the run has no orientation to give"
                          " Pass_T";
                    return false;
                }
            }

            pass.first     = forwardQ ? ArcOf(strand.front()) : ArcOf(strand.back());
            pass.last      = forwardQ ? ArcOf(strand.back())  : ArcOf(strand.front());
            pass.arc_count = static_cast<Int>(strand.size());
            pass.overQ     = over.empty() ? false : static_cast<bool>(over[0]);
            pass.activeQ   = true;

            // path[0] and path[last] name the start and end arcs; everything
            // between them is an arc to cross.
            path.Resize(static_cast<Int>(cross.size()) + Int(2));
            path[Int(0)] = PD_T::ToDarc(pass.first, Tail);
            for( std::size_t i = 0; i < cross.size(); ++i )
            {
                path[static_cast<Int>(i) + Int(1)] = cross[i];
            }
            path[static_cast<Int>(cross.size()) + Int(1)]
                = PD_T::ToDarc(pass.last, Tail);

            why.clear();
            return true;
        }

        template<typename Pass_T, typename Path_T>
        static bool FromPassAndPath(
            cref<PD_T> pd, cref<Pass_T> pass, cref<Path_T> path,
            mref<PassDescriptor> out, mref<std::string> why
        )
        {
            out = PassDescriptor();

            if( path.Size() < Int(2) )
            {
                why = "path has fewer than two entries";
                return false;
            }

            // Walk the strand from `first` to `last` along the orientation.
            Int a = pass.first;
            Int guard = 0;
            const Int limit = Int(2) * pd.MaxArcCount() + Int(2);
            out.strand.push_back(PD_T::ToDarc(a,Head));
            while( (a != pass.last) && (guard++ < limit) )
            {
                a = pd.NextArc(a,Head);
                out.strand.push_back(PD_T::ToDarc(a,Head));
            }
            if( a != pass.last )
            {
                why = "walking from pass.first never reached pass.last";
                return false;
            }

            for( Int p = Int(1); p + Int(1) < path.Size(); ++p )
            {
                out.cross.push_back(path[p]);
                out.over.push_back(pass.overQ);
            }

            // depart / land name faces; pick the darc of W's own end arc that
            // bounds the corridor's first / last face, which is always one of
            // the two flanking that arc (check 4).
            const Int F_dep = out.cross.empty()
                ? LeftFace(pd, PD_T::ToDarc(pass.first,Tail))
                : LeftFace(pd, out.cross.front());
            const Int F_land = out.cross.empty()
                ? F_dep
                : RightFace(pd, out.cross.back());

            auto darc_of_face = [&pd]( Int arc, Int face ) -> Int
            {
                const Int d0 = PD_T::ToDarc(arc,Tail);
                const Int d1 = PD_T::ToDarc(arc,Head);
                if( LeftFace(pd,d0) == face ) { return d0; }
                if( LeftFace(pd,d1) == face ) { return d1; }
                return Int(-1);
            };

            out.depart = darc_of_face(pass.first, F_dep);
            out.land   = darc_of_face(pass.last,  F_land);

            if( (out.depart < Int(0)) || (out.land < Int(0)) )
            {
                why = "the corridor's first or last face does not bound the"
                      " strand's end arc, so no depart/land darc names it";
                return false;
            }

            out.middlepassQ = false;

            why.clear();
            return true;
        }

    }; // struct PassDescriptor

} // namespace Knoodle
