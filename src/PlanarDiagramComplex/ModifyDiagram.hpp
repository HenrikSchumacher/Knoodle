public:

/*!@brief **UNSAFE.** Calls the corresponding routine on `Diagram(diagram_idx)`. */
template<bool silentQ = false>
bool SwitchCrossing( const Int diagram_idx, const Int c )
{
    if( LockedQ() ) { LockMessage("SwitchCrossing"); return false; }
    
    TOOLS_PTIMER(timer,MethodName("SwitchCrossing"));
    
    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            Msgr::wprint("SwitchCrossing", "Diagram(",diagram_idx,") is invalid. Doing nothing.");
        }
        return Uninitialized;
    }
    
    const bool changedQ = pd.SwitchCrossing(c);
    if( changedQ ) { ClearCache(); }
    return changedQ;
}

/*!@brief This is a relatively "safe" routine as we do not allow modification of any invalid diagrams. (You might need to `Push` an unknot first.*/
template<bool silentQ = false>
void RequireCrossingCount( const Int diagram_idx, const Int min_crossing_count )
{
    TOOLS_PTIMER(timer,MethodName("RequireCrossingCount"));
    
    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            Msgr::wprint("RequireCrossingCount", "Diagram(",diagram_idx,") is invalid. Doing nothing.");
        }
        return;
    }
    
    pd.RequireCrossingCount(min_crossing_count);
}

/*!@brief A make a Reidemeister I move at an arc to create a new crossing.*/
template<bool silentQ = false, bool assertsQ = true>
Int CreateLoop(
    const Int diagram_idx,
    const Int a, const bool side, CrossingState_T handedness
)
{
    TOOLS_PTIMER(timer,MethodName("CreateLoop"));

    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            Msgr::wprint("CreateLoop","Diagram(",diagram_idx,") is invalid. Doing nothing.");
        }
        return Uninitialized;
    }
    
    Int result = pd.template CreateLoop<silentQ,assertsQ>(a,side,handedness);
    if( result != Uninitialized ) { ClearCache(); }
    return result;
}


/*!@brief **UNSAFE.** Cut the two arcs `a` and `b` and reconnect.*/
template<bool silentQ = false, bool assertsQ = true>
bool Connect( const Int diagram_idx, const Int a, const Int b )
{
    TOOLS_PTIMER(timer,MethodName("Connect"));

    PD_T & pd = Diagram_Private(diagram_idx);
    
    if( pd.InvalidQ() )
    {
        if constexpr (!silentQ)
        {
            Msgr::wprint("Connect", "Diagram(",diagram_idx,") is invalid. Doing nothing.");
        }
        return false;
    }
    
    const bool changedQ = pd.template Connect<silentQ,assertsQ>(a,b);
    if( changedQ ) { ClearCache(); }
    return changedQ;
}

public:

/*!@brief **UNSAFE.** Reverse all arcs with color indicated by `color`. Returns the number of arcs reversed.*/
Int ReverseColoredArcs( const Int color )
{
    if( LockedQ() ) { LockMessage("ReverseColoredArcs"); return Int(0); }
    
    Int counter= 0;
    for( auto & pd : pd_list )
    {
        counter += pd.ReverseColoredArcs_Private(color);
    }
    
    return counter;
}
