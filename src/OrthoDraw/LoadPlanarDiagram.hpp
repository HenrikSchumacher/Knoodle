private:

// DEBUGGING
template<bool debugQ = false, bool verboseQ = false, typename ExtInt>
void LoadPlanarDiagram(
    cref<PlanarDiagram<ExtInt>> pd,
    const ExtInt exterior_region_
)
{
    std::string tag = MethodName("LoadPlanarDiagram") + "<" + ToString(debugQ) + ","+ToString(verboseQ)+ "," + TypeName<ExtInt> + ">";
    
    TOOLS_PTIMER(timer,tag);

//    TOOLS_DUMP(pd.CountActiveCrossings());
//    TOOLS_DUMP(pd.CrossingCount());
//    TOOLS_DUMP(pd.CountActiveArcs());
//    TOOLS_DUMP(pd.ArcCount());
    
    C_A  = pd.Crossings(); // copy data
    A_C  = pd.Arcs();      // copy data
    
    R_dA = pd.FaceDarcs();
    
    crossing_count = int_cast<Int>(pd.CrossingCount());
    arc_count      = int_cast<Int>(pd.ArcCount());

    const Int C_count = C_A.Dim(0);
    const Int A_count = A_C.Dim(0);
    
    if constexpr ( verboseQ )
    {
        TOOLS_LOGDUMP(C_count);
        TOOLS_LOGDUMP(A_count);
        
        TOOLS_LOGDUMP(crossing_count);
        TOOLS_LOGDUMP(arc_count);
    }
    
    // TODO: Allow more general bend sequences.
    
    exterior_region = int_cast<Int>(exterior_region_);
    
    const Int maximum_region = int_cast<Int>(pd.MaximumFace());
    
    exterior_region = ((exterior_region < Int(0)) || (exterior_region >= R_dA.SublistCount()))
                    ? maximum_region
                    : exterior_region;

    // TODO: I have to filter out inactive crossings and inactive arcs!
    switch( settings.bend_min_method )
    {
        case 1:
        {
            A_bends = ComputeBends_Clp(pd,exterior_region);
            break;
        }
        case 0:
        {
            A_bends = ComputeBends_MCF(pd,exterior_region);
            break;
        }
        default:
        {
            A_bends = ComputeBends_MCF(pd,exterior_region);
            break;
        }
    }

    if( A_bends.Size() <= Int(0) )
    {
        eprint(tag + ": Bend optimization failed. Aborting.");
        return;
    }

    if( settings.redistribute_bendsQ )
    {
        RedistributeBends(pd,A_bends);
    }

    
    // Compute maximum face size as that will be useful for later allocations.
    // This also gives us the opportunity to compute the total number of bends.
    max_face_size = 0;
    bend_count    = 0;

    const Int r_count = R_dA.SublistCount();
    
    for( ExtInt r = 0; r < r_count; ++r )
    {
        Int r_size = R_dA.SublistSize(r);
        
        for( auto da : R_dA.Sublist(r) )
        {
            auto [a,d]  = FromDarc(da);
            const Int b = int_cast<Int>(Abs(A_bends[a]));
            bend_count += b;
            r_size     += b;
        }
        
        max_face_size = Max( max_face_size, r_size );
    }
    
    // We walk through each undirected arc twice, therefore, we divide by 2.
    bend_count /= Int(2);
    
    // Prepare vertices and edges.
    
    // This counts all vertices and edges, not only the active ones.
    const Int V_count = C_count + bend_count;
    const Int E_count = A_count + bend_count + (settings.turn_regularizeQ ? bend_count/Int(2) : Int(0) );
    
    vertex_count    = crossing_count + bend_count;
    edge_count      = arc_count      + bend_count;
    
    if constexpr ( verboseQ )
    {
        TOOLS_LOGDUMP(V_count);
        TOOLS_LOGDUMP(E_count);
        TOOLS_LOGDUMP(vertex_count);
        TOOLS_LOGDUMP(edge_count);
        TOOLS_LOGDUMP(bend_count);
    }
    
    // General purpose buffers. May be used in all routines as temporary space.
    V_scratch    = Tensor1<Int,Int> ( Int(2) * V_count );
    E_scratch    = Tensor1<Int,Int> ( Int(2) * E_count );
    
    mptr<Dir_T> C_dir = reinterpret_cast<Dir_T *>(V_scratch.data());
    fill_buffer( C_dir, NoDir, C_A.Dim(0));
    
    using DarcNode = PlanarDiagram<ExtInt>::DarcNode;
    
    constexpr ExtInt UninitializedArc = PlanarDiagram<ExtInt>::Uninitialized;
    
    // Tell each crossing what its absolute orientation is.
    // This would be hard to parallelize
    pd.DepthFirstSearch(
        [&C_dir,/*&C_A,*/this]( cref<DarcNode> A )
        {
            if( A.da == UninitializedArc )
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
                Dir_T dir = dir_lut[io_0][lr_0];
                
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
                
                C_dir[c_1] = (dir - dir_lut[io_1][lr_1]) % Dir_T(4);
            }
        }
    );
    
