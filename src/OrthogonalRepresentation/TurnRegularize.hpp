public:


Int LastActiveEdge() const
{
    Int last_active_edge = E_V.Dim(0) - Int(1);
    
    for( Int e = E_V.Dim(0); e --> Int(0);  )
    {
        if( DedgeActiveQ(ToDedge<Head>(e)) )
        {
            last_active_edge = e;
            break;
        }
    }
    
    return last_active_edge;
}

void TurnRegularize()
{
    TOOLS_PTIMER(timer,ClassName()+"::TurnRegularize");

    if( proven_turn_regularQ ) { return; }
    
    this->ClearCache();
    
    // We need two reflex corners per virtual edge.
    const Int old_max_edge_count = E_V.Dim(0);
    const Int max_edge_count = max_arc_count + bend_count + bend_count/Int(2);
    

    if( max_edge_count > old_max_edge_count )
    {
        Resize(max_edge_count);
    }
    
    for( Int de_ptr = 0; de_ptr < Int(2) * E_V.Dim(0); ++de_ptr )
    {
        TurnRegularizeFace(de_ptr);
    }
    
    Resize(LastActiveEdge() + Int(1));

    proven_turn_regularQ = true;
}


Turn_T FaceTurns( const Int de_ptr ) const
{
    if( !DedgeActiveQ(de_ptr) ) { return Turn_T(0); }
    
    cptr<Turn_T> dE_left_dE = E_left_dE.data();
    cptr<Turn_T> dE_turn    = E_turn.data();
    Turn_T rot = 0;
    
    Int de = de_ptr;
    
    do
    {
        rot += dE_turn[de];
        de = dE_left_dE[de];
    }
    while( de != de_ptr );
    
    return rot;
}

bool CheckFaceTurns( const Int de_ptr ) const
{
    if( !DedgeActiveQ(de_ptr) ) { return true; }
    
    const Turn_T rot = FaceTurns(de_ptr);
        
    if( DedgeExteriorQ(de_ptr) )
    {
        if( rot != Turn_T(-4) )
        {
            eprint(ClassName() + "::CheckFaceTurns: found exterior face with incorrect rotation number.");
            TOOLS_DDUMP(rot);
            TOOLS_DDUMP(de_ptr);
            
            return false;
        }
    }
    else
    {
        if( rot != Turn_T(4) )
        {
            eprint(ClassName() + "::CheckFaceTurns: found interior face with incorrect rotation number.");
            TOOLS_DDUMP(rot);
            TOOLS_DDUMP(de_ptr);
            
            return false;
        }
    }
    
    return true;
}

private:


