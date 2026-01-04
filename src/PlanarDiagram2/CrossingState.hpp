class CrossingState_T
{
public:
    
    using Int = Int8;
    
private:
    
    Int state { 0 };
    
public:
    
    explicit constexpr CrossingState_T( Int s )
    :   state { s }
    {}
    
    constexpr CrossingState_T() = default;
    
    constexpr ~CrossingState_T() = default;

    static constexpr CrossingState_T Inactive()
    {
        return CrossingState_T();
    }
    
    static constexpr CrossingState_T RightHanded()
    {
        return CrossingState_T(Int( 1));
    }
    
    static constexpr CrossingState_T LeftHanded()
    {
        return CrossingState_T(Int(-1));
    }
    
//    static constexpr CrossingState_T Inactive    = CrossingState_T(Int( 0));
//    static constexpr CrossingState_T RightHanded = CrossingState_T(Int( 1));
//    static constexpr CrossingState_T LeftHanded  = CrossingState_T(Int(-1));
    
    constexpr bool ActiveQ() const
    {
        return (state != Int(0));
    }
    
    constexpr bool InactiveQ() const
    {
        return (state == Int(0));
    }
    
    constexpr bool RightHandedQ() const
    {
        return (state == Int(1));
    }
    
    constexpr bool LeftHandedQ() const
    {
        return (state == Int(-1));
    }
    
    friend constexpr bool OppositeHandednessQ( CrossingState_T s_0, CrossingState_T s_1 )
    {
        // Careful, this evaluates to true if both are `Inactive`.
        return ( Sign(s_0.state) == -Sign(s_1.state) );
    }
    
    friend constexpr bool SameHandednessQ( CrossingState_T s_0, CrossingState_T s_1 )
    {
        // Careful, this evaluates to true if both are `Inactive`.
        return ( Sign(s_0.state) == Sign(s_1.state) );
    }
    
    friend std::string ToString( CrossingState_T c_state )
    {
        switch( c_state.state )
        {
            case Int( 0) : return "Inactive";
                
            case Int( 1) : return "RightHanded";
                
            case Int(-1) : return "LeftHanded";
                
            default:
            {
                eprint( "ToString: Argument s = " + ToString(c_state.state) + " is invalid." );
                return "Unknown";
            }
        }
    }
    
    friend constexpr Int ToUnderlying( CrossingState_T c_state )
    {
        return c_state.state;
    }
};
