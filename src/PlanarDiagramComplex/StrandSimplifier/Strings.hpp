private:

static std::string OverQString( const bool overQ_ )
{
    return (overQ_ ? "over" : "under");
}

std::string OverQString() const
{
    return OverQString(overQ);
}


/*!@brief Creates string with the details of the range of arcs from `a` (included) to `b` (included). Caution: It is assumed implicitly that `a` and `b` lie on the same link component.
 */
std::string ArcRangeString( const Int a, const Int b ) const
{
    PD_PRINT(MethodName("ArcRangeString"));


    std::string s;

    Int e = a;
    Int i = 0;
    
    s += ToString(0) + " : " + CrossingString(pd->A_cross(e,Tail)) + "\n\t"
       + ToString(0) + " : " + ArcString(e)
       + " (" + OverQString(pd->ArcOverQ(e,Head)) + ")\n"
    + ToString(1) + " : " + CrossingString(pd->A_cross(e,Head)) + "\n\t";

    if( a != b )
    {
        do
        {
            ++i;
            e = pd->NextArc(e,Head);
            s += ToString(i  ) + " : " +  ArcString(e)
            + " (" + OverQString(pd->ArcOverQ(e,Head)) + ")\n"
            + ToString(i+1) + " : " +  CrossingString(pd->A_cross(e,Head)) + "\n\t";
        }
        while( (e != b) && (i <= pd->arc_count) );
    }
    
    if( i > pd->arc_count)
    {
        wprint(MethodName("ArcRangeString") + ": " + ArcString(a) + " and " + ArcString(b) + " do not lie on the same link component.");
        return "Failed.";
    }
    else
    {
        return s;
    }
}

std::string ShortArcRangeString( const Int a, const Int b ) const
{
//    PD_PRINT(MethodName("ShortArcRangeString"));
    
    std::string s;
    Int e = a;
    Int i = 0;
    
    s += "{ " + ToString(e);
    
    if( a != b )
    {
        do
        {
            ++i;
            e = NextArc(e,Head);
            s += ", " +  ToString(e);
        }
        while( (e != b) && (i <= pd->arc_count) );
    }
    
    s += " }";
    
    if( i > pd->arc_count)
    {
        wprint(MethodName("ShortArcRangeString") + ": " + ArcString(a) + " and " + ArcString(b) + " do not lie on the same link component.");
        return "Failed.";
    }
    else
    {
        return s;
    }
}


std::string PathString()
{
//    PD_PRINT(MethodName("PathString"));
    
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

std::string ShortPathString()
{
//    PD_PRINT(MethodName("ShortPathString"));
    
    return ArrayToString(&path[0],{path_length});
}