template<bool verboseQ = false>
std::tuple<Int,Int> FindKittyCorner( const Int de_ptr ) const
{
    if constexpr ( verboseQ )
    {
        print("FindKittyCorner("+ToString(de_ptr)+")");
    }
    
    if( !ValidIndexQ(de_ptr) )
    {
        return {Uninitialized,Uninitialized};
    }
    
    cptr<Int>   dE_left_dE = E_left_dE.data();
    cptr<Int>   dE_turn    = E_turn.data();
    
    // RE = reflex edge
    mptr<Int>   RE_rot = V_scratch.data();
    mptr<Int>   RE_de  = E_scratch.data();
    mptr<Int>   RE_d   = E_scratch.data() + edge_count;
    
    std::unordered_map<Turn_T,std::vector<Int>> rot_lut;

    const bool exteriorQ = DedgeExteriorQ(de_ptr);
    
    Int dE_counter = 0;
    Int RE_counter = 0;
    
    // Compute rotations.
    {
        Int rot = 0;
        Int de = de_ptr;
        do
        {
            if( dE_turn[de] == Turn_T(-1) )
            {
                RE_de [RE_counter] = de;
                RE_d  [RE_counter] = dE_counter;
                RE_rot[RE_counter] = rot;
                rot_lut[rot].push_back(RE_counter);
                
                RE_counter++;
                
//                // DEBUGGING
//                if( exteriorQ != DedgeExteriorQ(de) )
//                {
//                    eprint(ClassName()+"::FindKittyCorner: dedge " + ToString(de) + " has bounday flag set to " + ToString(DedgeExteriorQ(de)) + ", but face's boundary flag is " + ToString(exteriorQ) + "." );
//                }
            }
            ++dE_counter;
            rot += dE_turn[de];
            de = dE_left_dE[de];
        }
        while( (de != de_ptr) && dE_counter < edge_count );
        
        // TODO: The checks on reflex_counter should be irrelevant if the graph structure is intact.

        if( (de != de_ptr) && (dE_counter >= edge_count) )
        {
            eprint(ClassName()+"::FindKittyCorner: Aborted because two many elements were collected. The data structure is probably corrupted.");

            return {Uninitialized,Uninitialized};
        }
    }
    
    if( RE_counter <= Int(1) )
    {
        return {Uninitialized,Uninitialized};
    }

    if constexpr ( verboseQ )
    {
        TOOLS_DUMP(dE_counter);
        TOOLS_DUMP(RE_counter);
    }
    
    Int p_min = 0;
    Int q_min = 0;
    Int d_min = Scalar::Max<Int>;
    
    constexpr Turn_T targets [2] = { Turn_T(2), Turn_T(-6) };
    
    for( Int q = 0; q < RE_counter; ++q )
    {
        const Int rot = RE_rot[q];
        
        for( Int k = 0; k < Int(1) + exteriorQ; ++k )
        {
            const Int target = rot - targets[k];
            
            if( rot_lut.count(target) <= Size_T(0) ) { continue; }
            

            const Int j = RE_d[q];
            
            auto & list = rot_lut[target];
            
            if constexpr ( verboseQ )
            {
                print("===============");
                TOOLS_DUMP(q);
                TOOLS_DUMP(RE_rot[q]);
                TOOLS_DUMP(rot - Turn_T(2));
                TOOLS_DUMP(j);
                
                std::string s = "list = ";
                for( auto & p : list )
                {
                    s += ToString(RE_d[p]) + ",";
                }
                
                print(s);
            }
            

            
            const Int p = BinarySearch( list.data(), RE_d, int_cast<Int>(list.size()), j );

            const Int i = RE_d[p];
            const Int d = ModDistance(dE_counter,i,j);

            if constexpr ( verboseQ )
            {
                TOOLS_DUMP(p);
                TOOLS_DUMP(RE_rot[p]);
                TOOLS_DUMP(i);
                TOOLS_DUMP(d);
            }
            
            if( (i < j) && (d < d_min) )
            {
                p_min = p;
                q_min = q;
                d_min = d;
            }
        }
    }
    
    if constexpr ( verboseQ )
    {
        TOOLS_DUMP(p_min);
        TOOLS_DUMP(q_min);
        TOOLS_DUMP(d_min);
        print(">>>>>>>>>>>>>>>>>");
    }
    
    if( p_min != q_min )
    {
        if constexpr ( verboseQ )
        {
            TOOLS_DUMP(RE_de[p_min]);
            TOOLS_DUMP(RE_de[q_min]);
        }
        
        return {RE_de[p_min],RE_de[q_min]};
    }
    else
    {
        if constexpr ( verboseQ )
        {
            print("No kitty corners found.");
        }
        return {Uninitialized,Uninitialized};
    }
}

private:

/*!@brief Check whether directed edge `de_ptr` is active and invisited. If affirmative, check whether the left face of `de_ptr` is turn-regular. If yes, return false (nothing changed); otherwise split the face by inserting a virtual edge and apply `TurnRegularizeFace` to both directed edges of the inserted virtual edge.
 *
 */

