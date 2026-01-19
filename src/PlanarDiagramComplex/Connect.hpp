bool Connect( const Int diagram_idx, const Int a, const Int b )
{
    if( (diagram_idx < 0) || (diagram_idx >= DiagramCount() ) )
    {
        wprint(MethodName("Connect") + ": Diagram index a = " + ToString(diagram_idx) + " is out of bounds. Doing nothing.");
        return false;
    }
    
    PD_T & pd = pd_list[diagram_idx];
    
    if( pd.InvalidQ() )
    {
        return false;
    }
    
    bool succeededQ = pd.Connect(a,b);
    
    if( succeededQ )
    {
        ClearCache();
    }
    
    return succeededQ;
}


template<bool silentQ = false>
bool ConnectWithIsthmus(
    const Int diagram_idx, const Int a, const Int b,
    CrossingState_T handedness = CrossingState_T::RightHanded
)
{
    if( (diagram_idx < 0) || (diagram_idx >= DiagramCount() ) )
    {
        wprint(MethodName("ConnectWithIsthmus") + ": Diagram index a = " + ToString(diagram_idx) + " is out of bounds. Doing nothing.");
        return false;
    }
    
    PD_T & pd = pd_list[diagram_idx];
    
    if( pd.InvalidQ() )
    {
        return false;
    }
    
    bool succeededQ = pd.ConnectWithIsthmus(a,b,handedness);
    
    if( succeededQ )
    {
        ClearCache();
    }
    
    return succeededQ;
}

template<bool silentQ = false>
bool ConnectWithDoubleIsthmus(
    const Int diagram_idx, const Int a, const Int b,
    CrossingState_T handedness_0 = CrossingState_T::RightHanded,
    CrossingState_T handedness_1 = CrossingState_T::RightHanded
)
{
    if( (diagram_idx < 0) || (diagram_idx >= DiagramCount() ) )
    {
        wprint(MethodName("ConnectWithDoubleIsthmus") + ": Diagram index a = " + ToString(diagram_idx) + " is out of bounds. Doing nothing.");
        return false;
    }
    
    PD_T & pd = pd_list[diagram_idx];
    
    if( pd.InvalidQ() )
    {
        return false;
    }
    
    bool succeededQ = pd.ConnectWithDoubleIsthmus(a,b,handedness_0,handedness_1);
    
    if( succeededQ )
    {
        ClearCache();
    }
    
    return succeededQ;
}