//    logvalprint("C_dir",ArrayToString(C_dir,{C_A.Dim(0)}));
//    TOOLS_LOGDUMP(pd.CrossingStates())

    V_dE   = VertexContainer_T      ( V_count, Uninitialized );
    E_V    = EdgeContainer_T        ( E_count, Uninitialized );
    
    V_flag = VertexFlagContainer_T  ( V_count, VertexFlag_T::Inactive );
    E_flag = EdgeFlagContainer_T    ( E_count, EdgeFlag_T(0) );

    // Needed for turn regularity and for building E_left_dE.
    E_turn = EdgeTurnContainer_T    ( E_count, Turn_T(0) );
    // E_turn(a,Head) = turn between a and E_left_dE(a,Head);
    // E_turn(a,Tail) = turn between a and E_left_dE(a,Tail);

    E_dir  = Tensor1<Dir_T,Int>     ( E_count, NoDir );
    E_A    = Tensor1<Int,Int>       ( E_count, Uninitialized );
    
    A_V    = RaggedList<Int,Int>(A_count + Int(1), E_count + arc_count);
    A_E    = RaggedList<Int,Int>(A_count + Int(1), E_count );

    cptr<CrossingState> C_state = pd.CrossingStates().data();
    
    A_overQ = Tiny::VectorList_AoS<2,bool,Int>(A_count);

    
    if constexpr ( verboseQ )
    {
        logprint("Assigning vertex flags to crossings.");
    }
    
    // Cycle only over the crossings.
    for( Int c = 0; c < C_count; ++c )
    {
        switch(C_state[c])
        {
            case CrossingState::Inactive:
            {
                V_flag[c] = VertexFlag_T::Inactive;
                break;
            }
            case CrossingState::LeftHanded:
            {
                V_flag[c] = VertexFlag_T::LeftHanded;
                break;
            }
            case CrossingState::RightHanded:
            {
                V_flag[c] = VertexFlag_T::RightHanded;
                break;
            }
            default:
            {
                eprint(tag + ": Invalid crossing state " + ToString(C_state[c]) +".");
                break;
            }
        }
    }
    
    if constexpr ( verboseQ ) { logprint("Loading edge flags for arcs."); }
    
    for( Int a = 0; a < A_count; ++a )
    {
        ExtInt a_ = static_cast<ExtInt>(a);
        
        if( !pd.ArcActiveQ(a_) ) { continue; }
        
        E_flag(a,Tail) = EdgeActiveMask;
        E_flag(a,Head) = EdgeActiveMask;
        
        A_overQ(a,Tail) = pd.template ArcOverQ<Tail>(a_); // pd needed
        A_overQ(a,Head) = pd.template ArcOverQ<Head>(a_); // pd needed
    }
    
    // From here on we do not need pd itself anymore. But we need the temporarily created C_dir buffer whose initialization requires a pd object (because of DepthFirstSearch).
    
    // Vertices, 0,...,C_count-1 correspond to crossings 0,...,C_count-1. The rest is newly inserted vertices.
    
    V_end = C_count;
    E_end = A_count;
    
    if constexpr ( verboseQ ) { logprint("Subdividing arcs."); }
    
    for( Int a = 0; a < A_count; ++a )
    {
        if( !EdgeActiveQ(a) )
        {
            A_V.FinishSublist();
            A_E.FinishSublist();
            continue;
        }
        
        const Turn_T b       = A_bends(a);
        const Int    abs_b   = static_cast<Int>(Abs(b));
        const Turn_T sign_b  = Sign<Turn_T>(b);     // bend per turn.
        const Int    c_0     = A_C(a,Tail);
        const Int    c_1     = A_C(a,Head);
        
//        TOOLS_LOGDUMP(a);
//        TOOLS_LOGDUMP(c_0);
//        TOOLS_LOGDUMP(c_1);
        
        // We have to subdivde the arc a into abs_b + 1 edges.

//         |                arc a                  |
//      c_0|        v_1       v_2       v_3        |
//         X-------->+-------->+-------->+-------->X
//         |    a        e_1       e_2       e_3   |c_1
//         |                                       |
        
        Dir_T e_dir = (dir_lut[Out][C_A(c_0,Out,Right) == a] + C_dir[c_0]) % Dir_T(4);
        
        A_V.Push(c_0);
        A_E.Push(a);
        
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
        
        // Inserting additional corners and edges
        for( Int k = 1; k <= abs_b; ++k )
        {
            v = V_end++;
            e = E_end++;
            V_flag[v] = VertexFlag_T::Corner;
            E_flag(e,Tail) = EdgeActiveMask;
            E_flag(e,Head) = EdgeActiveMask;
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
            A_V.Push(v);
            A_E.Push(e);
            
            E_V   (f,Head) = v;
            E_turn(f,Head) = sign_b;
            
            E_V   (e,Tail) = v;
            E_turn(e,Tail) = -sign_b;

            f = e;
        }
        
        A_V.Push(c_1);
        
//                               | A_left_A(a,Head);
//                     v      (*)|
//                   --+-------->X
//                          e    |c_1
//                               |
        
        V_dE(c_1,(e_dir + UInt(2)) % 4) = ToDedge<Tail>(e);
        
        E_V   (e,Head) = c_1;
        E_turn(e,Head) = Turn_T(1);
        
        A_V.FinishSublist();
        A_E.FinishSublist();
    };
    
    
    if constexpr ( verboseQ )
    {
        TOOLS_LOGDUMP(V_end);
        TOOLS_LOGDUMP(V_count);
        
        TOOLS_LOGDUMP(A_count);
        TOOLS_LOGDUMP(E_end);
        TOOLS_LOGDUMP(E_count);
        
        TOOLS_LOGDUMP(A_V.SublistCount());
        TOOLS_LOGDUMP(A_E.SublistCount());
    }
    
    if( V_end != V_count )
    {
        eprint(tag + ": V_end != V_count.");
        TOOLS_LOGDUMP(V_count);
        TOOLS_LOGDUMP(V_end);
    }
    
    if( E_end > E_count )
    {
        eprint(tag + ": E_end > E_count.");
        TOOLS_LOGDUMP(E_count);
        TOOLS_LOGDUMP(E_end);
    }
    
    if( A_V.SublistCount() != A_count )
    {
        eprint(tag + ": A_V.SublistCount() != A_count.");
        TOOLS_LOGDUMP(E_count);
        TOOLS_LOGDUMP(A_V.SublistCount());
    }
    
    if( A_E.SublistCount() != A_count )
    {
        eprint(tag + ": A_E.SublistCount() != A_count.");
        TOOLS_LOGDUMP(E_count);
        TOOLS_LOGDUMP(A_E.SublistCount());
    }
    
    if constexpr ( debugQ )
    {
        logprint("Checking all edges.");
        
        for( Int e = 0; e < E_end; ++e )
        {
            if( EdgeActiveQ(e) )
            {
                this->template CheckEdge<1,true>(e);
            }
            else
            {
                this->template CheckEdge<0,true>(e);
            }
        }
        
        logprint("Checking all vertices.");
        
        for( Int v = 0; v < V_end; ++v )
        {
            if( VertexActiveQ(v) )
            {
                this->template CheckVertex<1,true>(v);
            }
            else
            {
                this->template CheckVertex<0,true>(v);
            }
        }

        if( !this->template CheckEdgeDirections<verboseQ>() )
        {
            eprint(tag + ": CheckEdgeDirections() failed.");
        }
    }
    
    // We can compute this only after E_V and V_dE have been computed completed.
    ComputeEdgeLeftDedges();

    {
        const Int da = R_dA.Elements()[
            R_dA.Pointers()[exterior_region]
        ];
        
        // DEBUGGING
        if( da == Uninitialized )
        {
            eprint(tag + ": first darc if exterior region " + ToString(exterior_region) + " is not initialized.");
        }
        
        MarkFaceAsExterior( da );
    }
}
