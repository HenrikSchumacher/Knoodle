// CAUTION: The methods here may perform topological changes.
// CAUTION: Therefore ought to be considered UNSAFE.

public:

/*! @brief Checks whether crossing `c` is active. In the affirmative case it flips the handedness of that crossing, clears the internal cache, and returns `true`. Otherwise, this routine just returns `false` (keeping the internal cache as it was).
 * _Use this with extreme caution as this might invalidate some invariants of the PlanarDiagram class._ _Never _ use it in productive code unless you really, really know what you are doing! This feature is highly experimental and we expose it only for debugging purposes and for experiments.
 *
 * @param c The crossing to be switched.
 *
 * @tparam silentQ If set to `true`, then warings are suppressed whenever `c` is deactivated. Default is `false`.
 */

template<bool silentQ = false>
bool SwitchCrossing( const Int c )
{
    bool changedQ = this->template Private_SwitchCrossing<silentQ>(c);
    
    if( changedQ ) { this->ClearCache(); }
    
    return changedQ;
}


private:

/*! @brief Checks whether crossing `c` is active. In the affirmative case it flips the handedness of that crossing and returns `true`. Otherwise, this routine just returns `false` (keeping the internal cache as it was).
 *
 * @param c The crossing to be switched.
 *
 * @tparam silentQ If set to `true`, then warings are suppressed whenever `c` is deactivated.
 */

template<bool silentQ>
bool Private_SwitchCrossing( const Int c )
{
    if( !ActiveQ(C_state[c]) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Private_SwitchCrossing")+": Crossing " + CrossingString(c) + " was already deactivated. Doing nothing.");
        }
        return false;
    }
    else
    {
        C_state[c] = Switch(C_state[c]);
        return true;
    }
}

public:

std::tuple<Int,Int,Int> CreateCrossing(
    const CrossingState_T handedness = CrossingState_T::RightHanded
)
{
    RequireCrossingCount( CrossingCount() + Int(1) );
    
    const Int c     = NextInactiveCrossing();
    C_state[c]      = handedness;
    crossing_count += 1;
    
    const Int a     = NextInactiveArc();
    A_state[a]      = ArcState_T::Active;
    A_color[a]      = Uninitialized;
    arc_count      += 1;
    
    const Int b     = NextInactiveArc();
    A_state[b]      = ArcState_T::Active;
    A_color[b]      = Uninitialized;
    arc_count      += 1;
    
    return {c,a,b};
}

/*!@brief Creates a new loop on an arc. Returns the index of the new loop arc if successful. Returns `Uninitialized` otherwise.
 *
 * @param a The arc to which we want to attach the loop
 *
 * @param side The side of the arc to which we want to attach the loop.
 *
 * @param handedness The handedness for the newly created crossing.
 */

template<bool silentQ = false, bool assertsQ = true>
Int CreateLoop( const Int a, const bool side, CrossingState_T handedness )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("CreateLoop"); };
    
    TOOLS_PTIMER(timer,MethodName("CreateLoop"));
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Diagram is invalid. Doing nothing.");
        }
        return Uninitialized;
    }
    
    if( handedness == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Supplied handedness is `Inactive'. Doing nothing.");
        }
        return Uninitialized;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return Uninitialized;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return Uninitialized;
    }
    
    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }
    
    RequireCrossingCount(CrossingCount()+Int(1));
    
    auto [c,a_prev,a_next] = CreateCrossing(handedness);
    
    A_color[a_prev]  = A_color[a];
    A_color[a_next]  = A_color[a];

    const Int c_0 = A_cross(a,Tail);
    const Int c_1 = A_cross(a,Head);
    
    A_cross(a,Tail) = c;
    A_cross(a,Head) = c;

    A_cross(a_prev,Tail) = c_0;
    A_cross(a_prev,Head) = c;
    A_cross(a_next,Tail) = c;
    A_cross(a_next,Head) = c_1;
    
    C_arcs(c,Out,!side) = a_next;
    C_arcs(c,Out, side) = a;
    C_arcs(c,In ,!side) = a_prev;
    C_arcs(c,In , side) = a;
    
    SetMatchingPortTo<Out>(c_0,a,a_prev);
    SetMatchingPortTo<In >(c_1,a,a_next);

    ClearCache();
    
    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }
    
    return a;
}





template<bool silentQ = false, bool assertsQ = true>
bool Connect( const Int a, const Int b )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Connect"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": Diagram is invalid. Doing nothing.");
        }
        return false;
    }

    if( a == b )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": Arc indices a = " + ToString(a) + " and b = " + ToString(b) + " coincide. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": Index a = " + ToString(b) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": " + ArcString(b) + " is inactive. Doing nothing.");
        }
        return false;
    }
    
    if( A_color[a] != A_color[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": Colors of " + ArcString(a) + " and " + ArcString(b) + " do not match. Doing nothing.");
        }
        return false;
    }

    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }

    // TODO: This is an expensive check. Make it optional.
    
    if( ArcLinkComponents()[a] == ArcLinkComponents()[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag()+": " + ArcString(a) + " and " + ArcString(b) + " lie on the same link component. Doing nothing.");
        }
        return false;
    }
    
    const Int c_a = A_cross(a,Head);
    const Int c_b = A_cross(b,Head);
    
    A_cross(a,Head) = c_b;
    A_cross(b,Head) = c_a;
    
    SetMatchingPortTo<In>(c_a,a,b);
    SetMatchingPortTo<In>(c_b,b,a);

    ClearCache();
    
    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }
    
    return true;
}


// TODO: This is a combination of CreateLoop and Connect. Recode it.
template<bool silentQ = false, bool assertsQ = true>
bool ConnectWithIsthmus(
    const Int a, const Int b,
    CrossingState_T handedness = CrossingState_T::RightHanded
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ConnectWithIsthmus"); };
    
    TOOLS_PTIMER(timer,tag());
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Diagram is invalid. Doing nothing.");
        }
        return false;
    }
    
    if( handedness == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Supplied handedness is `Inactive'. Doing nothing.");
        }
        return false;
    }

    if( a == b )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Arc indices a = " + ToString(a) + " and b = " + ToString(b) + " coincide. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Index a = " + ToString(b) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(b) + " is inactive. Doing nothing.");
        }
        return false;
    }
    
    if( A_color[a] != A_color[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Colors of " + ArcString(a) + " and " + ArcString(b) + " do not match. Doing nothing.");
        }
        return false;
    }
    
    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }
    
    if( ArcLinkComponents()[a] == ArcLinkComponents()[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(a) + " and " + ArcString(b) + " lie on the same link component. Doing nothing.");
        }
        return false;
    }
    RequireCrossingCount(CrossingCount()+Int(1));
    
    auto [c,a_next,b_next] = CreateCrossing(handedness);
    A_color[a_next]  = A_color[a];
    A_color[b_next]  = A_color[b];

    const Int c_a = A_cross(a,Head);
    const Int c_b = A_cross(b,Head);
    
    A_cross(a     ,Head) = c;
    A_cross(b     ,Head) = c;
    A_cross(a_next,Tail) = c;
    A_cross(b_next,Tail) = c;
    A_cross(a_next,Head) = c_b;
    A_cross(b_next,Head) = c_a;
    
    C_arcs(c,Out,Left ) = b_next;
    C_arcs(c,Out,Right) = a_next;
    C_arcs(c,In ,Left ) = a;
    C_arcs(c,In ,Right) = b;
    
    SetMatchingPortTo<In>(c_a,a,b_next);
    SetMatchingPortTo<In>(c_b,b,a_next);

    ClearCache();
    
    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }
    
    return true;
}


