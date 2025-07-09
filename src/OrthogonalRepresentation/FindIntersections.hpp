public:

/*!@brief Computes the minimal Manhattan distance between two line segments `{X_0,X_1}` and `{Y_0,Y_1}` that are horizontal or vertical.
 */

static Int EdgeEdgeManhattanDistance(
    cptr<Int> X_0, cptr<Int> X_1, cptr<Int> Y_0, cptr<Int> Y_1
)
{
    Int dist = 0;
    
    for( Int k = 0; k < Int(2); ++k )
    {
        auto [a,b] = MinMax( X_0[k], X_1[k] );
        auto [c,d] = MinMax( Y_0[k], Y_1[k] );
        
        const Int A = Max(a,c);
        const Int B = Min(b,d);
        
        const Int x = (A > B) ? (A-B) : Int(0);
        
        dist += Abs(x);
    }

    return dist;
}

/*!@brief Computes the minimal Manhattan distance between two line segments given by the undirected edges `e_0` and `e_1` with vertex coordinates given by `coords`.
 */

Int EdgeEdgeManhattanDistance(
    cref<CoordsContainer_T> coords, const Int e_0, const Int e_1
)
{    
    const Int v [2][2] = {
        { E_V(e_0,Tail), E_V(e_0,Head) },
        { E_V(e_1,Tail), E_V(e_1,Head) },
    };
    
    // If the edges interect because their end vertices coincide (which can happen, e.g., if the PlanarDiagram had a Reidemeister I loop), then we have to report "no intersection"
    if(
       ( v[0][0] == v[1][0] )
       ||
       ( v[0][0] == v[1][1] )
       ||
       ( v[0][1] == v[1][0] )
       ||
       ( v[0][1] == v[1][1] )
    )
    {
        return Int(1);
    }
    
    return EdgeEdgeManhattanDistance(
        coords.data(v[0][0]),
        coords.data(v[0][1]),
        coords.data(v[1][0]),
        coords.data(v[1][1])
    );
}

template<bool verboseQ = false>
Tiny::VectorList_AoS<3,Int,Int> FindAllIntersections(
    cref<CoordsContainer_T> coords
)
{
    std::vector<std::array<Int,3>> agg;

    for( Int f = 0; f < face_count; ++f )
    {
        this->template FindIntersections<verboseQ>(coords,agg,f);
    }
    
    Tiny::VectorList_AoS<3,Int,Int> result ( int_cast<Int>( agg.size() ) );
    result.Read( &agg[0][0] );
    
    return result;
}

/*!@brief Cycles around the face ` f` and finds the first directed edge `de` that has at least one intersection with another edge. Then it returns the triple `{de,da,db}`, where `da` and `db` are the first edge and the last edge of the face that `de` intersects.
 *
 */
