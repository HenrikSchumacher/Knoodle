private:

static constexpr Int ToDarc( const Int a, const bool headtail )
{
    return PD_T::ToDarc(a,headtail);
}

static constexpr Int FlipDarc( Int da )
{
    return PD_T::FlipDarc(da);
}

template<bool must_be_activeQ>
void AssertArc( const Int a_ ) const
{
#ifdef PD_DEBUG
    if constexpr( must_be_activeQ )
    {
        if( !pd.ArcActiveQ(a_) )
        {
            pd_eprint("AssertArc<1>: " + ArcString(a_) + " is not active.");
        }
        PD_ASSERT(pd.CheckArc(a_));
//                PD_ASSERT(CheckArcLeftArc(a_));
    }
    else
    {
        if( pd.ArcActiveQ(a_) )
        {
            pd_eprint("AssertArc<0>: " + ArcString(a_) + " is not inactive.");
        }
    }
#else
    (void)a_;
#endif
}

std::string ArcString( const Int a_ )  const
{
    return pd.ArcString(a_) + " (mark = " + ToString(A_mark(a_)) + ")";
}

template<bool must_be_activeQ>
void AssertCrossing( const Int c_ ) const
{
#ifdef PD_DEBUG
    if constexpr( must_be_activeQ )
    {
        if( !pd.CrossingActiveQ(c_) )
        {
            pd_eprint("AssertCrossing<1>: " + CrossingString(c_) + " is not active.");
        }
//                PD_ASSERT(pd.CheckCrossing(c_));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,Out,Left )));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,Out,Right)));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,In ,Left )));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,In ,Right)));
    }
    else
    {
        if( pd.CrossingActiveQ(c_) )
        {
            pd_eprint("AssertCrossing<0>: " + CrossingString(c_) + " is not inactive.");
        }
    }
#else
    (void)c_;
#endif
}
    
std::string CrossingString( const Int c_ ) const
{
    return pd.CrossingString(c_) + " (mark = " + ToString(C_mark(c_)) + ")";
}
