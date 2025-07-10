
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


Tensor1<COIN_Real,COIN_Int> Lengthsx_ObjectiveVector()
{
    auto & G = Dv();
    auto & E      = G.Edges();
    auto & E_cost = DvEdgeCosts();
    
    Tensor1<COIN_Real,COIN_Int> c ( static_cast<COIN_Int>(G.VertexCount()), COIN_Real(0) );
    
    mptr<COIN_Real> c_ptr = c.data();
    
    // objective =
    //  \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])
    
    for( Int e = 0; e < E.Dim(0); ++e )
    {
        const Int  v_0    = E(e,Tail);
        const Int  v_1    = E(e,Head);
        const COIN_Real w = E_cost[e];
        
        c_ptr[ v_0 ] += -w;
        c_ptr[ v_1 ] +=  w;
    }

    return c;
}

Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Lengthsx_ConstraintMatrix()
{
    TOOLS_PTIMER(timer,ClassName()+"::Lengthsx_ConstraintMatrix");
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    using S = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    auto & G = Dv();
    
    TripleAggregator<I,I,S,J> agg ( J(3) * static_cast<J>(G.EdgeCount()) );
    
    const auto & E = G.Edges();
    
    for( I e = 0; e < int_cast<I>(E.Dim(0)); ++e )
    {
        const I v_0 = static_cast<I>(E(e,Tail));
        const I v_1 = static_cast<I>(E(e,Head));
        
        // lv[e] = y[v_1] - y[v_0] >= 1
        agg.Push( v_1, e, S( 1) );
        agg.Push( v_0, e, S(-1) );
    }
    
    const COIN_Int n = static_cast<COIN_Int>(G.VertexCount());
    const COIN_Int m = static_cast<COIN_Int>(G.EdgeCount());
    
    Sparse::MatrixCSR<S,I,J> A ( agg, n, m, true, false );
    
    return A;
}

Tensor1<COIN_Real,COIN_Int> Lengthsx_LowerBoundsOnVariables()
{
    auto & G = Dv();
    const COIN_Int n = static_cast<COIN_Int>(G.VertexCount());
    
    return Tensor1<COIN_Real,COIN_Int>(n,COIN_Real(0));
}

Tensor1<COIN_Real,COIN_Int> Lengthsx_UpperBoundsOnVariables(
    bool minimize_widthQ = false
)
{
    TOOLS_MAKE_FP_STRICT();
    
    auto & G = Dv();
    const COIN_Int n = static_cast<COIN_Int>(G.VertexCount());
    
    COIN_Real w  = minimize_widthQ
                 ? G.TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;

    return Tensor1<COIN_Real,COIN_Int>(n,w);
}

Tensor1<COIN_Real,COIN_Int> Lengthsx_LowerBoundsOnConstraints()
{
    auto & G = Dv();
    const COIN_Int m = static_cast<COIN_Int>(G.EdgeCount());
    
    // Each edge must have length >= 1.
    return Tensor1<COIN_Real,COIN_Int>(m,COIN_Real(1));
}

Tensor1<COIN_Real,COIN_Int> Lengthsx_UpperBoundsOnConstraints()
{
    TOOLS_MAKE_FP_STRICT();
    
    auto & G = Dv();
    const COIN_Int m = static_cast<COIN_Int>(G.EdgeCount());
    
    // No upper bound for edge length.
    
    return Tensor1<COIN_Real,COIN_Int>(m,Scalar::Infty<COIN_Real>);
}

Tensor1<Int,Int> Compute_x_ByLengths_Variant3( bool minimize_widthQ = false )
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_PTIMER( timer, ClassName()+"::Compute_x_ByLengths_Variant3");
    
    ClpWrapper<double,Int> clp(
        Lengthsx_ObjectiveVector(),
        Lengthsx_LowerBoundsOnVariables(),
        Lengthsx_UpperBoundsOnVariables(minimize_widthQ),
        Lengthsx_ConstraintMatrix(),
        Lengthsx_LowerBoundsOnConstraints(),
        Lengthsx_UpperBoundsOnConstraints(),
        { .dualQ = settings.use_dual_simplexQ }
    );
    
    return clp.IntegralPrimalSolution();
}









// y  : V(Dh) -> \R
// lv : E(Dh) -> \R (lengths of vertical edges)
//
// objective =
//    \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])
//
// matrix constraints:
//
//  for e \in E(Dh): x[Dh.Edges()(e,1)] - x[Dh.Edges()(e,0)] >= 1

//
// box constraints:
//
// for v \in E(Dh): y[v] >= 0
//
// optional box constraints:
//
// for v \in E(Dh): y[v] <= height