template<bool verboseQ = false>
void FindIntersections(
    cref<CoordsContainer_T> coords,
    mref<std::vector<std::array<Int,3>>> agg,
    const Int f
)
{
//    print("===============================================");
    if constexpr ( verboseQ )
    {
        print("FindIntersections("+ToString(f)+")");
    }
    
    const Int face_begin = F_dE_ptr[f    ];
    const Int face_end   = F_dE_ptr[f + 1];
    
    const Int n = face_end - face_begin;
    
    
//    TOOLS_DUMP(n);
    
    if( n <= Int(4) )
    {
        return;
    }
    
//    valprint("face", ArrayToString( &F_dE_idx[face_begin], {n} ) );
    
    for( Int i = 0; i < n; ++i )
    {
        const Int de_i = F_dE_idx[face_begin + i];
        auto [e_i,d_i] = FromDarc(de_i);
        
        Int da = Uninitialized;
        Int db = Uninitialized;
        
//        TOOLS_DUMP(i);
//        TOOLS_DUMP(de_i);
//        TOOLS_DUMP(e_i);
        
        for( Int delta = Int(2); delta < n - Int(1); ++delta )
        {
//            print("forward run");
//            TOOLS_DUMP(delta);
//            TOOLS_DUMP(i + delta);
            Int j = (i + delta < n) ? (i + delta) : (i + delta - n);
            const Int de_j = F_dE_idx[face_begin + j];
            auto [e_j,d_j] = FromDarc(de_j);
            
//            TOOLS_DUMP(j);
//            TOOLS_DUMP(de_j);
//            TOOLS_DUMP(e_j);
//            TOOLS_DUMP(ModDistance(n,i,j));
            
            const Int dist = EdgeEdgeManhattanDistance(coords,e_i,e_j);
            
            if( dist <= Int(0) )
            {
                da = de_j;
                break;
            }
        }
        
        if( da == Uninitialized )
        {
//            print(">>>>>>>>>>>> forward run found no intersection for de_i = " + ToString(de_i));
            continue;
        }
        
//        print(">>>>>>>>>>>> forward run found intersection for de_i = " + ToString(de_i) + " at da = " + ToString(da));
        
        for( Int delta = n - Int(1); delta --> Int(2); )
        {
//            print("backward run");
//            TOOLS_DUMP(delta);
//            TOOLS_DUMP(i + delta);
            
            Int j = (i + delta < n) ? (i + delta) : (i + delta - n);
            const Int de_j = F_dE_idx[face_begin + j];
            auto [e_j,d_j] = FromDarc(de_j);
            
//            TOOLS_DUMP(delta);
//            TOOLS_DUMP(j);
//            TOOLS_DUMP(de_j);
//            TOOLS_DUMP(e_j);
//            TOOLS_DUMP(ModDistance(n,i,j));
            
            const Int dist = EdgeEdgeManhattanDistance(coords,e_i,e_j);
            
            if( dist <= Int(0) )
            {
                db = de_j;
                break;
            }
        }
        
        std::array<Int,3> result = {de_i,da,db};
        
//        const Int a = da / 2;
//        const Int b = db / 2;
//        
//        TOOLS_DUMP(de_i);
//        TOOLS_DUMP(e_i);
//        TOOLS_DUMP(da);
//        TOOLS_DUMP(a);
//        TOOLS_DUMP(db);
//        TOOLS_DUMP(b);
//        
//        TOOLS_DUMP(FaceSize(f));
//        
//        TOOLS_DUMP(E_V(a,Tail));
//        TOOLS_DUMP(E_V(a,Head));
//        
//        TOOLS_DUMP(E_V(b,Tail));
//        TOOLS_DUMP(E_V(b,Head));
//        
//        valprint("X_0", ArrayToString(coords.data(E_V(a,Tail)),{2}));
//        valprint("X_1", ArrayToString(coords.data(E_V(a,Head)),{2}));
//        valprint("Y_0", ArrayToString(coords.data(E_V(b,Tail)),{2}));
//        valprint("Y_1", ArrayToString(coords.data(E_V(b,Head)),{2}));
//        
//        TOOLS_DUMP(result);
        
        agg.push_back(std::move(result));
        
    } // for( Int i = face_begin; i < face_end; ++i )
}

