class ArcState_T
{
public:
    
    using Int = UInt8;
    
private:
    
    UInt8 state { 0 };
    
public:
    
    constexpr ArcState_T() = default;
    
    constexpr explicit ArcState_T( Int s )
    :   state { s }
    {}
    
    constexpr ~ArcState_T() = default;

//    static constexpr ArcState_T Inactive = ArcState_T();
    
    static constexpr ArcState_T Inactive()
    {
        return ArcState_T();
    }
    
    template<bool headtail> static constexpr Int ActiveBit      = 0 + 4 * headtail;
    template<bool headtail> static constexpr Int SideBit        = 1 + 4 * headtail;
    template<bool headtail> static constexpr Int HandednessBit  = 2 + 4 * headtail;
    template<bool headtail> static constexpr Int OverUnderBit   = 3 + 4 * headtail;
    
    template<bool headtail> static constexpr Int ActiveMask     = (Int(1) << ActiveBit    <headtail>);
    template<bool headtail> static constexpr Int SideMask       = (Int(1) << SideBit      <headtail>);
    template<bool headtail> static constexpr Int HandednessMask = (Int(1) << HandednessBit<headtail>);
    template<bool headtail> static constexpr Int OverUnderMask  = (Int(1) << OverUnderBit <headtail>);
    
    template<bool headtail> static constexpr Int mask =
        ActiveMask<headtail> | SideMask<headtail> | HandednessMask<headtail> | OverUnderMask<headtail>;

    
    constexpr bool ActiveQ() const
    {
        constexpr Int active = ActiveMask<Head> | ActiveMask<Tail>;
        
        return ((state & active) == active);
    }
    
    constexpr bool InactiveQ() const
    {
        return (state == Int(0));
    }
    
    constexpr bool ValidQ() const
    {
        return InactiveQ() || ActiveQ();
    }

    template<bool headtail>
    void Set(bool side, const CrossingState_T handedness )
    {
        const bool right_handedQ = handedness.RightHandedQ();
        
        const Int s = (Int(1)                                    << ActiveBit    <headtail>)
                    | (Int(side)                                 << SideBit      <headtail>)
                    | (Int(right_handedQ)                        << HandednessBit<headtail>)
                    | (Int((side == right_handedQ) == headtail)  << OverUnderBit <headtail>);
        
        /*
         *          ^     ^         headtail      = Tail  == 0
         *           \   /          side          = Right == 1
         *            \ /           right_handedQ = True  == 1
         *             / <--- c     overQ         = False == 0
         *            ^ ^
         *           /   \
         *          /     \ a
         */
        
        state = (state & ~mask<headtail>) | s;
    }
    
    void Set( bool headtail, bool side, const CrossingState_T handedness )
    {
        if( headtail )
        {
            this->template Set<Head>(side,handedness);
        }
        else
        {
            this->template Set<Tail>(side,handedness);
        }
    }
    
    
    template<bool headtail>
    void Copy( const ArcState_T other )
    {
        // TODO: Test this.
        state = (state & ~mask<headtail>) | (other.state & mask<headtail>);
    }
    
    void Copy( bool headtail, const ArcState_T other )
    {
        if( headtail )
        {
            this->template Copy<Head>(other);
        }
        else
        {
            this->template Copy<Tail>(other);
        }
    }
    

    template<bool headtail>
    constexpr bool Side()  const
    {
        return get_bit(state, SideBit<headtail>);
    }
    
    constexpr bool Side( const bool headtail )  const
    {
        if( headtail )
        {
            return this->template Side<Head>();
        }
        else
        {
            return this->template Side<Tail>();
        }
    }
    
    template<bool headtail>
    constexpr bool RightHandedQ() const
    {
        return get_bit(state, HandednessBit<headtail>);
    }
    
    constexpr bool RightHandedQ( const bool headtail ) const
    {
        if( headtail )
        {
            return this->template RightHandedQ<Head>();
        }
        else
        {
            return this->template RightHandedQ<Tail>();
        }
    }
    
    
    template<bool headtail>
    constexpr bool LeftHandedQ() const
    {
        return !get_bit(state, HandednessBit<headtail>);
    }
    
    constexpr bool LeftHandedQ( const bool headtail ) const
    {
        if( headtail )
        {
            return this->template LeftHandedQ<Head>();
        }
        else
        {
            return this->template LeftHandedQ<Tail>();
        }
    }
    
    template<bool headtail>
    constexpr bool OverQ() const
    {
        return get_bit(state, OverUnderBit<headtail>);
    }
    
    constexpr bool OverQ( const bool headtail ) const
    {
        if( headtail )
        {
            return this->template OverQ<Head>();
        }
        else
        {
            return this->template OverQ<Tail>();
        }
    }
    
    
    template<bool headtail>
    constexpr bool UnderQ() const
    {
        return !get_bit(state, OverUnderBit<headtail>);
    }
    
    constexpr bool UnderQ( const bool headtail ) const
    {
        if( headtail )
        {
            return this->template UnderQ<Head>();
        }
        else
        {
            return this->template UnderQ<Tail>();
        }
    }
    
    
    friend std::string ToString( ArcState_T a_state )
    {
        return std::format("{:08b}", a_state.state);
    }
    
    friend constexpr Int ToUnderlying( ArcState_T a_state )
    {
        return a_state.state;
    }
    
}; // ArcState_T