Tensor1<COIN_Real,COIN_Int> Lengthsy_ObjectiveVector()
{
    auto & G      = Dh();
    auto & E      = G.Edges();
    auto & E_cost = DhEdgeCosts();
    
    Tensor1<COIN_Real,COIN_Int> c ( static_cast<COIN_Int>(G.VertexCount()), COIN_Real(0) );
    
    mptr<COIN_Real> c_ptr = c.data();
    
    // objective =
    //  \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])

    for( Int e = 0; e < E.Dim(0); ++e )
    {
        const Int  v_0    = E(e,Tail);
        const Int  v_1    = E(e,Head);
        const COIN_Real w = E_cost[e];
        
        c_ptr[ v_0 ] += -w;
        c_ptr[ v_1 ] +=  w;
    }

    return c;
}

Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Lengthsy_ConstraintMatrix()
{
    TOOLS_PTIMER(timer,MethodName("Lengthsy_ConstraintMatrix"));
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    using S = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    auto & G = Dh();
    
    TripleAggregator<I,I,S,J> agg ( J(3) * static_cast<J>(G.EdgeCount()) );
    
    {
        const auto & E = G.Edges();
        
        for( I e = 0; e < int_cast<I>(E.Dim(0)); ++e )
        {
            const I v_0 = static_cast<I>(E(e,Tail));
            const I v_1 = static_cast<I>(E(e,Head));
            
            // lv[e] = y[v_1] - y[v_0] >= 1
            agg.Push( v_1, e, S( 1) );
            agg.Push( v_0, e, S(-1) );
        }
    }
    
    const COIN_Int n = static_cast<COIN_Int>(G.VertexCount());
    const COIN_Int m = static_cast<COIN_Int>(G.EdgeCount());
    
    // DEBUGGING
    print("Lengthsy_ConstraintMatrix");
    
    Sparse::MatrixCSR<S,I,J> A (agg,n,m,true,false);
    
    
    // DEBUGGING
    TOOLS_DUMP(A.ToTensor2().Max());
//    TOOLS_DUMP(A.ToTensor2());
    
    return A;
}

Tensor1<COIN_Real,COIN_Int> Lengthsy_LowerBoundsOnVariables()
{
    auto & G = Dh();
    const COIN_Int n = static_cast<COIN_Int>(G.VertexCount());
    
    return Tensor1<COIN_Real,COIN_Int>(n,COIN_Real(0));
}

Tensor1<COIN_Real,COIN_Int> Lengthsy_UpperBoundsOnVariables(
    bool minimize_areaQ = false
)
{
    TOOLS_MAKE_FP_STRICT();
    
    auto & G = Dh();
    const COIN_Int n = static_cast<COIN_Int>(G.VertexCount());

    COIN_Real h  = minimize_areaQ
                 ? G.TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;

    return Tensor1<COIN_Real,COIN_Int>(n,h);
}

Tensor1<COIN_Real,COIN_Int> Lengthsy_LowerBoundsOnConstraints()
{
    auto & G = Dh();
    const COIN_Int m = static_cast<COIN_Int>(G.EdgeCount());
    
    // Each edge must have length >= 1.
    return Tensor1<COIN_Real,COIN_Int>(m,COIN_Real(1));
}

Tensor1<COIN_Real,COIN_Int> Lengthsy_UpperBoundsOnConstraints()
{
    TOOLS_MAKE_FP_STRICT();
    
    // No upper bound for edge length.
    
    auto & G = Dh();
    const COIN_Int m = static_cast<COIN_Int>(G.EdgeCount());
    
    return Tensor1<COIN_Real,COIN_Int>(m,Scalar::Infty<COIN_Real>);
}

Tensor1<Int,Int> Compute_y_ByLengths_Variant3( bool minimize_heightQ = false )
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_PTIMER( timer, ClassName()+"::Compute_y_ByLengths_Variant3");
    
    ClpWrapper<double,Int> clp(
        Lengthsy_ObjectiveVector(),
        Lengthsy_LowerBoundsOnVariables(),
        Lengthsy_UpperBoundsOnVariables(minimize_heightQ),
        Lengthsy_ConstraintMatrix(),
        Lengthsy_LowerBoundsOnConstraints(),
        Lengthsy_UpperBoundsOnConstraints(),
        { .dualQ = settings.use_dual_simplexQ }
    );
    
    return clp.IntegralPrimalSolution();
}



void ComputeVertexCoordinates_ByLengths_Variant3( bool minimize_areaQ = false )
{
    TOOLS_PTIMER( timer, ClassName()+"::ComputeVertexCoordinates_ByLengths_Variant3");
    
    Tensor1<Int,Int> x;
    Tensor1<Int,Int> y;
    
    ParallelDo(
        [&x,&y,minimize_areaQ,this](const Int thread)
        {
            if( thread == Int(0) )
            {
                x = Compute_x_ByLengths_Variant3(minimize_areaQ);
            }
            else if( thread == Int(1) )
            {
                y = Compute_y_ByLengths_Variant3(minimize_areaQ);
            }
        },
        Int(2),
        (settings.parallelizeQ ? Int(2) : (Int(1)))
    );
    
    ComputeVertexCoordinates(x,y);
}
