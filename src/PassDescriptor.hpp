#pragma once

// Not included by Knoodle.hpp: include it after Knoodle.hpp where it is used.

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
        // needs the feasibility witness (#feas/#fvar, tools/witness_check.hpp),
        // which is not checked here.
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
            // The membership clause -- no crossed arc belongs to the strand --
            // binds `middlepass` only. For a uniform `pass`, Proposition C′
            // (middlestrands slide-realization-theorem §5.2, ratified
            // 2026-09-16) shows a corridor crossing W is still an isotopy: that
            // crossing simply goes with W. Nothing corresponding is proved for
            // mixed tags, so a middlepass keeps the clause.
            if( middlepassQ )
            {
                for( std::size_t i = 0; i < k; ++i )
                {
                    for( std::size_t j = 0; j < m; ++j )
                    {
                        if( ArcOf(cross[i]) == ArcOf(strand[j]) )
                        {
                            return fail("corridor crosses its own strand at arc "
                                + Tools::ToString(ArcOf(cross[i]))
                                + " (check 1; allowed for kind=pass, not for"
                                  " kind=middlepass)");
                        }
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

            // -- check 1 (cont.): the strand is a consecutive run ALONG ITS
            //    COMPONENT. Meeting at a crossing is not enough: at a crossing
            //    four arcs meet, and only one of them continues the strand --
            //    the one straight through, `NextArc`, in the same direction.
            //    Turning onto the other branch is an oriented smoothing wearing
            //    a pass move's clothes (on the trefoil, strand=1,9 depart=0
            //    land=8 was well formed and made a Hopf link; ROUND-24 §6(d)).
            //    Proposition C′ assumes this (its hypothesis 1).
            for( std::size_t i = 0; i + 1 < m; ++i )
            {
                if( DarcHeadCrossing(pd,strand[i])
                    != DarcTailCrossing(pd,strand[i+1]) )
                {
                    return fail("strand darcs " + Tools::ToString(strand[i]) + " and "
                        + Tools::ToString(strand[i+1])
                        + " are not consecutive (check 1)");
                }

                const Int  a = ArcOf(strand[i]);
                const bool d = DirOf(strand[i]);

                if( (ArcOf(strand[i+1]) != pd.NextArc(a,d))
                    || (DirOf(strand[i+1]) != d) )
                {
                    return fail("strand darcs " + Tools::ToString(strand[i]) + " and "
                        + Tools::ToString(strand[i+1]) + " meet at crossing "
                        + Tools::ToString(DarcHeadCrossing(pd,strand[i]))
                        + " but the second does not continue the first -- the"
                          " strand turns onto the other branch there (check 1)");
                }
            }

            const Int tail_anchor = DarcTailCrossing(pd,strand.front());
            const Int head_anchor = DarcHeadCrossing(pd,strand.back());

            // -- check 4 (cont.): the anchors. For `pass` this is NSI₂ --
            //    neither anchor is an interior crossing of the strand -- and
            //    the anchors MAY coincide: a lasso, which leaves crossing I by
            //    the first arc's port and comes back by the last arc's, two
            //    different ports of I. Proposition C′ as re-ratified 2026-09-25
            //    (middlestrands §5.2, c0bff9c) has exactly these hypotheses;
            //    its proof keeps W* away from both anchors, so whether they are
            //    one crossing never enters (ROUND-24). The lasso is the only
            //    move that reduces the descending trefoil shadow.
            //
            //    `middlepass` keeps the distinct-anchors clause: Theorem B has
            //    not been re-ratified for coinciding anchors.
            if( middlepassQ )
            {
                if( tail_anchor == head_anchor )
                {
                    return fail("tail and head anchors are the same crossing"
                        " (allowed for kind=pass, not for kind=middlepass)"
                        " (check 4)");
                }
            }
            else
            {
                for( std::size_t i = 0; i + 1 < m; ++i )
                {
                    const Int x = DarcHeadCrossing(pd,strand[i]);
                    if( (x == tail_anchor) || (x == head_anchor) )
                    {
                        return fail("anchor crossing " + Tools::ToString(x)
                            + " is also an interior crossing of the strand: the"
                              " strand passes back through its own anchor (NSI2,"
                              " check 4)");
                    }
                }

                // A lasso whose two ports at I lie on ONE branch: the strand
                // is its whole component, and I's other branch belongs to
                // another component. C′ covers it, but nobody needs it, and
                // for a knot NSI₂ already makes it impossible (ROUND-24 §4).
                if( (tail_anchor == head_anchor)
                    && (pd.NextArc(ArcOf(strand.back()),DirOf(strand.back()))
                        == ArcOf(strand.front()))
                    && (DirOf(strand.back()) == DirOf(strand.front())) )
                {
                    return fail("the strand is its whole component: it closes up"
                        " on its own branch at crossing "
                        + Tools::ToString(tail_anchor) + " (check 4)");
                }
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
            //
            //    Only the entries on arcs that SURVIVE count: a W entry goes
            //    with W (check 1, Proposition C′), becomes no crossing, and
            //    needs no room. That is also exactly the corridor `Reroute` is
            //    handed, with W hidden (ROUND-22 §4).
            std::size_t k_live = 0;
            for( std::size_t i = 0; i < k; ++i )
            {
                bool on_wQ = false;
                for( std::size_t j = 0; j < m; ++j )
                {
                    if( ArcOf(cross[i]) == ArcOf(strand[j]) ) { on_wQ = true; break; }
                }
                if( !on_wQ ) { ++k_live; }
            }
            if( k_live + 1 > m )
            {
                return fail("the corridor has " + Tools::ToString(k_live)
                    + " crossings off the strand but the strand has only "
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

            // -- check 3 (cont.): for `pass`, the corridor visits no face
            //    twice. A face chain is not an embedded arc: two passages
            //    through one face whose ends interleave round its boundary
            //    must cross each other, a crossing no descriptor names and
            //    `AfterDiagram` does not make. Measured 2026-09-25 on 30
            //    random 12-gon projections: of ~28,000 well-formed passes,
            //    89 changed the HOMFLY polynomial, and every one of them
            //    revisited a face; no face-simple one did. Proposition C′'s
            //    reading is proved for face-simple routes (ROUND-24 §6(c)),
            //    and a shortest-path search never revisits a face, so nothing
            //    an emitter legitimately wants is excluded. (`middlepass` is
            //    covered by its witness: V0 is UNCHECKED on a revisit.)
            if( !middlepassQ )
            {
                std::vector<Int> F;
                F.reserve(k + 1);
                F.push_back(F_dep);
                for( Int da : cross ) { F.push_back(RightFace(pd,da)); }
                std::sort(F.begin(), F.end());
                const auto dup = std::adjacent_find(F.begin(), F.end());
                if( dup != F.end() )
                {
                    return fail("the corridor visits face " + Tools::ToString(*dup)
                        + " twice; a pass corridor must be face-simple (check 3)");
                }
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

        //======================================================================
        // The diagram the move produces: "the diagram minus W"
        //
        // The pass picture is a superposition of two states, each one deletion
        // away. Delete the corridor and you have the diagram we were handed --
        // true by construction, since that is what was drawn. Delete the strand
        // W, smoothing the crossings it made, and what remains must be, up to
        // relabelling, the diagram the applier is supposed to return.
        //
        // `AfterDiagram` builds that second one from the descriptor alone,
        // without going near `PassSimplifier::Reroute`. That makes it an
        // independent oracle: if the applier disagrees with it, one of the two
        // is wrong, and the picture is where that shows up.
        //
        // It lives here, beside the checks, and not in the drawing code: it
        // reads a PlanarDiagram and this descriptor and returns a
        // PlanarDiagram. No layout is involved, and a verifier that never
        // draws anything (knoodleprove) must not have to build one.
        //
        // Surviving crossings and arcs keep their indices -- what the move
        // deletes is left inactive in place, what it creates is appended -- so
        // the before/after correspondence is the identity on everything the
        // move promised not to touch, the two anchors included. That is exactly
        // the matching the picture needs in order to say where the move
        // attaches.
        //======================================================================

        /*!@brief The diagram the move produces, plus the colours of any
         * components that come FREE of it.
         *
         * A pass move can split a crossingless loop off the diagram: if a
         * transversal closes up through nothing but interior crossings of W,
         * then deleting W takes away every crossing it had. A
         * `PlanarDiagram` cannot represent such a loop alongside crossings
         * (`AnelloQ` is a state of a whole diagram), so those components are
         * REPORTED instead -- one colour each, appended to `freed` -- exactly
         * as `PlanarDiagram::FromLinkEmbedding` hands back its diagram and its
         * unlinks side by side.
         *
         * The three-argument overload is for callers that do not expect a
         * split. It FAILS rather than quietly hand back a diagram that is
         * missing a component.
         *
         */
        PD_T AfterDiagram(
            cref<PD_T> pd, mref<std::string> why, mref<std::vector<Int>> freed
        ) const
        {
            freed.clear();

            auto fail = [&why]( std::string msg ) -> PD_T
            {
                why = std::move(msg);
                return PD_T();
            };

            if( !WellFormedQ(pd,why) ) { return PD_T(); }

            const Int n_c = pd.MaxCrossingCount();
            const Int n_a = pd.MaxArcCount();
            const Int L   = static_cast<Int>(strand.size());
            const Int k   = static_cast<Int>(cross.size());

            const Int m_c = n_c + k;
            const Int m_a = Int(2) * m_c;   // PD_T requires exactly this

            std::vector<Int>             C ( static_cast<std::size_t>(Int(4)*m_c), PD_T::Uninitialized );
            std::vector<CrossingState_T> CS( static_cast<std::size_t>(m_c), CrossingState_T::Inactive );
            std::vector<Int>             A ( static_cast<std::size_t>(Int(2)*m_a), PD_T::Uninitialized );
            std::vector<ArcState_T>      AS( static_cast<std::size_t>(m_a), ArcState_T::Inactive );
            std::vector<Int>             AC( static_cast<std::size_t>(m_a), PD_T::Uninitialized );

            for( Int c = 0; c < n_c; ++c )
            {
                for( Int io = 0; io < 2; ++io )
                {
                    for( Int lr = 0; lr < 2; ++lr )
                    {
                        C[static_cast<std::size_t>(Int(4)*c + Int(2)*io + lr)]
                            = pd.Crossings()(c,io,lr);
                    }
                }
                CS[static_cast<std::size_t>(c)] = pd.CrossingStates()[c];
            }
            for( Int a = 0; a < n_a; ++a )
            {
                A [static_cast<std::size_t>(Int(2)*a    )] = pd.Arcs()(a,0);
                A [static_cast<std::size_t>(Int(2)*a + 1)] = pd.Arcs()(a,1);
                AS[static_cast<std::size_t>(a)] = pd.ArcStates()[a];
                AC[static_cast<std::size_t>(a)] = pd.ArcColors()[a];
            }

            auto Cx = [&C]( Int c, Int io, Int lr ) -> Int &
            { return C[static_cast<std::size_t>(Int(4)*c + Int(2)*io + lr)]; };
            auto Aend = [&A]( Int a, Int ht ) -> Int &
            { return A[static_cast<std::size_t>(Int(2)*a + ht)]; };

            constexpr Int In_ = Int(1), Out_ = Int(0);

            // Repoint ONE END of an arc: its head sits in an In port of `c`,
            // its tail in an Out port. The side has to be given, because a
            // loop arc occupies both kinds of port at one crossing. Healing can
            // make one: a transversal that leaves crossing c, runs through
            // interior crossings of W only, and comes back to c heals into a
            // curl at c. Searching both sides then repointed its tail when the
            // split meant its head (ROUND-22 §3, n12s3 steps 386 and 500).
            auto repoint = [&]( Int c, Int io, Int from, Int to ) -> bool
            {
                for( Int lr = 0; lr < 2; ++lr )
                {
                    if( Cx(c,io,lr) == from ) { Cx(c,io,lr) = to; return true; }
                }
                return false;
            };

            std::vector<Int> w (static_cast<std::size_t>(L));
            for( Int i = 0; i < L; ++i )
            {
                w[static_cast<std::size_t>(i)]
                    = ArcOf(strand[static_cast<std::size_t>(i)]);
            }

            const Int T = DarcTailCrossing(pd, strand.front());
            const Int H = DarcHeadCrossing(pd, strand.back());

            // -- the transversal at each interior crossing, as a_0 -> a_1 ----
            std::vector<Int> heal_next(static_cast<std::size_t>(m_a), PD_T::Uninitialized);
            std::vector<Int> interior;

            for( Int i = 1; i < L; ++i )
            {
                const Int x = DarcHeadCrossing(
                    pd, strand[static_cast<std::size_t>(i-1)] );
                interior.push_back(x);

                Int a0 = PD_T::Uninitialized, a1 = PD_T::Uninitialized;
                const Int wp = w[static_cast<std::size_t>(i-1)];
                const Int wn = w[static_cast<std::size_t>(i)];
                for( Int lr = 0; lr < 2; ++lr )
                {
                    const Int in_a  = Cx(x,In_ ,lr);
                    const Int out_a = Cx(x,Out_,lr);
                    if( (in_a  != wp) && (in_a  != wn) ) { a0 = in_a;  }
                    if( (out_a != wp) && (out_a != wn) ) { a1 = out_a; }
                }
                if( (a0 == PD_T::Uninitialized) || (a1 == PD_T::Uninitialized) )
                {
                    return fail("could not identify the transversal at interior"
                        " crossing " + std::to_string(x));
                }
                heal_next[static_cast<std::size_t>(a0)] = a1;
            }

            // -- transversal CYCLES: the split-off components -----------------
            //
            // Following `heal_next` walks a transversal from one interior
            // crossing of W to the next. Usually that walk ends -- the strand
            // leaves W's neighbourhood and the chain has a first and a last
            // arc. But it can also CLOSE UP, and then the transversal is a
            // closed curve whose only crossings are interior crossings of W.
            //
            // That is a real move outcome, not a degenerate case: deleting W
            // takes every one of those crossings away, so the curve survives
            // the move as a component with NO crossings at all -- a free
            // unknot, split from the rest of the diagram. (Middlestrands hit
            // this from the other side: their applier calls
            // `CreateUnlinkFromArc` when collapsing a strand closes a loop.)
            //
            // Two outcomes, and they differ by whether the new corridor
            // crosses the loop:
            //
            //   nobody crosses it  ->  it really does come free. Its arcs
            //                          leave the diagram and its colour is
            //                          reported to the caller, because a PD
            //                          cannot hold a crossingless component
            //                          alongside crossings (`AnelloQ` is a
            //                          whole-diagram state).
            //
            //   the corridor crosses it -> it is NOT free: the corridor gives
            //                          it crossings back. Break the cycle open
            //                          at a crossed arc and it heals like any
            //                          other chain.
            //
            // Both were broken before. A cycle has no chain start, so the
            // representative pass below skipped it entirely: the first case
            // left arcs active while their crossings went inactive, and the
            // second indexed `Aend(rep_of[b0], ...)` with `Uninitialized`.
            std::vector<Int> heal_prev(static_cast<std::size_t>(m_a), PD_T::Uninitialized);
            for( Int a = 0; a < m_a; ++a )
            {
                const Int nx = heal_next[static_cast<std::size_t>(a)];
                if( nx != PD_T::Uninitialized )
                {
                    heal_prev[static_cast<std::size_t>(nx)] = a;
                }
            }

            std::vector<Int> freed_arcs;     // arcs of genuinely split-off loops

            {
                std::vector<char> on_cycle(static_cast<std::size_t>(m_a), char(0));
                std::vector<char> seen    (static_cast<std::size_t>(m_a), char(0));

                for( Int a0 = 0; a0 < m_a; ++a0 )
                {
                    if( seen[static_cast<std::size_t>(a0)] ) { continue; }
                    if( heal_next[static_cast<std::size_t>(a0)] == PD_T::Uninitialized
                     && heal_prev[static_cast<std::size_t>(a0)] == PD_T::Uninitialized )
                    {
                        continue;
                    }

                    // Walk back to a start; if we return to a0 it is a cycle.
                    Int cur = a0;
                    bool cycleQ = false;
                    for(;;)
                    {
                        const Int pv = heal_prev[static_cast<std::size_t>(cur)];
                        if( pv == PD_T::Uninitialized ) { break; }
                        if( pv == a0 ) { cycleQ = true; break; }
                        cur = pv;
                    }

                    // Mark the whole orbit seen either way.
                    std::vector<Int> orbit;
                    Int walk = cycleQ ? a0 : cur;
                    for(;;)
                    {
                        if( seen[static_cast<std::size_t>(walk)] ) { break; }
                        seen[static_cast<std::size_t>(walk)] = char(1);
                        orbit.push_back(walk);
                        if( cycleQ ) { on_cycle[static_cast<std::size_t>(walk)] = char(1); }
                        const Int nx = heal_next[static_cast<std::size_t>(walk)];
                        if( nx == PD_T::Uninitialized ) { break; }
                        walk = nx;
                    }

                    if( !cycleQ ) { continue; }

                    // Does the corridor cross this loop anywhere?
                    Int crossed = PD_T::Uninitialized;
                    for( Int j = 0; j < k; ++j )
                    {
                        const Int b = ArcOf(
                            cross[static_cast<std::size_t>(j)] );
                        for( Int o : orbit )
                        {
                            if( o == b ) { crossed = b; break; }
                        }
                        if( crossed != PD_T::Uninitialized ) { break; }
                    }

                    if( crossed != PD_T::Uninitialized )
                    {
                        // Not free after all. Open the cycle just before the
                        // crossed arc, so that arc becomes the chain's start
                        // and the corridor splits it as usual.
                        const Int pv = heal_prev[static_cast<std::size_t>(crossed)];
                        if( pv != PD_T::Uninitialized )
                        {
                            heal_next[static_cast<std::size_t>(pv)] = PD_T::Uninitialized;
                            heal_prev[static_cast<std::size_t>(crossed)] = PD_T::Uninitialized;
                        }
                    }
                    else
                    {
                        // ONE colour per component, not per arc. The whole
                        // orbit is one loop and shares a colour; pushing per
                        // arc would make `freed.size()` an arc count wearing a
                        // component count's clothes, and the three-argument
                        // overload reports exactly that number to the caller.
                        freed.push_back(AC[static_cast<std::size_t>(orbit.front())]);
                        for( Int o : orbit ) { freed_arcs.push_back(o); }
                    }
                }
            }

            // -- healed-arc representatives (a transversal can chain) --------
            // `chain_pos` is an arc's place along its healed arc, counted from
            // the chain start: the healed arc runs through its pieces in that
            // order, so it is also the order of anything that lies on them.
            std::vector<Int> rep_of   (static_cast<std::size_t>(m_a), PD_T::Uninitialized);
            std::vector<Int> chain_pos(static_cast<std::size_t>(m_a), PD_T::Uninitialized);
            {
                std::vector<bool> is_second(static_cast<std::size_t>(m_a), false);
                for( Int a = 0; a < m_a; ++a )
                {
                    const Int nx = heal_next[static_cast<std::size_t>(a)];
                    if( nx != PD_T::Uninitialized )
                    {
                        is_second[static_cast<std::size_t>(nx)] = true;
                    }
                }
                for( Int a = 0; a < m_a; ++a )
                {
                    if( is_second[static_cast<std::size_t>(a)] ) { continue; }
                    Int cur = a;
                    Int pos = 0;
                    while( cur != PD_T::Uninitialized )
                    {
                        rep_of   [static_cast<std::size_t>(cur)] = a;
                        chain_pos[static_cast<std::size_t>(cur)] = pos++;
                        cur = heal_next[static_cast<std::size_t>(cur)];
                    }
                }
            }

            // -- heal: the chain start absorbs the rest of its chain ---------
            for( Int a = 0; a < n_a; ++a )
            {
                if( rep_of[static_cast<std::size_t>(a)] != a ) { continue; }

                Int last = a;
                while( heal_next[static_cast<std::size_t>(last)] != PD_T::Uninitialized )
                {
                    const Int nxt = heal_next[static_cast<std::size_t>(last)];
                    AS[static_cast<std::size_t>(nxt)] = ArcState_T::Inactive;
                    last = nxt;
                }
                if( last != a )
                {
                    const Int new_head = Aend(last,Int(1));
                    Aend(a,Int(1)) = new_head;
                    if( !repoint(new_head,In_,last,a) )
                    {
                        return fail("healing: crossing " + std::to_string(new_head)
                            + " does not mention arc " + std::to_string(last));
                    }
                }
            }

            // -- retire W and the crossings it made --------------------------
            for( Int i = 0; i < L; ++i )
            {
                AS[static_cast<std::size_t>(w[static_cast<std::size_t>(i)])]
                    = ArcState_T::Inactive;
            }

            // -- and retire the loops that came free -------------------------
            // Their colours go to the caller: a PlanarDiagram cannot hold a
            // crossingless component next to crossings, so the split-off
            // unknots have to be reported rather than represented. This is the
            // same convention `PlanarDiagram::FromLinkEmbedding` uses, which
            // hands back the diagram and the unlinks' colours side by side.
            // (The colours were recorded when the loops were identified --
            // one per loop. Here the arcs simply leave the diagram.)
            for( Int a : freed_arcs )
            {
                AS[static_cast<std::size_t>(a)] = ArcState_T::Inactive;
            }
            for( Int x : interior ) { CS[static_cast<std::size_t>(x)] = CrossingState_T::Inactive; }

            // -- the corridor -----------------------------------------------
            // A `cross` entry can name an arc of W itself. That crossing
            // goes with W: the rerouted strand does not meet the strand it
            // replaces, so there is nothing left there to cross. Only the
            // entries on arcs that SURVIVE the move become crossings. Their
            // slots keep their original index `n_c + j` (so a drawing's
            // corridor crossing `j` still names crossing `n_c + j`); the
            // slots of W's entries simply stay inactive.
            std::vector<Int> live;          // indices j into cross, in order
            for( Int j = 0; j < k; ++j )
            {
                const Int b = ArcOf(cross[static_cast<std::size_t>(j)]);
                if( std::find(w.begin(), w.end(), b) == w.end() )
                {
                    live.push_back(j);
                }
            }
            const Int k_live = static_cast<Int>(live.size());

            // The move frees W's arcs and every transversal half it healed
            // away; between those and the slots the array grew by there is
            // always room for the k_live+1 corridor arcs and the k_live split
            // pieces.
            std::vector<Int> free_labels;
            for( Int a = 0; a < m_a; ++a )
            {
                if( AS[static_cast<std::size_t>(a)] == ArcState_T::Inactive )
                {
                    free_labels.push_back(a);
                }
            }
            if( static_cast<Int>(free_labels.size()) < Int(2)*k_live + Int(1) )
            {
                return fail("not enough arc slots for the corridor: need "
                    + std::to_string(Int(2)*k_live + Int(1)) + ", have "
                    + std::to_string(free_labels.size()));
            }

            std::size_t next_free = 0;
            std::vector<Int> p (static_cast<std::size_t>(k_live+1));
            for( Int e = 0; e <= k_live; ++e ) { p[static_cast<std::size_t>(e)] = free_labels[next_free++]; }
            std::vector<Int> q (static_cast<std::size_t>(k_live));
            for( Int e = 0; e < k_live; ++e ) { q[static_cast<std::size_t>(e)] = free_labels[next_free++]; }

            const Int color = pd.ArcColors()[w[0]];
            for( Int j = 0; j <= k_live; ++j )
            {
                AS[static_cast<std::size_t>(p[static_cast<std::size_t>(j)])] = ArcState_T::Active;
                AC[static_cast<std::size_t>(p[static_cast<std::size_t>(j)])] = color;
            }

            if( !repoint(T, Out_, w[0], p[0]) )
            {
                return fail("tail anchor " + std::to_string(T)
                    + " does not mention the strand's first arc");
            }
            if( !repoint(H, In_, w[static_cast<std::size_t>(L-1)],
                            p[static_cast<std::size_t>(k_live)]) )
            {
                return fail("head anchor " + std::to_string(H)
                    + " does not mention the strand's last arc");
            }
            Aend(p[0],Int(0)) = T;
            Aend(p[static_cast<std::size_t>(k_live)],Int(1)) = H;

            // The corridor may cross one healed arc more than once -- on two of
            // the pieces it was healed from, e.g. a chord of W and a transversal
            // arc at one of its ends. The order of those crossings along the
            // healed arc is the order of their pieces in the chain (check 1
            // makes the crossed arcs distinct, so there is at most one crossing
            // per piece). Each split below cuts the healed arc's CURRENT head
            // end, so splitting in decreasing chain position lays the crossings
            // down in chain order from the tail.
            //
            // `order` holds positions e along the LIVE corridor crossings;
            // `live[e]` is the descriptor entry each one came from.
            std::vector<Int> order (static_cast<std::size_t>(k_live));
            for( Int e = 0; e < k_live; ++e ) { order[static_cast<std::size_t>(e)] = e; }
            auto cross_arc = [&]( Int e ) -> Int
            {
                return ArcOf(cross[static_cast<std::size_t>(
                    live[static_cast<std::size_t>(e)])]);
            };
            std::stable_sort( order.begin(), order.end(),
                [&]( Int i, Int j )
                {
                    return chain_pos[static_cast<std::size_t>(cross_arc(i))]
                         > chain_pos[static_cast<std::size_t>(cross_arc(j))];
                }
            );

            for( Int e : order )
            {
                const Int j  = live[static_cast<std::size_t>(e)];
                const Int y  = n_c + j;
                const Int b0 = ArcOf(cross[static_cast<std::size_t>(j)]);
                const Int b  = rep_of[static_cast<std::size_t>(b0)];

                const Int qj       = q[static_cast<std::size_t>(e)];
                const Int old_head = Aend(b,Int(1));

                AS[static_cast<std::size_t>(qj)] = ArcState_T::Active;
                AC[static_cast<std::size_t>(qj)] = AC[static_cast<std::size_t>(b)];
                Aend(qj,Int(0)) = y;
                Aend(qj,Int(1)) = old_head;
                if( !repoint(old_head,In_,b,qj) )
                {
                    return fail("splitting: crossing " + std::to_string(old_head)
                        + " does not mention arc " + std::to_string(b));
                }
                Aend(b,Int(1)) = y;

                const Int a_in  = p[static_cast<std::size_t>(e)];
                const Int a_out = p[static_cast<std::size_t>(e+1)];
                Aend(a_in ,Int(1)) = y;
                Aend(a_out,Int(0)) = y;

                // Port layout and handedness follow the same formula the
                // applier uses (Reroute.hpp). That part is the geometry of the
                // crossing, exercised by every production reroute; the label
                // bookkeeping the aliasing bug lives in is above, and is ours.
                const bool l2rQ  = DirOf(cross[static_cast<std::size_t>(j)]);
                const bool overQ = static_cast<bool>(over[static_cast<std::size_t>(j)]);

                CS[static_cast<std::size_t>(y)]
                    = BooleanToCrossingState(l2rQ ? overQ : !overQ);

                if( l2rQ )
                {
                    Cx(y,Out_,Int(0)) = qj;     Cx(y,Out_,Int(1)) = a_out;
                    Cx(y,In_ ,Int(0)) = a_in;   Cx(y,In_ ,Int(1)) = b;
                }
                else
                {
                    Cx(y,Out_,Int(0)) = a_out;  Cx(y,Out_,Int(1)) = qj;
                    Cx(y,In_ ,Int(0)) = b;      Cx(y,In_ ,Int(1)) = a_in;
                }
            }

            if( k_live == Int(0) ) { Aend(p[0],Int(1)) = H; }

            why.clear();

            return PD_T(
                m_c, C.data(), CS.data(), A.data(), AS.data(), AC.data(),
                pd.LastColorDeactivated(), false, false
            );
        }

        /*!@brief `AfterDiagram` for callers that do not expect the move to
         * split a component off.
         *
         * If one does come free, this FAILS instead of returning a diagram
         * that is quietly missing it. A caller that gets `PD_T()` and a `why`
         * mentioning split components wants the four-argument overload.
         */
        PD_T AfterDiagram( cref<PD_T> pd, mref<std::string> why ) const
        {
            std::vector<Int> freed;
            PD_T after = AfterDiagram(pd,why,freed);

            if( why.empty() && !freed.empty() )
            {
                why = "this move splits " + std::to_string(freed.size())
                    + " crossingless component(s) off the diagram, which a"
                      " PlanarDiagram cannot hold alongside crossings; use the"
                      " overload that reports their colours";
                return PD_T();
            }

            return after;
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
        // is exactly what `AfterDiagram` (below) computes, and the two
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
