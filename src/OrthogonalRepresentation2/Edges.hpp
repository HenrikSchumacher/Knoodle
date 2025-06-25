public:


// You should always do a check that e >= 0 before calling this!
static constexpr Int ToDedge( const Int e, const HeadTail_T d )
{
    return Int(2) * e + d;
}

template<bool d>
static constexpr Int ToDedge( const Int e )
{
    return Int(2) * e + d;
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

//bool EdgeActiveQ( const Int e ) const
//{
//    return DedgeActiveQ(ToDedge<Tail>(e)) && DedgeActiveQ(ToDedge<Head>(e));
//}


bool DedgeVisitedQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeVisitedMask) != EdgeFlag_T(0);
}

void DedgeVisit( const Int de ) const
{
    E_flag.data()[de] |= EdgeVisitedMask;
}

void DedgeUnvisit( const Int de ) const
{
    E_flag.data()[de] &= (~EdgeVisitedMask);
}

bool DedgeExteriorQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeExteriorMask) != EdgeFlag_T(0);
}

bool DedgeVirtualQ( const Int de ) const
{
    return (E_flag.data()[de] & EdgeVirtualMask) != EdgeFlag_T(0);
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
    TOOLS_PTIMER(timer,ClassName()+"::ComputeEdgeLeftDedges" );
    
    E_left_dE = EdgeContainer_T ( E_V.Dim(0), Int(-1) );
    
    mptr<Int>    dE_left_dE = E_left_dE.data();
    cptr<Turn_T> dE_turn    = E_turn.data();
    cptr<Int>    dE_V       = E_V.data();
    
    for( Int e = 0; e < E_V.Dim(0); ++e )
    {
        const Int de_0 = ToDedge<Tail>(e);
        const Int de_1 = ToDedge<Head>(e);
        
        if( !DedgeActiveQ(de_0) || !DedgeActiveQ(de_1) )
        {
//            wprint(ClassName()+"::ComputeEdgeLeftDedges: Skipping edge " + ToString(e) + ".");
            continue;
        }

        const Dir_T e_dir = E_dir(e);
        
        const Turn_T t_0  = dE_turn[de_0];
        const Turn_T t_1  = dE_turn[de_1];
        const Dir_T  s_0  = static_cast<Dir_T>(t_0 + Turn_T(2) + e_dir) % Dir_T(4);
        const Dir_T  s_1  = static_cast<Dir_T>(t_1 +           + e_dir) % Dir_T(4);
        const Int    c_0  = dE_V[de_0];
        const Int    c_1  = dE_V[de_1];
        dE_left_dE[de_0]  = V_dE(c_0,s_0);
        dE_left_dE[de_1]  = V_dE(c_1,s_1);
        
//        print("=========");
//        TOOLS_DUMP(E_turn(e,Tail));
//        TOOLS_DUMP(s_0);
//        TOOLS_DUMP(c_0);
//        valprint("V_dE.data(c_0)", ArrayToString(V_dE.data(c_0),{4}));
//        TOOLS_DUMP(E_left_dE(e,Tail));
//        print("=========");
//        TOOLS_DUMP(E_turn(e,Head));
//        TOOLS_DUMP(s_1);
//        TOOLS_DUMP(c_1);
//        valprint("V_dE.data(c_1)", ArrayToString(V_dE.data(c_1),{4}));
//        TOOLS_DUMP(E_left_dE(e,Head));
    }
}


public:

template<bool bound_checkQ = true,bool verboseQ = true>
bool CheckEdgeDirection( Int e )
{
    if constexpr( bound_checkQ )
    {
        if ( (e < Int(0)) || (e >= E_V.Dim(0)) )
        {
            eprint(ClassName()+"::CheckEdgeDirection: Index " + ToString(e) + " is out of bounds.");
            
            return false;
        }
    }
    
    if( DedgeActiveQ(Int(2) * e + Tail) != DedgeActiveQ(Int(2) * e + Head) )
    {
        eprint(ClassName()+"::CheckEdgeDirection: The two dedges of edge " + ToString(e) + " have different activicty status.");
        
        TOOLS_DUMP(DedgeActiveQ(Int(2) * e + Tail));
        TOOLS_DUMP(DedgeActiveQ(Int(2) * e + Head));
        
        return false;
    }
    
    if( !DedgeActiveQ(Int(2) * e) )
    {
        return true;
    }
       
    const Int tail = E_V(e,Tail);
    const Int head = E_V(e,Head);
    
    const Dir_T dir       = E_dir[e];
    const Dir_T tail_port = E_dir[e];
    const Dir_T head_port = static_cast<Dir_T>( (static_cast<UInt>(dir) + Dir_T(2) ) % Dir_T(4) );
    
    
    const bool tail_okayQ = (V_dE(tail,tail_port) == ToDedge<Head>(e));
    const bool head_okayQ = (V_dE(head,head_port) == ToDedge<Tail>(e));
    
    if constexpr( verboseQ )
    {
        if( !tail_okayQ )
        {
            eprint(ClassName()+"::CheckEdgeDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its tail " + ToString(tail) + ".");
        }
        if( !head_okayQ )
        {
            eprint(ClassName()+"::CheckEdgeDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its head " + ToString(head) + ".");
        }
    }
    return tail_okayQ && head_okayQ;
}

template<bool verboseQ = true>
bool CheckEdgeDirections()
{
    for( Int e = 0; e < E_V.Dim(0); ++e )
    {
        if ( !this->template CheckEdgeDirection<false,verboseQ>(e) )
        {
            return false;
        }
    }
    return true;
}