template<bool debugQ = false>
bool TurnRegularizeFace( const Int de_ptr )
{
    if( !DedgeActiveQ(de_ptr) || DedgeVisitedQ(de_ptr) ) { return false; }
    
    
    if constexpr ( debugQ )
    {
        if( !CheckFaceTurns(de_ptr) )
        {
            eprint(ClassName()+"TurnRegularize(): CheckFaceTurns failed on entry of TurnRegularizeFace on de_ptr = " + ToString(de_ptr) + ".");
        }
    }
    
    mptr<Int>   dE_V       = E_V.data();
    mptr<Int>   dE_left_dE = E_left_dE.data();
    mptr<Int>   dE_turn    = E_turn.data();
    mptr<UInt8> dE_flag    = E_flag.data();

    // TODO: We should cycle around the face just once and collect all directed edges.
    
//    print("FindKittyCorner...");
    auto [da_0,db_0] = this->template FindKittyCorner<false>(de_ptr);
//    print("FindKittyCorner done.");
    

    if( !ValidIndexQ(da_0) )
    {
        MarkFaceAsVisited(de_ptr);
        return false;
    }
    
    const bool exteriorQ = DedgeExteriorQ(de_ptr);

    const Int db_1 = dE_left_dE[db_0];
    const Int da_1 = dE_left_dE[da_0];
    
    const Int c_0 = dE_V[da_0];
    const Int c_1 = dE_V[db_0];
    
//      This situation, up to orientation preserving rotations:
//      the edge a_0, a_1, b_0, b_1 must be pairwise distinct.
//
//                            X
//                            ^
//                            |
//                        db_1|
//                            |   db_0
//                         c_1X<--------X
//                          /
//                        /
//             da_0  c_0/   e
//          X-------->X
//                    |
//                da_1|
//                    |
//                    v
//                    X
    
    
    // Create new edge.
    const Int e = edge_count;
    ++edge_count;
    ++virtual_edge_count;
    ++face_count;

    // The new edge; de_0 in forward direction; de_1 in reverse direction.
    Int de_0 = ToDarc(e,Tail);
    Int de_1 = ToDarc(e,Head);
    dE_V[de_0] = c_0;
    dE_V[de_1] = c_1;

    // See image above.
    dE_left_dE[da_0] = de_1;
    dE_left_dE[db_0] = de_0;
    dE_left_dE[de_0] = da_1;
    dE_left_dE[de_1] = db_1;

    // TODO: Randomize this?
    bool e_parallel_to_da_0 = true;
    
    // If e_parallel_to_da_0 == true, then s_0 = TRE_dir[a_0]; otherwise we turn by 90 degrees.
    
    auto [a_0,d_0] = FromDarc(da_0);
    
    Dir_T s_0 = (E_dir[a_0] + !e_parallel_to_da_0) % Dir_T(4);
    // s_1 is always the opposite slot of s_0.
    Dir_T s_1 = (s_0 + Dir_T(2)) % Dir_T(4);
    
    if( !d_0 ) // da_0 is a_0 reversed.
    {
        std::swap(s_0,s_1);
    }
    
    V_dE(c_0,s_0) = de_1;
    V_dE(c_1,s_1) = de_0;
     
    E_dir[e] = s_0;
    dE_turn[de_0] = Turn_T( e_parallel_to_da_0);
    dE_turn[de_1] = Turn_T( e_parallel_to_da_0);
    dE_turn[da_0] = Turn_T(!e_parallel_to_da_0);
    dE_turn[db_0] = Turn_T(!e_parallel_to_da_0);
    
    dE_flag[de_0] = ( EdgeActiveMask | EdgeVirtualMask );
    dE_flag[de_1] = ( EdgeActiveMask | EdgeVirtualMask );
    
    // After splitting the face, we mark the bigger of the two residual faces as exterior face.
    if( exteriorQ )
    {
        const Int t_0 = FaceTurns(de_0);
        const Int t_1 = FaceTurns(de_1);
        
        if( (t_0 == Turn_T(4)) && (t_1 == Turn_T(-4)) )
        {
            MarkFaceAsInterior(de_0);
            MarkFaceAsExterior(de_1);
        }
        else if( (t_0 == Turn_T(-4)) && (t_1 == Turn_T(4)) )
        {
            MarkFaceAsExterior(de_0);
            MarkFaceAsInterior(de_1);
        }
        else
        {
            eprint(MethodName("TurnRegularizeFace") + ": Inconsistent split of exterior face detected.");
        }
    }
    
    bool de_0_split = TurnRegularizeFace(de_0);
    bool de_1_split = TurnRegularizeFace(de_1);

    if constexpr ( debugQ )
    {
        if( !de_0_split )
        {
            if( !CheckFaceTurns(de_0) )
            {
                eprint(ClassName()+"TurnRegularize(): CheckFaceTurns failed after calling TurnRegularizeFace on de_0 = " + ToString(de_0) + ".");
            }
        }
        if( !de_1_split )
        {
            if( !CheckFaceTurns(de_1) )
            {
                eprint(ClassName()+"TurnRegularize(): CheckFaceTurns failed after calling TurnRegularizeFace on de_1 = " + ToString(de_1) + ".");
            }
        }
    }
    
    return de_0_split || de_1_split;
}

