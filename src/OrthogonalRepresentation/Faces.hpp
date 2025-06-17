public:

void ComputeFaces()
{
    TOOLS_PTIC(ClassName()+"::ComputeFaces");
    
    cptr<Int> dE_left_dE = E_left_dE.data();
    
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
            dE_F[ToDedge<Tail>(e)] = Uninitialized;
            dE_F[ToDedge<Head>(e)] = Uninitialized;
        }
        else
        {
            dE_F[ToDedge<Tail>(e)] = Uninitialized - Int(1);
            dE_F[ToDedge<Head>(e)] = Uninitialized - Int(1);
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
        auto [a,d] = FromDedge(da);
        
        if( (!EdgeActiveQ(a)) || (dE_F[da] != Uninitialized) ) { continue; }

        // This is to traverse the _crossings_ of each face in the same order as in pd.
        // Moreover, the first vertex in each face is guaranteed to be a crossing.
        const Int de_0 = d
                       ? ToDedge(A_E_idx[A_E_ptr[a       ]       ],d)
                       : ToDedge(A_E_idx[A_E_ptr[a+Int(1)]-Int(1)],d);
        
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
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeFaces");
}

template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
void TraverseAllFaces(
    PreVisit_T  && pre_visit,
    EdgeFun_T   && edge_fun,
    PostVisit_T && post_visit
)
{
    TOOLS_PTIC(ClassName()+"::TraverseAllFaces");
    
    for( Int f = 0; f < face_count; ++f )
    {
        const Int k_begin = F_dE_ptr[f    ];
        const Int k_end   = F_dE_ptr[f + 1];
        
        pre_visit(f);
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int de = F_dE_idx[k];
            
            edge_fun(f,k,de);
        }
        
        post_visit(f);
    }
    
    TOOLS_PTOC(ClassName()+"::TraverseAllFaces");
}

// Only for debugging, I guess.
Tensor1<Turn_T,Int> FaceDedgeRotations()
{
    TOOLS_PTIC(ClassName()+"::FaceDedgeRotations");
    
    Turn_T rot = 0;
    Tensor1<Turn_T,Int> rotations ( F_dE_ptr[face_count] );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f ){ (void)f; rot = Turn_T(0); },
        [&rotations,dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            rotations[k] = rot;
            rot += dE_turn[de];
        },
        []( const Int f ){ (void)f; }
    );
    
    TOOLS_PTOC(ClassName()+"::FaceDedgeRotations");
    
    return rotations;
}

Tensor1<Turn_T,Int> FaceRotations()
{
    TOOLS_PTIC(ClassName()+"::FaceRotations");
    
    Turn_T rot = 0;
    Tensor1<Turn_T,Int> rotations ( face_count );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f ){ rot = Turn_T(0); },
        [dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            rot += dE_turn[de];
        },
        [&rotations,&rot]( const Int f ){ rotations[f] = rot; }
    );
    
    TOOLS_PTOC(ClassName()+"::FaceRotations");
    
    return rotations;
}

bool CheckFaceTurns()
{
    bool okayQ = true;
    Turn_T rot = 0;
    Tensor1<Turn_T,Int> rotations ( face_count );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f ){
            rot = Turn_T(0);
        },
        [dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            rot += dE_turn[de];
        },
        [&rot,&okayQ,this]( const Int f )
        {
            if( f == exterior_face )
            {
                okayQ = okayQ && ( rot == Turn_T(-4) );
            }
            else
            {
                okayQ = okayQ && ( rot == Turn_T( 4) );
            }
        }
    );
    
    return okayQ;
}
