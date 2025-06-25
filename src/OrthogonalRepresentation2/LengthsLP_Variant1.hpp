
// x  : V(Dv) -> \R
// lh : E(Dv) -> \R (lengths of horizontal edges)
// y  : V(Dh) -> \R
// lv : E(Dh) -> \R (lengths of vertical edges)



// objective =  \sum_{e \in E(Dv)} DvE_cost[e] * lh[e]
//              +
//              \sum_{e \in E(Dh)} DhE_cost[e] * lv[e]
//
// matrix constraints:
//
//  for e \in E(Dv): x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)] == lh[e]
//  for e \in E(Dh): x[Dh.Edges()(e,1)] - x[Dh.Edges()(e,0)] == lv[e]

//
// box constraints:
//
// for v \in E(Dv): x[v]  >= 0
// for v \in E(Dh): y[v]  >= 0
// for e \in E(Dv): lh[e] >= 1
// for e \in E(Dh): lv[e] >= 1
//
// optional box constraints:
//
// for v \in E(Dv): x[v] <= width
// for v \in E(Dh): y[v] <= height

private:

struct Variant1_Dimensions_T
{
    COIN_Int DvV_count;
    COIN_Int DvE_count;
    COIN_Int DhV_count;
    COIN_Int DhE_count;
    
    COIN_Int DvV_offset;
    COIN_Int DvE_offset;
    COIN_Int DhV_offset;
    COIN_Int DhE_offset;
    
    COIN_Int var_count;
    COIN_Int con_count;
};

public:

cref<Variant1_Dimensions_T> Variant1_Dimensions() const
{
    if( !this->InCacheQ("Variant1_Dimensions") )
    {
        Variant1_Dimensions_T d;
        
        const auto & Dh_ = Dh();
        const auto & Dv_ = Dv();
        
        d.DhV_count = static_cast<COIN_Int>(Dh_.VertexCount());
        d.DhE_count = static_cast<COIN_Int>(Dh_.EdgeCount());
        d.DvV_count = static_cast<COIN_Int>(Dv_.VertexCount());
        d.DvE_count = static_cast<COIN_Int>(Dv_.EdgeCount());
        
        d.var_count = d.DhV_count + d.DhE_count + d.DvV_count + d.DvE_count;
        
        d.con_count = d.DhE_count + d.DvE_count;
        
        // Storing  [x,lh,y,lv].
        d.DvV_offset = 0;
        d.DvE_offset = d.DvV_offset + d.DvV_count;
        d.DhV_offset = d.DvE_offset + d.DvE_count;
        d.DhE_offset = d.DhV_offset + d.DhV_count;
        
        this->SetCache("Variant1_Dimensions",std::move(d));
    }
    
    return this->GetCache<Variant1_Dimensions_T>("Variant1_Dimensions");
}


Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Lengths_ConstraintMatrix_Variant1()
{
    TOOLS_PTIMER(timer,ClassName()+"::Lengths_ConstraintMatrix_Variant1"
//        + "<" + TypeName<S>
//        + "," + TypeName<I>
//        + "," + TypeName<J>
//        + ">"
    );
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    using S = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();
    
    TripleAggregator<I,I,S,J> agg ( J(3) * static_cast<J>(d.con_count) );
    
    {
        const auto & Dv_ = Dv();
        const auto & DvE = Dv_.Edges();
        
        for( Int e = 0; e < d.DvE_count; ++e )
        {
            const I v_0 = static_cast<I>(DvE(e,Tail));
            const I v_1 = static_cast<I>(DvE(e,Head));
            
            // lh[e] == x[v_1] - x[v_0]
            
            // x[v_1] - x[v_0] - lh[e] == 0
            agg.Push( d.DvV_offset + v_1, e, S( 1) );
            agg.Push( d.DvV_offset + v_0, e, S(-1) );
            agg.Push( d.DvE_offset + e  , e, S(-1) );
        }
    }
    
    {
        const auto & Dh_ = Dh();
        const auto & DhE = Dh_.Edges();
        
        for( Int e = 0; e < d.DhE_count; ++e )
        {
            const I v_0 = static_cast<I>(DhE(e,Tail));
            const I v_1 = static_cast<I>(DhE(e,Head));
            
            // lv[e] == y[v_1] - y[v_0]
            
            // y[v_1] - y[v_0] - lv[e] == 0
            agg.Push( d.DhV_offset + v_1, e + d.DvE_count, S( 1) );
            agg.Push( d.DhV_offset + v_0, e + d.DvE_count, S(-1) );
            agg.Push( d.DhE_offset + e  , e + d.DvE_count, S(-1) );
        }
    }
    
    Sparse::MatrixCSR<S,I,J> A (agg,d.var_count,d.con_count,true,false);
    
    return A;
}

Tensor1<COIN_Real,COIN_Int> Lengths_LowerBoundsOnVariables_Variant1()
{
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();

    Tensor1<COIN_Real,COIN_Int> lb ( d.var_count );
    
    for( COIN_Int v = 0; v < d.DvV_count; ++v )
    {
        lb[ v + d.DvV_offset ] = COIN_Real(0); // x[v] >= 0
    }
    for( COIN_Int e = 0; e < d.DvE_count; ++e )
    {
        lb[ e + d.DvE_offset ] = COIN_Real(1); // lh[e] >= 1
    }
    for( COIN_Int v = 0; v < d.DhV_count; ++v )
    {
        lb[ v + d.DhV_offset ] = COIN_Real(0); // y[v] >= 0
    }
    for( COIN_Int e = 0; e < d.DhE_count; ++e )
    {
        lb[ e + d.DhE_offset ] = COIN_Real(1); // lv[e] >= 1
    }
    return lb;
}

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnVariables_Variant1()
{
    TOOLS_MAKE_FP_STRICT();
    
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();
    
    // TODO: Add height and width constraints.
    
    return Tensor1<COIN_Real,COIN_Int>( d.var_count, Scalar::Infty<COIN_Real> );
}