public:

// TODO: Reimplement this.
//bool CheckTurnRegularity()
//{
//    for( Int de = 0; de < Int(2) * TRE_count; ++de )
//    {
//        TRE_Unvisit(de);
//    }
//    
//    bool okayQ = true;
//    
//    for( Int de_ptr = 0; de_ptr < Int(2) * TRE_count; ++de_ptr )
//    {
//        if( !TRE_ActiveQ(de_ptr) || TRE_VisitedQ(de_ptr) ) { continue; }
//        
//        auto [da,db] = FindKittyCorner_Naive(de_ptr);
//        
//        if( da != Uninitialized )
//        {
//            okayQ = false;
//            
//            eprint(ClassName()+"::CheckTurnRegularity: Face containing TRE " + ToString(de_ptr) + " is not regular.");
//            
//            TOOLS_DUMP(da);
//            TOOLS_DUMP(db);
//        }
//    }
//    
//    return okayQ;
//}

// TODO: Reimplement this.
//template<bool bound_checkQ = true,bool verboseQ = true>
//bool CheckTreDirection( Int e )
//{
//    if constexpr( bound_checkQ )
//    {
//        if ( (e < Int(0)) || (e >= TRE_count) )
//        {
//            eprint(ClassName()+"::CheckTreDirection: Index " + ToString(e) + " is out of bounds.");
//            
//            return false;
//        }
//    }
//    
//    if ( (e < edge_count) && !EdgeActiveQ(e) )
//    {
//        return true;
//    }
//       
//    const Int tail = TRE_V(e,Tail);
//    const Int head = TRE_V(e,Head);
//    
//    const Dir_T dir       = TRE_dir[e];
//    const Dir_T tail_port = TRE_dir[e];
//    const Dir_T head_port = static_cast<Dir_T>(
//        (static_cast<UInt>(dir) + Dir_T(2) ) % Dir_T(4)
//    );
//    
//    const bool tail_okayQ = (V_dTRE(tail,tail_port) == ToDedge<Head>(e));
//    const bool head_okayQ = (V_dTRE(head,head_port) == ToDedge<Tail>(e));
//    
//    if constexpr( verboseQ )
//    {
//        if( !tail_okayQ )
//        {
//            eprint(ClassName()+"::CheckTreDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its tail " + ToString(tail) + ".");
//        }
//        if( !head_okayQ )
//        {
//            eprint(ClassName()+"::CheckTreDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its head " + ToString(head) + ".");
//        }
//    }
//    return tail_okayQ && head_okayQ;
//}

// TODO: Reimplement this.
//template<bool verboseQ = true>
//bool CheckTreDirections()
//{
//    bool okayQ = true;
//    
//    for( Int e = 0; e < TRE_count; ++e )
//    {
//        if ( !this->template CheckTreDirection<false,verboseQ>(e) )
//        {
//            okayQ = false;
//        }
//    }
//    return okayQ;
//}


//private:
//
//// Turn-regularized edges (TRE)
//
//bool TRE_ActiveQ( const Int de ) const
//{
//    return (TRE_flag.data()[de] & ActiveMask) != UInt(0);
//}
//
//bool TRE_VisitedQ( const Int de ) const
//{
//    return (TRE_flag.data()[de] & VisitedMask) != UInt(0);
//}
//
//void TRE_Visit( const Int de ) const
//{
//    TRE_flag.data()[de] |= VisitedMask;
//}
//
//void TRE_Unvisit( const Int de ) const
//{
//    TRE_flag.data()[de] &= (~VisitedMask);
//}
//
//bool TRE_BoundaryQ( const Int de ) const
//{
//    return (TRE_flag.data()[de] & BoundaryMask) != UInt(0);
//}
//
//bool TRE_VirtualQ( const Int de ) const
//{
//    return (TRE_flag.data()[de] & VirtualMask) != UInt(0);
//}

