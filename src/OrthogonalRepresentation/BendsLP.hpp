public:

// TODO: Try some of the suggestions from https://stackoverflow.com/a/63254942/8248900:
// TODO: Coin-OR's Lemon library
// TODO: Coin-OR's Network Simplex

/*! @brief Computes a vector `bends` whose entries are signed integers. For each arc `a` the entry `bends[a]` is the number of 90-degree bends for that arc. Positive numbers mean bends to the left; positive numbers mean bend to the right. The l^1 norm of `bends` subject to the constraints that for each face `f` the sum of bends along bounday arcs of `d` and of the number of corners of `f` have to sum to 4, which corresponds to winding number 1. These constraints are linear, so that this is a linearly constraint and l^1 optimization problem. It is reformulated as linear programming problem and solved with a simplex-based method to obtain a basis solution `bend`. By the structure of this problem, all entries of `bends` are guaranteed to be integers.
 *  This approach is similar to Tamassia, On embedding a graph in the grid with the minimum number of bends. Siam J. Comput. 16 (1987) http://dx.doi.org/10.1137/0216030. The main difference is that we can assume that each vertex in the graph underlying the planar diagram has valence 4. Moreover, we do not use the elaborate formulation as min cost flow problem, which would allow us to use a potentially faster network solver. Instead, we use a CLP, a generic solver for linear problems.
 *
 * @param pd Planar diagram whose bends we want to compute.
 *
 * @param ext_f Specify which face shall be treated as exterior face. If `ext_f < 0` or `ext_f > pd.FaceCount()`, then a face with maximum number of arcs is chosen.
 */

