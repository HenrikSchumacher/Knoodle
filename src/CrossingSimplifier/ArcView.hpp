public:
    
class ArcView final
{
    Int a;
    mptr<Int> A;
    ArcState & S;
    
public:
    
    ArcView( const Int a_, mptr<Int> A_, ArcState & S_ )
    : a { a_ }
    , A { A_ }
    , S { S_ }
    {}

    // TODO: Value semantics.
    
    Int Idx() const
    {
        return a;
    }
    
    ArcState & State()
    {
        return S;
    }
    
    const ArcState & State() const
    {
        return S;
    }
    
    Int & operator()( const bool headtail )
    {
        return A[headtail];
    }
    
    const Int & operator()( const bool headtail ) const
    {
        return A[headtail];
    }
    
    bool ActiveQ() const
    {
        return (ToUnderlying(S) & Underlying_T<ArcState>(1));
    }
    
    inline friend bool operator==( const ArcView & A_0, const ArcView & A_1 )
    {
        return A_0.a == A_1.a;
    }
    
    inline friend bool operator==( const ArcView & A_0, const Int a_1 )
    {
        return A_0.a == a_1;
    }
    
    inline friend bool operator==( const Int a_0, const ArcView & A_1 )
    {
        return a_0 == A_1.a;
    }
    
    std::string String() const
    {
        return "arc " +Tools::ToString(a) +" = { " +
        Tools::ToString(A[0])+", "+Tools::ToString(A[1])+" } (" + Knoodle::ToString(S) +")";
    }
    
    friend std::string ToString( const ArcView & A_ )
    {
        return A_.String();
    }
    
}; // class ArcView
    
    

ArcView GetArc(const Int a )
{
    AssertArc<0>(a);
    
    return ArcView( a, A_cross.data(a), A_state[a] );
}

bool HeadQ( const ArcView & A, const CrossingView & C )
{
    return A(Head) == C;
}

bool TailQ( const ArcView & A, const CrossingView & C )
{
    return A(Tail) == C;
}


/*!
 * @brief Deactivates arc represented by `ArcView` object `A`. Only for internal use.
 */

void Deactivate( mref<ArcView> A )
{
    if( A.ArcActiveQ() )
    {
        --pd.arc_count;
        A.State() = ArcState::Inactive;
    }
    else
    {
#if defined(PD_DEBUG)
        wprint("Attempted to deactivate already inactive " + A.String() + ".");
#endif
    }
    
    PD_ASSERT( pd.arc_count >= Int(0) );
}

void Reconnect( ArcView & A, const bool headtail, ArcView & B )
{
    // Read:
    // Reconnect arc A with its tip/tail to where B pointed/started.
    // Then deactivates B.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    PD_ASSERT( A.Idx() != B.Idx() );
    
    PD_ASSERT( A.ActiveQ() );
//    PD_ASSERT( B.ActiveQ() ); // Could have been deactivated already.
    
    auto C = GetCrossing(B(headtail) );
    
    PD_ASSERT( (C(headtail,Left) == B.Idx()) || (C(headtail,Right) == B.Idx()) );
    
    PD_ASSERT(pd.CheckArc(B.Idx()));
    PD_ASSERT(pd.CheckCrossing(C.Idx()));
    
    PD_ASSERT(C.ActiveQ());
    PD_ASSERT(CrossingActiveQ(A( headtail)));
    PD_ASSERT(CrossingActiveQ(A(!headtail)));
    
    
    A(headtail) = C.Idx();

    const bool side = (C(headtail,Right) == B.Idx());
    
    C(headtail,side) = A.Idx();
    
    DeactivateArc(B.Idx());
}


template<bool headtail>
void Reconnect( ArcView & A, ArcView & B )
{
    // Read:
    // Reconnect arc A with its tip/tail to where B pointed/started.
    // Then deactivates B.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    PD_ASSERT( A.Idx() != B.Idx() );
    
    PD_ASSERT( A.ActiveQ() );
//    PD_ASSERT( B.ActiveQ() ); // Could have been deactivated already.
    
    auto C = GetCrossing(B(headtail) );
    
    
    PD_ASSERT( (C(headtail,Left) == B.Idx()) || (C(headtail,Right) == B.Idx()) );
    
    PD_ASSERT(pd.CheckArc(B.Idx()));
    PD_ASSERT(pd.CheckCrossing(C.Idx()));
    
    PD_ASSERT(C.ActiveQ());
    PD_ASSERT(CrossingActiveQ(A( headtail)));
    PD_ASSERT(CrossingActiveQ(A(!headtail)));
    
    
    A(headtail) = C.Idx();

    const bool side = (C(headtail,Right) == B.Idx());
    
    C(headtail,side) = A.Idx();
    
    DeactivateArc(B.Idx());
}


/*!
 * @brief Returns the ArcView object next to ArcView object `A`, i.e., the ArcView object reached by going straight through the crossing at the head/tail of `A`.
 */

template<bool headtail>
ArcView NextArc( const ArcView & A )
{
    AssertArc<0>(A.Idx());
    
    const Int c = A(headtail);

    AssertCrossing<0>(c);

    // We leave through the arc at the port opposite to where a arrives.
    const bool side = (C_arcs(c,headtail,Right) != A.Idx());

    return GetArc(C_arcs(c,!headtail,side) );
}
