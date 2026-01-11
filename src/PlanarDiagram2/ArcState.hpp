// TODO: Template this.
// bool fancyQ   -- whether the flags other than an active bit ought to be used at all.
// bool lutQ     -- whether lookup tables for the bits and mask ought to be used.
// bool set_lutQ -- whether the Set routine ought to be further optmized by another lookup table.

class ArcState_T
{
public:
    
    using Int = std::uint_least8_t; // Type used for storing
    
    using I   = int;                // Type used for computing
    
    static constexpr bool lutQ     = true;
    static constexpr bool set_lutQ = false;
    
private:
    
    Int state { 0 };
    
public:
    
    constexpr ArcState_T() = default;
    
    constexpr explicit ArcState_T( const Int s )
    :   state { s }
    {}
    
    constexpr ~ArcState_T() = default;
    
    static constexpr ArcState_T Inactive()
    {
        return ArcState_T();
    }

    
    [[maybe_unused]] static constexpr int ActiveBit_     [2] = { 4 * 0 + 0, 4 * 1 + 0 };
    [[maybe_unused]] static constexpr int SideBit_       [2] = { 4 * 0 + 1, 4 * 1 + 1 };
    [[maybe_unused]] static constexpr int HandednessBit_ [2] = { 4 * 0 + 2, 4 * 1 + 2 };
    [[maybe_unused]] static constexpr int OverUnderBit_  [2] = { 4 * 0 + 3, 4 * 1 + 3 };
    
    [[maybe_unused]] static constexpr I   ActiveMask_    [2] = { 1 << ActiveBit_   [0], 1 << ActiveBit_    [1] };
    [[maybe_unused]] static constexpr I   SideMask_      [2] = { 1 << SideBit_     [0], 1 << SideBit_      [1] };
    [[maybe_unused]] static constexpr I   HandednessMask_[2] = { 1 <<HandednessBit_[0], 1 << HandednessBit_[1] };
    [[maybe_unused]] static constexpr I   OverUnderMask_ [2] = { 1 << OverUnderBit_[0], 1 << OverUnderBit_ [1] };

    static constexpr int ActiveBit( bool ht )
    {
        if constexpr (lutQ )
        {
            return ActiveBit_[ht];
        }
        else
        {
            return I(4) * I(ht) + I(0);
        }
    }
    
    static constexpr I ActiveMask( bool ht )
    {
        if constexpr (lutQ )
        {
            return ActiveMask_[ht];
        }
        else
        {
            return (I(1) << ActiveBit(ht));
        }
    }
    
    
    static constexpr int SideBit( bool ht )
    {
        if constexpr (lutQ )
        {
            return SideBit_[ht];
        }
        else
        {
            return I(4) * I(ht) + I(1);
        }
    }
        
    static constexpr I SideMask( bool ht )
    {
        if constexpr (lutQ )
        {
            return SideMask_[ht];
        }
        else
        {
            return (I(1) << SideBit(ht));
        }
    }
    
    
    static constexpr int HandednessBit( bool ht )
    {
        if constexpr (lutQ )
        {
            return HandednessBit_[ht];
        }
        else
        {
            return I(4) * I(ht) + I(2);
        }
    }
    
    static constexpr I HandednessMask( bool ht )
    {
        if constexpr (lutQ )
        {
            return HandednessMask_[ht];
        }
        else
        {
            return (I(1) << SideBit(ht));
        }
    }
    
    
    static constexpr int OverUnderBit( bool ht )
    {
        if constexpr (lutQ )
        {
            return OverUnderBit_[ht];
        }
        else
        {
            return I(4) * I(ht) + I(3);
        }
    }
    
    static constexpr I OverUnderMask( bool ht )
    {
        if constexpr (lutQ )
        {
            return OverUnderMask_[ht];
        }
        else
        {
            return (I(1) << SideBit(ht));
        }
    }
    
    
    [[maybe_unused]] static constexpr I HeadTailMask_ [2] = {
        ActiveMask(0) | SideMask(0) | HandednessMask(0) | OverUnderMask(0),
        ActiveMask(1) | SideMask(1) | HandednessMask(1) | OverUnderMask(1),
    };
    
    static constexpr I HeadTailMask( bool ht )
    {
        if constexpr (lutQ )
        {
            return HeadTailMask_[ht];
        }
        else
        {
            return ActiveMask(ht) | SideMask(ht) | HandednessMask(ht) | OverUnderMask(ht);
        }
    }

    
    constexpr bool ActiveQ() const
    {
        constexpr I active = ActiveMask(Head) | ActiveMask(Tail);
        
        return ((static_cast<I>(state) & active) == active);
    }
    
    constexpr bool InactiveQ() const
    {
        return (state == Int(0));
    }
    
    constexpr bool ValidQ() const
    {
        return InactiveQ() || ActiveQ();
    }

    
private:
    
    static constexpr I set_lu( bool headtail, bool side, bool right_handedQ, bool overQ )
    {
        return (I(1)                             << ActiveBit    (headtail))
               | (static_cast<I>(side)           << SideBit      (headtail))
               | (static_cast<I>(right_handedQ)  << HandednessBit(headtail))
               | (static_cast<I>(overQ)          << OverUnderBit (headtail));

    }

public:
    
