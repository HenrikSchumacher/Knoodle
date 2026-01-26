public:

/*! @brief Calls the corresponding routine on `Diagram(diagram_idx)`.
 */

template<bool silentQ = false>
bool SwitchCrossing( const Int diagram_idx, const Int c )
{
    auto tag = []()-> std::string { return MethodName("SwitchCrossing"); };
    TOOLS_PTIMER(timer,tag());
    
    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            wprint(tag()+": Diagram("+ToString(diagram_idx)+") is invalid. Doing nothing.");
        }
        return Uninitialized;
    }
    
    const bool changedQ = pd.SwitchCrossing(c);
    if( changedQ ) { ClearCache(); }
    return changedQ;
}

// As an exception, this is a relatively "safe" routine as we do not allow modification of any unvalid diagrams. (You might need to `Push` an unknot first.
template<bool silentQ = false>
void RequireCrossingCount( const Int diagram_idx, const Int min_crossing_count )
{
    auto tag = []()-> std::string { return MethodName("RequireCrossingCount"); };
    TOOLS_PTIMER(timer,tag());
    
    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            wprint(tag()+": Diagram("+ToString(diagram_idx)+") is invalid. Doing nothing.");
        }
        return;
    }
    
    pd.RequireCrossingCount(min_crossing_count);
}

template<bool silentQ = false, bool assertsQ = true>
Int CreateLoop(
    const Int diagram_idx,
    const Int a, const bool side, CrossingState_T handedness
)
{
    auto tag = []()-> std::string { return MethodName("CreateLoop"); };
    TOOLS_PTIMER(timer,tag());

    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            wprint(tag()+": Diagram("+ToString(diagram_idx)+") is invalid. Doing nothing.");
        }
        return Uninitialized;
    }
    
    Int result = pd.template CreateLoop<silentQ,assertsQ>(a,side,handedness);
    if( result != Uninitialized ) { ClearCache(); }
    return result;
}

template<bool silentQ = false, bool assertsQ = true>
bool Connect( const Int diagram_idx, const Int a, const Int b )
{
    auto tag = []()-> std::string { return MethodName("Connect"); };
    TOOLS_PTIMER(timer,tag());

    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            wprint(tag()+": Diagram("+ToString(diagram_idx)+") is invalid. Doing nothing.");
        }
        return false;
    }
    
    const bool changedQ = pd.template Connect<silentQ,assertsQ>(a,b);
    if( changedQ ) { ClearCache(); }
    return changedQ;
}
