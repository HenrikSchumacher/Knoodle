public:


/*! @brief Cycle around `de_ptr`'s left face and evaluate `edge_fun` on each directed edge.
 */

template<typename EdgeFun_T>
void TRF_Traverse( const Int de_ptr, EdgeFun_T && edge_fun )
{
    Int de = de_ptr;
    do
    {
        edge_fun(de);
        de = TRE_left_dTRE.data()[de];
    }
    while( de != de_ptr );
}


/*! @brief Cycle around `de_ptr`'s and count the number of edges.
 */

Int TRF_TRE_Count( const  Int de_ptr )
{
    Int de_counter = 0;
    
    TRF_Traverse(
        de_ptr,
        [&de_counter]( const Int de ) { ++de_counter; }
    );
    
    return de_counter;
}


void TRF_MarkAsExterior( const Int de_ptr )
{
    TRF_Traverse(
        de_ptr,
        [this]( const Int de ) { TRE_flag.data()[de] |= BoundaryMask; }
    );
}

void TRF_MarkAsInterior( const Int de_ptr )
{
    TRF_Traverse(
        de_ptr,
        [this]( const Int de ) { TRE_flag.data()[de] ^= (~BoundaryMask); }
    );
}

void TRF_MarkAsVisited( const Int de_ptr )
{
    TRF_Traverse(
        de_ptr,
        [this]( const Int de ) { TRE_flag.data()[de] |= VisitedMask; }
    );
}


void TurnRegularize( bool regularizeQ = true )
{
    TOOLS_PTIC(ClassName()+"::TurnRegularize");
    
    Int TRE_max_count = Int(2) * edge_count;
    
    V_dTRE        = V_dE;
    TRE_V         = EdgeContainer_T    ( TRE_max_count, Uninitialized );
    TRE_left_dTRE = EdgeContainer_T    ( TRE_max_count, Uninitialized );
    TRE_turn      = EdgeTurnContainer_T( TRE_max_count, Turn_T(0) );
    TRE_dir       = Tensor1<Dir_T,Int> ( TRE_max_count, East );
    
    mptr<Int> dTRE_V         = TRE_V.data();
    mptr<Int> dTRE_left_dTRE = TRE_left_dTRE.data();
    mptr<Int> dTRE_turn      = TRE_turn.data();
    
    E_V.Write( dTRE_V );
    E_left_dE.Write( dTRE_left_dTRE );
    E_turn.Write( dTRE_turn );
    E_dir.Write( TRE_dir.data() );
    
    tre_count = edge_count;

    TRE_flag = Tiny::VectorList_AoS<2,UInt8,Int>( TRE_max_count );
    
//    print("Mark active edges.");
    for( Int e = 0; e < edge_count; ++e )
    {
        if( EdgeActiveQ(e) )
        {
            TRE_flag(e,Tail) = ActiveMask;
            TRE_flag(e,Head) = ActiveMask;
        }
        else
        {
            TRE_flag(e,Tail) = UInt8(0);
            TRE_flag(e,Head) = UInt8(0);
        }
    }
    for( Int e = edge_count; e < TRE_max_count; ++e )
    {
        TRE_flag(e,Tail) = UInt8(0);
        TRE_flag(e,Head) = UInt8(0);
    }

    // TODO: This seems to be the only place where we need face information F_dE_ptr and F_dE_idx. We can avoid this by marking the edges already in LoadPlanarDiagram.
    
    ;
    
    TRF_MarkAsExterior( F_dE_idx[F_dE_ptr[exterior_face]] );

    if( regularizeQ )
    {
        for( Int de_ptr = 0; de_ptr < Int(2) * edge_count; ++de_ptr )
        {
            TurnRegularizeFace(de_ptr);
        }
    }
    
    TOOLS_PTOC(ClassName()+"::TurnRegularize");
}




// TODO: This routine has runtime quadradic in the number of arcs of the face.
// TODO: Improve this by implementing the method that has linear runtime.

/*!@brief Find the next best kitty corner. Start the search at `de_0` and then cycle counterclockwise around its left face. Return `{Uninitialized,Uninitialized}` if no kitty corner is found.
 *
 */

