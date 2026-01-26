template<typename ExtInt2>
Tensor1<Turn_T,Int> Bends_RelaxIV(
    cref<PD_T> pd,
    const ExtInt2 ext_region = -1
)
{
    TOOLS_MAKE_FP_STRICT();

    std::string tag = MethodName("Bends_RelaxIV");
    
    TOOLS_PTIMER(timer,tag);
    
    Int ext_region_ = int_cast<Int>(ext_region);
    
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
    
    cptr<Int> dA_F = pd.ArcFaces().data();
    auto A_idx = Bends_ArcIndices(pd);
    
    const I m = Bends_VarCount<I>(pd);
    const I n = Bends_ConCount<I>(pd);
    
    Tensor1<R,I> capacities(m,Scalar::Max<R>);
    Tensor1<I,I> tails (m);
    Tensor1<I,I> heads (m);

    const Int a_count = pd.Arcs().Dim(0);
    
    for( Int a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        // right face of a
        const I f_0  = static_cast<I>( dA_F[pd.ToDarc(a,Tail)] );
        // left  face of a
        const I f_1  = static_cast<I>( dA_F[pd.ToDarc(a,Head)] );
        
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

    Tensor1<R,Int> s ( Int(2) * a_count );

    mcf.MCFGetX(s.data());
    
    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );

    for( Int a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const Int tail = A_idx(a,0);
            const Int head = A_idx(a,1);
            bends[a] = static_cast<Turn_T>(std::round(s[head] - s[tail]));
        }
        else
        {
            bends[a] = Turn_T(0);
        }
    }
    
    return bends;
}
