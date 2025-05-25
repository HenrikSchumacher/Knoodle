public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<Real,I,J> LevelsMatrixLP( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_PTIC(ClassName() + "::LevelsMatrixLP");
    
//    if( !IntFitsIntoTypeQ<I>(??) )
//    {
//        eprint( tag + ": integer " + ToString(??) + " does not fit into type " + TypeName<I> + ".");
//    }
    
    const I m = static_cast<I>(pd.ArcCount());
    const I n = static_cast<I>(pd.CrossingCount());

//    TOOLS_DUMP(pd.Crossings());
    
    cptr<Int>           C_arcs   = pd.Crossings().data();
    cptr<CrossingState> C_states = pd.CrossingStates().data();
    
    // TODO: allocate upfront.
    TripleAggregator<I,I,Real,J> agg ( J(7) * m + J(1) );
    
    constexpr Real one = 1;
    
    // We assemble the matrix transpose because CLP assumes column-major ordering.
    
    for( I c = 0; c < n; ++c )
    {
        
//      a_0     a_1
//        ^     ^
//         \   /
//          \ /
//           X
//          / \
//         /   \
//      b_1     b_0
        
        const I a_0 = static_cast<I>(C_arcs[I(4) * c + I(0)]);
        const I a_1 = static_cast<I>(C_arcs[I(4) * c + I(1)]);
        const I b_1 = static_cast<I>(C_arcs[I(4) * c + I(2)]);
        const I b_0 = static_cast<I>(C_arcs[I(4) * c + I(3)]);

        const Real s = static_cast<Real>(ToUnderlying(C_states[c]));
        
        // Over/under constraints
        agg.Push( a_0, c, -s );
        agg.Push( a_1, c,  s );
        
//        const I col_0 = n     + I(2) * c;
//        const I col_1 = n + m + I(2) * c;

        // difference operator
        agg.Push( b_0    , b_0 + n, -one );
        agg.Push( a_0    , b_0 + n,  one );
        agg.Push( b_1    , b_1 + n, -one );
        agg.Push( a_1    , b_1 + n,  one );
        
        agg.Push( b_0 + m, b_0 + n, -one );
        agg.Push( b_1 + m, b_1 + n, -one );
        
        I k = n + m;
        
        // negative difference operator
        agg.Push( b_0    , b_0 + k,  one );
        agg.Push( a_0    , b_0 + k, -one );
        agg.Push( b_1    , b_1 + k,  one );
        agg.Push( a_1    , b_1 + k, -one );
        
        agg.Push( b_0 + m, b_0 + k, -one );
        agg.Push( b_1 + m, b_1 + k, -one );
    }
    
    // Base point constraint
    agg.Push( I(0), n + I(2) * m,  one );
    
    Sparse::MatrixCSR<Real,I,J> A (
        agg, I(2) * m, n + I(2) * m + I(1), I(1), true, false
    );

    TOOLS_PTOC(ClassName() + "::LevelsMatrixLP");
    
    return A;
}



template<typename Int>
Tensor1<Real,Int> LevelsColLowerBounds( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<Real,Int>( Int(2) * pd.ArcCount(), -Scalar::Infty<Real> );
}

template<typename Int>
Tensor1<Real,Int> LevelsColUpperBounds( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<Real,Int>( Int(2) * pd.ArcCount(), +Scalar::Infty<Real> );
}

template<typename Int>
Tensor1<Real,Int> LevelsRowLowerBounds( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    const Int m = pd.ArcCount();
    const Int n = pd.CrossingCount();
    Tensor1<Real,Int> v ( n + Int(2) * m + Int(1), -Scalar::Infty<Real> );
    
    v.Last() = Real(0);
    
    return v;
}

template<typename Int>
Tensor1<Real,Int> LevelsRowUpperBounds( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    const Int m = pd.ArcCount();
    const Int n = pd.CrossingCount();
    Tensor1<Real,Int> v ( n + Int(2) * m + Int(1) );
    
    fill_buffer( &v[0], Real(-1), n                   );
    fill_buffer( &v[n], Real( 0), Int(2) * m + Int(1) );
    
    return v;
}


template<typename Int>
Tensor1<Real,Int> LevelsObjectiveVector( mref<PlanarDiagram<Int>> pd )
{
    const Int m = pd.ArcCount();
    Tensor1<Real,Int> v ( Int(2) * m );
    
    fill_buffer( &v[0], Real(0), m );
    fill_buffer( &v[m], Real(1), m );
    
    return v;
}
