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
    
    PRNG_T engine;
    
    if( settings.randomizeQ )
    {
        engine = InitializedRandomEngine<PRNG_T>();
    }
    
    this->ClearCache();
    
    // We need two reflex corners per virtual edge.
    const Int old_E_count = E_V.Dim(0);
    const Int E_count = A_C.Dim(0) + bend_count + bend_count/Int(2);

    if( E_count > old_E_count )
    {
        Resize(E_count);
    }
    
    const Int dE_count = Int(2) * E_V.Dim(0);
    
    for( Int de_ptr = 0; de_ptr < dE_count; ++de_ptr )
    {
        TurnRegularizeFace(engine,de_ptr);
    }
    
    Resize(LastActiveEdge() + Int(1));

    proven_turn_regularQ = true;
}


Turn_T FaceTurns( const Int de_ptr ) const
{
    if( !DedgeActiveQ(de_ptr) ) { return Turn_T(0); }
    
    cptr<Turn_T> dE_turn = E_turn.data();
    Turn_T rot = 0;
    
    TraverseFace(
        de_ptr,
        [&rot,dE_turn]( const Int de ) { rot += dE_turn[de]; },
        false
    );
//    
//    Int de = de_ptr;
//    do
//    {
//        rot += dE_turn[de];
//        de = dE_left_dE[de];
//    }
//    while( de != de_ptr );
    
    return rot;
}

bool CheckFaceTurns( const Int de_ptr ) const
{
    if( !DedgeActiveQ(de_ptr) ) { return true; }
    
    cptr<Turn_T> dE_turn = E_turn.data();
    Turn_T rot = 0;
    std::vector<Int> face;
    
//    const Turn_T rot = FaceTurns(de_ptr);
    
    TraverseFace(
        de_ptr,
        [&rot,&face,dE_turn]( const Int de )
        {
            face.push_back(de);
            rot += dE_turn[de];
        },
        false
    );
        
    if( DedgeExteriorQ(de_ptr) )
    {
        if( rot != Turn_T(-4) )
        {
            eprint(MethodName("CheckFaceTurns") + "(" + ToString(de_ptr) + "): found exterior face with incorrect rotation number.");
            TOOLS_DDUMP(rot);
            TOOLS_DDUMP(face);
            return false;
        }
    }
    else
    {
        if( rot != Turn_T(4) )
        {
            eprint(MethodName("CheckFaceTurns") + "(" + ToString(de_ptr) + "): found interior face with incorrect rotation number.");
            TOOLS_DDUMP(rot);
            TOOLS_DDUMP(face);
            return false;
        }
    }
    
    return true;
}

private:

