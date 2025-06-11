public:

// TODO: Try some of the suggestions from https://stackoverflow.com/a/63254942/8248900:
// TODO: Coin-OR's Lemon library
// TODO: Coin-OR's Network Simplex

/*! @brief Computes a vector `bends` whose entries are signed integers. For each arc `a` the entry `bends[a]` is the number of 90-degree bends for that arc. Positive numbers mean bends to the left; positive numbers mean bend to the right. The l^1 norm of `bends` subject to the constraints that for each face `f` the sum of bends along bounday arcs of `d` and of the number of corners of `f` have to sum to 4, which corresponds to winding number 1. These constraints are linear, so that this is a linearly constraint and l^1 optimization problem. It is reformulated as linear programming problem and solved with a simplex-based method to obtain a basis solution `bend`. By the structure of this problem, all entries of `bends` are guaranteed to be integers.
 *  This approach is similar to Tamassia, On embedding a graph in the grid with the minimum number of bends. Siam J. Comput. 16 (1987) http://dx.doi.org/10.1137/0216030. The main difference is that we can assume that each vertex in the graph underlying the planar diagram has valence 4. Moreover, we do not use the elaborate formulation as min cost flow problem, which would allow us to use a potentially faster network solver. Instead, we use a CLP, a generic solver for linear problems.
 *
 * @param pd Planar diagram whose bends we want to compute.
 *
 * @param ext_face Specify which face shall be treated as exterior face. If `ext_face < 0`, then a face with maximum number of arcs is chosen.
 */

template<typename Int>
Tensor1<Int,Int> BendsByLP(
    mref<PlanarDiagram<Int>> pd, const Int ext_face = -1
)
{
    TOOLS_MAKE_FP_STRICT();

    {
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.ArcCount());
        Size_T nnz     = Size_T(4) * static_cast<Size_T>(pd.ArcCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<COIN_Int>::max() ) )
        {
            eprint(ClassName()+"::BendsByLP: Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<Int,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<COIN_LInt>::max() ) )
        {
            eprint(ClassName()+"::BendsByLP: System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<Int,Int>();
        }
    }
    
    TOOLS_PTIC( ClassName()+"::BendsByLP"
        + "<" + TypeName<Int>
        + ">"
    );
    
    ClpSimplex LP;
    LP.setMaximumIterations(1000000);
    LP.setOptimizationDirection(1); // +1 -> minimize; -1 -> maximize
    
    auto A = this->template BendsMatrix<COIN_Int,COIN_LInt>(pd);
    
    auto col_lower_bnd = BendsColLowerBounds(pd);
    auto col_upper_bnd = BendsColUpperBounds(pd);
    auto row_eq_vec    = BendsRowEqualityVector(pd);
    auto obj_vec       = BendsObjectiveVector(pd);
    
    LP.loadProblem(
        A.RowCount(), A.ColCount(),
        A.Outer().data(), A.Inner().data(), A.Values().data(),
        col_lower_bnd.data(), col_upper_bnd.data(),
        obj_vec.data(),
        row_eq_vec.data(), row_eq_vec.data()
    );

    LP.primal();
    
    iter = LP.getIterationCount();

    Tensor1<Int,Int> bends ( pd.ArcCount() );
    
    cptr<Real> sol = LP.primalColumnSolution();

    for( Int a = 0; a < pd.ArcCount(); ++a )
    {
        bends[a] = static_cast<Int>(std::round(sol[2 * a] - sol[2 * a + 1]));
    }

    TOOLS_PTOC( ClassName()+"::BendsByLP"
        + "<" + TypeName<Int>
        + ">"
    );
    
    return bends;
}
