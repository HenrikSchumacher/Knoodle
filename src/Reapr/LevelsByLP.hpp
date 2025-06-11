public:

template<typename Int>
Tensor1<Real,Int> LevelsByLP( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();

    {
        Size_T n       = static_cast<Size_T>(pd.CrossingCount());
        Size_T m       = static_cast<Size_T>(pd.ArcCount());
        Size_T max_idx = n + Size_T(2) * m + Size_T(1);
        Size_T nnz     = Size_T(7) * m + Size_T(1);
        
        if( std::cmp_greater( max_idx, std::numeric_limits<COIN_Int>::max() ) )
        {
            eprint(ClassName()+"::LevelsByLP: Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<Real,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<COIN_LInt>::max() ) )
        {
            eprint(ClassName()+"::LevelsByLP: System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<Real,Int>();
        }
    }
    
    
    TOOLS_PTIC( ClassName()+"::LevelsByLP"
        + "<" + TypeName<Int>
        + ">"
    );
    
    ClpSimplex LP;
    LP.setMaximumIterations(1000000);
    LP.setOptimizationDirection(1); // +1 - minimize; -1 - maximize
    
    auto A = this->template LevelsMatrixLP<COIN_Int,COIN_LInt>(pd);
    
    auto col_lower_bnd = LevelsColLowerBounds(pd);
    auto col_upper_bnd = LevelsColUpperBounds(pd);
    auto row_lower_bnd = LevelsRowLowerBounds(pd);
    auto row_upper_bnd = LevelsRowUpperBounds(pd);
    auto obj_vec       = LevelsObjectiveVector(pd);
    
    LP.loadProblem(
        A.RowCount(), A.ColCount(),
        A.Outer().data(), A.Inner().data(), A.Values().data(),
        col_lower_bnd.data(), col_upper_bnd.data(),
        obj_vec.data(),
        row_lower_bnd.data(), row_upper_bnd.data()
    );

    LP.primal();
    
    iter = LP.getIterationCount();

    Tensor1<Real,Int> L ( pd.ArcCount() );
    
    cptr<Real> sol = LP.primalColumnSolution();
    
    Real minimum = std::round( sol[0] );
    
    for( Int i = 0; i < pd.ArcCount(); ++i )
    {
        L[i] = std::round( sol[i] );
        
        minimum = Min( minimum, L[i] );
    }
    
    for( Int i = 0; i < pd.ArcCount(); ++i )
    {
        L[i] -= minimum;
    }

    TOOLS_PTOC( ClassName()+"::LevelsByLP"
        + "<" + TypeName<Int>
        + ">"
    );
    
    return L;
}
