
// x  : V(Dv) -> \R
// lh : E(Dv) -> \R (lengths of horizontal edges)

// objective =
//    \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])
//
// matrix constraints:
//
//  for e \in E(Dv): x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)] >= 1
//
// box constraints:
//
// for v \in E(Dv): x[v] >= 0
//
// optional box constraints:
//
// for v \in E(Dv): x[v] <= width

public:

void ComputeVertexCoordinates_Compaction_CLP( bool minimize_areaQ = false )
{
    TOOLS_PTIMER(timer,MethodName("ComputeVertexCoordinates_Compaction_CLP"));
    
    Tensor1<Int,Int> x;
    Tensor1<Int,Int> y;
    
    ParallelDo(
        [&x,&y,minimize_areaQ,this](const Int thread)
        {
            if( thread == Int(0) )
            {
                x = Compaction_CLP( Dv(), DvEdgeCosts(), minimize_areaQ );
            }
            else if( thread == Int(1) )
            {
                y = Compaction_CLP( Dh(), DhEdgeCosts(), minimize_areaQ );
            }
        },
        Int(2),
        (settings.parallelizeQ ? Int(2) : (Int(1)))
    );
    
    ComputeVertexCoordinates(x,y);
}


private:

Tensor1<Int,Int> Compaction_CLP(
    cref<DiGraph_T> D,
    cref<Tensor1<Cost_T,Int>> E_cost,
    bool minimize_widthQ = false
)
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_PTIMER(timer,MethodName("Compaction_CLP"));
    
    ClpWrapper<double,Int> clp(
        Compaction_CLP_ObjectiveVector( D, E_cost ),
        Compaction_CLP_LowerBoundsOnVariables( D ),
        Compaction_CLP_UpperBoundsOnVariables( D, minimize_widthQ),
        Compaction_CLP_ConstraintMatrix( D ),
        Compaction_CLP_LowerBoundsOnConstraints( D ),
        Compaction_CLP_UpperBoundsOnConstraints( D ),
        { .dualQ = settings.use_dual_simplexQ }
    );
    
    return clp.IntegralPrimalSolution();
}


Tensor1<COIN_Real,COIN_Int> Compaction_CLP_ObjectiveVector(
    cref<DiGraph_T> D, cref<Tensor1<Cost_T,Int>> E_cost
)
{
    auto & E = D.Edges();
    
    Tensor1<COIN_Real,COIN_Int> c ( static_cast<COIN_Int>(D.VertexCount()), COIN_Real(0) );
    
    mptr<COIN_Real> c_ptr = c.data();
    
    // objective =
    //  \sum_{e \in E(D)} DvE_cost[e] * (x[E(e,1)] - x[E(e,0)])
    
    const Int e_count = E.Dim(0);
    
    for( Int e = 0; e < e_count; ++e )
    {
        const Int  v_0    = E(e,Tail);
        const Int  v_1    = E(e,Head);
        const COIN_Real w = E_cost[e];
        
        c_ptr[ v_0 ] += -w;
        c_ptr[ v_1 ] +=  w;
    }

    return c;
}

Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Compaction_CLP_ConstraintMatrix( cref<DiGraph_T> D )
{
//    TOOLS_PTIMER(timer,ClassName()+"::Compaction_CLP_ConstraintMatrix");
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    using S = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    auto & G = Dv();
    
    TripleAggregator<I,I,S,J> agg ( J(3) * static_cast<J>(G.EdgeCount()) );
    
    const auto & E = D.Edges();
    const I e_count = int_cast<I>(E.Dim(0));
    
    for( I e = 0; e < e_count; ++e )
    {
        const I v_0 = static_cast<I>(E(e,Tail));
        const I v_1 = static_cast<I>(E(e,Head));
        
        // lv[e] = y[v_1] - y[v_0] >= 1
        agg.Push( v_1, e, S( 1) );
        agg.Push( v_0, e, S(-1) );
    }
    
    const COIN_Int n = static_cast<COIN_Int>(D.VertexCount());
    const COIN_Int m = static_cast<COIN_Int>(D.EdgeCount());
    
    Sparse::MatrixCSR<S,I,J> A ( agg, n, m, true, false );
    
    return A;
}

Tensor1<COIN_Real,COIN_Int> Compaction_CLP_LowerBoundsOnVariables( cref<DiGraph_T> D )
{
    const COIN_Int n = static_cast<COIN_Int>(D.VertexCount());
    
    return Tensor1<COIN_Real,COIN_Int>(n,COIN_Real(0));
}

Tensor1<COIN_Real,COIN_Int> Compaction_CLP_UpperBoundsOnVariables(
    cref<DiGraph_T> D, bool minimize_widthQ = false
)
{
    TOOLS_MAKE_FP_STRICT();

    const COIN_Int n = static_cast<COIN_Int>(D.VertexCount());
    
    COIN_Real w  = minimize_widthQ
                 ? D.TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;

    return Tensor1<COIN_Real,COIN_Int>(n,w);
}

Tensor1<COIN_Real,COIN_Int> Compaction_CLP_LowerBoundsOnConstraints( cref<DiGraph_T> D )
{
    const COIN_Int m = static_cast<COIN_Int>(D.EdgeCount());
    
    // Each edge must have length >= 1.
    return Tensor1<COIN_Real,COIN_Int>(m,COIN_Real(1));
}

Tensor1<COIN_Real,COIN_Int> Compaction_CLP_UpperBoundsOnConstraints( cref<DiGraph_T> D )
{
    TOOLS_MAKE_FP_STRICT();
    
    const COIN_Int m = static_cast<COIN_Int>(D.EdgeCount());
    
    // No upper bound for edge length.
    
    return Tensor1<COIN_Real,COIN_Int>(m,Scalar::Infty<COIN_Real>);
}