// TODO: This is a combination of 2x CreateLoop and Connect. Recode it.
template<bool silentQ = false, bool assertsQ = true>
bool ConnectWithDoubleIsthmus(
    const Int a, const Int b,
    CrossingState_T handedness_0 = CrossingState_T::RightHanded,
    CrossingState_T handedness_1 = CrossingState_T::RightHanded
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ConnectWithDoubleIsthmus"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Diagram is invalid. Doing nothing.");
        }
        return false;
    }
    
    if( handedness_0 == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Supplied handedness_0 is `Inactive'. Doing nothing.");
        }
        return false;
    }
    
    if( handedness_1 == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Supplied handedness_1 is `Inactive'. Doing nothing.");
        }
        return false;
    }

    if( a == b )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Arc indices a = " + ToString(a) + " and b = " + ToString(b) + " coincide. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Index a = " + ToString(b) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(b) + " is inactive. Doing nothing.");
        }
        return false;
    }
    
    if( A_color[a] != A_color[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": Colors of " + ArcString(a) + " and " + ArcString(b) + " do not match. Doing nothing.");
        }
        return false;
    }
    
    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }
    
    if( ArcLinkComponents()[a] == ArcLinkComponents()[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(tag() + ": " + ArcString(a) + " and " + ArcString(b) + " lie on the same link component. Doing nothing.");
        }
        return false;
    }

    RequireCrossingCount(CrossingCount()+Int(2));
    
    auto [c_0,a_prev,b_prev] = CreateCrossing(handedness_0);
    A_color[b_prev]  = A_color[a];
    A_color[a_prev]  = A_color[b];
    
    auto [c_1,a_next,b_next] = CreateCrossing(handedness_1);
    A_color[a_next]  = A_color[a];
    A_color[b_next]  = A_color[b];
    
    A_cross(a_prev,Tail) = A_cross(a,Tail);
    A_cross(a_prev,Head) = c_0;
    A_cross(b_prev,Tail) = A_cross(b,Tail);
    A_cross(b_prev,Head) = c_1;
    A_cross(a_next,Tail) = c_1;
    A_cross(a_next,Head) = A_cross(b,Head);
    A_cross(b_next,Tail) = c_0;
    A_cross(b_next,Head) = A_cross(a,Head);
    
    SetMatchingPortTo<Out>(A_cross(a,Tail),a,a_prev);
    SetMatchingPortTo<Out>(A_cross(b,Tail),b,b_prev);
    SetMatchingPortTo<In >(A_cross(a,Head),a,b_next);
    SetMatchingPortTo<In >(A_cross(b,Head),b,a_next);
    
    A_cross(a,Tail) = c_0;
    A_cross(a,Head) = c_1;
    A_cross(b,Tail) = c_1;
    A_cross(b,Head) = c_0;
    
    C_arcs(c_0,Out,Left ) = a;
    C_arcs(c_0,Out,Right) = b_next;
    C_arcs(c_0,In ,Left ) = b;
    C_arcs(c_0,In ,Right) = a_prev;
    
    C_arcs(c_1,Out,Left ) = b;
    C_arcs(c_1,Out,Right) = a_next;
    C_arcs(c_1,In ,Left ) = a;
    C_arcs(c_1,In ,Right) = b_prev;

    if constexpr ( assertsQ ) { PD_ASSERT(CheckAll()); }

    return true;
}
