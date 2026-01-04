public:

// Caution: This assumes that PD is freshly ordered! Check first that CompressedOrderQ() returns true.
template<bool nontrivialQ = true>
bool VerticalSummandQ( const Int a, const Int b, const bool overQ )
{
    bool goodQ = (ModDistance(arc_count,a,b) > Int(1));

    
    if constexpr ( nontrivialQ )
    {
        goodQ = goodQ && (overQ != ArcOverQ<Tail>(a)) && (overQ != ArcOverQ<Head>(b));

        if( goodQ )
        {   // Tail of a should cross with complement of [a,b[.
            const Int i = NextLeftArc (a,Tail).first;
            const Int j = NextRightArc(a,Tail).first;
            goodQ = !InIntervalQ(i,a,b) && !InIntervalQ(j,a,b);
        }
        if( goodQ )
        {   // Head of a should cross with [a,b[.
            const Int i = NextLeftArc (a,Head).first;
            const Int j = NextRightArc(a,Head).first;
            goodQ = InIntervalQ(i,a,b) && InIntervalQ(j,a,b);
        }
        if( goodQ )
        {   // Tail of b should cross with [a,b[.
            const Int i = NextLeftArc (b,Tail).first;
            const Int j = NextRightArc(b,Tail).first;
            goodQ = InIntervalQ(i,a,b) && InIntervalQ(j,a,b);
        }
        if( goodQ )
        {   // Head of b should cross with complement of [a,b[.
            const Int i = NextLeftArc (b,Head).first;
            const Int j = NextRightArc(b,Head).first;
            goodQ = !InIntervalQ(i,a,b) && !InIntervalQ(j,a,b);
        }
    }
    
    Int selfcrossingcount = 0;
    
    Int arc = a;
    
    while( goodQ && (arc < b) )
    {
        const Int c = A_cross(arc,Head);
        
        Int i = C_arcs(c,In,Left);
        Int j = C_arcs(c,In,Right);
        
        int s = ( overQ ? int(1) : -int(1) ) * ToUnderlying(C_state[c]);
        
        // Make sure that i == arc.
        if( i != arc )
        {
            std::swap(i,j);
            s = -s;
        }
        
        if( InIntervalQ(j,a,b) )
        {
            ++selfcrossingcount;
        }
        else
        {
            goodQ = goodQ && (s > int(0));
        }
        
        ++arc;
    }
    
    if constexpr( nontrivialQ )
    {
        goodQ = goodQ && (selfcrossingcount > Int(0)) && (selfcrossingcount < crossing_count);
    }

    return goodQ;
}