std::tuple<Int,Int> FindKittyCorner_Naive( const Int de_ptr ) const
{
//    print("FindKittyCorner_Naive("+ToString(de_ptr)+")");
    
    if( !ValidIndexQ(de_ptr) )
    {
        return {Uninitialized,Uninitialized};
    }
    
    cptr<Int>   dTRE_left_dTRE = TRE_left_dTRE.data();
    cptr<Int>   dTRE_turn      = TRE_turn.data();
    
    mptr<Int>   rotations      = V_scratch.data();
    mptr<Int>   reflex_edges   = E_scratch.data();
    
//    std::unordered_map<Turn_T,std::vector<Int>> rotation_lut;

    const bool exteriorQ = TRE_BoundaryQ(de_ptr);
    
    Int reflex_counter = 0;
    
    // Compute rotations.
    {
        Int rot = 0;
        Int de = de_ptr;
        do
        {
            if( dTRE_turn[de] == Turn_T(-1) )
            {
                reflex_edges[reflex_counter] = de;
                rotations[reflex_counter] = rot;
                reflex_counter++;
                
                // DEBUGGING
                if( exteriorQ != TRE_BoundaryQ(de) )
                {
                    eprint(ClassName()+"::FindKittyCorner_Naive: TREdge " + ToString(de) + " has bounday flag set to " + ToString(TRE_BoundaryQ(de)) + ", but face's boundary flag is " + ToString(exteriorQ) + "." );
                }
            }
            rot += dTRE_turn[de];
            de = dTRE_left_dTRE[de];
        }
        while( (de != de_ptr) && reflex_counter < edge_count );
        // TODO: The checks on reflex_counter should be irrelevant if the graph structure is intact.

        if( (de != de_ptr) && (reflex_counter >= edge_count) )
        {
            eprint(ClassName()+"::FindKittyCorner_Naive: Aborted because two many elements were collected. The data structure is probably corrupted.");

            return {Uninitialized,Uninitialized};
        }
    }

//    valprint("rotation", ArrayToString(rotation,{reflex_counter}));

//    const Int target_turn = exteriorQ ? Turn_T(2) : Turn_T(-6);
    
    if( exteriorQ )
    {
        for( Int i = 0; i < reflex_counter; ++i )
        {
            for( Int j = i; j < reflex_counter; ++j )
            {
                const Int rot = rotations[j] - rotations[i];
                
                if( (rot == Turn_T(2)) || (rot == Turn_T(-6)) )
                {
                    return {reflex_edges[i],reflex_edges[j]};
                }
            }
        }
    }
    else
    {
        for( Int i = 0; i < reflex_counter; ++i )
        {
            for( Int j = i; j < reflex_counter; ++j )
            {
                const Int rot = rotations[j] - rotations[i];
                
                if( rot == Turn_T(2) )
                {
                    return {reflex_edges[i],reflex_edges[j]};
                }
            }
        }
    }
    
    return {Uninitialized,Uninitialized};
}

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
    
    cptr<Int>   dTRE_left_dTRE = TRE_left_dTRE.data();
    cptr<Int>   dTRE_turn      = TRE_turn.data();
    
    // RE = reflex edge
    mptr<Int>   RE_rot = V_scratch.data();
    mptr<Int>   RE_de  = E_scratch.data();
    mptr<Int>   RE_d   = E_scratch.data() + edge_count;
    
    std::unordered_map<Turn_T,std::vector<Int>> rot_lut;

    const bool exteriorQ = TRE_BoundaryQ(de_ptr);
    
    Int dE_counter = 0;
    Int RE_counter = 0;
    
    // Compute rotations.
    {
        Int rot = 0;
        Int de = de_ptr;
        do
        {
            if( dTRE_turn[de] == Turn_T(-1) )
            {
                RE_de [RE_counter] = de;
                RE_d  [RE_counter] = dE_counter;
                RE_rot[RE_counter] = rot;
                rot_lut[rot].push_back(RE_counter);
                
                RE_counter++;
                
                // DEBUGGING
                if( exteriorQ != TRE_BoundaryQ(de) )
                {
                    eprint(ClassName()+"::FindKittyCorner: TREdge " + ToString(de) + " has bounday flag set to " + ToString(TRE_BoundaryQ(de)) + ", but face's boundary flag is " + ToString(exteriorQ) + "." );
                }
            }
            ++dE_counter;
            rot += dTRE_turn[de];
            de = dTRE_left_dTRE[de];
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

//    valprint("rotation", ArrayToString(rotation,{reflex_counter}));

//    const Int target_turn = exteriorQ ? Turn_T(2) : Turn_T(-6);
    
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

/*!@brief Check whether directed edge `de_ptr` is active and invisited. If affirmative, check whether the left face of `de_ptr` is turn-regular. If yes, return false (nothing changed); otherwise split the face by inserting a virtual edge and apply `TurnRegularizeFace` to both directed edges of the inserted virtual edge.
 *
 */
bool TurnRegularizeFace( const Int de_ptr )
{
    if( !TRE_ActiveQ(de_ptr) || TRE_VisitedQ(de_ptr) ) { return false; }
 
    mptr<Int>   dTRE_V         = TRE_V.data();
    mptr<Int>   dTRE_left_dTRE = TRE_left_dTRE.data();
    mptr<Int>   dTRE_turn      = TRE_turn.data();
    mptr<UInt8> dTRE_flag      = TRE_flag.data();

    // TODO: We should cycle around the face just once and collect all directed edges.
    
//    print("FindKittyCorner...");
    auto [da_0,db_0] = this->template FindKittyCorner<false>(de_ptr);
//    print("FindKittyCorner done.");

    if( !ValidIndexQ(da_0) )
    {
        TRF_MarkAsVisited(de_ptr);
        return false;
    }
    
    const bool externalQ = TRE_BoundaryQ(de_ptr);

    const Int db_1 = dTRE_left_dTRE[db_0];
    const Int da_1 = dTRE_left_dTRE[da_0];
    
    const Int c_0 = dTRE_V[da_0];
    const Int c_1 = dTRE_V[db_0];
    
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
    const Int e = tre_count;
    ++tre_count;

    // The new edge; de_0 in forward direction; de_1 in reverse direction.
    Int de_0 = ToDarc(e,Tail);
    Int de_1 = ToDarc(e,Head);
    dTRE_V[de_0] = c_0;
    dTRE_V[de_1] = c_1;

    // See image above.
    dTRE_left_dTRE[da_0] = de_1;
    dTRE_left_dTRE[db_0] = de_0;
    dTRE_left_dTRE[de_0] = da_1;
    dTRE_left_dTRE[de_1] = db_1;

    // TODO: Randomize this?
    bool e_parallel_to_da_0 = true;
    
    // If e_parallel_to_da_0 == true, then s_0 = TRE_dir[a_0]; otherwise we turn by 90 degrees.
    
    auto [a_0,d_0] = FromDarc(da_0);
    
//        TOOLS_DUMP(a_0);
//        TOOLS_DUMP(TRE_dir[a_0]);
    
    Dir_T s_0 = (TRE_dir[a_0] + !e_parallel_to_da_0) % Dir_T(4);
    // s_1 is always the opposite slot of s_0.
    Dir_T s_1 = (s_0 + Dir_T(2)) % Dir_T(4);
    
    if( !d_0 ) // da_0 is a_0 reversed.
    {
        std::swap(s_0,s_1);
    }

//        TOOLS_DUMP(s_0);
//        TOOLS_DUMP(s_1);
//
//        TOOLS_DUMP(V_dTRE(c_0,s_0));
//        TOOLS_DUMP(V_dTRE(c_1,s_1));
    
    V_dTRE(c_0,s_0) = de_1;
    V_dTRE(c_1,s_1) = de_0;
    
//        TOOLS_DUMP(V_dTRE(c_0,s_0));
//        TOOLS_DUMP(V_dTRE(c_1,s_1));
     
    TRE_dir[e] = s_0;
    dTRE_turn[de_0] = Turn_T( e_parallel_to_da_0);
    dTRE_turn[de_1] = Turn_T( e_parallel_to_da_0);
    dTRE_turn[da_0] = Turn_T(!e_parallel_to_da_0);
    dTRE_turn[db_0] = Turn_T(!e_parallel_to_da_0);
    
    dTRE_flag[de_0] = ( ActiveMask | VirtualMask );
    dTRE_flag[de_1] = ( ActiveMask | VirtualMask );
    
    // After splitting the face, we mark the bigger of the two residual faces as exterior face.
    if( externalQ )
    {
        const Int edge_count_0 = TRF_TRE_Count(de_0);
        const Int edge_count_1 = TRF_TRE_Count(de_1);
        
        if( edge_count_0 < edge_count_1 )
        {
            TRF_MarkAsInterior(de_0);
            TRF_MarkAsExterior(de_1);
        }
        else
        {
            TRF_MarkAsExterior(de_0);
            TRF_MarkAsInterior(de_1);
        }
    }
    
    bool de_0_split = TurnRegularizeFace(de_0);
    bool de_1_split = TurnRegularizeFace(de_1);
    
    return de_0_split || de_1_split;
}

bool CheckTurnRegularity()
{
    for( Int de = 0; de < Int(2) * tre_count; ++de )
    {
        TRE_Unvisit(de);
    }
    
    bool okayQ = true;
    
    for( Int de_ptr = 0; de_ptr < Int(2) * tre_count; ++de_ptr )
    {
        if( !TRE_ActiveQ(de_ptr) || TRE_VisitedQ(de_ptr) ) { continue; }
        
        auto [da,db] = FindKittyCorner_Naive(de_ptr);
        
        if( da != Uninitialized )
        {
            okayQ = false;
            
            eprint(ClassName()+"::CheckTurnRegularity: Face containing TRE " + ToString(de_ptr) + " is not regular.");
            
            TOOLS_DUMP(da);
            TOOLS_DUMP(db);
        }
    }
    
    return okayQ;
}


template<bool bound_checkQ = true,bool verboseQ = true>
bool CheckTreDirection( Int e )
{
    if constexpr( bound_checkQ )
    {
        if ( (e < Int(0)) || (e >= tre_count) )
        {
            eprint(ClassName()+"::CheckTreDirection: Index " + ToString(e) + " is out of bounds.");
            
            return false;
        }
    }
    
    if ( (e < edge_count) && !EdgeActiveQ(e) )
    {
        return true;
    }
       
    const Int tail = TRE_V(e,Tail);
    const Int head = TRE_V(e,Head);
    
    const Dir_T dir       = TRE_dir[e];
    const Dir_T tail_port = TRE_dir[e];
    const Dir_T head_port = static_cast<Dir_T>(
        (static_cast<UInt>(dir) + Dir_T(2) ) % Dir_T(4)
    );
    
    const bool tail_okayQ = (V_dTRE(tail,tail_port) == ToDedge(e,Head));
    const bool head_okayQ = (V_dTRE(head,head_port) == ToDedge(e,Tail));
    
    if constexpr( verboseQ )
    {
        if( !tail_okayQ )
        {
            eprint(ClassName()+"::CheckTreDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its tail " + ToString(tail) + ".");
        }
        if( !head_okayQ )
        {
            eprint(ClassName()+"::CheckTreDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its head " + ToString(head) + ".");
        }
    }
    return tail_okayQ && head_okayQ;
}

template<bool verboseQ = true>
bool CheckTreDirections()
{
    bool okayQ = true;
    
    for( Int e = 0; e < tre_count; ++e )
    {
        if ( !this->template CheckTreDirection<false,verboseQ>(e) )
        {
            okayQ = false;
        }
    }
    return okayQ;
}




private:

// Turn-regularized edges (TRE)

bool TRE_ActiveQ( const Int de ) const
{
    return (TRE_flag.data()[de] & ActiveMask) != UInt(0);
}

bool TRE_VisitedQ( const Int de ) const
{
    return (TRE_flag.data()[de] & VisitedMask) != UInt(0);
}

void TRE_Visit( const Int de ) const
{
    TRE_flag.data()[de] |= VisitedMask;
}

void TRE_Unvisit( const Int de ) const
{
    TRE_flag.data()[de] &= (~VisitedMask);
}

bool TRE_BoundaryQ( const Int de ) const
{
    return (TRE_flag.data()[de] & BoundaryMask) != UInt(0);
}

bool TRE_VirtualQ( const Int de ) const
{
    return (TRE_flag.data()[de] & VirtualMask) != UInt(0);
}



public:


template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
void TRF_TraverseAll(
    PreVisit_T  && pre_visit,
    EdgeFun_T   && edge_fun,
    PostVisit_T && post_visit
)
{
    TOOLS_PTIC(ClassName()+"::TRF_TraverseAll");
    
    cptr<Int> dTRE_left_dTRE = TRE_left_dTRE.data();
    
    for( Int de = 0; de < Int(2) * tre_count; ++de )
    {
        TRE_Unvisit(de);
    }
    
    for( Int de_0 = 0; de_0 < Int(2) * tre_count; ++de_0 )
    {
        if( !TRE_ActiveQ(de_0) || TRE_VisitedQ(de_0) ) { continue; }
        
        pre_visit();
        
        Int de = de_0;
        do
        {
            edge_fun(de);
            TRE_Visit(de);
            de = dTRE_left_dTRE[de];
        }
        while( de != de_0 );
        
        post_visit();
    }
    
    TOOLS_PTOC(ClassName()+"::TRF_TraverseAll");
}

Tensor1<Int,Int> TRF_TRE_Pointers()
{
    TOOLS_PTIC(ClassName()+"::TRF_TRE_Pointers");
    
    Aggregator<Int,Int> agg ( vertex_count );
    agg.Push(Int(0));
    
    Int dE_counter = 0;
    
    TRF_TraverseAll(
        [](){},
        [&dE_counter]( const Int de )
        {
            ++dE_counter;
        },
        [&agg,&dE_counter]()
        {
            agg.Push(dE_counter);
        }
    );
    
    TOOLS_PTOC(ClassName()+"::TRF_TRE_Pointers");
    
    return agg.Get();
}

Tensor1<Int,Int> TRF_TRE_Indices()
{
    TOOLS_PTIC(ClassName()+"::TRF_TRE_Indices");
    
    Aggregator<Int,Int> agg ( vertex_count );
    
    TRF_TraverseAll(
        [](){},
        [&agg]( const Int de )
        {
            agg.Push(de);
        },
        [](){}
    );
    
    TOOLS_PTOC(ClassName()+"::TRF_TRE_Indices");
    
    return agg.Get();
}


Int TRE_Count() const
{
    return tre_count;
}

cref<EdgeContainer_T> TRE_Edges() const
{
    return TRE_V;
}

cref<FlagContainer_T> TRE_Flags() const
{
    return TRE_flag;
}





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
