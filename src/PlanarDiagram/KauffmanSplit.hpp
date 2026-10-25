public:

struct PlanarDiagram_with_Unlinks
{
    PD_T pd;
    Int  unlink_count = 0;
};


/*!@brief **EXPERIMENTAL.** Given a planar diagram (and some unlinks), resolve crossing `c` in two ways as mandated by the recursive procedure to compute thr Kauffman bracket.
 *
 * CAUTION: The arc colors of the outpus will be freshly recomputed, independent from the inputs.
 *
 * Convention for the return variable `r`:
 * @verbatim
    `p.pd`            `r.first.pd`    `r.second.pd`
  
    O     O              O     O         O-----O
     \   /               |     |
      \ /                |     |
       / c       =>      |     |
      / \                |     |
     /   \               |     |
    O     O              O     O         O-----O
   @endverbatim
 *
 * @param p The instance of `PlanarDiagram_with_Unlinks` to be processed.
 *
 * @param c The crossing to be resolved.
 *
 * @tparam warningQ If set to `true`, then a warning is issued whenever `c` is deactivated. Default is `false`.
 */

template<bool warningQ = false, bool verboseQ = false>
friend std::pair<PlanarDiagram_with_Unlinks, PlanarDiagram_with_Unlinks>
KauffmanSplit( cref<PlanarDiagram_with_Unlinks> p, const Int c )
{
    return p.pd.template KauffmanSplit<warningQ,verboseQ>(p.unlink_count, c);
}

template<bool warningQ = false, bool verboseQ = false>
std::pair<PlanarDiagram_with_Unlinks, PlanarDiagram_with_Unlinks>
KauffmanSplit( const Int c ) const
{
    return this->template KauffmanSplit<warningQ,verboseQ>(Int{0}, c);
}

