public:


template<typename T = Real>
Tensor1<T,Int> LevelsLP_MCF( cref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    using MCFSimplex_T = MCF::MCFSimplex;
    using R = MCFSimplex_T::FNumber;
    using I = MCFSimplex_T::Index;
    
    std::string tag = MethodName("LevelsLP_MCF");
    
    TOOLS_PTIMER(timer,tag);
    
    {
        Size_T max_idx = Size_T(5) * static_cast<Size_T>(pd.CrossingCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<I>::max() ) )
        {
            eprint(tag + ": Too many arcs to fit into type " + TypeName<I> + ".");
            
            return Tensor1<T,Int>();
        }
    }
    
    // We use LevelsLP_ArcIndices to take care of gaps in the data structure. Moreover, it facilitates random permutation of the arcs if this feature is active.
    
    cptr<Int> A_pos   = LevelsLP_ArcIndices(pd).data();
    cptr<Int> A_next  = pd.ArcNextArc().data();
    
    const I n = I(4) * static_cast<I>(pd.CrossingCount());
    const I m = I(5) * static_cast<I>(pd.CrossingCount());
    
    Tensor1<R,I> upper   (m,Scalar::Infty<R>);
    Tensor1<I,I> tails   (m);
    Tensor1<I,I> heads   (m);
    Tensor1<R,I> costs   (m);
    Tensor1<R,I> defects (n);
    
    // First, we loop over all (active) arcs to generate the cycle edges.
    
    const Int a_count = pd.MaxArcCount();
    
    for( Int a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        const I v_0 = static_cast<I>(A_pos[a]);
        const I v_1 = static_cast<I>(A_pos[A_next[a]]);
        const I w   = static_cast<I>(a_count) + v_0;

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
    
    const Int c_count = pd.MaxCrossingCount();
          Int c_pos   = n;
    
    cptr<Int>             C_arcs   = pd.Crossings().data();
    cptr<CrossingState_T> C_states = pd.CrossingStates().data();
    
    for( Int c = 0; c < c_count; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; };
        
        const I v_L = static_cast<I>(A_pos[C_arcs[Int(4) * c + Int(0)]]);
        const I v_R = static_cast<I>(A_pos[C_arcs[Int(4) * c + Int(1)]]);
        
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

    Tensor1<R,Int> potentials ( n );

    mcf.MCFGetPi(potentials.data());
    
    Tensor1<T,Int> L ( pd.ArcCount() );
    
    for( Int a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            L[a] = static_cast<T>(potentials[A_pos[a]]);
        }
        else
        {
            L[a] = 0;
        }
    }
    
    return L;
}

cref<Tensor1<Int,Int>> LevelsLP_ArcIndices( cref<PlanarDiagram<Int>> pd ) const
{
    std::string tag (MethodName("LevelsLP_ArcIndices"));
    
    if(!pd.InCacheQ(tag))
    {
        const Int a_count = pd.MaxArcCount();
        
        Tensor1<Int,Int> A_idx ( a_count );
        Permutation<Int> perm;
        
        Int a_idx = 0;
        
        if( permute_randomQ )
        {
            perm = Permutation<Int>::RandomPermutation(
               a_count, Int(1), random_engine
            );
            
            cptr<Int> p = perm.GetPermutation().data();
            
            for( Int a = 0; a < a_count; ++a )
            {
                if( pd.ArcActiveQ(a) )
                {
                    A_idx(a) = p[a_idx];
                    ++a_idx;
                }
                else
                {
                    A_idx(a) = PD_T::Uninitialized;
                }
            }
        }
        else
        {
            for( Int a = 0; a < a_count; ++a )
            {
                if( pd.ArcActiveQ(a) )
                {
                    A_idx(a) = a_idx;
                    ++a_idx;
                }
                else
                {
                    A_idx(a) = PD_T::Uninitialized;
                }
            }
        }
        
        pd.SetCache(tag,std::move(A_idx));
    }
    
    return pd.template GetCache<Tensor1<Int,Int>>(tag);
}
