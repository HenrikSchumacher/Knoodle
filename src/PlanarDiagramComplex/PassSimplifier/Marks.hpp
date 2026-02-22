private:

static constexpr Int  max_mark = static_cast<Int>(std::numeric_limits<Int>::max() >> Int(2));

static constexpr bool LeftToRight = 1;
static constexpr bool RightToLeft = 0;

//DEBUGGING
public:

void MarkCrossing( const Int c, const Int mark )
{
    C_mark(c) = mark;
}
void MarkCrossing( const Int c )
{
    MarkCrossing( c, current_mark );
}

Int CrossingMark( const Int c ) const
{
    return C_mark(c);
}

bool CrossingMarkedQ( const Int c ) const
{
    return C_mark(c) == current_mark;
}

void MarkArc( const Int a, const Int mark )
{
    A_mark(a) = mark;
}

void MarkArc( const Int a )
{
    MarkArc(a, current_mark );
}

bool ArcMarkedQ( const Int a ) const
{
    return (A_mark(a) == current_mark);
}

bool ArcRecentlyMarkedQ( const Int a ) const
{
    return (A_mark(a) >= initial_mark);
}


Int ArcMark( const Int a ) const
{
    return A_mark(a);
}

public:

// TODO: Need a guard against a and b lying on different link components.
Int MarkArcs(const Int a, const Int b, const Int mark )
{
    const Int e_begin = a;
    const Int e_end   = NextArc(b,Head);
    Int e = a;
    
    Int counter = 0;
    
    do
    {
        ++counter;
        MarkArc(e,mark);
        e = NextArc(e,Head);
    }
    while( (e != e_end) && (e != e_begin) );
    
    return counter;
}

Int MarkArcs(const Int a, const Int b )
{
    return MarkArcs(a,b,current_mark);
}


// TODO: Need a guard against a and b lying on different link components.
Int CountArcsInRange(const Int a, const Int b )
{
    const Int e_begin = a;
    const Int e_end   = NextArc(b,Head);
    Int e = a;

    Int counter = 0;
    
    do
    {
        ++counter;
        e = NextArc(e,Head);
    }
    while( (e != e_end) && (e != e_begin) );
    
    return counter;
}
