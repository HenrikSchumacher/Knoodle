public:

std::string EdgeString( const Int e ) const
{
    return "edge " + Tools::ToString(e) + " = "
        + ArrayToString(E_V.data(e),{2})
        + " (" + (EdgeActiveQ(e) ? "Active" : "Inactive") + ")";
}

std::string DedgeString( const Int de ) const
{
    const Int c_0 = E_V.data()[FlipDedge(de)];
    const Int c_1 = E_V.data()[de];
    
    return "dedge " + Tools::ToString(de) + " = { "
    + ToString(c_0) + ", "
    + ToString(c_1) + " } (" + (DedgeActiveQ(de) ? "Active" : "Inactive") + ")";
}


// You should always do a check that e >= 0 before calling this!
static constexpr Int ToDedge( const Int e, const HeadTail_T d )
{
    return Int(2) * e + Int(d);
}

template<bool d>
static constexpr Int ToDedge( const Int e )
{
    return Int(2) * e + Int(d);
}

// You should always do a check that de >= 0 before calling this!
static constexpr std::pair<Int,HeadTail_T> FromDedge( const Int de )
{
    return std::pair( de / Int(2), de % Int(2) );
}

static constexpr Int FlipDedge( const Int de )
{
    return de ^ Int(1);
}


bool DedgeActiveQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeActiveMask) != EdgeFlag_T(0);
}

bool EdgeActiveQ( const Int e ) const
{
    return DedgeActiveQ(ToDedge<Tail>(e));
//    return DedgeActiveQ(ToDedge<Tail>(e)) && DedgeActiveQ(ToDedge<Head>(e));
}


bool DedgeVisitedQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeVisitedMask) != EdgeFlag_T(0);
}

bool DedgeExteriorQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeExteriorMask) != EdgeFlag_T(0);
}

bool DedgeVirtualQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeVirtualMask) != EdgeFlag_T(0);
}

bool DedgeUnconstrainedQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeUnconstrainedMask) != EdgeFlag_T(0);
}




void MarkDedgeAsExterior( const Int de ) const
{
    E_flag.data()[de] |= EdgeExteriorMask;
}

void MarkDedgeAsInterior( const Int de ) const
{
    E_flag.data()[de] &= ~EdgeExteriorMask;
}

void MarkDedgeAsVisited( const Int de ) const
{
    E_flag.data()[de] |= EdgeVisitedMask;
}

void MarkDedgeAsUnvisited( const Int de ) const
{
    E_flag.data()[de] &= ~EdgeVisitedMask;
}

void MarkDedgeAsUnconstrained( const Int de ) const
{
    E_flag.data()[de] |= EdgeUnconstrainedMask;
}

void MarkDedgeAsConstrained( const Int de ) const
{
    E_flag.data()[de] &= ~EdgeUnconstrainedMask;
}



private:

//Int DiEdgeLeftDiEdge( const Int de )
//{
//    auto [e,d] = FromDedge(de);
//
//    const UInt e_dir = static_cast<UInt>(E_dir(e));
//
//    const UInt t = E_turn.data()[de];
//    const UInt s = (e_dir + (d ? UInt(2) : UInt(0)) ) % UInt(4);
//    const Int  c = E_V.data()[de];
//
//    return V_dE(c,s);
//}

void ComputeEdgeLeftDedges()
{
    TOOLS_PTIMER(timer,MethodName("ComputeEdgeLeftDedges"));
    
    const Int e_count = E_V.Dim(0);
    
    E_left_dE = EdgeContainer_T ( e_count, Uninitialized );
    
    mptr<Int>    dE_left_dE = E_left_dE.data();
    cptr<Turn_T> dE_turn    = E_turn.data();
    cptr<Int>    dE_V       = E_V.data();

    for( Int e = 0; e < e_count; ++e )
    {
        const Int de_0 = ToDedge<Tail>(e);
        const Int de_1 = ToDedge<Head>(e);
        
        if ( !DedgeActiveQ(de_0) || !DedgeActiveQ(de_1) ) { continue; }

//        TOOLS_LOGDUMP(e);
//        TOOLS_LOGDUMP(de_0);
//        TOOLS_LOGDUMP(de_1);
        
        const Dir_T e_dir = E_dir(e);
        
        const Turn_T t_0  = dE_turn[de_0];
        const Turn_T t_1  = dE_turn[de_1];
        const Dir_T  s_0  = static_cast<Dir_T>(t_0 + Turn_T(2) + e_dir) % Dir_T(4);
        const Dir_T  s_1  = static_cast<Dir_T>(t_1 +           + e_dir) % Dir_T(4);
        const Int    c_0  = dE_V[de_0];
        const Int    c_1  = dE_V[de_1];
        
        const Int df_0 = dE_left_dE[de_0] = V_dE(c_0,s_0);
        const Int df_1 = dE_left_dE[de_1] = V_dE(c_1,s_1);
        
        // TODO: I think this ought to be removed.
        if ( (t_0 == Turn_T(-1)) && (dE_turn[df_0] == Turn_T(-1)) )
        {
            MarkDedgeAsUnconstrained(df_0);
        }
        // TODO: I think this ought to be removed.
        if ( (t_1 == Turn_T(-1)) && (dE_turn[df_1] == Turn_T(-1)) )
        {
            MarkDedgeAsUnconstrained(df_1);
        }
    }
    
//    TOOLS_LOGDUMP(E_left_dE);
}


