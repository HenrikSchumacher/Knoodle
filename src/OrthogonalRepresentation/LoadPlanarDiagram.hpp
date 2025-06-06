private:

void LoadPlanarDiagram( mref<PlanarDiagram_T> pd, const Int exterior_face_ )
{
    TOOLS_PTIC(ClassName() + "::LoadPlanarDiagram");
    
    // CAUTION: This assumes no gaps in pd!
    A_C                 = pd.Arcs();
    max_crossing_count  = pd.Crossings().Dimension(0);
    max_arc_count       = pd.Arcs().Dimension(0);
    
    crossing_count      = pd.CrossingCount();
    arc_count           = pd.ArcCount();
    face_count          = pd.FaceCount();
    
    // TODO: Allow more general bend sequences.
    exterior_face = (exterior_face_ < Int(0)) ? pd. MaximumFace() : exterior_face_;

    A_bends = BendsByLP(pd,exterior_face);
    
    if( A_bends.Size() != max_arc_count)
    {
        eprint(ClassName() + "::LoadPlanarDiagram: Bend optimization failed. Aborting.");
        return;
    }
    
    bend_count = 0;
    
    // TODO: Should be part of the info received from BendsByLP.
    for( Int a = 0; a < arc_count; ++a )
    {
        bend_count += Abs(A_bends[a]);
    }

    // This counts all vertices and edges, not only the active ones.
    vertex_count = max_crossing_count + bend_count;
    edge_count   = max_arc_count      + bend_count;
    
    // General purpose buffers. May be used in all routines as temporary space.
    V_scratch    = Tensor1<Int,Int> ( Int(2) * vertex_count );
    E_scratch    = Tensor1<Int,Int> ( Int(2) * edge_count   );
    
    mptr<Dir_T> C_dir = reinterpret_cast<Dir_T *>(V_scratch.data());
    fill_buffer( C_dir, Dir_T(-1), max_crossing_count);
    
    // C_dir[c] == -1 means invalid.
    // C_dir[c] == 0 means C_A(c,Out,Right) points east.
    // C_dir[c] == 1 means C_A(c,Out,Right) points north.
    // C_dir[c] == 1 means C_A(c,Out,Right) points west.
    // C_dir[c] == 2 means C_A(c,Out,Right) points south.

    // The translation between PlanarDiagram's ports and the the cardinal directions under the assumption that C_dir[c] == North;
    constexpr Dir_T lut [2][2] = { {North,East}, {West,South} };
    
    const auto & C_A = pd.Crossings();
    
    // Tell each crossing what its absolute orientation is.
    // This would be hard to parallelize
    pd.DepthFirstSearch(
        [&C_dir,&C_A,this]( cref<DirectedArcNode> A )
        {
            if( A.da < Int(0) )
            {
                C_dir[A.head] = Dir_T(0);
            }
            else
            {
                auto [a,d] = FromDiArc(A.da);

                const Int  c_0  = A.tail;
                const Int  c_1  = A.head;
                const bool io_0 = !d;
                const bool lr_0 = (C_A(c_0,io_0,Right) == a);
                
                // Direction where a would leave the standard-oriented port.
                Dir_T dir = lut[io_0][lr_0];
                
                // Take orientation of c_0 into account.
                dir += C_dir[c_0];
                
                // Take bends into account.
                dir += (d ? A_bends[a] : -A_bends[a]);
                // Arc enters through opposite direction.
                dir += Dir_T(2);
                
                // a_dir % 4 is the port to dock to.
                const bool io_1 = d;
                const bool lr_1 = (C_A(c_1,io_1,Right) == a);

                // Now we have to rotate c_1 by rot so that C_A(c_1,io_1,lr_1) equals a_dir:
                // lut[io_1][lr_1] + C_dir[c_1] == dir mod 4
                
                C_dir[c_1] = (dir - lut[io_1][lr_1]) % Dir_T(4);
            }
        }
    );
    
    V_dE         = VertexContainer_T( vertex_count, Uninitialized );
    E_V          = EdgeContainer_T  ( edge_count,   Uninitialized );
    
    V_state      = Tensor1<VertexState,Int>( vertex_count, VertexState::Corner );
    E_state      = Tensor1<EdgeState,Int>  ( edge_count  , EdgeState::Virtual );

    // Needed for turn regularity and for building E_left_dE.
    E_turn       = EdgeTurnContainer_T( edge_count );
    // E_turn(a,Head) = turn between a and E_left_dE(a,Head);
    // E_turn(a,Tail) = turn between a and E_left_dE(a,Tail);

    E_dir        = Tensor1<Dir_T,Int>( edge_count );
    E_A          = Tensor1<Int,Int>( edge_count );
    
    A_V_ptr      = Tensor1<Int,Int>( max_arc_count + Int(1) );
    A_V_ptr[0]   = 0;
    A_V_idx      = Tensor1<Int,Int>( edge_count + arc_count, Uninitialized );
    
    A_E_ptr      = Tensor1<Int,Int>( max_arc_count + Int(1) );
    A_E_ptr[0]   = 0;
    A_E_idx      = Tensor1<Int,Int>( edge_count            , Uninitialized );
        
    
    // Vertices, 0,...,max_crossing_count-1 correspond to crossings 0,...,max_crossing_count-1. The rest is newly inserted vertices.
    
    Int V_counter = max_crossing_count;
    Int E_counter = max_arc_count;

    
    cptr<CrossingState> C_state = pd.CrossingStates().data();
    cptr<ArcState>      A_state = pd.ArcStates().data();
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        V_state[c] = VertexState(ToUnderlying(C_state[c]));
    }
    
    // Subdivide each arc.
    for( Int a = 0; a < max_arc_count; ++a )
    {
        E_state[a] = EdgeState(ToUnderlying(A_state[a]));
        
        if( !EdgeActiveQ(a) ) { continue; }
        
        const Turn_T b       = A_bends(a);
        const Int    abs_b   = static_cast<Int>(Abs(b));
        const Turn_T sign_b  = Sign<Turn_T>(b);     // bend per turn.
        const Int    c_0     = A_C(a,Tail);
        const Int    c_1     = A_C(a,Head);
        
        const Int A_V_pos  = A_V_ptr[a];
        const Int A_E_pos  = A_E_ptr[a];
        
        A_V_ptr[a+1] = A_V_pos + abs_b + Int(2);
        A_E_ptr[a+1] = A_E_pos + abs_b + Int(1);
        
        // We have to subdivde the arc a into abs_b + 1 edges.

//         |                arc a                  |
//      c_0|        v_1       v_2       v_3        |
//         X-------->+-------->+-------->+-------->X
//         |    a        e_1       e_2       e_3   |c_1
//         |                                       |
        
        Dir_T e_dir = (lut[Out][C_A(c_0,Out,Right) == a] + C_dir[c_0]) % Dir_T(4);
        
        A_V_idx[A_V_pos] = c_0;
        A_E_idx[A_E_pos] = a;
        
        V_dE(c_0,e_dir) = ToDiEdge(a,Head);
        E_dir(a) = e_dir;
        E_A(a) = a;
        
        E_V   (a,Tail) = c_0;
        E_turn(a,Tail) = Turn_T(1);
        
//               |
//            c_0|   e     ?
//               X-------->+--
//               |(*)
//               | +----------------------+
//                                        v
        // We use f for storing the "previous edge".
        Int f = a;
        // We use e for storing the "current edge".
        Int e = a;
        // We use v for storing the tail of the "current edge" e.
        Int v = c_0;
        
//        TOOLS_DUMP(v);
//        TOOLS_DUMP(V_counter);
//        TOOLS_DUMP(A_V_pos - a);
//        TOOLS_DUMP(e);
//        TOOLS_DUMP(E_counter);
//        TOOLS_DUMP(A_E_pos);
        
        // Inserting additional corners and edges
        for( Int k = 1; k <= abs_b; ++k )
        {
            v = V_counter++;
            e = E_counter++;
//                                 + ??
//  sign_b > 0  mean turn left     ^
//                                 |
//                                 | e
//                    ???     f    |
//                     --+-------->+ v
            
            V_dE(v,(e_dir + Dir_T(2)) % Dir_T(4)) = ToDiEdge(f,Tail);
            
            e_dir = static_cast<Dir_T>(e_dir + sign_b) % Dir_T(4);
            E_dir(e) = e_dir;
            
            
            V_dE(v,e_dir) = ToDiEdge(e,Head);
            E_A(e) = a;
            A_V_idx[A_V_pos + k] = v;
            A_E_idx[A_E_pos + k] = e;
            
            E_V   (f,Head) = v;
            E_turn(f,Head) = sign_b;
            
            E_V   (e,Tail) = v;
            E_turn(e,Tail) = -sign_b;

            f = e;
        }
        
        A_V_idx(A_V_pos + abs_b + Int(1)) = c_1;
        
//                               | A_left_A(a,Head);
//                     v      (*)|
//                   --+-------->X
//                          e    |c_1
//                               |
        
        V_dE(c_1,(e_dir + UInt(2)) % 4) = ToDiEdge(e,Tail);
        
        E_V   (e,Head) = c_1;
        E_turn(e,Head) = Turn_T(1);
    };
    
    // We can compute this only after E_V and V_dE have been computed completed.
    ComputeEdgeLeftDiEdge();
    
    ComputeFaces(pd);
    
    TOOLS_PTOC(ClassName() + "::LoadPlanarDiagram");
}