//template<bool verboseQ = false>
//std::tuple<Int,Int> FindCollisions(
//    cref<CoordsContainer_T> coords,
//    const Int de_ptr
//) const
//{
//    if constexpr ( verboseQ )
//    {
//        print("FindCollisions("+ToString(de_ptr)+")");
//    }
//    
//    if( !ValidIndexQ(de_ptr) )
//    {
//        return {Uninitialized,Uninitialized};
//    }
//    
//    cptr<Int>   dTRE_left_dTRE = TRE_left_dTRE.data();
//    cptr<Int>   dTRE_turn      = TRE_turn.data();
//    
//    // RE = reflex edge
//    mptr<Int>   RE_rot = V_scratch.data();
//    mptr<Int>   RE_de  = E_scratch.data();
//    mptr<Int>   RE_d   = E_scratch.data() + edge_count;
//    
//    std::unordered_map<Turn_T,std::vector<Int>> rot_lut;
//
//    const bool exteriorQ = TRE_BoundaryQ(de_ptr);
//    
//    Int dE_counter = 0;
//    Int RE_counter = 0;
//    
//    // Compute rotations.
//    {
//        Int rot = 0;
//        Int de = de_ptr;
//        do
//        {
//            if( dTRE_turn[de] == Turn_T(-1) )
//            {
//                RE_de [RE_counter] = de;
//                RE_d  [RE_counter] = dE_counter;
//                RE_rot[RE_counter] = rot;
//                rot_lut[rot].push_back(RE_counter);
//                
//                RE_counter++;
//                
//                // DEBUGGING
//                if( exteriorQ != TRE_BoundaryQ(de) )
//                {
//                    eprint(ClassName()+"::FindCollisions: TREdge " + ToString(de) + " has bounday flag set to " + ToString(TRE_BoundaryQ(de)) + ", but face's boundary flag is " + ToString(exteriorQ) + "." );
//                }
//            }
//            ++dE_counter;
//            rot += dTRE_turn[de];
//            de = dTRE_left_dTRE[de];
//        }
//        while( (de != de_ptr) && dE_counter < edge_count );
//        // TODO: The checks on reflex_counter should be irrelevant if the graph structure is intact.
//
//        if( (de != de_ptr) && (dE_counter >= edge_count) )
//        {
//            eprint(ClassName()+"::FindCollisions: Aborted because two many elements were collected. The data structure is probably corrupted.");
//
//            return {Uninitialized,Uninitialized};
//        }
//    }
//
//
////    valprint("rotation", ArrayToString(rotation,{reflex_counter}));
//
////    const Int target_turn = exteriorQ ? Turn_T(2) : Turn_T(-6);
//    
//    if constexpr ( verboseQ )
//    {
//        TOOLS_DUMP(dE_counter);
//        TOOLS_DUMP(RE_counter);
//    }
//    
//    Int p_min = 0;
//    Int q_min = 0;
//    Int d_min = Scalar::Max<Int>;
//    
//    constexpr Turn_T targets [2] = { Turn_T(2), Turn_T(-6) };
//    
//    for( Int q = 0; q < RE_counter; ++q )
//    {
//        const Int rot = RE_rot[q];
//        
//        const Int de_q = RE_de[q];
//        auto [e_q,d_q] = FromDirEdge(de_q);
//        const Dir_T s_q = TRE_dir[e_q];
//        const bool verticalQ  = s_q % Dir_T(2);
//        const bool reverse_qQ = ((s_q >> Dir_T(2)) == Dir_T(1));
//        
//        Tiny::Vector<2,Int,Int> Y_0 = coords.data(TRE_V(e_q, reverse_qQ));
//        Tiny::Vector<2,Int,Int> Y_1 = coords.data(TRE_V(e_q,!reverse_qQ));
//        
//        // Y_1 is either to the east or to the north of Y_0.
//        
//        for( Int k = 0; k < Int(1) + exteriorQ; ++k )
//        {
//            const Int target = rot - targets[k];
//            
//            if( rot_lut.count(target) <= Size_T(0) ) { continue; }
//            
//            auto & list = rot_lut[target];
//            
//            for( Int p : list )
//            {
//                const Int de_p = RE_de[p];
//                auto [e_p,d_p]  = FromDirEdge(de_p);
//                const Dir_T s_p = TRE_dir[e_q];
//                const bool reverse_pQ = ((s_p >> Dir_T(2)) == Dir_T(1));
//                Tiny::Vector<2,Int,Int> X_0 = coords.data(TRE_V(e_p, reverse_pQ));
//                Tiny::Vector<2,Int,Int> X_1 = coords.data(TRE_V(e_p,!reverse_pQ));
//                
//                // X_1 is either to the east or to the north of Y_0.
//                
//                bool collisionQ;
//                
//                if( verticalQ )
//                {
//                    if(
//                        (X_0[0] == Y_0[0])
//                        &&
//                        !( (Y_1[1] < X_0[1]) || (X_1[1] < Y_0[1]))
//                    )
//                    {
//                        print("collision {" + ToString(de_p) + "," + ToString(de_q) + "}");
//                        return {de_p,de_q}
//                    }
//                else
//                {
//                    if(
//                        (X_0[1] == Y_0[1])
//                        &&
//                        collisionQ = !( (Y_1[0] < X_0[0]) || (X_1[0] < Y_0[0])) )
//                    )
//                    {
//                        print("collision {" + ToString(de_p) + "," + ToString(de_q) + "}");
//                        return {de_p,de_q}
//                    }
//                }
//            }
//        }
//    }
//}
