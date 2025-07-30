public:

template<typename T = Real, typename Int>
Tensor1<T,Int> LevelsByLP( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();

    std::string tag = ClassName()+"::LevelsByLP"
    + "<" + TypeName<Int>
    + ">";
    
    
    TOOLS_PTIMER(timer,ClassName()+"::LevelsByLP"
        + "<" + TypeName<Int>
        + ">"
    );
    
    {
        Size_T n       = ToSize_T(pd.CrossingCount());
        Size_T m       = ToSize_T(pd.ArcCount());
        Size_T max_idx = n + Size_T(2) * m + Size_T(1);
        Size_T nnz     = Size_T(7) * m + Size_T(1);

        if( !std::in_range<COIN_Int>(max_idx) )
        {
            eprint(tag + ": Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<T,Int>();
        }
        
        if( !std::in_range<COIN_Int>(nnz) )
        {
            eprint(tag + ": System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<T,Int>();
        }
    }
    
//    ClpSimplex LP;
//    LP.setMaximumIterations(1000000);
//    LP.setOptimizationDirection(1); // +1 - minimize; -1 - maximize
//    
//    auto A = this->template LevelsByLP_Matrix<COIN_Int,COIN_LInt>(pd);
//    
//    auto col_lower_bnd = LevelsByLP_LowerBoundsOnVariables(pd);
//    auto col_upper_bnd = LevelsByLP_UpperBoundsOnVariables(pd);
//    auto row_lower_bnd = LevelsByLP_LowerBoundsOnConstraints(pd);
//    auto row_upper_bnd = LevelsByLP_UpperBoundsOnConstraints(pd);
//    auto obj_vec       = LevelsByLP_ObjectiveVector(pd);
//    
//    LP.loadProblem(
//        A.RowCount(), A.ColCount(),
//        A.Outer().data(), A.Inner().data(), A.Values().data(),
//        col_lower_bnd.data(), col_upper_bnd.data(),
//        obj_vec.data(),
//        row_lower_bnd.data(), row_upper_bnd.data()
//    );
//
//    LP.primal();
    
    using R = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    ClpWrapper<R,I,J> clp(
        this->template LevelsByLP_ObjectiveVector<R,I>(pd),
        this->template LevelsByLP_LowerBoundsOnVariables<R,I>(pd),
        this->template LevelsByLP_UpperBoundsOnVariables<R,I>(pd),
        this->template LevelsByLP_Matrix<R,I,J>(pd),
        this->template LevelsByLP_LowerBoundsOnConstraints<R,I>(pd),
        this->template LevelsByLP_UpperBoundsOnConstraints<R,I>(pd)
    );
    
    auto s = clp.template IntegralPrimalSolution<R>();
    
    cptr<Int> A_pos = pd.ArcPositions().data();
    
    const Int A_count = pd.MaxArcCount();
    
    Tensor1<T,Int> L (A_count);
    
    for( Int a = 0; a < A_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            L[a] = s[A_pos[a]];
        }
        else
        {
            L[a] = 0;
        }
    }

////    Tensor1<Real,Int> L ( pd.ArcCount() );
////    
////    cptr<Real> sol = LP.primalColumnSolution();
////    
////    Real minimum = std::round( sol[0] );
//    
//    const Int m = pd.ArcCount();
        
    // TODO: Check integrality.
    
//    for( Int a = 0; a < m; ++a )
//    {
//        L[a] = std::round( sol[a] );
//        
//        minimum = Min( minimum, L[a] );
//    }
//    
//    for( Int a = 0; a < m; ++a )
//    {
//        L[a] -= minimum;
//    }
    
    return L;
}


public:

template<
    typename R = Real, typename I = COIN_Int, typename J = COIN_LInt,
    typename Int
>
Sparse::MatrixCSR<R,I,J> LevelsByLP_Matrix( mref<PlanarDiagram<Int>> pd ) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    std::string tag = ClassName() + "::LevelsByLP_Matrix"
    + "<" + TypeName<R>
    + "," + TypeName<I>
    + "," + TypeName<J>
    + "," + TypeName<Int>
    + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    const Int row_count = Size_T(2) * ToSize_T(pd.ArcCount());
    const Int col_count = ToSize_T(pd.CrossingCount()) + Size_T(2) * ToSize_T(pd.ArcCount()) + Size_T(1);
    
    Size_T max_index = Max(row_count,col_count);
    if( !std::in_range<I>(max_index) )
    {
        eprint(tag + ": Type " + TypeName<I> + " is too small to store maximum index = " + ToString(max_index) + ". Aborting.");
        return Sparse::MatrixCSR<Real,I,J>();
    }
    
    Size_T nnz = Size_T(7) * ToSize_T(pd.ArcCount()) + Size_T(1);
    
    if( !std::in_range<J>(nnz) )
    {
        eprint(tag + ": Type " + TypeName<J> + " is too small to store number of nonzero elements = " + ToString(nnz) + ". Aborting.");
        return Sparse::MatrixCSR<Real,I,J>();
    }
    
    cptr<Int>           C_arcs   = pd.Crossings().data();
    cptr<CrossingState> C_states = pd.CrossingStates().data();
    cptr<Int>           A_pos    = pd.ArcPositions().data();
    
    TripleAggregator<I,I,R,J> agg ( nnz );
    
    const Int C_count = pd.MaxCrossingCount();
    
    const I m = static_cast<I>(pd.ArcCount());
    const I n = static_cast<I>(pd.CrossingCount());
    I c_pos = 0;
    
    constexpr R one = 1;
    
    // We assemble the matrix transpose because CLP assumes column-major ordering.
    
    for( Int c = 0; c < C_count; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }

        const Int a_0 = C_arcs[Int(4) * c + Int(0)];
        const Int a_1 = C_arcs[Int(4) * c + Int(1)];
        const Int b_1 = C_arcs[Int(4) * c + Int(2)];
        const Int b_0 = C_arcs[Int(4) * c + Int(3)];
        
        const I a_0_pos = static_cast<I>(A_pos[a_0]);
        const I a_1_pos = static_cast<I>(A_pos[a_1]);
        const I b_1_pos = static_cast<I>(A_pos[b_1]);
        const I b_0_pos = static_cast<I>(A_pos[b_0]);

        const R s = static_cast<R>(ToUnderlying(C_states[c]));
        
        //  Case: right-handed.
        //
        //      a_0     a_1
        //        ^     ^
        //         \   /
        //          \ /
        //           /
        //          / \
        //         /   \
        //        /     \
        //      b_1     b_0
        //
        // If s == 1 (right-handed), then the levels `x` have to satisfy the following inequalities:
        // x[a_1] >= x[a_0] + 1
        // - x[a_0] + x[a_1] >= 1
        //   x[a_0] - x[a_1] <= -1
        
        // Over/under constraints
        agg.Push( a_0_pos, c_pos,  s );
        agg.Push( a_1_pos, c_pos, -s );
        
//        const I col_0 = n     + I(2) * c;
//        const I col_1 = n + m + I(2) * c;

        // difference operator
        agg.Push( b_0_pos    , b_0_pos + n, -one );
        agg.Push( a_0_pos    , b_0_pos + n,  one );
        agg.Push( b_1_pos    , b_1_pos + n, -one );
        agg.Push( a_1_pos    , b_1_pos + n,  one );
        
        agg.Push( b_0_pos + m, b_0_pos + n, -one );
        agg.Push( b_1_pos + m, b_1_pos + n, -one );
        
        const I k = n + m;
        
        // negative difference operator
        agg.Push( b_0_pos    , b_0_pos + k,  one );
        agg.Push( a_0_pos    , b_0_pos + k, -one );
        agg.Push( b_1_pos    , b_1_pos + k,  one );
        agg.Push( a_1_pos    , b_1_pos + k, -one );
        
        agg.Push( b_0_pos + m, b_0_pos + k, -one );
        agg.Push( b_1_pos + m, b_1_pos + k, -one );
        
        ++c_pos;
    }
    
    // Base point constraint
    agg.Push( I(0), n + I(2) * m,  one );
    
    Sparse::MatrixCSR<R,I,J> A (
        agg, row_count, col_count, I(1), true, false
    );
    
    return A;
}

