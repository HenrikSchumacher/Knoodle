
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
    TOOLS_PTIC( ClassName()+"::ComputeEdgeLeftDedges" );
    
    E_left_dE = EdgeContainer_T ( edge_count, Int(-1) );
    
    for( Int e = 0; e < edge_count; ++e )
    {
        if( !EdgeActiveQ(e) )
        {
            wprint(ClassName()+"::ComputeEdgeLeftDedges: Skipping edge " + ToString(e) + ".");
            continue;
        }

        const Dir_T e_dir = E_dir(e);
        
        const Turn_T t_0  = E_turn(e,Tail);
        const Turn_T t_1  = E_turn(e,Head);
        const Dir_T  s_0  = static_cast<Dir_T>(t_0 + Turn_T(2) + e_dir) % Dir_T(4);
        const Dir_T  s_1  = static_cast<Dir_T>(t_1 +           + e_dir) % Dir_T(4);
        const Int    c_0  = E_V(e,Tail);
        const Int    c_1  = E_V(e,Head);
        E_left_dE(e,Tail) = V_dE(c_0,s_0);
        E_left_dE(e,Head) = V_dE(c_1,s_1);
        
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
    
    TOOLS_PTOC( ClassName()+"::ComputeEdgeLeftDedges" );
}


public:

template<bool bound_checkQ = true,bool verboseQ = true>
bool CheckEdgeDirection( Int e )
{
    if constexpr( bound_checkQ )
    {
        if ( (e < Int(0)) || (e >= edge_count) )
        {
            eprint(ClassName()+"::CheckEdgeDirection: Index " + ToString(e) + " is out of bounds.");
            
            return false;
        }
    }
    
    if ( !EdgeActiveQ(e) )
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
    for( Int e = 0; e < edge_count; ++e )
    {
        if ( !this->template CheckEdgeDirection<false,verboseQ>(e) )
        {
            return false;
        }
    }
    return true;
}
