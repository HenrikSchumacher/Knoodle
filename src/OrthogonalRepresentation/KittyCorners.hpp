public:

//void ResolveKittyCorners()
//{
//    Int max_edge_count = Int(2) * edge_count;
//    
//    VertexContainer_T   V_dE_mod = V_dE;
//    EdgeContainer_T     E_V_mod       ( max_edge_count );
//    EdgeContainer_T     E_left_dE_mod ( max_edge_count );
//    EdgeTurnContainer_T E_turn_mod    ( max_edge_count );
//    Tensor1<Dir_T,Int>  E_dir_mod     ( max_edge_count );
//    
//    mptr<Int> dE_V       = E_V_mod.data();
//    mptr<Int> dE_left_dE = E_left_dE_mod.data();
//    mptr<Int> dE_turn    = E_turn_mod.data();
//    
//    E_V.Write( dE_V );
//    E_left_dE.Write( dE_left_dE );
//    E_turn.Write( dE_turn );
//    E_dir.Write( E_dir_mod.data() );
//    
//    Int all_edge_count = edge_count;
//    Int virtual_edge_count = 0;
//    
//    Tiny::VectorList<2,UInt8,Int> E_flag ( max_edge_count, UInt8(0) );
//    mptr<UInt8> dE_flag = E_flag.data();
//    // if get_bit(dE_flag[de],1) == true, then de is visited.
//    // if get_bit(dE_flag[de],2) == true, then face left to de is exterior face.
//
//    // Make edges of exterior face as such.
//    {
//        const int k_begin = F_dA_ptr[exterior_face  ];
//        const int k_end   = F_dA_ptr[exterior_face+1];
//        for( Int k = k_begin; k < k_end; ++k )
//        {
//            const Int da = F_dA_idx[k];
//            // unvisited; boundary of exterior face.
//            dE_flag[da] = UInt(2);
//            
//            North
//        }
//    }
//    
//    
//    mptr<Int> rotation = V_scratch.data();
//    mptr<Int> reflex   = E_scratch.data();
//    
//    
//    for( Int de_0 = 0; de_0 < Int(2) * edge_count; ++de_0 )
//    {
//        if( get_bit(dE_flag[de_0],2) ) { continue }
//        
//        // TODO: When to mark de as visited?
//        // TODO: Find first pair of Kitty-corners.
//        // TODO: Insert Kitty-corners.
//        
//        
//        auto [da_0,db_0] = FindKittyCorner(
//            dE_left_dE, dE_turn, dE_flag, de_0, rotation, reflex
//        );
//
//        if( !ValidIndexQ(da_0) )
//        {
//            // TODO: Mark all edges of face as visited and continue.
//        }
//        
//        // TODO: Mark all edges strictly between da_0 and db_1 as visited.
//        
//        const Int db_1 = dE_left_dE[db_0];
//        const Int da_1 = dE_left_dE[da_0];
//        
//        const Int c_0 = dE_V[da_0];
//        const Int c_1 = dE_V[db_0];
//        
//        Int de_0 = ToDiArc(all_vertex_count,Tail);
//        Int de_1 = ToDiArc(all_vertex_count,Head);
//        ++all_vertex_count;
//        
////                X
////                ^
////                |
////           db_1 |
////                |   db_0
////            c_1 X<--------X
////                ^
////                .
////                .
////         da_0   .
////      X-------->X c_0
////                |
////                | da_1
////                |
////                v
////                X
//        
//        dE_V[da_0] = c_0;
//        dE_V[db_0] = c_1;
//        
//        E_
//        
//        V_dE(c_0,??) = de_0;
//        V_dE(c_1,??) = de_1;
//        
//        
//        
//        
//        dE_flag[de_0] = UInt(1); // Mark as visited.
//        
//        if( exteriorQ )
//        {
//            dE_flag[de_1] = UInt(2); // Mark as unvisited boundary edge.
//        }
//        else
//        {
//            dE_flag[de_1] = UInt(0); // Mark as unvisited interior edge.
//        }
//        
//    }
//}

