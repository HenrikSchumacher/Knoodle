private:

static constexpr Int  max_mark = std::numeric_limits<Int>::max()/Int(2) - Int(1);
static constexpr bool LeftToRight = 1;
static constexpr bool RightToLeft = 0;

void NewMark()
{
    ++current_mark;
}

void MarkCrossing( const Int c )
{
    C_mark(c) = current_mark;
}

Int CrossingMark( const Int c ) const
{
    return C_mark(c);
}

bool CrossingMarkedQ( const Int c ) const
{
    return C_mark(c) == current_mark;
}

void MarkArc( const Int a )
{
    A_mark(a) = current_mark;
}

bool ArcMarkedQ( const Int a ) const
{
    return (A_mark(a) == current_mark);
}

Int ArcMark( const Int a ) const
{
    return A_mark(a);
}

void SetDualArc(
    const Int a, const bool forwardQ, const Int from, const bool left_to_rightQ )
{
    D_data(a,0) = (current_mark << Int(1)) | Int(forwardQ);
    D_data(a,1) = (from << Int(1))         | Int(left_to_rightQ);
    // CAUTION: D_data(a,1) is not the darc we came from! It is the _arc_ we came from + the directions of da when we travelled through it!
}

bool DualArcMarkedQ( const Int a )
{
    return ((D_data(a,0) >> Int(1)) == current_mark);
}

bool DualArcForwardQ( const Int a )
{
    return static_cast<bool>(D_data(a,0) & Int(1));
}

Int DualArcFrom( const Int a )
{
    return (D_data(a,1) >> Int(1));
}

bool DualArcLeftToRightQ( const Int a )
{
    return static_cast<bool>(D_data(a,1) & Int(1));
}

public:

Int MarkArcs(const Int a, const Int b )
{
    const Int e_begin = a;
    const Int e_end   = NextArc(b,Head);
    Int e = a;
    
    Int counter = 0;
    
    do
    {
        ++counter;
        MarkArc(e);
        e = NextArc(e,Head);
    }
    while( (e != e_end) && (e != e_begin) );
    
    return counter;
}
