public:

// TODO: Make this useful.
//template<typename Fun_T>
//void TraverseFaces( Fun_T && fun )
//{
//    TOOLS_PTIC(ClassName() + "::TraverseFaces");
//    
//    cptr<Int>  dE_left_dE  = E_left_dE.data();
//    mptr<bool> dE_visitedQ = reinterpret_cast<bool *>(E_scratch.data());
//    fill_buffer( dE_visitedQ, false, edge_count );
//
//    for( Int de_ptr = 0; de_ptr < Int(2) * edge_count; ++de_ptr )
//    {
//        if( !dE_visitedQ(de_ptr) ) { continue; }
//        
//        Int de = de_ptr;
//        do
//        {
//            fun(de);
//            dE_visitedQ[de] = true;
//            de = dE_left_dE[de];
//        }
//        while( de != de_ptr );
//    }
//    
//    TOOLS_PTOC(ClassName() + "::TraverseFaces");
//}

void ComputeFaces( mref<PlanarDiagram_T> pd )
{
    TOOLS_PTIC(ClassName()+"::ComputeFaces");
    
    cptr<Int> dE_left_dE = E_left_dE.data();
    
//    TOOLS_DUMP(E_left_dE);

    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    E_F = EdgeContainer_T( edge_count );
    
    mptr<Int> dE_F = E_F.data();
    
    // Convention: _Right_ face first:
    //
    //            E_F(e,0)
    //
    //            <------------|  e
    //
    //            E_F(e,1)
    //
    // This way the directed edge de = 2 * e + Head has its left face in dE_F[de].
    
    
    // Entry -1 means "unvisited but to be visited".
    // Entry -2 means "do not visit".
    
    for( Int e = 0; e < edge_count; ++ e )
    {
        if( EdgeActiveQ(e) )
        {
            dE_F[ToDiEdge(e,Tail)] = Uninitialized;
            dE_F[ToDiEdge(e,Head)] = Uninitialized;
        }
        else
        {
            dE_F[ToDiEdge(e,Tail)] = Uninitialized - Int(1);
            dE_F[ToDiEdge(e,Head)] = Uninitialized - Int(1);
        }
    }
    
//    TOOLS_DUMP(A_E_ptr);
//    TOOLS_DUMP(A_E_idx);
    
    
    const Int dA_count = Int(2) * edge_count;

    F_dE_ptr = Tensor1<Int,Int>( face_count + Int(1) );
    F_dE_ptr[0] = 0;
    F_dE_idx = Tensor1<Int,Int>( dA_count );

    Int dE_counter = 0;
    Int F_counter = 0;
    
    // Same traversal as in PlanarDiagram_T::RequireFaces in the sense that the faces are traversed in the same order.
    // We only have to cycle over the directed arcs, because every edge has to belong to one.
    
    for( Int da = 0; da < dA_count; ++da )
    {
        auto [a,d] = pd.FromDiArc(da);
        
        if( (!EdgeActiveQ(a)) || (dE_F[da] != Uninitialized) ) { continue; }
        //        print("==============================");
//        TOOLS_DUMP(F_counter);
//        TOOLS_DUMP(dE_counter);
        
        // This is to traverse the _crossings_ of each face in the same order as in pd.
        // Moreover, the first vertex in each face is guaranteed to be a crossing.
        const Int de_0 = d
                       ? ToDiEdge(A_E_idx[A_E_ptr[a       ]       ],d)
                       : ToDiEdge(A_E_idx[A_E_ptr[a+Int(1)]-Int(1)],d);
        
//        TOOLS_DUMP(da);
//        TOOLS_DUMP(a);
//        TOOLS_DUMP(d);
//        TOOLS_DUMP(de_0);
        
        Int de = de_0;
        do
        {
//            print("de = " + ToString(de) + "; e = " + ToString(de/2));
            // Declare current face to be a face of this directed arc.
            dE_F[de] = F_counter;
            
            // Declare this arc to belong to the current face.
            F_dE_idx[dE_counter++] = de;
            
            // Move to next arc.
            de = dE_left_dE[de];
        }
        while( de != de_0 );
        
        F_dE_ptr[++F_counter] = dE_counter;
        
//        TOOLS_DUMP(E_F);
//        TOOLS_DUMP(F_dE_ptr);
//        TOOLS_DUMP(F_dE_idx);
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeFaces");
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

Tensor1<Int,Int> FaceRotations() const
{
    TOOLS_PTIC(ClassName() + "::FaceRotations");
    
    Tensor1<Int,Int> rotations ( face_count );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    for( Int f = 0; f < face_count; ++f )
    {
        const Int k_begin = F_dE_ptr[f  ];
        const Int k_end   = F_dE_ptr[f+1];
        
        Int rot = 0;
        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int de = F_dE_idx[k];
            rot += static_cast<Int>(dE_turn[de]);
        }
        rotations[f] = rot;
    }
    
    TOOLS_PTOC(ClassName() + "::FaceRotations");
    
    return rotations;
}

bool CheckFaceTurns() const
{
    cptr<Turn_T> dE_turn = E_turn.data();
    
    bool okayQ = true;
    
    for( Int f = 0; f < face_count; ++f )
    {
        Turn_T total_turns = 0;
        const Int k_begin = F_dE_ptr[f  ];
        const Int k_end   = F_dE_ptr[f+1];
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int de = F_dE_idx[k];
            
            total_turns += dE_turn[de];
        }
        
        if( f == exterior_face )
        {
            okayQ = okayQ && ( total_turns == Turn_T(-4) );
        }
        else
        {
            okayQ = okayQ && ( total_turns == Turn_T( 4) );
        }
    }
    
    return okayQ;
}
