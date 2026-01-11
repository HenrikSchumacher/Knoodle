private:

std::string ArcRangeString( const Int a_begin, const Int a_end ) const
{
    std::string s;

    Int a = a_begin;
    Int i = 0;
    
    s += ToString(0) + " : " + CrossingString(A_cross(a,Tail)) + ")\n"
       + ToString(0) + " : " + ArcString(a)
       + " (" + (pd.ArcOverQ(a,Head) ? "over" : "under") + ")\n"
    + ToString(1) + " : " + CrossingString(A_cross(a,Head)) + "\n";

    do
    {
        ++i;
        a = pd.NextArc(a,Head);
        s += ToString(i  ) + " : " +  ArcString(a)
           + " (" + (pd.ArcOverQ(a,Head) ? "over" : "under") + ")\n"
            + ToString(i+1) + " : " +  CrossingString(A_cross(a,Head)) + "\n";
    }
    while( a != a_end );
    
    return s;
}

std::string PathString()
{
    if( path_length <= 0 )
    {
        return "{}";
    }
    
    std::string s;
    
    for( Int p = 0; p < path_length; ++p )
    {
        const Int a = path[p];

        s += ToString(p  ) + " : " +  ArcString(a)
           + " (" + ToString( Sign(D_mark(a)) ) + ")\n";
    }
    
    return s;
}
