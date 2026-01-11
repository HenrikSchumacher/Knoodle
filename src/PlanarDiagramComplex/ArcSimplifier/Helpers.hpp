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

void DeactivateCrossing( const Int c_ )
{
    pd.DeactivateCrossing(c_);
}

void SwitchCrossing( const Int c_ )
{
    (void) pd.SwitchCrossing(c_);
}


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
    return pd.CrossingString(c_);
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
void AssertArc( const Int a_ )
{
#ifdef PD_DEBUG
    pd.template AssertArc<must_be_activeQ>(a_);
#else
    (void)a_;
#endif
}

std::string ArcString( const Int a_ ) const
{
    return pd.ArcString(a_);
}


void Reconnect( const Int a_, const bool headtail, const Int b_ )
{
    pd.Reconnect(a_,headtail,b_);
}

template<bool headtail>
void Reconnect( const Int a_, const Int b_ )
{
    pd.template Reconnect<headtail>(a_,b_);
}
