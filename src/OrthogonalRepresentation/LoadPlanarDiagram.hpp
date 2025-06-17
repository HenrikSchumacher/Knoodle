private:

template<typename ExtInt>
void LoadPlanarDiagram(
    mref<PlanarDiagram<ExtInt>> pd,
    const ExtInt exterior_face_,
    bool use_dual_simplexQ = false
)
{
    using DirectedArcNode = PlanarDiagram<ExtInt>::DirectedArcNode;
    
    TOOLS_PTIC(ClassName()+"::LoadPlanarDiagram");
    
    // CAUTION: This might assume that there are no gaps in pd!

//    using A_C_T = PlanarDiagram_T::CrossingContainer_T;
//    using C_A_T = PlanarDiagram_T::ArcContainer_T;
//    
//    A_C_T A_C_buffer;
//    C_A_T C_A_buffer;
//    
//    if constexpr ( !SameQ<ExtInt,Int> )
//    {
//        C_A_buffer = pd.Crossings();
//    }
    
    
    using C_A_T = std::conditional_t<
        SameQ<ExtInt,Int>,
        const typename PlanarDiagram_T::CrossingContainer_T &,
        const typename PlanarDiagram_T::CrossingContainer_T
    >;
    
    using A_C_T = std::conditional_t<
        SameQ<ExtInt,Int>,
        const typename PlanarDiagram_T::ArcContainer_T &,
        const typename PlanarDiagram_T::ArcContainer_T
    >;
    // If ExtInt is not the same as Int, then we make copies here for the conversion.
    // Otherwise, we simply take a reference.
    
    C_A_T C_A = pd.Crossings();
    A_C_T A_C = pd.Arcs();
    
    max_crossing_count  = int_cast<Int>(pd.Crossings().Dimension(0));
    max_arc_count       = int_cast<Int>(pd.Arcs().Dimension(0));
    
    crossing_count      = int_cast<Int>(pd.CrossingCount());
    arc_count           = int_cast<Int>(pd.ArcCount());
    face_count          = int_cast<Int>(pd.FaceCount());
    
    maximum_face        = int_cast<Int>(pd.MaximumFace());
    max_face_size       = int_cast<Int>(pd.MaxFaceSize());
    
    F_scratch = Tensor1<Int,Int>( max_face_size );
    
    // TODO: Allow more general bend sequences.
    exterior_face = (exterior_face_ < ExtInt(0))
                    ? maximum_face
                    : int_cast<Int>(exterior_face_);

    A_bends = Bends(pd,exterior_face);
    
    if( A_bends.Size() != max_arc_count)
    {
        eprint(ClassName()+"::SubdividePlanarDiagram: Bend optimization failed. Aborting.");
        return;
    }
    
    bend_count = 0;
    
    // TODO: Hsould be part of the info received from BendsByLP.
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
    
    // Tell each crossing what its absolute orientation is.
    // This would be hard to parallelize
    pd.DepthFirstSearch(
        [&C_dir,&C_A,this]( cref<DirectedArcNode> A )
        {
            if( A.da < ExtInt(0) )
            {
                C_dir[A.head] = Dir_T(0);
            }
            else
            {
                auto [a,d] = FromDarc(A.da);

                const ExtInt c_0  = A.tail;
                const ExtInt c_1  = A.head;
                const bool   io_0 = !d;
                const bool   lr_0 = (C_A(c_0,io_0,Right) == a);
                
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
    
    A_overQ = Tiny::VectorList_AoS<2,bool,Int>(arc_count);
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        V_state[c] = VertexState(ToUnderlying(C_state[c]));
    }
    
    // Subdivide each arc.
    for( Int a = 0; a < max_arc_count; ++a )
    {
        E_state[a] = EdgeState(ToUnderlying(A_state[a]));
        
        ExtInt a_ = static_cast<ExtInt>(a);
        
        if( (a < arc_count) && (pd.ArcActiveQ(a_)) )
        {
            A_overQ(a,Tail) = pd.template ArcOverQ<Tail>(a_);
            A_overQ(a,Head) = pd.template ArcOverQ<Head>(a_);
        }
        
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
        
        V_dE(c_0,e_dir) = ToDedge<Head>(a);
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
            
            V_dE(v,(e_dir + Dir_T(2)) % Dir_T(4)) = ToDedge<Tail>(f);
            
            e_dir = static_cast<Dir_T>(e_dir + sign_b) % Dir_T(4);
            E_dir(e) = e_dir;
            
            
            V_dE(v,e_dir) = ToDedge<Head>(e);
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
        
        V_dE(c_1,(e_dir + UInt(2)) % 4) = ToDedge<Tail>(e);
        
        E_V   (e,Head) = c_1;
        E_turn(e,Head) = Turn_T(1);
    };
    
    TOOLS_PTOC(ClassName()+"::LoadPlanarDiagram");
}
