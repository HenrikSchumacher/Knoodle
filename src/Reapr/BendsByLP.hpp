public:

template<typename Int>
Tensor1<Real,Int> BendsByLP( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();

    {
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.ArcCount());
        Size_T nnz     = Size_T(4) * static_cast<Size_T>(pd.ArcCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<COIN_Int>::max() ) )
        {
            eprint(ClassName()+"::BendsByLP: Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<Real,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<COIN_LInt>::max() ) )
        {
            eprint(ClassName()+"::BendsByLP: System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<Real,Int>();
        }
    }
    
    TOOLS_PTIC( ClassName() + "::BendsByLP"
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
    
//    TOOLS_DUMP(col_lower_bnd);
//    TOOLS_DUMP(col_upper_bnd);
//    TOOLS_DUMP(col_lower_bnd);
//    TOOLS_DUMP(obj_vec);
    
    LP.loadProblem(
        A.RowCount(), A.ColCount(),
        A.Outer().data(), A.Inner().data(), A.Values().data(),
        col_lower_bnd.data(), col_upper_bnd.data(),
        obj_vec.data(),
        row_eq_vec.data(), row_eq_vec.data()
    );

    LP.primal();
    
    iter = LP.getIterationCount();

    Tensor1<Real,Int> bends ( pd.ArcCount() );
    
    cptr<Real> sol = LP.primalColumnSolution();
    
//    TOOLS_DUMP(ArrayToString(sol,{2*pd.ArcCount()}));
    
    for( Int a = 0; a < pd.ArcCount(); ++a )
    {
        bends[a] = std::round(sol[2 * a] - sol[2 * a + 1]);
    }

    TOOLS_PTOC( ClassName() + "::BendsByLP"
        + "<" + TypeName<Int>
        + ">"
    );
    
    return bends;
}