public:

template<bool verboseQ = true>
bool CheckEdgeDirection( Int e )
{
    std::string tag = MethodName("CheckEdgeDirection")+"(" + ToString(e) + ")";
    
    if constexpr ( verboseQ )
    {
        logprint(tag);
    }
    
    if ( !InIntervalQ(e,Int(0),E_V.Dim(0)) )
    {
        if constexpr ( verboseQ )
        {
            eprint(tag + ": Edge index " + ToString(e) + " is out of bounds.");
        }
        return false;
    }
    
    const Int de_0 = ToDedge<Tail>(e);
    const Int de_1 = ToDedge<Head>(e);
    
    if ( DedgeActiveQ(de_0) != DedgeActiveQ(de_1) )
    {
        if constexpr ( verboseQ )
        {
            eprint(tag + ": The two dedges of " + EdgeString(e) + " have different activicty status.");
            
            TOOLS_DDUMP(DedgeActiveQ(ToDedge<Tail>(e)));
            TOOLS_DDUMP(DedgeActiveQ(ToDedge<Head>(e)));
        }
        return false;
    }
    
    if ( !DedgeActiveQ(de_0) ) { return true; }
    
    if ( !this->CheckDedge<1,verboseQ>(de_0) )
    {
        eprint(tag + ": Backward " + DedgeString(de_0) + " is not okay.");
        return false;
    }
    if ( !this->CheckDedge<1,verboseQ>(de_1) )
    {
        eprint(tag + ": Forward " + DedgeString(de_1) + " is not okay.");
        return false;
    }
       
    const Int tail = E_V(e,Tail);
    const Int head = E_V(e,Head);
    
    if ( !this->CheckVertex<1,verboseQ>(tail) )
    {
        eprint(tag + ": Tail " + VertexString(tail) + " is not okay.");
        return false;
    }
    if ( !this->CheckVertex<1,verboseQ>(head) )
    {
        eprint(tag + ": Head " + VertexString(head) + " is not okay.");
        return false;
    }
    
    
    const Dir_T dir       = E_dir[e];
    const Dir_T tail_port = E_dir[e];
    const Dir_T head_port = static_cast<Dir_T>( (static_cast<UInt>(dir) + Dir_T(2) ) % Dir_T(4) );
    
    const bool tail_okayQ = (V_dE(tail,tail_port) == de_1);
    const bool head_okayQ = (V_dE(head,head_port) == de_0);
    
    if constexpr ( verboseQ )
    {
        if ( !tail_okayQ )
        {
            if constexpr ( verboseQ )
            {
                eprint(tag + ": " + EdgeString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its tail " + VertexString(tail) + ".");
            }
        }
        if ( !head_okayQ )
        {
            if constexpr ( verboseQ )
            {
                eprint(tag + ": " + EdgeString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(head_port) + "ern port of its head " + VertexString(head) + ".");
            }
        }
    }
    
    return tail_okayQ && head_okayQ;
}

template<bool verboseQ = false>
bool CheckEdgeDirections()
{
    TOOLS_PTIMER(timer,MethodName("CheckEdgeDirections"));
    
    const Int E_count = E_V.Dim(0);
    
    for( Int e = 0; e < E_count; ++e )
    {
        if ( !EdgeActiveQ(e) ) { continue; }
        
        if ( !this->template CheckEdgeDirection<verboseQ>(e) )
        {
            return false;
        }
    }
    return true;
}

private:

// TODO: Add connectivity checks.
template<bool required_activity,bool verboseQ = true>
bool CheckDedge( const Int de ) const
{
//        logprint(MethodName("AssertDedge<"+ToString(required_activity)+">(" + DedgeString(de) + ")"));
    
    if ( !InIntervalQ(de,Int(0),Int(2) * E_V.Dim(0)) )
    {
        if constexpr ( verboseQ )
        {
            eprint(MethodName("CheckDedge<1>") + ": Dedge index " + ToString(de) + " is out of bounds.");
        }
        return false;
    }
    
    if ( required_activity != DedgeActiveQ(de) )
    {
        if constexpr ( verboseQ )
        {
            eprint(MethodName("CheckDedge<"+ToString(required_activity)+">") + ": " + DedgeString(de) + " is not " + ( required_activity ? "active" : "inactive") + ".");
        }
        return false;
    }
    
    if constexpr( required_activity )
    {
        const Int c_0 = E_V.data()[FlipDedge(de)];
        const Int c_1 = E_V.data()[de];

        const Int V_count = V_dE.Dim(0);
        
        if ( !InIntervalQ(c_0,Int(0),V_count) )
        {
            if constexpr ( verboseQ )
            {
                eprint(MethodName("CheckDedge<1>") +": Tail " + VertexString(c_0) + " of " + DedgeString(de) + " is out of bounds.");
            }
            return false;
        }
        
        if ( !VertexActiveQ(c_0) )
        {
            if constexpr ( verboseQ )
            {
                eprint(MethodName("CheckDedge<1>") +": Tail " + VertexString(c_0) + " of " + DedgeString(de) + " is not active.");
            }
            return false;
        }
        
        if ( !InIntervalQ(c_1,Int(0),V_count) )
        {
            if constexpr ( verboseQ )
            {
                eprint(MethodName("CheckDedge<1>") +": Head " + VertexString(c_1) + " of " + DedgeString(de) + " is out of bounds.");
            }
            return false;
        }
        
        if ( !VertexActiveQ(c_1) )
        {
            if constexpr ( verboseQ )
            {
                eprint(MethodName("AssertDedge<1>") +": Head " + VertexString(c_1) + " of " + DedgeString(de) + " is not active.");
            }
            return false;
        }
    }
    
    return true;
}

template<bool required_activity, bool verboseQ = true>
bool CheckEdge( const Int e ) const
{
//        logprint(MethodName("AssertEdge<"+ToString(required_activity)+">(" + EdgeString(e) + ")"));
    
    if ( e == Uninitialized )
    {
        if constexpr( required_activity )
        {
            if constexpr ( verboseQ )
            {
                eprint(MethodName("CheckEdge<1>") +" " + EdgeString(e) + " is not active.");
            }
            return false;
        }
        else
        {
            return true;
        }
        
    }
    
    if ( !InIntervalQ(e,Int(0),E_V.Dim(0)) )
    {
        if constexpr ( verboseQ )
        {
            eprint(MethodName("CheckEdge<1>") + ": Edge index " + ToString(e) + " is out of bounds.");
        }
        return false;
    }
    
    return this->template CheckDedge<required_activity,verboseQ>(ToDedge<Head>(e));
}

template<bool verboseQ = true>
bool CheckDedgeConnectivity( const Int de_1 ) const
{
    std::string tag = MethodName("CheckDedgeConnectivity");
    
    if constexpr ( verboseQ )
    {
        logprint(tag);
    }
    
    if ( !this->template CheckDedge<1,verboseQ>(de_1) )
    {
        if constexpr ( verboseQ )
        {
            eprint(tag + ": dedge " + ToString(de_1) +  " is not okay.");
        }
        return false;
    }
    
    const Int de_0 = FlipDedge(de_1);
    
    const Int v_0 = E_V.data()[de_0];
    const Int v_1 = E_V.data()[de_1];
    
    if ( (V_dE(v_0,East) != de_1) && (V_dE(v_0,North) != de_1) && (V_dE(v_0,West) != de_1) && (V_dE(v_0,South) != de_1) )
    {
        if constexpr ( verboseQ )
        {
            eprint(tag + ": Tail " + VertexString(v_0) +  " of " + DedgeString(de_1) + "): is not properly attached.");
        }
        return false;
    }
    
    if ( (V_dE(v_1,East) != de_0) && (V_dE(v_1,North) != de_0) && (V_dE(v_1,West) != de_0) && (V_dE(v_1,South) != de_0) )
    {
        if constexpr ( verboseQ )
        {
            eprint(tag + ": tail " + VertexString(v_1) + " of " + DedgeString(de_0) + "): is not properly attached.");
        }
        return false;
    }
    
    return true;
}