private:

//Int DiEdgeLeftDiEdge( const Int de )
//{
//    auto [e,d] = FromDiEdge(de);
//    
//    const UInt e_dir = static_cast<UInt>(E_dir(e));
//    
//    const UInt t = E_turn.data()[de];
//    const UInt s = (e_dir + (d ? UInt(2) : UInt(0)) ) % UInt(4);
//    const Int  c = E_V.data()[de];
//    
//    return V_dE(c,s);
//}

void ComputeEdgeLeftDiEdge()
{
    TOOLS_PTIC( ClassName() + "::ComputeEdgeLeftDiEdge" );
    
    E_left_dE = EdgeContainer_T ( edge_count, Int(-1) );
    
    for( Int e = 0; e < edge_count; ++e )
    {
        TOOLS_DUMP( E_state[e] );
        if( !EdgeActiveQ(e) )
        {
            wprint(ClassName() + "::ComputeEdgeLeftDiEdge: Skipping edge " + ToString(e) + ".");
            continue;
        }

        const Dir_T e_dir = E_dir(e);
        
        const Turn_T t_0  = E_turn(e,Tail);
        const Turn_T t_1  = E_turn(e,Head);
        
        const Dir_T s_0   = static_cast<Dir_T>(t_0 + Turn_T(2) + e_dir) % Dir_T(4);
        const Dir_T s_1   = static_cast<Dir_T>(t_1 +           + e_dir) % Dir_T(4);
        
        const Int   c_0   = E_V(e,Tail);
        const Int   c_1   = E_V(e,Head);
        
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
    
    TOOLS_PTOC( ClassName() + "::ComputeEdgeLeftDiEdge" );
}

public:

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
        dE_F[ToDiEdge(e,Tail)] = Int(-1);
        dE_F[ToDiEdge(e,Head)] = Int(-1);
        
        // TODO: Revisit this if edge can be deactive.
//        if( EdgeActiveQ(e) )
//        {
//            dE_F[de + Tail] = Int(-1);
//            dE_F[de + Head] = Int(-1);
//        }
//        else
//        {
//            dE_F[de + Tail] = Int(-2);
//            dE_F[de + Head] = Int(-2);
//        }
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
        
        if( (!EdgeActiveQ(a)) || (dE_F[da] != Int(-1)) )
        {
            continue;
        }
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