// DEBUGGING
template<bool debugQ = false,bool verboseQ = false>
std::tuple<Int,Int> FindKittyCorner( const Int de_ptr ) const
{
//    if constexpr ( verboseQ )
//    {
//        logprint("FindKittyCorner("+ToString(de_ptr)+")");
//    }
    
    if( !ValidIndexQ(de_ptr) )
    {
        return {Uninitialized,Uninitialized};
    }
    
//    cptr<Int>    dE_left_dE = E_left_dE.data();
    cptr<Turn_T> dE_turn    = E_turn.data();
    
    const Int E_count = E_V.Dim(0);
    
    // RE = reflex edge
    mptr<Int>   RE_rot = V_scratch.data();
    mptr<Int>   RE_de  = E_scratch.data();
    mptr<Int>   RE_d   = E_scratch.data() + E_count;
    
    std::unordered_map<Turn_T,std::vector<Int>> rot_lut;

    const bool exteriorQ = DedgeExteriorQ(de_ptr);
    
    Int dE_counter = 0;
    Int RE_counter = 0;
    
    // Compute rotations.
    {
        Int rot = 0;
    
        this->TraverseFace<debugQ,verboseQ>(
            de_ptr,
            [
                &rot,&dE_counter,&RE_counter,&rot_lut,
                dE_turn,RE_rot,RE_de,RE_d,exteriorQ,this
            ]
            ( const Int de )
            {
                if( dE_turn[de] == Turn_T(-1) )
                {
                    RE_de [RE_counter] = de;
                    RE_d  [RE_counter] = dE_counter;
                    RE_rot[RE_counter] = rot;
                    rot_lut[rot].push_back(RE_counter);
                    
                    RE_counter++;
                    
                    if constexpr ( debugQ )
                    {
                        if( exteriorQ != this->DedgeExteriorQ(de) )
                        {
                            eprint(ClassName()+"::FindKittyCorner: dedge " + this->DedgeString(de) + " has boundaty flag set to " + ToString(this->DedgeExteriorQ(de)) + ", but face's boundary flag is " + ToString(exteriorQ) + "." );
                        }
                    }
                    else
                    {
                        (void)this;
                        (void)exteriorQ;
                    }
                }
                ++dE_counter;
                rot += dE_turn[de];
            },
            false
        );
    
        
//        // TODO: Use TraverseFace?
//        Int de = de_ptr;
//        do
//        {
//            this->template AssertDedge<true,debugQ>(de);
//
//            if( dE_turn[de] == Turn_T(-1) )
//            {
//                RE_de [RE_counter] = de;
//                RE_d  [RE_counter] = dE_counter;
//                RE_rot[RE_counter] = rot;
//                rot_lut[rot].push_back(RE_counter);
//                
//                RE_counter++;
//                
////                // DEBUGGING
////                if( exteriorQ != DedgeExteriorQ(de) )
////                {
////                    eprint(ClassName()+"::FindKittyCorner: dedge " + ToString(de) + " has bounday flag set to " + ToString(DedgeExteriorQ(de)) + ", but face's boundary flag is " + ToString(exteriorQ) + "." );
////                }
//            }
//            ++dE_counter;
//            rot += dE_turn[de];
//            de = dE_left_dE[de];
//        }
//        while( (de != de_ptr) && dE_counter < E_count );
//
//        // TODO: The checks on reflex_counter should be irrelevant if the graph structure is intact.
//
//        if( (de != de_ptr) && (dE_counter >= E_count) )
//        {
//            eprint(MethodName("FindKittyCorner")+": Aborted because too many elements were collected. The data structure is probably corrupted.");
//
//            return {Uninitialized,Uninitialized};
//        }
    }
    
    if( RE_counter <= Int(1) )
    {
        return {Uninitialized,Uninitialized};
    }

    if constexpr ( verboseQ )
    {
        TOOLS_LOGDUMP(dE_counter);
        TOOLS_LOGDUMP(RE_counter);
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
                logprint("===============");
                TOOLS_LOGDUMP(q);
                TOOLS_LOGDUMP(RE_rot[q]);
                TOOLS_LOGDUMP(rot - Turn_T(2));
                TOOLS_LOGDUMP(j);
                
                std::string s = "list = ";
                for( auto & p : list )
                {
                    s += ToString(RE_d[p]) + ",";
                }
                
                logprint(s);
            }
            
            const Int p = BinarySearch( list.data(), RE_d, int_cast<Int>(list.size()), j );

            const Int i = RE_d[p];
            
            const Int d = ModDistance(dE_counter,i,j);

            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(p);
                TOOLS_LOGDUMP(RE_rot[p]);
                TOOLS_LOGDUMP(i);
                TOOLS_LOGDUMP(d);
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
        TOOLS_LOGDUMP(p_min);
        TOOLS_LOGDUMP(q_min);
        TOOLS_LOGDUMP(d_min);
        logprint(">>>>>>>>>>>>>>>>>");
    }
    
    if( p_min != q_min )
    {
        if constexpr ( verboseQ )
        {
            TOOLS_LOGDUMP(RE_de[p_min]);
            TOOLS_LOGDUMP(RE_de[q_min]);
        }
        
        return {RE_de[p_min],RE_de[q_min]};
    }
    else
    {
        if constexpr ( verboseQ )
        {
            logprint("No kitty corners found.");
        }
        return {Uninitialized,Uninitialized};
    }
}

private:

/*!@brief Check whether directed edge `de_ptr` is active and invisited. If affirmative, check whether the left face of `de_ptr` is turn-regular. If yes, return false (nothing changed); otherwise split the face by inserting a virtual edge and apply `TurnRegularizeFace` to both directed edges of the inserted virtual edge.
 *
 */

