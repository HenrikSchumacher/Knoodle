public:

Tensor1<Real,I_T> Optimize_CLP( mref<PlanarDiagram_T> pd )
{
#ifndef REAPR_USE_CLP
    eprint(ClassName()+"::Optimize_CLP: Use of CLP is deactivated. Define the macro `REAPR_USE_CLP` and recompile.");
#endif
    
    if( std::cmp_greater( pd.ArcCount(), std::numeric_limits<int>::max() ) )
    {
        eprint(ClassName()+"::Optimize_CLP: Too many arcs to fit into type `int`.");
        
        return Tensor1<Real,I_T>();
    }
    
    if( std::cmp_greater( 7 * m + 1, std::numeric_limits<CoinBigIndex>::max() ) )
    {
        eprint(ClassName()+"::Optimize_CLP: System matrix has more nonzeroes as can be countet by type `CoinBigIndex` ( a.k.a. " + ToString(CoinBigIndex) + "  ).");
        
        return Tensor1<Real,I_T>();
    }
       
    const int m = static_cast<int>(pd.ArcCount());
    const int n = static_cast<int>(pd.CrossingCount());

    
    TripleAggregator<int,int,double,CoinBigIndex> agg;

//    mptr<CoinBigIndex> outer  = A.Outer().data();
//    mptr<int>          inner  = A.Inner().data();
//    mptr<double>       values = A.Value().data();
    
    for( int c = 0; c < n; ++c )
    {
        
//      a_0     a_1
//         \   /
//          \ /
//           X
//          / \
//         /   \
//      b_1     b_0
        
        
        const int  a_0 = C_arcs[(c << 2)    ];
        const int  a_1 = C_arcs[(c << 2) | 1];
        const int  b_1 = C_arcs[(c << 2) | 2];
        const int  b_0 = C_arcs[(c << 2) | 3];

        const double s = static_cast<double>(ToUnderlying(C_states[c]));
        
        // Over/under constraints
        agg.Push( a_0, c, -s );
        agg.Push( a_1, c,  s );
        
        const int col_0 = n     + 2 * c;
        const int col_1 = n + m + 2 * c;


        // difference operator
        agg.Push( b_0, col_0    ,  -Scalar::One<double> );
        agg.Push( a_0, col_0    ,   Scalar::One<double> );
        agg.Push( b_1, col_0 + 1,  -Scalar::One<double> );
        agg.Push( a_1, col_0 + 1,   Scalar::One<double> );
        
//        agg.Push( a_0, a_0 + ,  -Scalar::One<double> );
        
        // negative difference operator
        agg.Push( b_0, col_1    ,   Scalar::One<double> );
        agg.Push( a_0, col_1    ,  -Scalar::One<double> );
        agg.Push( b_1, col_1 + 1,   Scalar::One<double> );
        agg.Push( a_1, col_1 + 1,  -Scalar::One<double> );
    }
    
    agg.Push( 0, n + 2 * m,  Scalar::One<double> );
    
    Sparse::MatrixCSR<double,int,CoinBigIndex> A (
        agg, 2 * m, n + 2 * m + 1, 1, true, false
    );
    
    
    A.ToTensor2().WriteToFile("/Users/Henrik/a.txt");
    

    Tensor1<Real,I_T> x     (m+n,Real(0));

    return x;
}