    [[maybe_unused]] static constexpr I set_lut [2][2][2][2] = {
        {
            { {set_lu(0,0,0,0), set_lu(0,0,0,1)}, { set_lu(0,0,1,0), set_lu(0,0,1,1)} },
            { {set_lu(0,1,0,0), set_lu(0,1,0,1)}, { set_lu(0,1,1,0), set_lu(0,1,1,1)} }
        },
        {
            { {set_lu(1,0,0,0), set_lu(1,0,0,1)}, { set_lu(1,0,1,0), set_lu(1,0,1,1)} },
            { {set_lu(1,1,0,0), set_lu(1,1,0,1)}, { set_lu(1,1,1,0), set_lu(1,1,1,1)} }
        }
    };

    constexpr void Set(bool headtail, bool side, bool right_handedQ, bool overQ )
    {
        I s;
        
        if constexpr ( set_lutQ)
        {
            s = set_lut[headtail][side][right_handedQ][overQ];
        }
        else
        {
            s= set_lu(headtail,side,right_handedQ,overQ);
        }
        
        state = static_cast<Int>( (static_cast<I>(state) & ~HeadTailMask(headtail)) | s );
    }

    constexpr void Set(bool headtail, bool side, bool right_handedQ )
    {
        const bool overQ         = ((side == right_handedQ) != headtail);
        
        /*
         *          ^     ^         headtail      = Tail  == 1
         *           \   /          side          = Right == 1
         *            \ /           right_handedQ = True  == 1
         *             / <--- c     overQ         = False == 0
         *            ^ ^
         *           /   \
         *          /     \ a
         */
        
        Set(headtail,side,right_handedQ,overQ);
    }
    
    constexpr void Set(bool headtail, bool side, const CrossingState_T handedness )
    {
        Set(headtail,side,handedness.RightHandedQ());
    }
    
    void Copy( bool headtail, const ArcState_T other )
    {
        state = static_cast<Int>(
            (static_cast<I>(state      ) & ~HeadTailMask(headtail))
            |
            (static_cast<I>(other.state) &  HeadTailMask(headtail))
        );
    }
    
    // Swap Head and Tail
    constexpr ArcState_T Reverse() const
    {
        // Head/tail  swap.
        // Handedness stays fixed.
        // Left/right flips.
        // Over/under stays fixed.
        
        constexpr I trafo_mask = SideMask(Tail) | SideMask(Head);
        
        const I s = static_cast<I>(state) ^ trafo_mask;
        
        return ArcState_T( static_cast<Int>( ((s & HeadTailMask(Head)) >> I(4)) | ((s & HeadTailMask(Tail)) << I(4)) ) );
    }
    
    // Reflect the arrow in the coordinate plane
    constexpr ArcState_T Reflect() const
    {
        // Head/tail  stays fixed.
        // Handedness flips!
        // Left/right flips!
        // Over/under stays fixed?
        
        constexpr I trafo_mask = SideMask(Tail) | HandednessMask(Tail)
                                 | SideMask(Head) | HandednessMask(Head);

        return ArcState_T( static_cast<I>(state) ^ trafo_mask );
    }
    
    // Reflect the arrow in the coordinate plane and reverse at the same time.
    constexpr ArcState_T ReflectReverse() const
    {
        // Head/tail  swap.
        // Handedness flips.
        // Left/right stay fixed?
        // Over/under flips?
        
        constexpr I trafo_mask = HandednessMask(Tail) | OverUnderMask(Tail)
                                 | HandednessMask(Head) | OverUnderMask(Head);
        
        const Int s = static_cast<I>(state) ^ trafo_mask;
        
        return ArcState_T( ((s & HeadTailMask(Head)) >> I(4)) | ((s & HeadTailMask(Tail)) << I(4)) );
    }

    
    constexpr bool Side( const bool headtail )  const
    {
        return get_bit(static_cast<I>(state), SideBit(headtail));
    }

    constexpr bool RightHandedQ( const bool headtail ) const
    {
        return get_bit(static_cast<I>(state), HandednessBit(headtail));
    }
    
    constexpr bool LeftHandedQ( const bool headtail ) const
    {
        return !get_bit(static_cast<I>(state), HandednessBit(headtail));
    }

    constexpr bool OverQ( const bool headtail ) const
    {
        return get_bit(static_cast<I>(state), OverUnderBit(headtail));
    }
    
    constexpr bool UnderQ( const bool headtail ) const
    {
        return !get_bit(static_cast<I>(state), OverUnderBit(headtail));
    }
    
    
    friend std::string ToString( ArcState_T a_state )
    {
        return std::format("{:08b}", a_state.state);
    }
    
    friend constexpr Int ToUnderlying( ArcState_T a_state )
    {
        return a_state.state;
    }
    
    friend constexpr bool operator==( ArcState_T a_state, ArcState_T b_state )
    {
        return (a_state.state == b_state.state);
    }
    
    friend constexpr bool operator!=( ArcState_T a_state, ArcState_T b_state )
    {
        return (a_state.state != b_state.state);
    }
    
}; // ArcState_T