// DEBUGGING
template<bool debugQ = false, bool verboseQ = false>
bool TurnRegularizeFace( mref<PRNG_T> engine, const Int de_ptr )
{
    if( !DedgeActiveQ(de_ptr) || DedgeVisitedQ(de_ptr) ) { return false; }

    if constexpr ( debugQ )
    {
        logprint("TurnRegularizeFace(" + ToString(de_ptr) + ")");
    
        this->template CheckDedge<1,true>(de_ptr);
        
        if( !CheckFaceTurns(de_ptr) )
        {
            eprint(MethodName("TurnRegularize") + "(" + ToString(de_ptr) + "): CheckFaceTurns failed on entry.");
        }
    }
    
    mptr<Int>    dE_V       = E_V.data();
    mptr<Int>    dE_left_dE = E_left_dE.data();
    mptr<Turn_T> dE_turn    = E_turn.data();
    mptr<UInt8>  dE_flag    = E_flag.data();

    // TODO: We should cycle around the face just once and collect all directed edges.
    
    if constexpr ( debugQ )
    {
        logprint("FindKittyCorner("+ToString(de_ptr)+")...");
    }
    auto [da_0,db_0] = this->FindKittyCorner<debugQ,verboseQ>(de_ptr);
    if constexpr ( debugQ )
    {
        logprint("FindKittyCorner("+ToString(de_ptr)+") done.");
    }
    

    if( !ValidIndexQ(da_0) )
    {
        MarkFaceAsVisited(de_ptr);
        return false;
    }
    
    if constexpr (debugQ)
    {
        this->template CheckDedge<1,true>(da_0);
        this->template CheckDedge<1,true>(db_0);
    }
    
    const bool exteriorQ = DedgeExteriorQ(de_ptr);

    const Int db_1 = dE_left_dE[db_0];
    const Int da_1 = dE_left_dE[da_0];
    
    if constexpr (debugQ)
    {
        this->template CheckDedge<1,true>(da_1);
        this->template CheckDedge<1,true>(db_1);
    }
    
    const Int c_0 = dE_V[da_0];
    const Int c_1 = dE_V[db_0];
    
    if constexpr (debugQ)
    {
        this->template CheckVertex<1,true>(c_0);
        this->template CheckVertex<1,true>(c_1);
    }
//      This situation, up to orientation preserving rotations:
//      the edges a_0, a_1, b_0, b_1 must be pairwise distinct.
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

    // No matter how we orient e, all the dedges da_0, da_1, db_0, db_1 become constrained:
    // If we make e is vertical, then:
    // - e is fused with da_1; fused dedge is constrained by da_0 and db_0
    // - e is fused with db_1; fused dedge is constrained by da_0 and db_0
    // - da_0 is constrained by db_1 and e
    // - db_0 is constrained by db_1 and e
    
    // If we make e is horizontal, then:
    // - e is fused with db_1; fused dedge is constrained by da_1 and db_1
    // - e is fused with da_1; fused dedge is constrained by da_1 and db_1
    // - da_1 is constrained by db_0 and e
    // - db_1 is constrained by db_0 and e

    // TODO: Do we have to switch this off if settings.soften_virtual_edgesQ?
    if( !settings.soften_virtual_edgesQ )
    {
        MarkDedgeAsConstrained(da_0);
        MarkDedgeAsConstrained(da_1);
        MarkDedgeAsConstrained(db_0);
        MarkDedgeAsConstrained(db_1);
    }
    
    
    // Create new edge.
    const Int e = E_end;
    ++E_end;
    ++edge_count;
    ++virtual_edge_count;
    
    // DEBUGGING
    if( E_end > E_V.Dim(0) )
    {
        eprint(MethodName("TurnRegularize") + "(" + ToString(de_ptr) + "): Edge overflow.");
    }

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

//    // TODO: Randomize this?
//    bool e_parallel_to_da_0 = true;
    
    std::uniform_int_distribution<int> dist (0,1);
    
    bool e_parallel_to_da_0 = settings.randomizeQ
                            ? dist(engine)
                            : true;
    
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

    if constexpr ( debugQ )
    {
        this->template CheckDedgeConnectivity<true>(de_0);
        this->template CheckDedgeConnectivity<true>(de_1);
        this->template CheckDedgeConnectivity<true>(da_0);
        this->template CheckDedgeConnectivity<true>(da_1);
        this->template CheckDedgeConnectivity<true>(db_0);
        this->template CheckDedgeConnectivity<true>(db_1);
    }

    
    // After splitting the face, we mark the bigger of the two residual faces as exterior face.
    if( exteriorQ )
    {
        const Turn_T t_0 = FaceTurns(de_0);
        const Turn_T t_1 = FaceTurns(de_1);
        
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
            eprint(MethodName("TurnRegularize") + "(" + ToString(de_ptr) + "): Inconsistent split of exterior face detected.");
            TOOLS_DDUMP(t_0);
            TOOLS_DDUMP(t_1);
        }
    }
    
    bool de_0_split = TurnRegularizeFace(engine,de_0);
    bool de_1_split = TurnRegularizeFace(engine,de_1);

    if constexpr ( debugQ )
    {
        if( !de_0_split )
        {
            if( !CheckFaceTurns(de_0) )
            {
                eprint(MethodName("TurnRegularize")+"(" + ToString(de_ptr) + "): CheckFaceTurns failed after calling TurnRegularizeFace on de_0 = " + ToString(de_0) + ".");
            }
        }
        if( !de_1_split )
        {
            if( !CheckFaceTurns(de_1) )
            {
                eprint(MethodName("TurnRegularize")+"(" + ToString(de_ptr) + "): CheckFaceTurns failed after calling TurnRegularizeFace on de_1 = " + ToString(de_1) + ".");
            }
        }
    }
    
    return de_0_split || de_1_split;
}

private:

static Int BinarySearch(
    cptr<Int> pos, cptr<Int> val, const Int n, cref<Int> j
)
{
    static_assert(IntQ<Int>,"");
    
    if( n <= Int(0) )
    {
        eprint(MethodName("FindNearestPosition_BinarySearch")+": n <= 0.");
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
