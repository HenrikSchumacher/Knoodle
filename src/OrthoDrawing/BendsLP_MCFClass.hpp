template<typename ExtInt, typename ExtInt2>
Tensor1<Turn_T,Int> ComputeBends_MCF(
    mref<PlanarDiagram<ExtInt>> pd,
    const ExtInt2 ext_region = -1,
    bool dualQ = false
)
{
    TOOLS_MAKE_FP_STRICT();

    TOOLS_PTIMER(timer,MethodName("ComputeBends_MCF"));
    
    ExtInt ext_region_ = int_cast<ExtInt>(ext_region);
    
    
    using MCFSimplex_T = MCF::MCFSimplex;
    using R = MCFSimplex_T::FNumber;
    using I = MCFSimplex_T::Index;
//    using J = MCFSimplex_T::Index;
            
    {
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dim(0));
        Size_T nnz     = Size_T(4) * static_cast<Size_T>(pd.ArcCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<I>::max() ) )
        {
            eprint(MethodName("ComputeBends_MCF")+": Too many arcs to fit into type " + TypeName<I> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<I>::max() ) )
        {
            eprint(MethodName("ComputeBends_MCF")+": System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<I> + "  ).");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    
    const I m = I(2) * int_cast<I>(pd.Arcs().Dim(0));
    const I n = int_cast<I>(pd.FaceCount());
    
    Tensor1<I,I> tails ( m );
    Tensor1<I,I> heads ( m );

    const ExtInt a_count = pd.Arcs().Dim(0);
    
    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        const I da_0 = static_cast<I>( pd.template ToDarc<Tail>(a) );
        const I da_1 = static_cast<I>( pd.template ToDarc<Head>(a) );
        
        const I f_0  = static_cast<I>(dA_F[da_0]); // right face of a
        const I f_1  = static_cast<I>(dA_F[da_1]); // left  face of a

        // MCF uses 1-bases vertex indices!
        
        tails[da_0] = f_1 + I(1);
        heads[da_0] = f_0 + I(1);
        
        tails[da_1] = f_0 + I(1);
        heads[da_1] = f_1 + I(1);
    }
    
//    std::unique_ptr<MCFSimplex_T> mcf = std::make_unique<MCFSimplex_T>(n,m);
//
//    mcf->LoadNet(
//        n, // maximal number of vertices
//        m, // maximal number of edges
//        n, // current number of vertices
//        m, // current number of edges
//        Bends_UpperBoundsOnVariables<R,I>(pd).data(),
//        Bends_ObjectiveVector<R,I>(pd).data(),
//        Bends_EqualityConstraintVector<R,I>(pd,ext_region_).data(),
//        tails.data(),
//        heads.data()
//    );
//    
//    mcf->SolveMCF();
    
    MCFSimplex_T mcf (n,m);

    mcf.LoadNet(
        n, // maximal number of vertices
        m, // maximal number of edges
        n, // current number of vertices
        m, // current number of edges
        Bends_UpperBoundsOnVariables<R,I>(pd).data(),
        Bends_ObjectiveVector<R,I>(pd).data(),
        Bends_EqualityConstraintVector<R,I>(pd,ext_region_).data(),
        tails.data(),
        heads.data()
    );
    
    mcf.SolveMCF();

    Tensor1<R,ExtInt> s ( ExtInt(2) * a_count );

    mcf.MCFGetX(s.data());

    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );

    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const ExtInt head = pd.ToDarc(a,Head);
            const ExtInt tail = pd.ToDarc(a,Tail);
            bends[a] = static_cast<Turn_T>(std::round(s[head] - s[tail]));
        }
        else
        {
            bends[a] = Turn_T(0);
        }
    }

    return bends;
}
