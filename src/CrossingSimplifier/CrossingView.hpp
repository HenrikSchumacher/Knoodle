
public:

class CrossingView
{
    Int c;
    mptr<Int> C;
    CrossingState & S;
    
public:
    
    CrossingView( const Int c_, mptr<Int> C_, CrossingState & S_ )
    : c { c_ }
    , C { C_ }
    , S { S_ }
    {}

    ~CrossingView() = default;
    
    // TODO: Value semantics.
    
    Int Idx() const
    {
        return c;
    }
    
    CrossingState & State()
    {
        return S;
    }
    
    const CrossingState & State() const
    {
        return S;
    }
    
    Int & operator()( const bool io, const bool side )
    {
        return C[2 * io + side];
    }
    
    const Int & operator()( const bool io, const bool side ) const
    {
        return C[2 * io + side];
    }
    
    bool ActiveQ() const
    {
        return (ToUnderlying(S) % 2);
    }
    
    inline friend bool operator==( const CrossingView & C_0, const CrossingView & C_1 )
    {
        return C_0.c == C_1.c;
    }
    
    inline friend bool operator==( const CrossingView & C_0, const Int c_1 )
    {
        return C_0.c == c_1;
    }
    
    inline friend bool operator==( const Int c_0, const CrossingView & C_1 )
    {
        return c_0 == C_1.c;
    }
    
    std::string String() const
    {
        return "crossing " + Tools::ToString(c) +" = { { "
        + Tools::ToString(C[0]) + ", "
        + Tools::ToString(C[1]) + " }, { "
        + Tools::ToString(C[2]) + ", "
        + Tools::ToString(C[3]) + " } } ("
        + KnotTools::ToString(S) +")";
    }
    
    friend std::string ToString( const CrossingView & C )
    {
        return C.String();
    }
    
}; // class CrossingView

CrossingView GetCrossing(const Int c )
{
    AssertCrossing<0>(c);

    return CrossingView( c, C_arcs.data(c), C_state[c] );
}

bool OppositeHandednessQ( const CrossingView & C_0, const CrossingView & C_1 ) const
{
    return KnotTools::OppositeHandednessQ(C_0.State(),C_1.State());
}

bool SameHandednessQ( const CrossingView & C_0, const CrossingView & C_1 ) const
{
    return KnotTools::SameHandednessQ(C_0.State(),C_1.State());
}

void RotateCrossing( CrossingView & C, const bool dir )
{
/* Before:
//
//    C(Out,Left ) O       O C(Out,Right)
//                  ^     ^
//                   \   /
//                    \ /
//                     X
//                    / \
//                   /   \
//                  /     \
//    C(In ,Left ) O       O C(In ,Right)
*/
    if( dir == Right )
    {
/* After:
//
//    C(Out,Left ) O       O C(Out,Right)
//                  \     ^
//                   \   /
//                    \ /
//                     X
//                    / \
//                   /   \
//                  /     v
//    C(In ,Left ) O       O C(In ,Right)
*/
        const Int buffer = C(Out,Left );
        
        C(Out,Left ) = C(Out,Right);
        C(Out,Right) = C(In ,Right);
        C(In ,Right) = C(In ,Left );
        C(In ,Left ) = buffer;
    }
    else
    {
/* After:
//
//    C(Out,Left ) O       O C(Out,Right)
//                  ^     /
//                   \   /
//                    \ /
//                     X
//                    / \
//                   /   \
//                  v     \
//    C(In ,Left ) O       O C(In ,Right)
*/
        const Int buffer = C(Out,Left );

        C(Out,Left ) = C(In ,Left );
        C(In ,Left ) = C(In ,Right);
        C(In ,Right) = C(Out,Right);
        C(Out,Right) = buffer;
    }
}

/*!
 * @brief Deactivates crossing represented by `CrossingView` object `C`. Only for internal use.
 */

void Deactivate( mref<CrossingView> C )
{
    if( C.ActiveQ() )
    {
        --pd.crossing_count;
        C.State() = CrossingState::Inactive;
    }
    else
    {
#if defined(PD_DEBUG)
        wprint("Attempted to deactivate already inactive " + C.String() + ".");
#endif
    }
    
    PD_ASSERT( pd.crossing_count >= 0 );
    
#ifdef PD_DEBUG
    for( bool io : { In, Out } )
    {
        for( bool side : { Left, Right } )
        {
            const Int a  = C(io,side);
            
            if( ArcActiveQ(a) && (A_cross(a,io) == C.Idx()) )
            {
                pd_eprint(ClassName()+"DeactivateCrossing: active " + ArcString(a) + " is still attached to deactivated " + ToString(C) + ".");
            }
        }
    }
#endif
}
