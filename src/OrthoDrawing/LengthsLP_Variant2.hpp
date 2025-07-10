
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


Tensor1<COIN_Real,COIN_Int> Lengths_ObjectiveVector_Variant2()
{
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    Tensor1<COIN_Real,COIN_Int> c ( d.var_count, COIN_Real(0) );
    
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
            
            c_ptr[ v_0 + offset ] += -w;
            c_ptr[ v_1 + offset ] +=  w;
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
            
            c_ptr[ v_0 + offset ] += -w;
            c_ptr[ v_1 + offset ] +=  w;
        }
    }

    return c;
}

Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Lengths_ConstraintMatrix_Variant2()
{
    TOOLS_PTIMER(timer,MethodName("Lengths_ConstraintMatrix_Variant2"));
    
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
        
        for( I e = 0; e < int_cast<I>(E.Dim(0)); ++e )
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
        
        for( I e = 0; e < int_cast<I>(E.Dim(0)); ++e )
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

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnVariables_Variant2(
    bool minimize_areaQ = false
)
{
    TOOLS_MAKE_FP_STRICT();
    
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();
    
    Tensor1<COIN_Real,COIN_Int> v (d.var_count);
    
    
    COIN_Real w  = minimize_areaQ
                 ? Dv().TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;
    
    COIN_Real h  = minimize_areaQ
                 ? Dh().TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;
    
    
    fill_buffer( v.data(d.DvV_offset), w, d.DvV_count );
    fill_buffer( v.data(d.DhV_offset), h, d.DhV_count );
    
    return v;
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

void ComputeVertexCoordinates_ByLengths_Variant2( bool minimize_areaQ = false )
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_PTIMER(timer, MethodName("ComputeVertexCoordinates_ByLengths_Variant2"));
    
    ClpWrapper<double,Int> clp(
        Lengths_ObjectiveVector_Variant2(),
        Lengths_LowerBoundsOnVariables_Variant2(),
        Lengths_UpperBoundsOnVariables_Variant2(minimize_areaQ),
        Lengths_ConstraintMatrix_Variant2(),
        Lengths_LowerBoundsOnConstraints_Variant2(),
        Lengths_UpperBoundsOnConstraints_Variant2(),
        { .dualQ = settings.use_dual_simplexQ }
    );
    
    auto s = clp.IntegralPrimalSolution();
    
    cref<Variant2_Dimensions_T> d = Variant2_Dimensions();

    // The integer conversion is safe as we have rounded s correctly already.
    Tensor1<Int,Int> x ( s.data(d.DvV_offset), d.DvV_count );
    Tensor1<Int,Int> y ( s.data(d.DhV_offset), d.DhV_count );

    ComputeVertexCoordinates(x,y);
}

