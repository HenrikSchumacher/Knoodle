public:

template<typename T = Real>
Tensor1<T,Int> LevelsLP_CLP( cref<PD_T> pd )
{
    TOOLS_MAKE_FP_STRICT();

    std::string tag = ClassName() + "::LevelsLP_CLP";
    
    TOOLS_PTIMER(timer,tag);
    
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

    ClpWrapper<COIN_Real,COIN_Int,COIN_LInt> clp(
        this->template LevelsLP_CLP_ObjectiveVector         <COIN_Real,COIN_Int          >(pd),
        this->template LevelsLP_CLP_LowerBoundsOnVariables  <COIN_Real,COIN_Int          >(pd),
        this->template LevelsLP_CLP_UpperBoundsOnVariables  <COIN_Real,COIN_Int          >(pd),
        this->template LevelsLP_CLP_Matrix                  <COIN_Real,COIN_Int,COIN_LInt>(pd),
        this->template LevelsLP_CLP_LowerBoundsOnConstraints<COIN_Real,COIN_Int          >(pd),
        this->template LevelsLP_CLP_UpperBoundsOnConstraints<COIN_Real,COIN_Int          >(pd)
    );
    
    auto s = clp.template IntegralPrimalSolution<COIN_Real>();
    
    cptr<Int> A_pos = LevelsLP_ArcIndices(pd).data();
    
    const Int A_count = pd.MaxArcCount();
    
    Tensor1<T,Int> L (A_count);
    
    for( Int a = 0; a < A_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            L[a] = s.data()[A_pos[a]];
        }
        else
        {
            L[a] = 0;
        }
    }
    
    return L;
}


public:

template<
    typename R = Real, typename I = COIN_Int, typename J = COIN_LInt
>
Sparse::MatrixCSR<R,I,J> LevelsLP_CLP_Matrix( cref<PD_T> pd ) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    std::string tag = ClassName() + "::LevelsLP_CLP_Matrix"
    + "<" + TypeName<R>
    + "," + TypeName<I>
    + "," + TypeName<J>
    + ">";
    
//    TOOLS_PTIMER(timer,tag);
    
    const Size_T row_count = Size_T(2) * ToSize_T(pd.ArcCount());
    const Size_T col_count = ToSize_T(pd.CrossingCount()) + Size_T(2) * ToSize_T(pd.ArcCount()) + Size_T(1);
    
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
    
    cptr<Int>             C_arcs   = pd.Crossings().data();
    cptr<CrossingState_T> C_states = pd.CrossingStates().data();
//    cptr<Int>           A_pos    = pd.ArcPositions().data();
    cptr<Int>             A_pos    = LevelsLP_ArcIndices(pd).data();
    
    TripleAggregator<I,I,R,J> agg ( int_cast<J>(nnz) );
    
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
        
        /*  Case: right-handed.
         *
         *      a_0     a_1
         *        ^     ^
         *         \   /
         *          \ /
         *           /
         *          / \
         *         /   \
         *        /     \
         *      b_1     b_0
         *
         * If s == 1 (right-handed), then the levels `z` have to satisfy the following inequalities:
         * z[a_1] >= z[a_0] + 1
         * - z[a_0] + z[a_1] >= 1
         *   z[a_0] - z[a_1] <= -1
         */
        
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
        agg, int_cast<I>(row_count), int_cast<I>(col_count), I(1), true, false
    );
    
    return A;
}

template<typename R = Real, typename I = COIN_Int>
Tensor1<R,I> LevelsLP_CLP_LowerBoundsOnVariables( cref<PD_T> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<R,I>( Size_T(2) * ToSize_T(pd.ArcCount()), -Scalar::Infty<R> );
}

template<typename R = Real, typename I = COIN_Int>
Tensor1<R,I> LevelsLP_CLP_UpperBoundsOnVariables( cref<PD_T> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<R,I>( Size_T(2) * ToSize_T(pd.ArcCount()), +Scalar::Infty<R> );
}

template<typename R = Real, typename I = COIN_Int>
Tensor1<R,I> LevelsLP_CLP_LowerBoundsOnConstraints( cref<PD_T> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    
    const Size_T m = ToSize_T(pd.ArcCount());
    const Size_T n = ToSize_T(pd.CrossingCount());
    Tensor1<R,I> v ( n + Size_T(2) * m + Size_T(1), -Scalar::Infty<R> );
    
    v.Last() = R(0);
    
    return v;
}

template<typename R = Real, typename I = COIN_Int>
Tensor1<R,I> LevelsLP_CLP_UpperBoundsOnConstraints( cref<PD_T> pd ) const
{
    TOOLS_MAKE_FP_STRICT();
    
    const Size_T m = ToSize_T(pd.ArcCount());
    const Size_T n = ToSize_T(pd.CrossingCount());
    Tensor1<R,I> v ( n + Size_T(2) * m + Size_T(1) );
    
    fill_buffer( &v[0], R(-1), n                         );
    fill_buffer( &v[n], R( 0), Size_T(2) * m + Size_T(1) );
    
    return v;
}

template<typename R = Real, typename I = COIN_Int>
Tensor1<R,I> LevelsLP_CLP_ObjectiveVector( cref<PD_T> pd ) const
{
    const Size_T m = ToSize_T(pd.ArcCount());
    Tensor1<R,I> v ( Size_T(2) * m );
    
    fill_buffer( &v[0], R(0), m );
    fill_buffer( &v[m], R(1), m );
    
    pd.ClearCache(MethodName("LevelsLP_ArcIndices"));
    
    return v;
}

cref<Tensor1<Int,Int>> LevelsLP_ArcIndices( cref<PD_T> pd ) const
{
    std::string tag (MethodName("LevelsLP_ArcIndices"));
    
    if(!pd.InCacheQ(tag))
    {
        const Int a_count = pd.MaxArcCount();
        
        Tensor1<Int,Int> A_idx ( a_count );
        Permutation<Int> perm;
        
        Int a_idx = 0;
        
        if( settings.permute_randomQ )
        {
            perm = Permutation<Int>::RandomPermutation(
               a_count, Int(1), random_engine
            );
            
            cptr<Int> p = perm.GetPermutation().data();
            
            for( Int a = 0; a < a_count; ++a )
            {
                if( pd.ArcActiveQ(a) )
                {
                    A_idx(a) = p[a_idx];
                    ++a_idx;
                }
                else
                {
                    A_idx(a) = PD_T::Uninitialized;
                }
            }
        }
        else
        {
            for( Int a = 0; a < a_count; ++a )
            {
                if( pd.ArcActiveQ(a) )
                {
                    A_idx(a) = a_idx;
                    ++a_idx;
                }
                else
                {
                    A_idx(a) = PD_T::Uninitialized;
                }
            }
        }
        
        pd.SetCache(tag,std::move(A_idx));
    }
    
    return pd.template GetCache<Tensor1<Int,Int>>(tag);
}