template<typename R = Real, typename I = COIN_Int, typename Int>
Tensor1<R,I> LevelsByLP_LowerBoundsOnVariables( mref<PlanarDiagram<Int>> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<R,I>( Size_T(2) * ToSize_T(pd.ArcCount()), -Scalar::Infty<R> );
}

template<typename R = Real, typename I = COIN_Int, typename Int>
Tensor1<R,I> LevelsByLP_UpperBoundsOnVariables( mref<PlanarDiagram<Int>> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<R,I>( Size_T(2) * ToSize_T(pd.ArcCount()), +Scalar::Infty<R> );
}

template<typename R = Real, typename I = COIN_Int, typename Int>
Tensor1<R,I> LevelsByLP_LowerBoundsOnConstraints( mref<PlanarDiagram<Int>> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    
    const Size_T m = ToSize_T(pd.ArcCount());
    const Size_T n = ToSize_T(pd.CrossingCount());
    Tensor1<R,I> v ( n + Size_T(2) * m + Size_T(1), -Scalar::Infty<R> );
    
    v.Last() = R(0);
    
    return v;
}

template<typename R = Real, typename I = COIN_Int, typename Int>
Tensor1<R,I> LevelsByLP_UpperBoundsOnConstraints( mref<PlanarDiagram<Int>> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    
    const Size_T m = ToSize_T(pd.ArcCount());
    const Size_T n = ToSize_T(pd.CrossingCount());
    Tensor1<R,I> v ( n + Size_T(2) * m + Size_T(1) );
    
    fill_buffer( &v[0], R(-1), n                         );
    fill_buffer( &v[n], R( 0), Size_T(2) * m + Size_T(1) );
    
    return v;
}

template<typename R = Real, typename I = COIN_Int, typename Int>
Tensor1<R,I> LevelsByLP_ObjectiveVector( mref<PlanarDiagram<Int>> pd ) const
{
    const Size_T m = ToSize_T(pd.ArcCount());
    Tensor1<R,I> v ( Size_T(2) * m );
    
    fill_buffer( &v[0], R(0), m );
    fill_buffer( &v[m], R(1), m );
    
    return v;
}
