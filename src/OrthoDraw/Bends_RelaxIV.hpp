template<typename ExtInt, typename ExtInt2>
Tensor1<Turn_T,Int> Bends_RelaxIV(
    cref<PlanarDiagram<ExtInt>> pd,
    const ExtInt2 ext_region = -1
)
{
    TOOLS_MAKE_FP_STRICT();

    std::string tag = MethodName("Bends_RelaxIV");
    
    TOOLS_PTIMER(timer,tag);
    
    ExtInt ext_region_ = int_cast<ExtInt>(ext_region);
    
    using RelaxIV_T = RelaxIV::RelaxIV;
    using R = RelaxIV_T::FNumber;
    using I = RelaxIV_T::Index;
    
    if( (pd.CrossingCount() != pd.MaxCrossingCount()) || (pd.ArcCount() != pd.MaxArcCount()) )
    {
        wprint(tag + ": Input diagram is not compressed.");
    }
            
    {
        // TODO: Replace pd.Arcs().Dim(0) by pd.ArcCount().
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dim(0));
        
        if( std::cmp_greater( max_idx, std::numeric_limits<I>::max() ) )
        {
            eprint(tag+": Too many arcs to fit into type " + TypeName<I> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    auto A_idx = Bends_ArcIndices(pd);
    
    const I m = Bends_VarCount<I>(pd);
    const I n = Bends_ConCount<I>(pd);
    
    Tensor1<R,I> capacities(m,Scalar::Max<R>);
    Tensor1<I,I> tails (m);
    Tensor1<I,I> heads (m);

    const ExtInt a_count = pd.Arcs().Dim(0);
    
    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        // right face of a
        const I f_0  = static_cast<I>( dA_F[pd.template ToDarc<Tail>(a)] );
        // left  face of a
        const I f_1  = static_cast<I>( dA_F[pd.template ToDarc<Head>(a)] );
        
        const I di_0 = static_cast<I>( A_idx(a,0) );
        const I di_1 = static_cast<I>( A_idx(a,1) );

        // RelaxIV uses 1-based vertex indices!
        
        tails[di_0] = f_1 + I(1);
        heads[di_0] = f_0 + I(1);
        
        tails[di_1] = f_0 + I(1);
        heads[di_1] = f_1 + I(1);
    }
    
    RelaxIV_T mcf (n,m);

    mcf.LoadNet(
        n, // maximal number of vertices
        m, // maximal number of edges
        n, // current number of vertices
        m, // current number of edges
        capacities.data(),
        Bends_ObjectiveVector<R,I>(pd).data(),
        Bends_EqualityConstraintVector<R,I>(pd,ext_region_).data(),
        tails.data(),
        heads.data()
    );
    
    mcf.SolveMCF();

    Tensor1<R,ExtInt> s ( ExtInt(2) * a_count );

    mcf.MCFGetX(s.data());
    
    TOOLS_LOGDUMP(s);
    
    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );

    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const ExtInt tail = A_idx(a,0);
            const ExtInt head = A_idx(a,1);
            bends[a] = static_cast<Turn_T>(std::round(s[head] - s[tail]));
        }
        else
        {
            bends[a] = Turn_T(0);
        }
    }
    
    return bends;
}