std::pair<Int,Int> FindKittyCorner(
    cptr<Int> dE_left_dE,
    cptr<Int> dE_turn,
    mptr<Int> dE_flag,
    const Int de_0,
    mptr<Int> rotation,
    mptr<Int> reflex_edge
) const
{
    TOOLS_PTIMER(timer,ClassName() + "::FindKittyCorner");

    if( !ValidIndexQ(de_0) )
    {
        return std::pair(Uninitialized,Uninitialized);
    }

    Int reflex_counter = 0;
    Int rot = 0;
    bool exteriorQ = false;
    {
        Int de = de_0;
        do
        {
            if( dE_turn[de] == Turn_T(-1) )
            {
                reflex_edge[reflex_counter] = de;
                rotation[reflex_counter] = rot;
                reflex_counter++;
                exteriorQ = exteriorQ || get_bit(dE_flag[de],2);
            }
            rot += dE_turn[de];
            de = dE_left_dE[de];
        }
        while( (de != de_0) && reflex_counter < edge_count );
        // TODO: The checks on reflex_counter should be irrelevant if the graph structure is intact.

        if( (de != de_0) && (reflex_counter >= edge_count) )
        {
            eprint(ClassName() + "::TurnRegularFaceQ: Aborted because two many elements were collected. The data structure is probably corrupted.");

            return std::pair(Uninitialized,Uninitialized);
        }
    }

    valprint("rotation", ArrayToString(rotation,{reflex_counter}));

    Int target_turn = exteriorQ ? Turn_T(2) : Turn_T(-6);
    
    for( Int i = 0; i < reflex_counter; ++i )
    {
        const Int t = target_turn + rotation[i];
        
        for( Int j = i; j < reflex_counter; ++j )
        {
            if( rotation[j] == t )
            {
                return std::pair(reflex_edge[i],reflex_edge[j]);
            }
        }
    }

    return std::pair(Uninitialized,Uninitialized);
}













// Only for debugging, I guess.
Tensor1<Int,Int> FaceEdgeRotations() const
{
    TOOLS_PTIC(ClassName() + "::FaceEdgeRotations");
    
    Tensor1<Int,Int> rotations ( F_dE_ptr[face_count] );
    cptr<Turn_T> dE_turn = E_turn.data();
    for( Int f = 0; f < face_count; ++f )
    {
        const Int k_begin = F_dE_ptr[f  ];
        const Int k_end   = F_dE_ptr[f+1];
        
        Int rot = 0;
        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int de = F_dE_idx[k];
            rotations[k] = rot;
            rot += static_cast<Int>(dE_turn[de]);
        }
    }
    
    TOOLS_PTOC(ClassName() + "::FaceEdgeRotations");
    
    return rotations;
}

// Only for debugging, I guess.
Tiny::VectorList_AoS<3,Int,Int> FindAllKittyCorners_ByFaces() const
{
    TOOLS_PTIC(ClassName() + "::FindAllKittyCorners_ByFaces");
    
//    TOOLS_DUMP(exterior_face);
    
    Aggregator<Int,Int> agg ( vertex_count );
    
    mptr<Int>    rotation = E_scratch.data();
    mptr<Int>    reflex_corners = V_scratch.data();
    cptr<Int>    dE_V = E_V.data();
    cptr<Turn_T> dE_turn = E_turn.data();

    for( Int f = 0; f < face_count; ++f )
    {
        const Int k_begin = F_dE_ptr[f  ];
        const Int k_end   = F_dE_ptr[f+1];
        Int reflex_counter = 0;
        Int rot = 0;
        for( Int k = k_begin; k < k_end; ++k)
        {
            const Int de = F_dE_idx[k];

            if( dE_turn[de] == Turn_T(-1) )
            {
                reflex_corners[reflex_counter] = dE_V[de];
                rotation[reflex_counter] = rot;
                reflex_counter++;
            }
            rot += static_cast<Int>(dE_turn[de]);
        }
//        // DEBUGGING
//        if( reflex_counter > Int(0) )
//        {
//            valprint(
//                "face " + ToString(f) + " reflex_corners",
//                ArrayToString(reflex_corners,{reflex_counter})
//            );
//            valprint(
//                "face " + ToString(f) + " rotations",
//                ArrayToString(rotation,{reflex_counter})
//            );
//        }
        
        Int target_turn = (f != exterior_face) ? Int(2) : Int(-6);
        
        for( Int i = 0; i < reflex_counter; ++i )
        {
            const Int t = target_turn + rotation[i];
            
            for( Int j = i; j < reflex_counter; ++j )
            {
                if( rotation[j] == t )
                {
                    agg.Push(f);
                    agg.Push(reflex_corners[i]);
                    agg.Push(reflex_corners[j]);
                }
            }
        }
    }
    
    Tiny::VectorList_AoS<3,Int,Int> result(agg.Size()/3);
    agg.Write(result.data());
    
    TOOLS_PTOC(ClassName() + "::FindAllKittyCorners_ByFaces");
    
    return result;
}