Tensor1<COIN_Real,COIN_Int> Lengths_LowerBoundsOnConstraints_Variant1()
{
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();
    
    // We have only equality constraints.
    return Tensor1<COIN_Real,COIN_Int>( d.con_count, COIN_Real(0) );
}

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnConstraints_Variant1()
{
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();
    
    // We have only equality constraints.
    return Tensor1<COIN_Real,COIN_Int>( d.con_count, COIN_Real(0) );
}

Tensor1<COIN_Real,COIN_Int> Lengths_ObjectiveVector_Variant1()
{
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();
    
    Tensor1<COIN_Real,COIN_Int> c ( d.var_count );
    
    for( COIN_Int v = 0; v < d.DvV_count; ++v )
    {
        c[ v + d.DvV_offset ] = COIN_Real(0); // x[v] >= 0
    }
    
    DvEdgeCosts().Write( c.data(d.DvE_offset) );
    
    for( COIN_Int v = 0; v < d.DhV_count; ++v )
    {
        c[ v + d.DhV_offset ] = COIN_Real(0); // y[v] >= 0
    }
    
    DhEdgeCosts().Write( c.data(d.DhE_offset) );

    return c;
}

void ComputeVertexCoordinates_ByLengths_Variant1()
{
    TOOLS_MAKE_FP_STRICT();

    TOOLS_PTIMER( timer, ClassName()+"::ComputeVertexCoordinates_ByLengths_Variant1");
    
    ClpSimplex LP;
    LP.setMaximumIterations(1000000);
    LP.setOptimizationDirection(1); // +1 -> minimize; -1 -> maximize
    LP.setLogLevel(0);
    
    auto AT      = Lengths_ConstraintMatrix_Variant1();
    auto var_lb = Lengths_LowerBoundsOnVariables_Variant1();
    auto var_ub = Lengths_UpperBoundsOnVariables_Variant1();
    auto con_lb = Lengths_LowerBoundsOnConstraints_Variant1();
    auto con_ub = Lengths_UpperBoundsOnConstraints_Variant1();
    auto c      = Lengths_ObjectiveVector_Variant1();
    
    if( settings.pm_matrixQ )
    {
        ClpPlusMinusOneMatrix A (
            MatrixCSR_transpose_to_CoinPackedMatrix(AT)
        );
        
        LP.loadProblem(
            A,
            var_lb.data(), var_ub.data(),
            c.data(),
            con_lb.data(), con_ub.data()
        );
    }
    else
    {
        LP.loadProblem(
            AT.RowCount(), AT.ColCount(),
            AT.Outer().data(), AT.Inner().data(), AT.Values().data(),
            var_lb.data(), var_ub.data(),
            c.data(),
            con_lb.data(), con_ub.data()
        );
    }
    
//    LP.loadProblem(
//        A.RowCount(), A.ColCount(),
//        A.Outer().data(), A.Inner().data(), A.Values().data(),
//        var_lb.data(), var_ub.data(),
//        c.data(),
//        con_lb.data(), con_ub.data()
//    );
    
    if( settings.use_dual_simplexQ )
    {
        LP.dual();
    }
    else
    {
        LP.primal();
    }
    
    LP.checkSolution(1);
    
    bool successQ = LP.statusOfProblem();
    
    cref<Variant1_Dimensions_T> d = Variant1_Dimensions();
    
    if( !successQ )
    {
        eprint(ClassName()+"::ComputeVertexCoordinates_ByLengths_Variant1: Clp::Simplex::" + (settings.use_dual_simplexQ ? "dual" : "primal" )+ " reports a problem in the solve phase. The returned solution may be incorrect.");
        
        TOOLS_DUMP(LP.statusOfProblem());
        TOOLS_DUMP(LP.getIterationCount());
        TOOLS_DUMP(LP.numberPrimalInfeasibilities());
        TOOLS_DUMP(LP.numberDualInfeasibilities());
        TOOLS_DUMP(LP.largestPrimalError());
        TOOLS_DUMP(LP.largestDualError());
        
        TOOLS_DUMP(LP.objectiveValue());
    }
    
    
    
    Tensor1<Int,Int> x ( static_cast<Int>(d.DvV_count) );
    Tensor1<Int,Int> y ( static_cast<Int>(d.DhV_count) );
    
    cptr<COIN_Real> sol = LP.primalColumnSolution();
    
    
    // Checking whether solution is an integer solution.
    
    constexpr COIN_Real integer_tol = 0.000000001;
    COIN_Real diff_max = 0;
    for( Int i = 0; i < d.var_count; ++i )
    {
        diff_max = Max( diff_max, Abs(sol[i] - std::round(sol[i])));
    }
    
    if( diff_max > integer_tol )
    {
        eprint(ClassName() + "::ComputeVertexCoordinates_ByLengths_Variant1: CLP returned noninteger solution vector. Greatest deviation = " + ToStringFPGeneral(diff_max) +".");
    }
    
    // TODO: Check feasibility?
    for( Int i = 0; i < d.DvV_count; ++i )
    {
        x[i] = static_cast<Int>(std::round(sol[i+d.DvV_offset]));
    }
    for( Int i = 0; i < d.DhV_count; ++i )
    {
        y[i] = static_cast<Int>(std::round(sol[i+d.DhV_offset]));
    }

    Tensor1<COIN_Real,Int> s ( sol, static_cast<Int>(d.var_count) );
    
    ComputeVertexCoordinates(x,y);
}