//public:
//
//
//template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
//void TRF_TraverseAll(
//    PreVisit_T  && pre_visit,
//    EdgeFun_T   && edge_fun,
//    PostVisit_T && post_visit
//)
//{
//    TOOLS_PTIC(ClassName()+"::TRF_TraverseAll");
//    
//    cptr<Int> dTRE_left_dTRE = TRE_left_dTRE.data();
//    
//    for( Int de = 0; de < Int(2) * TRE_count; ++de )
//    {
//        TRE_Unvisit(de);
//    }
//    
//    for( Int de_0 = 0; de_0 < Int(2) * TRE_count; ++de_0 )
//    {
//        if( !TRE_ActiveQ(de_0) || TRE_VisitedQ(de_0) ) { continue; }
//        
//        pre_visit();
//        
//        Int de = de_0;
//        do
//        {
//            edge_fun(de);
//            TRE_Visit(de);
//            de = dTRE_left_dTRE[de];
//        }
//        while( de != de_0 );
//        
//        post_visit();
//    }
//    
//    TOOLS_PTOC(ClassName()+"::TRF_TraverseAll");
//}

//void Compute_TRF()
//{
//    TOOLS_PTIC(ClassName()+"::Compute_TRF");
//    
//    
//    print("Compute_TRF");
//    // This is only an initial guess.
//    TRF_count = TRE_count;
//    
//    Aggregator<Int,Int> TRF_dTRE_ptr_agg ( TRF_count );
//    TRF_dTRE_ptr_agg.Push(Int(0));
//    
//    TRF_dTRE_idx = Tensor1<Int,Int>( Int(2) * TRE_count );
//    
//    TRE_TRF = EdgeContainer_T( TRE_count );
//    mptr<Int> dTRE_TRF = TRE_TRF.data();
//        
//    Int dE_counter = 0;
//    
//    TRF_TraverseAll(
//        [](){},
//        [&TRF_dTRE_ptr_agg,&dE_counter,dTRE_TRF,this]( const Int de )
//        {
//            const Int f = TRF_dTRE_ptr_agg.Size();
//            dTRE_TRF[de] = f - Int(1);
//            TRF_dTRE_idx[dE_counter] = de;
//            
//            ++dE_counter;
//        },
//        [&TRF_dTRE_ptr_agg,&dE_counter]()
//        {
//            TRF_dTRE_ptr_agg.Push(dE_counter);
//        }
//    );
//    
//    TRF_dTRE_ptr = TRF_dTRE_ptr_agg.Get();
//    TRF_count = TRF_dTRE_ptr.Size() - Int(1);
//    
//    
//    TOOLS_DUMP(TRF_dTRE_ptr.Size());
//    TOOLS_DUMP(TRF_dTRE_idx.Size());
//    
//    TOOLS_PTOC(ClassName()+"::Compute_TRF");
//}



private:

static Int BinarySearch(
    cptr<Int> pos, cptr<Int> val, const Int n, cref<Int> j
)
{
    static_assert(IntQ<Int>,"");
    
    if( n <= Int(0) )
    {
        eprint(ClassName()+"::FindNearestPosition_BinarySearch:: n <= 0.");
        return Int(0);
    }
    
    if( n == Int(1) )
    {
        return pos[Int(0)];
    }
    
    Int L = Int(0);
    Int R = n - Int(1);
    
    // L < R.
    
    // Binary search.
    while( L < R )
    {
        const Int C = R - (R-L)/static_cast<Int>(2);
        
        if( j < val[pos[C]] )
        {
            R = C - Int(1);
        }
        else
        {
            L = C;
        }
    }
    
    return pos[L];
    
} // BinarySearch
