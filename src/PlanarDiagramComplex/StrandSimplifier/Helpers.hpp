// Methods that are just redirected to pd.

private:

bool CrossingActiveQ( const Int c_ ) const
{
    return pd.CrossingActiveQ(c_);
}

bool CrossingRightHandedQ( const Int c_ ) const
{
    return pd.CrossingRightHandedQ(c_);
}


bool CrossingLeftHandedQ( const Int c_ ) const
{
    return pd.CrossingLeftHandedQ(c_);
}

template<bool assertsQ = true>
void DeactivateCrossing( const Int c_ )
{
    pd.template DeactivateCrossing<assertsQ>(c_);
}

//void SwitchCrossing( const Int c_ )
//{
//    (void) pd.SwitchCrossing(c_);
//}

template<bool must_be_activeQ>
void AssertCrossing( const Int c_ ) const
{
#ifdef PD_DEBUG
    pd.template AssertCrossing<must_be_activeQ>(c_);
#else
    (void)c_;
#endif
}

std::string CrossingString( const Int c_ ) const
{
    return pd.CrossingString(c_) + " (mark = " + ToString(C_mark(c_)) + ")";
}





bool ArcActiveQ( const Int a_ ) const
{
    return pd.ArcActiveQ(a_);
}

void DeactivateArc( const Int a_ ) const
{
    return pd.DeactivateArc(a_);
}

void RecomputeArcState( const Int a_ )
{
    pd.RecomputeArcState(a_);
}
    
template<bool must_be_activeQ>
void AssertArc( const Int a_ ) const
{
#ifdef PD_DEBUG
    pd.template AssertArc<must_be_activeQ>(a_);
#else
    (void)a_;
#endif
}

std::string ArcString( const Int a_ )  const
{
    return pd.ArcString(a_) + " (mark = " + ToString(A_mark(a_)) + ")";
}

bool ArcSide( const Int a, const bool headtail ) const
{
    // TODO: We do it conservatively for now. Change this to ArcSide later.
    return pd.ArcSide_Reference(a,headtail);
}

bool ArcOverQ( const Int a, const bool headtail ) const
{
    // TODO: We do it conservatively for now. Change this to ArcSide later.
    return pd.ArcOverQ_Reference(a,headtail);
}

bool ArcUnderQ( const Int a, const bool headtail ) const
{
    // TODO: We do it conservatively for now. Change this to ArcSide later.
    return pd.ArcUnderQ_Reference(a,headtail);
}

bool NextArc( const Int a, const bool headtail, const Int c ) const
{
    // TODO: We do it conservatively for now. Change this to ArcSide later.
    return pd.NextArc_Reference(a,headtail,c);
}

bool NextArc( const Int a, const bool headtail ) const
{
    // TODO: We do it conservatively for now. Change this to ArcSide later.
    return pd.NextArc_Reference(a,headtail);
}



static constexpr Int ToDarc( const Int a, const bool headtail )
{
    return PD_T::ToDarc(a,headtail);
}

static constexpr Int FlipDarc( Int da )
{
    return PD_T::FlipDarc(da);
}