template<bool warningQ = false, bool verboseQ = false>
std::pair<PlanarDiagram_with_Unlinks, PlanarDiagram_with_Unlinks>
KauffmanSplit( const Int unlink_count, const Int c, const bool compressQ ) const
{
    [[maybe_unused]] constexpr auto tag = ct_string("KauffmanSplit_impl");
    
    if constexpr (verboseQ )
    {
        logprint(MethodName(tag));
    }
    
    if( !CrossingActiveQ(c) )
    {
        if constexpr ( warningQ )
        {
            Msgr::wprint(tag, "Crossing ", CrossingString(c), " was already deactivated. Aborting");
        }
        return {
            {this->CachelessCopy(),unlink_count},
            {InvalidDiagram(),Int{0}}
        };
    }

/*        ###O----<----O       O---->----O###
 *               e_0    ^     ^    e_1
 *                       \   /
 *                        \ /
 *                       c X
 *                        / \
 *                       /   \
 *               e_2    /     \    e_3
 *        ###O---->----O       O----<----O###
 */
    
    const bool right_handedQ = this->CrossingRightHandedQ(c);
    
    const Int e_0 = this->C_arcs(c,Out,Left );
    const Int e_1 = this->C_arcs(c,Out,Right);
    const Int e_2 = this->C_arcs(c,In ,Left );
    const Int e_3 = this->C_arcs(c,In ,Right);

    PlanarDiagram_with_Unlinks p_0 { this->CachelessCopy(), unlink_count };
    PlanarDiagram_with_Unlinks p_1 { this->CachelessCopy(), unlink_count };
    
    p_0.pd.proven_minimalQ = false;
    p_1.pd.proven_minimalQ = false;
    
    if constexpr ( verboseQ )
    {
        logprint("*this = ");
        logprint(*this);
        TOOLS_LOGDUMP(this->CrossingString(c));
        TOOLS_LOGDUMP(this->ArcString(e_0));
        TOOLS_LOGDUMP(this->ArcString(e_1));
        TOOLS_LOGDUMP(this->ArcString(e_2));
        TOOLS_LOGDUMP(this->ArcString(e_3));
    }
    
    auto create_result = [tag,compressQ,right_handedQ,&p_0,&p_1]()
    {
        if( compressQ )
        {
            p_0.pd.template Compress_Private<true>();
            p_1.pd.template Compress_Private<true>();
        }
        else
        {
            p_0.pd.ComputeArcColors_Private();
            p_1.pd.ComputeArcColors_Private();
        }
        
        if( !right_handedQ )
        {
            using std::swap;
            swap(p_0.pd,p_1.pd);
            swap(p_0.unlink_count,p_1.unlink_count);
        }
        
        if( !p_0.pd.ValidQ() ) { Msgr::eprint(tag,"p_0 invalid"); }
        if( !p_1.pd.ValidQ() ) { Msgr::eprint(tag,"p_1 invalid"); }
        
        if constexpr ( verboseQ )
        {
            logprint("");
            TOOLS_LOGDUMP(p_0.unlink_count);
            logprint("p_0.pd");
            logprint(p_0.pd);
            
            TOOLS_LOGDUMP(p_1.unlink_count);
            logprint("p_1.pd");
            logprint(p_1.pd);
            
        }
        
        return std::pair{p_0, p_1};
    };
    
    if( (e_0 == e_2) && (e_1 == e_3) )
    {
        if constexpr ( verboseQ )
        {
            logprint("\nCase: (e_0 == e_2) && (e_1 == e_3)");
        }
        // farfalla at crossing `c`.
        
        p_0.pd.DeactivateArc(e_0);
        p_0.pd.DeactivateArc(e_1);
        p_0.pd.DeactivateCrossing(c);
        p_0.unlink_count += Int{1} + Int{p_0.pd.CrossingCount() > Int{0}};
        
        p_1.pd.DeactivateArc(e_0);
        p_1.pd.DeactivateArc(e_1);
        p_1.pd.DeactivateCrossing(c);
        p_1.unlink_count += Int{p_1.pd.CrossingCount() > Int{0}};
        
        return create_result();
    }
    
    if( e_0 == e_2 )
    {
        if constexpr ( verboseQ )
        {
            logprint("\nCase: (e_0 == e_2) && (e_1 != e_3)");
        }
        
        /*           +----<----O       O---->----O###
         *           |   e_0    ^     ^    e_1
         *           |           \   /
         *           |            \ /
         *           |           c X
         *           |            / \
         *           |           /   \
         *           |   e_2    /     \    e_3
         *           +---->----O       O----<----O###
         */
        
        p_0.pd.template Reconnect<Head>(e_3,e_1);
        p_0.pd.DeactivateArc(e_0);
        p_0.pd.DeactivateCrossing(c);
        p_0.unlink_count += Int{1};
        
        p_1.pd.template Reconnect<Head>(e_3,e_1);
        p_1.pd.DeactivateArc(e_0);
        p_1.pd.DeactivateCrossing(c);
        
        return create_result();
    }
    
    if( e_1 == e_3 )
    {
        if constexpr ( verboseQ )
        {
            logprint("\nCase: (e_0 != e_2) && (e_1 == e_3)");
        }
        
        /*        ###O----<----O       O---->----+
         *               e_0    ^     ^    e_1   |
         *                       \   /           |
         *                        \ /            |
         *                       c X             |
         *                        / \            |
         *                       /   \           |
         *               e_2    /     \    e_3   |
         *        ###O---->----O       O----<----+
         */
        
        p_0.pd.template Reconnect<Head>(e_2,e_0);
        p_0.pd.DeactivateArc(e_1);
        p_0.pd.DeactivateCrossing(c);
        p_0.unlink_count += Int{1};
        
        p_1.pd.template Reconnect<Head>(e_2,e_0);
        p_1.pd.DeactivateArc(e_1);
        p_1.pd.DeactivateCrossing(c);

        return create_result();
    }

    if constexpr ( verboseQ )
    {
        logprint("\nCase: (e_0 != e_2) && (e_1 != e_3) (generic case)");
    }
    
    // TODO: It might be better to start with a = e_1. How can we decided this efficiently?
    Int a = e_0;
    
    while( (a != e_2) && (a != e_3) )
    {
        const Int c_0 = p_1.pd.A_cross(a,Tail);
        const Int c_1 = p_1.pd.A_cross(a,Head);
        
        if( c_1 == c_0 )
        {
            // This is a problem: when a is a loop edge then it may happen that (p_1.pd.C_arcs(c_1,In,Left) == a) && (p_1.pd.C_arcs(c_1,In,Right) == a). So we might get the wrong side here! That why we never allow `a` to be a loop arc. Instead we catch whether `a_1` is a loop arc, process it, and then jump over it. Tha
            
            Msgr::eprint(tag,"Current arc is loop arc. This should never happen");
        }
        
        const bool side = (p_1.pd.C_arcs(c_1,In,Right) == a);
        
        if( p_1.pd.C_arcs(c_1,In,side) != a )
        {
            Msgr::error(tag,"p_1.pd.C_arcs(c_1,In,side) != a. This should never happen.");
        }
        
        const auto C = p_1.pd.CopyCrossing(c_1);
        
        const Int a_1 = p_1.pd.C_arcs(c_1,Out,!side);
        const Int c_2 = p_1.pd.A_cross(a_1,Head);
        
        if constexpr ( verboseQ )
        {
            logvalprint("a (before)",p_1.pd.ArcString(a));
            logvalprint("c_1 (before)",p_1.pd.CrossingString(c_1));
            logvalprint("a_1 (before)",p_1.pd.ArcString(a_1));
            logvalprint("c_2 (before)",p_1.pd.CrossingString(c_2));
        }

        if( c_2 == c_1 )
        {
            const Int a_2 = p_1.pd.C_arcs(c_1,Out,side);
            
            if constexpr ( verboseQ )
            {
                Msgr::wprint(tag,"Next arc is loop arc. We preprocess and then skip it.");
                logvalprint("a_2 (before)",p_1.pd.ArcString(a_2));
            }
            if( a_2 == a )
            {
                Msgr::eprint(tag,"a_2 == a. This should never happen.");
            }
            
            /* side == Right; right-handed.
             *
             *  a_1 +---O       O a_2   a_1 +-->O       O a_2
             *      |    ^     ^            |    \     /
             *      |     \   /             |     \   /
             *      |      \ /              |      \ /
             *      |       / c_1    ==>    |       / c_1
             *      |      / \              |      / \
             *      |     /   \             |     /   \
             *      |    /     \            |    v     v
             *      +-->O       O a         +---O       O a
             */
            
            p_1.pd.C_arcs(c_1,Out,Left ) = C[In ][Right];
            p_1.pd.C_arcs(c_1,Out,Right) = C[In ][Left ];
            p_1.pd.C_arcs(c_1,In ,Left ) = C[Out][Right];
            p_1.pd.C_arcs(c_1,In ,Right) = C[Out][Left ];
            
            // a_1 is loop arc and need not be reversed.
            // c_1 keeps its handedness.
            p_1.pd.A_cross(a,Tail) = c_1;
            p_1.pd.A_cross(a,Head) = c_0;
            
            if constexpr ( verboseQ )
            {
                logvalprint("a (after)",p_1.pd.ArcString(a));
                logvalprint("c_1 (after)",p_1.pd.CrossingString(c_1));
                logvalprint("a_1 (after)",p_1.pd.ArcString(a_1));
            }
            a = a_2;
            continue;
        }


        /* side == Right
         *
         *     a_1  O       O       a_1 O       O
         *           ^     ^             \     ^
         *            \   /               \   /
         *             \ /                 \ /
         *              X c_1     ==>       X c_1
         *             / \                 / \
         *            /   \               /   \
         *           /     \             /     v
         *          O       O a         O       O a
         */
        
        p_1.pd.C_arcs(c_1,Out,Left ) = C[!side][ side];
        p_1.pd.C_arcs(c_1,Out,Right) = C[ side][ side];
        p_1.pd.C_arcs(c_1,In ,Left ) = C[!side][!side];
        p_1.pd.C_arcs(c_1,In ,Right) = C[ side][!side];
        
        p_1.pd.template SwitchCrossing_Private<warningQ>(c_1);
        
//        // DEBUGGING
//        logprint("switched crossing " + ToString(c_1));
        
        p_1.pd.A_cross(a,Tail) = c_1;
        p_1.pd.A_cross(a,Head) = c_0;
        
        if constexpr ( verboseQ )
        {
            logvalprint("a (after)",p_1.pd.ArcString(a));
            logvalprint("c_1 (after)",p_1.pd.CrossingString(c_1));
            logvalprint("a_1 (after)",p_1.pd.ArcString(a_1));
        }
        
        a = a_1;
        
    } // while( (a != e_2) && (a != e_3) )

    // Now a == e_2 or or a == e_3.
    // This arc will be deactivated anyways, but we have to turn it around to make `Reconnect` work correctly.
    {
        const Int c_0 = p_1.pd.A_cross(a,Tail);
        const Int c_1 = p_1.pd.A_cross(a,Head);
        
        if constexpr ( verboseQ )
        {
            logvalprint("a",p_1.pd.ArcString(a));
            logvalprint("c_0",p_1.pd.CrossingString(c_0));
            logvalprint("c_1",p_1.pd.CrossingString(c_1));
        }
        
        p_1.pd.A_cross(a,Tail) = c_1;
        p_1.pd.A_cross(a,Head) = c_0;
    }
    
    const Int b = (a == e_2) ? e_3 : e_2;
    
    if constexpr ( verboseQ )
    {
        logvalprint("a", p_1.pd.ArcString(a));
        logvalprint("b", p_1.pd.ArcString(b));
        logvalprint("e_0", p_1.pd.ArcString(e_0));
        logvalprint("e_1", p_1.pd.ArcString(e_1));
    }
    
    p_1.pd.template Reconnect<Head>(e_0,e_1);
    p_1.pd.template Reconnect<Head>(b,a);
    p_1.pd.DeactivateArc(e_1);
    p_1.pd.DeactivateArc(a);
    p_1.pd.DeactivateCrossing(c);
    
    if constexpr ( verboseQ )
    {
        logvalprint("a  ", p_1.pd.ArcString(a));
        logvalprint("b  ", p_1.pd.ArcString(b));
        logvalprint("e_0", p_1.pd.ArcString(e_0));
        logvalprint("e_1", p_1.pd.ArcString(e_1));
    }
    
    p_0.pd.template ResolveCrossing_Private<warningQ>(c);
    
    return create_result();
}
