public:

struct Pass_T
{
    Int  first     = Uninitialized;
    Int  last      = Uninitialized;
    Int  next      = Uninitialized; // Most be set when FindPass is done with pass.activeQ == true. Otherwise, no guarantees.
    Int  arc_count = 0;
    Int  mark      = Uninitialized;
    bool overQ     = false;
    bool activeQ   = false;
        
    /*
     *      |        |        |
     *      |  last  |  next  |
     * ------------->X------->X------->
     *      |c_0     |c_1     |c_2
     *      |        |        |
     */
    
public:
    
    Int CrossingCount() const
    {
        return (arc_count >= Int(1)) ? arc_count - Int(1) : Int(0);
    }
    
    Int ArcCount() const
    {
        return arc_count;
    }
    
    void SetInvalid()
    {
        *this = Pass_T();
    }
    
    bool ValidQ() const
    {
        return activeQ;
    }
    
    friend std::string ToString( cref<Pass_T> pass )
    {
        return std::string("{ ")
            +   "first = " + ToString(pass.first)
            + ", last = " + ToString(pass.last)
            + ", next = " + ToString(pass.next)
            + ", arc_count = " + ToString(pass.arc_count)
            + ", mark = " + ToString(pass.mark)
            + ", overQ = " + ToString(pass.overQ)
            + ", activeQ = " + ToString(pass.activeQ)
            + " }";
    }
};

std::string PassDetails( cref<Pass_T> pass ) const
{
    return ArcRangeString(pass.first,pass.last);
}

std::string PassString( cref<Pass_T> pass ) const
{
    return ShortArcRangeString(pass.first,pass.last);
}

Tensor1<Int,Int> PassToArray( cref<Pass_T> pass ) const
{
    Tensor1<Int,Int> p ( pass.arc_count );
    
    Int e = pass.first;
    Int i = 0;
    
    if( pass.first != pass.last )
    {
        do
        {
            ++i;
            e = NextArc(e,Head);
            p[i] = e;
        }
        while( (e != pass.last) && (i <= pass.arc_count) );
    }
    
    if( i > pass.arc_count)
    {
        wprint(MethodName("PassToArray") + ": pass.first = " + ArcString(pass.first) + " and pass.last" + ArcString(pass.last) + " do not lie on the same link component.");
        return "Failed.";
    }
    else
    {
        return Tensor1<Int,Int>();
    }
    
    return p;
}
