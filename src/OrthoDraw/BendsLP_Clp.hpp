public:

/*! @brief Computes a vector `bends` whose entries are signed integers. For each arc `a` the entry `bends[a]` is the number of 90-degree bends for that arc. Positive numbers mean bends to the left; positive numbers mean bend to the right. The l^1 norm of `bends` subject to the constraints that for each face `f` the sum of bends along bounday arcs of `d` and of the number of corners of `f` have to sum to 4, which corresponds to winding number 1. These constraints are linear, so that this is a linearly constraint and l^1 optimization problem. It is reformulated as linear programming problem and solved with a simplex-based method to obtain a basis solution `bend`. By the structure of this problem, all entries of `bends` are guaranteed to be integers.
 *  This approach is similar to Tamassia, On embedding a graph in the grid with the minimum number of bends. Siam J. Comput. 16 (1987) http://dx.doi.org/10.1137/0216030. The main difference is that we can assume that each vertex in the graph underlying the planar diagram has valence 4. Moreover, we do not use the elaborate formulation as min cost flow problem, which would allow us to use a potentially faster network solver. Instead, we use a CLP, a generic solver for linear problems.
 *
 * @param pd Planar diagram whose bends we want to compute.
 *
 * @param ext_region Specify which face shall be treated as exterior region. If `ext_region < 0` or `ext_region > pd.FaceCount()`, then a face with maximum number of arcs is chosen.
 */

// TODO: I have to filter out inactive crossings and inactive arcs!

template<typename ExtInt, typename ExtInt2>
Tensor1<Turn_T,Int> ComputeBends_Clp(
    mref<PlanarDiagram<ExtInt>> pd,
    const ExtInt2 ext_region = -1,
    bool dualQ = false
)
{
    TOOLS_MAKE_FP_STRICT();

    TOOLS_PTIMER(timer,ClassName()+"::ComputeBends_Clp");
    
    ExtInt ext_region_ = int_cast<ExtInt>(ext_region);
            
    {
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dim(0));
        Size_T nnz     = Size_T(4) * static_cast<Size_T>(pd.ArcCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<COIN_Int>::max() ) )
        {
            eprint(ClassName()+"::ComputeBends_Clp: Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<COIN_LInt>::max() ) )
        {
            eprint(ClassName()+"::ComputeBends_Clp: System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    
    using R = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    using Clp_T = ClpWrapper<double,Int,Int>;
    using Settings_T = Clp_T::Settings_T;
    
    auto con_eq = Bends_EqualityConstraintVector<R,I>(pd,ext_region_);
    
    std::shared_ptr<Clp_T> clp;
    
    Settings_T param { .dualQ = settings.use_dual_simplexQ };
    
    auto & A_idx = Bends_ArcIndices(pd);
    
    const ExtInt a_count = pd.Arcs().Dim(0);
    
    if( settings.network_matrixQ )
    {
        cptr<ExtInt> dA_F = pd.ArcFaces().data();
        
        
        const I n = Bends_VarCount<I>(pd);
//        const I m = Bends_ConCount<I>(pd);
        
        Tensor1<COIN_Int,I> tails ( n );
        Tensor1<COIN_Int,I> heads ( n );
 
        for( ExtInt a = 0; a < a_count; ++a )
        {
            if( !pd.ArcActiveQ(a) ) { continue; };
            
            // right face of a
            const I f_0  = static_cast<I>( dA_F[pd.template ToDarc<Tail>(a)] );
            // left  face of a
            const I f_1  = static_cast<I>( dA_F[pd.template ToDarc<Head>(a)] );

            // Clp seems to work with different sign than I.
            
            const I di_0 = static_cast<I>( A_idx(a,0) );
            const I di_1 = static_cast<I>( A_idx(a,1) );

            tails[di_0] = f_0;
            heads[di_0] = f_1;
            
            tails[di_1] = f_1;
            heads[di_1] = f_0;
        }
                
        clp = std::make_shared<Clp_T>(
            tails, heads,
            Bends_ObjectiveVector<R,I>(pd),
            Bends_LowerBoundsOnVariables<R,I>(pd),
            Bends_UpperBoundsOnVariables<R,I>(pd),
            con_eq,
            param
        );
    }
    else
    {
        clp = std::make_shared<Clp_T>(
            Bends_ObjectiveVector<R,I>(pd),
            Bends_LowerBoundsOnVariables<R,I>(pd),
            Bends_UpperBoundsOnVariables<R,I>(pd),
            Bends_ConstraintMatrix<R,I,J>(pd),
            con_eq,
            con_eq,
            param
        );
    }
    
    auto s = clp->template IntegralPrimalSolution<Turn_T>();

    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );
    
    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const ExtInt tail = A_idx(a,0);
            const ExtInt head = A_idx(a,1);
            bends[a] = s[head] - s[tail];
        }
        else
        {
            bends[a] = Turn_T(0);
        }
    }
    
    pd.ClearCache(MethodName("Bends_ArcIndices"));

    return bends;
}
