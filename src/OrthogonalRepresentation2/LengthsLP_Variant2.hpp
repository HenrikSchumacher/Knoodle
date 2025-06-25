
// x  : V(Dv) -> \R
// lh : E(Dv) -> \R (lengths of horizontal edges)
// y  : V(Dh) -> \R
// lv : E(Dh) -> \R (lengths of vertical edges)



// objective =
//    \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])
//    +
//    \sum_{e \in E(Dh)} DhE_cost[e] * (x[Dh.Edges()(e,1)] - x[Dh.Edges()(e,0)])
//
// matrix constraints:
//
//  for e \in E(Dv): x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)] >= 1
//  for e \in E(Dh): x[Dh.Edges()(e,1)] - x[Dh.Edges()(e,0)] >= 1

//
// box constraints:
//
// for v \in E(Dv): x[v] >= 0
// for v \in E(Dh): y[v] >= 0
//
// optional box constraints:
//
// for v \in E(Dv): x[v] <= width
// for v \in E(Dh): y[v] <= height


private:

struct Variant2_Dimensions_T
{
    COIN_Int DvV_count;
    COIN_Int DvE_count;
    COIN_Int DhV_count;
    COIN_Int DhE_count;
    
    COIN_Int DvV_offset;
    COIN_Int DhV_offset;
    
    COIN_Int DvE_offset;
    COIN_Int DhE_offset;
    
    COIN_Int var_count;
    COIN_Int con_count;
};

public:

