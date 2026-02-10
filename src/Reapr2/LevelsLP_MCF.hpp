public:


template<typename T = Real>
Tensor1<T,Int> LevelsLP_MCF( cref<PD_T> pd )
{
    using MCFSimplex_T = MCF::MCFSimplex;
    using R = MCFSimplex_T::FNumber;
    using I = MCFSimplex_T::Index;
    
    [[maybe_unused]] auto tag = [](){ return MethodName("LevelsLP_MCF"); };
    
    TOOLS_PTIMER(timer,tag());
    
    {
        Size_T max_idx = Size_T(5) * static_cast<Size_T>(pd.CrossingCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<I>::max() ) )
        {
            eprint(tag() + ": Too many arcs to fit into type " + TypeName<I> + ".");
            
            return Tensor1<T,Int>();
        }
    }
    
    // We use PackedArcIndices/RandomPackedArcIndices to take care of gaps in the data structure. Moreover, it facilitates random permutation of the arcs if this feature is active. In order to avoid allocation of short-time memory, we use C_scratch and A_scratch.
    
    if( settings.permute_randomQ )
    {
        pd.ScratchRandomPackedArcIndices( random_engine );
    }
    else
    {
        pd.ScratchPackedArcIndices();
    }
    
    cptr<Int> a_map  = pd.ArcScratchBuffer().data();
    cptr<Int> A_next = pd.ArcNextArc().data();

    const I n = I(4) * static_cast<I>(pd.CrossingCount());
    const I m = I(5) * static_cast<I>(pd.CrossingCount());
    
    Tensor1<R,I> upper   (m,Scalar::Infty<R>);
    Tensor1<I,I> tails   (m);
    Tensor1<I,I> heads   (m);
    Tensor1<R,I> costs   (m);
    Tensor1<R,I> defects (n);

    // First, we loop over all (active) arcs to generate the cycle edges.
    
    const I   arc_count     = static_cast<I>(pd.ArcCount());
    const Int max_arc_count = pd.MaxArcCount();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        const I v_0 = static_cast<I>(a_map[a]);
        const I v_1 = static_cast<I>(a_map[A_next[a]]);
        const I w   = arc_count + v_0;

        // MCF uses 1-based vertex indices!
        
        tails[v_0] = v_0 + I(1);
        heads[v_0] = w   + I(1);
        costs[v_0] = 0;
        
        tails[w  ] = v_1 + I(1);
        heads[w  ] = w   + I(1);
        costs[w  ] = 0;
        
        defects[v_0] = -2;
        defects[w  ] =  2;
    }
    
    // Next, we loop over all (active) crossing to generate the matching edges.
    
    const Int max_crossing_count = pd.MaxCrossingCount();
          Int c_pos   = n;
    
    cptr<Int>             C_arcs   = pd.Crossings().data();
    cptr<CrossingState_T> C_states = pd.CrossingStates().data();
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; };
        
        const I v_L = static_cast<I>(a_map[C_arcs[Int(4) * c + Int(0)]]);
        const I v_R = static_cast<I>(a_map[C_arcs[Int(4) * c + Int(1)]]);
        
        if( RightHandedQ(C_states[c]) )
        {
            tails[c_pos] = v_R + I(1);
            heads[c_pos] = v_L + I(1);
        }
        else
        {
            tails[c_pos] = v_L + I(1);
            heads[c_pos] = v_R + I(1);
        }
        
        costs[c_pos] = -1;
        
        ++c_pos;
    }
    
    MCFSimplex_T mcf (n,m);

    {
        TOOLS_MAKE_FP_STRICT()
        mcf.LoadNet(
            n, // maximal number of vertices
            m, // maximal number of edges
            n, // current number of vertices
            m, // current number of edges
            upper.data(),
            costs.data(),
            defects.data(),
            tails.data(),
            heads.data()
        );
        
        mcf.SolveMCF();
    }
    Tensor1<R,Int> potentials ( n );

    mcf.MCFGetPi(potentials.data());
    
    Tensor1<T,Int> L ( pd.ArcCount() );
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            L[a] = static_cast<T>(potentials[a_map[a]]);
        }
        else
        {
            L[a] = 0;
        }
    }
    
    return L;
}