template<typename ExtInt, typename ExtInt2>
Tensor1<Turn_T,Int> Bends(
    mref<PlanarDiagram<ExtInt>> pd, const ExtInt2 ext_f = -1, bool dualQ = false
)
{
    TOOLS_MAKE_FP_STRICT();

    TOOLS_PTIMER( timer, ClassName()+"::Bends");
    
    ExtInt ext_f_ = int_cast<ExtInt>(ext_f);
    
    {
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dimension(0));
        Size_T nnz     = Size_T(4) * static_cast<Size_T>(pd.ArcCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<COIN_Int>::max() ) )
        {
            eprint(ClassName()+"::Bends: Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<COIN_LInt>::max() ) )
        {
            eprint(ClassName()+"::Bends: System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    

    ClpSimplex LP;
    LP.setMaximumIterations(1000000);
    LP.setOptimizationDirection(1); // +1 -> minimize; -1 -> maximize
    
    
    using R = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    auto A             = Bends_ConstraintMatrix<R,I,J>(pd);
    auto var_lower_bnd = Bends_LowerBoundsOnVariables<R,I>(pd);
    auto var_upper_bnd = Bends_UpperBoundsOnVariables<R,I>(pd);
    auto constr_eq_vec = Bends_EqualityConstraintVector<R,I>(pd,ext_f_);
    auto obj_vec       = Bends_ObjectiveVector<R,I>(pd);
    
    LP.loadProblem(
        A.RowCount(), A.ColCount(),
        A.Outer().data(), A.Inner().data(), A.Values().data(),
        var_lower_bnd.data(), var_upper_bnd.data(),
        obj_vec.data(),
        constr_eq_vec.data(), constr_eq_vec.data()
    );

    if( dualQ )
    {
        LP.dual();
        
        if( !LP.statusOfProblem() )
        {
            eprint(ClassName()+"::Bends: Clp::Simplex::dual reports a problem in the solve phase. The returned solution may be incorrect.");
            
    //        valprint("Primal problem feasible" BoolString(LP.primalFeasible()) );
    //        valprint("Dual   problem feasible" BoolString(LP.dualFeasible()) );
            
            TOOLS_DUMP(LP.getIterationCount());
            TOOLS_DUMP(LP.primalFeasible());
            TOOLS_DUMP(LP.dualFeasible());
            TOOLS_DUMP(LP.largestPrimalError());
            TOOLS_DUMP(LP.largestDualError());
        }
    }
    else
    {
        LP.primal();
        
        if( !LP.statusOfProblem() )
        {
            eprint(ClassName()+"::Bends: Clp::Simplex::primal reports a problem in the solve phase. The returned solution may be incorrect.");
            
    //        valprint("Primal problem feasible" BoolString(LP.primalFeasible()) );
    //        valprint("Dual   problem feasible" BoolString(LP.dualFeasible()) );
            
            TOOLS_DUMP(LP.getIterationCount());
            TOOLS_DUMP(LP.primalFeasible());
            TOOLS_DUMP(LP.dualFeasible());
            TOOLS_DUMP(LP.largestPrimalError());
            TOOLS_DUMP(LP.largestDualError());
        }
    }
    

    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dimension(0) );
    mptr<Int> bends_ptr = bends.data();
    
    cptr<COIN_Real> sol = LP.primalColumnSolution();

    for( ExtInt a = 0; a < pd.Arcs().Dimension(0); ++a )
    {
        const ExtInt head = pd.ToDarc(a,Head);
        const ExtInt tail = pd.ToDarc(a,Tail);
        bends_ptr[a] = static_cast<Turn_T>(std::round(sol[head] - sol[tail]));
    }

    return bends;
}


public:

template<typename S, typename I, typename J, typename ExtInt>
static Sparse::MatrixCSR<S,I,J> Bends_ConstraintMatrix( mref<PlanarDiagram<ExtInt>> pd
)
{
    TOOLS_PTIC(ClassName()+"::Bends_ConstraintMatrix");
    
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    
    TripleAggregator<I,I,S,J> agg ( J(4) * static_cast<J>(pd.ArcCount()) );
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    for( ExtInt a = 0; a < pd.Arcs().Dimension(0); ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        const I da_0 = static_cast<I>( pd.template ToDarc<Tail>(a) );
        const I da_1 = static_cast<I>( pd.template ToDarc<Head>(a) );
        
        const I f_0  = static_cast<I>(dA_F[da_0]); // right face of a
        const I f_1  = static_cast<I>(dA_F[da_1]); // left  face of a
                                     
        agg.Push( da_0, f_0,  S(1) );
        agg.Push( da_0, f_1, -S(1) );
        agg.Push( da_1, f_0, -S(1) );
        agg.Push( da_1, f_1,  S(1) );
    }
    
    const I var_count        = I(2) *static_cast<I>(pd.ArcCount());
    const I constraint_count = static_cast<I>(pd.FaceCount());
    
    Sparse::MatrixCSR<S,I,J> A ( agg, var_count, constraint_count, true, false );

    TOOLS_PTOC(ClassName()+"::Bends_ConstraintMatrix");
    
    return A;
}



template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_LowerBoundsOnVariables( mref<PlanarDiagram<ExtInt>> pd )
{
    const I var_count = I(2) *static_cast<I>(pd.ArcCount());
    
    // All bends must be nonnegative.
    return Tensor1<S,I>( var_count, S(0) );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_UpperBoundsOnVariables( mref<PlanarDiagram<ExtInt>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    const I var_count = I(2) *static_cast<I>(pd.ArcCount());
    
    return Tensor1<S,I>( var_count, Scalar::Infty<S> );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_EqualityConstraintVector(
    mref<PlanarDiagram<ExtInt>> pd, const ExtInt ext_f = -1
)
{
    const I constraint_count = int_cast<I>(pd.FaceCount());
    
    Tensor1<S,I> v ( constraint_count );
    mptr<S> v_ptr = v.data();
    
    cptr<ExtInt> F_dA_ptr = pd.FaceDirectedArcPointers().data();

    ExtInt max_f_size = 0;
    ExtInt max_f      = 0;
    
    for( ExtInt f = 0; f < pd.FaceCount(); ++f )
    {
        const ExtInt f_size = F_dA_ptr[f+1] - F_dA_ptr[f];
        
        if( f_size > max_f_size )
        {
            max_f_size = f_size;
            max_f      = f;
        }
        
        v_ptr[f] = S(4) - S(f_size);
    }
    
    if( (ExtInt(0) <= ext_f) && (ext_f < pd.FaceCount()) )
    {
        v_ptr[ext_f] -= S(8);
    }
    else
    {
        v_ptr[max_f] -= S(8);
    }
    
    return v;
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_ObjectiveVector( mref<PlanarDiagram<ExtInt>> pd )
{
    const I var_count = I(2) *static_cast<I>(pd.ArcCount());
    
    return Tensor1<S,I>( var_count, S(1) );
}
