template<bool silentQ = false>
bool Connect( const Int a, const Int b )
{
    TOOLS_PTIMER(timer,MethodName("Connect"));
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": Diagram is invalid. Doing nothing.");
        }
        return false;
    }

    if( a == b )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": Arc indices a = " + ToString(a) + " and b = " + ToString(b) + " coincide. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": Index a = " + ToString(b) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": " + ArcString(b) + " is inactive. Doing nothing.");
        }
        return false;
    }
    
    if( A_color[a] != A_color[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": Colors of " + ArcString(a) + " and " + ArcString(b) + " do not match. Doing nothing.");
        }
        return false;
    }
    
    if( ArcLinkComponents()[a] == ArcLinkComponents()[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("Connect") + ": " + ArcString(a) + " and " + ArcString(b) + " lie on the same link component. Doing nothing.");
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
    
    return true;
}

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

template<bool silentQ = false>
bool ConnectWithIsthmus(
    const Int a, const Int b,
    CrossingState_T handedness = CrossingState_T::RightHanded
)
{
    TOOLS_PTIMER(timer,MethodName("ConnectWithIsthmus"));
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Diagram is invalid. Doing nothing.");
        }
        return false;
    }
    
    if( handedness == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Supplied handedness is `Inactive'. Doing nothing.");
        }
        return false;
    }

    if( a == b )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Arc indices a = " + ToString(a) + " and b = " + ToString(b) + " coincide. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Index a = " + ToString(b) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": " + ArcString(b) + " is inactive. Doing nothing.");
        }
        return false;
    }
    
    if( A_color[a] != A_color[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Colors of " + ArcString(a) + " and " + ArcString(b) + " do not match. Doing nothing.");
        }
        return false;
    }
    
    if( ArcLinkComponents()[a] == ArcLinkComponents()[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": " + ArcString(a) + " and " + ArcString(b) + " lie on the same link component. Doing nothing.");
        }
        return false;
    }
    RequireCrossingCount(CrossingCount()+Int(1),false);
    
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
    
    return true;
}

template<bool silentQ = false>
bool ConnectWithDoubleIsthmus(
    const Int a, const Int b,
    CrossingState_T handedness_0 = CrossingState_T::RightHanded,
    CrossingState_T handedness_1 = CrossingState_T::RightHanded
)
{
    TOOLS_PTIMER(timer,MethodName("ConnectWithDoubleIsthmus"));
    
    if( InvalidQ() )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Diagram is invalid. Doing nothing.");
        }
        return false;
    }
    
    if( handedness_0 == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Supplied handedness_0 is `Inactive'. Doing nothing.");
        }
        return false;
    }
    
    if( handedness_1 == CrossingState_T::Inactive )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Supplied handedness_1 is `Inactive'. Doing nothing.");
        }
        return false;
    }

    if( a == b )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Arc indices a = " + ToString(a) + " and b = " + ToString(b) + " coincide. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Index a = " + ToString(a) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(a) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": " + ArcString(a) + " is not active. Doing nothing.");
        }
        return false;
    }
    
    if( !ValidIndexQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Index a = " + ToString(b) + " is out of bounds. Doing nothing.");
        }
        return false;
    }
    
    if( !ArcActiveQ(b) )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": " + ArcString(b) + " is inactive. Doing nothing.");
        }
        return false;
    }
    
    if( A_color[a] != A_color[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": Colors of " + ArcString(a) + " and " + ArcString(b) + " do not match. Doing nothing.");
        }
        return false;
    }
    
    if( ArcLinkComponents()[a] == ArcLinkComponents()[b] )
    {
        if constexpr ( !silentQ )
        {
            wprint(MethodName("ConnectWithIsthmus") + ": " + ArcString(a) + " and " + ArcString(b) + " lie on the same link component. Doing nothing.");
        }
        return false;
    }

    RequireCrossingCount(CrossingCount()+Int(2),false);
    
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

    ClearCache();

    return true;
}