cref<Variant2_Dimensions_T> Variant2_Dimensions() const
{
    if( !this->InCacheQ("Variant2_Dimensions") )
    {
        Variant2_Dimensions_T d;
        
        {
            d.DvV_count = static_cast<COIN_Int>(Dv().VertexCount());
            d.DvE_count = static_cast<COIN_Int>(Dv().EdgeCount());
        }
        {
            d.DhV_count = static_cast<COIN_Int>(Dh().VertexCount());
            d.DhE_count = static_cast<COIN_Int>(Dh().EdgeCount());
        }

        d.var_count = d.DvV_count + d.DhV_count;
        
        d.con_count = d.DvE_count + d.DhE_count;
        
        // Variables in this order: [x,y].
        d.DvV_offset = 0;
        d.DhV_offset = d.DvV_offset + d.DvV_count;
        
        // Constraints in this order: [lh,lv]
        d.DvE_offset = 0;
        d.DhE_offset = d.DvE_offset + d.DvE_count;
        
        this->SetCache("Variant2_Dimensions",std::move(d));
    }
    
    return this->GetCache<Variant2_Dimensions_T>("Variant2_Dimensions");
}


Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Lengths_ConstraintMatrix_Variant2()
{
    TOOLS_PTIMER(timer,ClassName()+"::Lengths_ConstraintMatrix_Variant2"
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
    
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    TripleAggregator<I,I,S,J> agg ( J(3) * static_cast<J>(d.con_count) );
    
    {
        const auto & E   = Dv().Edges();
        const I v_offset = d.DvV_offset;
        const I e_offset = d.DvE_offset;
        
        for( Int e = 0; e < E.Dim(0); ++e )
        {
            const I v_0 = static_cast<I>(E(e,Tail));
            const I v_1 = static_cast<I>(E(e,Head));
            
            // lv[e] = y[v_1] - y[v_0] >= 1
            agg.Push( v_1 + v_offset, e + e_offset, S( 1) );
            agg.Push( v_0 + v_offset, e + e_offset, S(-1) );
        }
    }
    
    {
        const auto & E   = Dh().Edges();
        const I v_offset = d.DhV_offset;
        const I e_offset = d.DhE_offset;
        
        for( Int e = 0; e < E.Dim(0); ++e )
        {
            const I v_0 = static_cast<I>(E(e,Tail));
            const I v_1 = static_cast<I>(E(e,Head));
            
            // lv[e] = y[v_1] - y[v_0] >= 1
            agg.Push( v_1 + v_offset, e + e_offset, S( 1) );
            agg.Push( v_0 + v_offset, e + e_offset, S(-1) );
        }
    }
    
    Sparse::MatrixCSR<S,I,J> A (agg,d.var_count,d.con_count,true,false);
    
    return A;
}

Tensor1<COIN_Real,COIN_Int> Lengths_LowerBoundsOnVariables_Variant2()
{
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();

    return Tensor1<COIN_Real,COIN_Int>(d.var_count,COIN_Real(0));
}

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnVariables_Variant2()
{
    TOOLS_MAKE_FP_STRICT();
    
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    // TODO: Add height and width constraints.
    
    return Tensor1<COIN_Real,COIN_Int>(d.var_count,Scalar::Infty<COIN_Real>);
}

Tensor1<COIN_Real,COIN_Int> Lengths_LowerBoundsOnConstraints_Variant2()
{
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    // Each edge must have length >= 1.
    return Tensor1<COIN_Real,COIN_Int>(d.con_count,COIN_Real(1));
}

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnConstraints_Variant2()
{
    TOOLS_MAKE_FP_STRICT();
    
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    // No upper bound for edge length.
    
    return Tensor1<COIN_Real,COIN_Int>(d.con_count,Scalar::Infty<COIN_Real>);
}

Tensor1<COIN_Real,COIN_Int> Lengths_ObjectiveVector_Variant2()
{
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    Tensor1<COIN_Real,COIN_Int> c ( d.var_count );
    
    mptr<COIN_Real> c_ptr = c.data();
    
    // objective =
    //  \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])
    //  +
    //  \sum_{e \in E(Dh)} DhE_cost[e] * (y[Dh.Edges()(e,1)] - y[Dh.Edges()(e,0)])

    {
        auto & E      = Dv().Edges();
        auto & E_cost = DvEdgeCosts();
        Int    offset = d.DvV_offset;
        
        for( Int e = 0; e < E.Dim(0); ++e )
        {
            const Int  v_0    = E(e,Tail);
            const Int  v_1    = E(e,Head);
            const COIN_Real w = E_cost[e];
            
            c_ptr[ v_0 + offset ] = -w;
            c_ptr[ v_1 + offset ] =  w;
        }
    }
    
    {
        auto & E      = Dh().Edges();
        auto & E_cost = DhEdgeCosts();
        Int    offset = d.DhV_offset;
        
        for( Int e = 0; e < E.Dim(0); ++e )
        {
            const Int  v_0    = E(e,Tail);
            const Int  v_1    = E(e,Head);
            const COIN_Real w = E_cost[e];
            
            c_ptr[ v_0 + offset ] = -w;
            c_ptr[ v_1 + offset ] =  w;
        }
    }

    return c;
}

void ComputeVertexCoordinates_ByLengths_Variant2()
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_PTIMER( timer, ClassName()+"::ComputeVertexCoordinates_ByLengths_Variant2");
    
    // TODO: Try ClpPlusMinusOneMatrix.
    // TODO: Try ClpNetworkMatrix.
    ClpSimplex LP;
    LP.setMaximumIterations(1000000);
    LP.setOptimizationDirection(1); // +1 -> minimize; -1 -> maximize
    LP.setLogLevel(0);
    
    using I = COIN_Int;

    auto AT     = Lengths_ConstraintMatrix_Variant2();
    auto var_lb = Lengths_LowerBoundsOnVariables_Variant2();
    auto var_ub = Lengths_UpperBoundsOnVariables_Variant2();
    auto con_lb = Lengths_LowerBoundsOnConstraints_Variant2();
    auto con_ub = Lengths_UpperBoundsOnConstraints_Variant2();
    auto c      = Lengths_ObjectiveVector_Variant2();
    
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
    
    if( settings.use_dual_simplexQ )
    {
        LP.dual();
    }
    else
    {
        LP.primal();
    }
    
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
//
//    TOOLS_DUMP(LP.statusOfProblem());
//    TOOLS_DUMP(LP.getIterationCount());
//    TOOLS_DUMP(LP.numberPrimalInfeasibilities());
//    TOOLS_DUMP(LP.largestPrimalError());
//    TOOLS_DUMP(LP.sumPrimalInfeasibilities());
//    TOOLS_DUMP(LP.numberDualInfeasibilities());
//    TOOLS_DUMP(LP.largestDualError());
//    TOOLS_DUMP(LP.sumDualInfeasibilities());
//    
//    valprint(
//        "solution",
//        ArrayToString(
//            LP.primalColumnSolution(),{d.var_count},
//            [](COIN_Real x){ return ToStringFPGeneral(x); }
//        )
//    );
//    
//    print("LP.checkSolution(0);");
//    LP.checkSolution(0);
//
//    TOOLS_DUMP(LP.statusOfProblem());
//    TOOLS_DUMP(LP.getIterationCount());
//    TOOLS_DUMP(LP.numberPrimalInfeasibilities());
//    TOOLS_DUMP(LP.largestPrimalError());
//    TOOLS_DUMP(LP.sumPrimalInfeasibilities());
//    TOOLS_DUMP(LP.numberDualInfeasibilities());
//    TOOLS_DUMP(LP.largestDualError());
//    TOOLS_DUMP(LP.sumDualInfeasibilities());
//    
//    valprint(
//        "solution",
//        ArrayToString(
//            LP.primalColumnSolution(),{d.var_count},
//            [](COIN_Real x){ return ToStringFPGeneral(x); }
//        )
//    );
    
    
    print("LP.checkSolution(1);");
    LP.checkSolution(1);

    TOOLS_DUMP(LP.statusOfProblem());
    TOOLS_DUMP(LP.getIterationCount());
    TOOLS_DUMP(LP.numberPrimalInfeasibilities());
    TOOLS_DUMP(LP.largestPrimalError());
    TOOLS_DUMP(LP.sumPrimalInfeasibilities());
    TOOLS_DUMP(LP.numberDualInfeasibilities());
    TOOLS_DUMP(LP.largestDualError());
    TOOLS_DUMP(LP.sumDualInfeasibilities());
    
//    valprint(
//        "solution",
//        ArrayToString(
//            LP.primalColumnSolution(),{d.var_count},
//            [](COIN_Real x){ return ToStringFPGeneral(x); }
//        )
//    );
//    
//    print("LP.checkSolution(2);");
//    LP.checkSolution(2);
//
//    TOOLS_DUMP(LP.statusOfProblem());
//    TOOLS_DUMP(LP.getIterationCount());
//    TOOLS_DUMP(LP.numberPrimalInfeasibilities());
//    TOOLS_DUMP(LP.largestPrimalError());
//    TOOLS_DUMP(LP.sumPrimalInfeasibilities());
//    TOOLS_DUMP(LP.numberDualInfeasibilities());
//    TOOLS_DUMP(LP.largestDualError());
//    TOOLS_DUMP(LP.sumDualInfeasibilities());
//    
//    valprint(
//        "solution",
//        ArrayToString(
//            LP.primalColumnSolution(),{d.var_count},
//            [](COIN_Real x){ return ToStringFPGeneral(x); }
//        )
//    );
//    
//    LP.primal(1);
////    LP.checkSolution(2);
//    
//    TOOLS_DUMP(LP.statusOfProblem());
//    TOOLS_DUMP(LP.getIterationCount());
//    TOOLS_DUMP(LP.numberPrimalInfeasibilities());
//    TOOLS_DUMP(LP.largestPrimalError());
//    TOOLS_DUMP(LP.sumPrimalInfeasibilities());
//    TOOLS_DUMP(LP.numberDualInfeasibilities());
//    TOOLS_DUMP(LP.largestDualError());
//    TOOLS_DUMP(LP.sumDualInfeasibilities());
//    
//    valprint(
//        "solution",
//        ArrayToString(
//            LP.primalColumnSolution(),{d.var_count},
//            [](COIN_Real x){ return ToStringFPGeneral(x); }
//        )
//    );
//    

    
    if( !LP.statusOfProblem() )
    {
        eprint(ClassName()+"::ComputeVertexCoordinates_ByLengths_Variant2: Clp::Simplex::" + (settings.use_dual_simplexQ ? "dual" : "primal" )+ " reports a problem in the solve phase. The returned solution may be incorrect.");
        
        TOOLS_DUMP(LP.statusOfProblem());
        TOOLS_DUMP(LP.getIterationCount());
        
        TOOLS_DUMP(LP.numberPrimalInfeasibilities());
        TOOLS_DUMP(LP.largestPrimalError());
        TOOLS_DUMP(LP.sumPrimalInfeasibilities());
        
        TOOLS_DUMP(LP.numberDualInfeasibilities());
        TOOLS_DUMP(LP.largestDualError());
        TOOLS_DUMP(LP.sumDualInfeasibilities());
        
        
        TOOLS_DUMP(LP.objectiveValue());
        
        TOOLS_DUMP(d.var_count);
        TOOLS_DUMP(Dv().VertexCount()+Dh().VertexCount());
        
        valprint(
            "solution",
            ArrayToString(
                LP.primalColumnSolution(),{d.var_count},
                [](COIN_Real x){ return ToStringFPGeneral(x); }
            )
        );
    }
    
    auto v_str = []( const Tensor1<COIN_Real,I> & v )
    {
        return ArrayToString(
          v.data(),{v.Dim(0)},
          [](COIN_Real x){ return ToStringFPGeneral(x); }
        );
    };
    
    Tensor1<Int,Int> x ( static_cast<Int>(d.DvV_count) );
    Tensor1<Int,Int> y ( static_cast<Int>(d.DhV_count) );
    
    Tensor1<COIN_Real,I> s ( d.var_count );
    Tensor1<COIN_Real,I> b ( d.con_count );
    

    // Checking whether solution is an integer solution.
    
    constexpr COIN_Real integer_tol = 0.000000001;
    COIN_Real diff_max = 0;
    
    Int var_lb_infeasible_count = 0;
    Int var_ub_infeasible_count = 0;
    
    {
        cptr<COIN_Real> sol = LP.primalColumnSolution();
        
        for( I i = 0; i < d.var_count; ++i )
        {
            COIN_Real r_i = std::round(sol[i]);
            diff_max = Max( diff_max, Abs(sol[i] - r_i) );
            s[i] = r_i;
            
            var_lb_infeasible_count += (r_i < var_lb[i]);
            var_ub_infeasible_count += (r_i > var_ub[i]);
        }
    }
    
    valprint("s",v_str(s));
    {
        auto A_ = AT.Transpose();
        A_.Dot(COIN_Real(1),s.data(),COIN_Real(0),b.data());
    }
    
    valprint("b",v_str(b));
    
    if( diff_max > integer_tol )
    {
        eprint(ClassName() + "::ComputeVertexCoordinates_ByLengths_Variant2: CLP returned noninteger solution vector. Greatest deviation = " + ToStringFPGeneral(diff_max) + ".");
    }
    
    if( var_lb_infeasible_count > Int(0) )
    {
        eprint(ClassName() + "::ComputeVertexCoordinates_ByLengths_Variant2: violations for lower box constraints: " + ToString(var_lb_infeasible_count) +".");
        
        valprint("var_lb",v_str(var_lb));
    }
    
    if( var_ub_infeasible_count > Int(0) )
    {
        eprint(ClassName() + "::ComputeVertexCoordinates_ByLengths_Variant2: violations for upper box constraints: " + ToString(var_ub_infeasible_count) +".");
        
        valprint("var_ub",v_str(var_ub));
    }
    
    Int con_lb_infeasible_count = 0;
    Int con_ub_infeasible_count = 0;
    
    for( I j = 0; j < static_cast<Int>(d.con_count); ++j )
    {
        con_lb_infeasible_count += (b[j] < con_lb[j]);
        con_ub_infeasible_count += (b[j] > con_ub[j]);
    }
    
    if( con_lb_infeasible_count > Int(0) )
    {
        eprint(ClassName() + "::ComputeVertexCoordinates_ByLengths_Variant2: violations for lower matrix constraints: " + ToString(con_lb_infeasible_count) + ".");
        
        valprint("con_lb",v_str(con_lb));
    }
    
    if( con_ub_infeasible_count > Int(0) )
    {
        eprint(ClassName() + "::ComputeVertexCoordinates_ByLengths_Variant2: violations for upper matrix constraints: " + ToString(con_ub_infeasible_count) + ".");

        valprint("con_ub",v_str(con_ub));
    }
    
    // The integer conversion is safe as we have rounded s correctly already.
    x.Read( s.data(d.DvV_offset) );
    y.Read( s.data(d.DhV_offset) );

    ComputeVertexCoordinates(x,y);
}

