template<typename ExtInt2>
Tensor1<Turn_T,Int> Bends_MCF(
    cref<PD_T> pd,
    const ExtInt2 ext_region = -1
)
{
    TOOLS_MAKE_FP_STRICT();

    std::string tag = MethodName("Bends_MCF");
    
    TOOLS_PTIMER(timer,tag);
    
    Int ext_region_ = int_cast<Int>(ext_region);
    
    using MCFSimplex_T = MCF::MCFSimplex;
    using R = MCFSimplex_T::FNumber;
    using I = MCFSimplex_T::Index;
            
    {
        // TODO: Replace pd.Arcs().Dim(0) by pd.ArcCount().
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dim(0));
        
        if( std::cmp_greater( max_idx, std::numeric_limits<I>::max() ) )
        {
            eprint(tag+": Too many arcs to fit into type " + TypeName<I> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    
    cptr<Int> dA_F = pd.ArcFaces().data();
    auto A_idx = Bends_ArcIndices(pd);
    
    const I n = Bends_ConCount<I>(pd);
    const I m = Bends_VarCount<I>(pd);

    Tensor1<I,I> tails     (m);
    Tensor1<I,I> heads     (m);

    const Int a_count = pd.Arcs().Dim(0);
    
    TOOLS_LOGDUMP(pd.ArcCount());
    TOOLS_LOGDUMP(pd.MaxArcCount());
    TOOLS_LOGDUMP(pd.Arcs().Dim(0));
    TOOLS_LOGDUMP(n);
    TOOLS_LOGDUMP(m);
    
    for( Int a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        // right face of a
        const I f_0  = static_cast<I>( dA_F[pd.ToDarc(a,Tail)] );
        // left  face of a
        const I f_1  = static_cast<I>( dA_F[pd.ToDarc(a,Head)] );
        
        const I di_0 = static_cast<I>( A_idx(a,0) );
        const I di_1 = static_cast<I>( A_idx(a,1) );

        // MCF uses 1-based vertex indices!
        
        tails[di_0] = f_1 + I(1);
        heads[di_0] = f_0 + I(1);
        
        tails[di_1] = f_0 + I(1);
        heads[di_1] = f_1 + I(1);
    }
    
    auto costs      = Bends_ObjectiveVector<R,I>(pd);
    auto capacities = Bends_UpperBoundsOnVariables<R,I>(pd);
    auto demands    = Bends_EqualityConstraintVector<R,I>(pd,ext_region_);
    
    TOOLS_LOGDUMP(tails.MinMax());
    TOOLS_LOGDUMP(heads.MinMax());
    TOOLS_LOGDUMP(tails);
    TOOLS_LOGDUMP(heads);
    TOOLS_LOGDUMP(costs);
    TOOLS_LOGDUMP(capacities);
    TOOLS_LOGDUMP(demands);
    
    
    MCFSimplex_T mcf (n,m);

    {
        TOOLS_MAKE_FP_STRICT()
        mcf.LoadNet(
            n, // maximal number of vertices
            m, // maximal number of edges
            n, // current number of vertices
            m, // current number of edges
            capacities.data(),
            costs.data(),
            demands.data(),
            tails.data(),
            heads.data()
        );
        
        mcf.SolveMCF();
    }

    Tensor1<R,Int> s ( Int(2) * a_count );

    mcf.MCFGetX(s.data());

    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );
    
//    Turn_T min_bend = Scalar::Max<Turn_T>;
//    Turn_T max_bend = Scalar::Min<Turn_T>;

    for( Int a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const Int tail = A_idx(a,0);
            const Int head = A_idx(a,1);
            const Turn_T bend = static_cast<Turn_T>(std::round(s[head] - s[tail]));
            
//            min_bend = Min(min_bend,bend);
//            max_bend = Max(max_bend,bend);
                
            bends[a] = bend;
        }
        else
        {
            const Turn_T bend = Turn_T(0);
            
//            min_bend = Min(min_bend,bend);
//            max_bend = Max(max_bend,bend);
            
            bends[a] = bend;
        }
    }
    
//    TOOLS_LOGDUMP(min_bend);
//    TOOLS_LOGDUMP(max_bend);

    return bends;
}
