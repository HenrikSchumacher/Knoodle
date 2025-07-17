
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
//  for every virual edge e:
//
//  v_0 = E_V(e,Tail); v_1 = E_V(e,Head);
//
//  +/- x[V_DvV[v_0]] +/- y[V_DhV[v_0]]
//  +/- x[V_DvV[v_1]] +/- y[V_DhV[v_1]] >= 1.
//
// Here the signs depend on the kind of kitty-corner.

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

struct Variant4_Dimensions_T
{
    COIN_Int DvV_count;
    COIN_Int DvE_count;
    COIN_Int DhV_count;
    COIN_Int DhE_count;
    
    COIN_Int DvV_offset;
    COIN_Int DhV_offset;
    
    COIN_Int DvE_offset;
    COIN_Int DhE_offset;
    COIN_Int virtual_edge_offset;
    
    COIN_Int var_count;
    COIN_Int con_count;
};

public:

cref<Variant4_Dimensions_T> Variant4_Dimensions() const
{
    if( !this->InCacheQ("Variant4_Dimensions") )
    {
        Variant4_Dimensions_T dim;
        
        dim.DvV_count = static_cast<COIN_Int>(Dv().VertexCount());
        dim.DvE_count = static_cast<COIN_Int>(Dv().EdgeCount());
        dim.DhV_count = static_cast<COIN_Int>(Dh().VertexCount());
        dim.DhE_count = static_cast<COIN_Int>(Dh().EdgeCount());

        dim.var_count = dim.DvV_count + dim.DhV_count;
        
        dim.con_count = dim.DvE_count + dim.DhE_count + static_cast<COIN_Int>(virtual_edge_count);
        
        // Variables in this order: [x,y].
        dim.DvV_offset = 0;
        dim.DhV_offset = dim.DvV_offset + dim.DvV_count;
        
        // Constraints in this order: [lh,lv]
        dim.DvE_offset = 0;
        dim.DhE_offset = dim.DvE_offset + dim.DvE_count;
        dim.virtual_edge_offset = dim.DhE_offset + dim.DhE_count;
        
        
        this->SetCache("Variant4_Dimensions",std::move(dim));
    }
    
    return this->GetCache<Variant4_Dimensions_T>("Variant4_Dimensions");
}


Tensor1<COIN_Real,COIN_Int> Lengths_ObjectiveVector_Variant4()
{
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
    
    Tensor1<COIN_Real,COIN_Int> c ( dim.var_count, COIN_Real(0) );
    
    mptr<COIN_Real> c_ptr = c.data();
    
    // objective =
    //  \sum_{e \in E(Dv)} DvE_cost[e] * (x[Dv.Edges()(e,1)] - x[Dv.Edges()(e,0)])
    //  +
    //  \sum_{e \in E(Dh)} DhE_cost[e] * (y[Dh.Edges()(e,1)] - y[Dh.Edges()(e,0)])

    {
        auto &    E       = Dv().Edges();
        auto &    E_cost  = DvEdgeCosts();
              Int offset  = dim.DvV_offset;
        const Int e_count = E.Dim(0);
        
        for( Int e = 0; e < e_count; ++e )
        {
            const Int  v_0    = E(e,Tail);
            const Int  v_1    = E(e,Head);
            const COIN_Real w = E_cost[e];
            
            c_ptr[ v_0 + offset ] += -w;
            c_ptr[ v_1 + offset ] +=  w;
        }
    }
    
    {
        auto &    E       = Dh().Edges();
        auto &    E_cost  = DhEdgeCosts();
              Int offset  = dim.DhV_offset;
        const Int e_count = E.Dim(0);
        
        for( Int e = 0; e < e_count; ++e )
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

Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt> Lengths_ConstraintMatrix_Variant4()
{
    TOOLS_PTIMER(timer,MethodName("Lengths_ConstraintMatrix_Variant4"));
    
    print("Lengths_ConstraintMatrix_Variant4");
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    using S = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
    
    const auto & V_DvV = VertexToDvVertex();
    const auto & V_DhV = VertexToDhVertex();
    
    TripleAggregator<I,I,S,J> agg ( J(3) * static_cast<J>(dim.con_count) );
    
    // Constraints from horizontal edges.
    {
        const auto & E   = Dv().Edges();
        const I v_offset = dim.DvV_offset;
        const I e_offset = dim.DvE_offset;
        const I e_count  = int_cast<I>(E.Dim(0));
        
        for( I e = 0; e < e_count; ++e )
        {
            const I v_0 = static_cast<I>(E(e,Tail));
            const I v_1 = static_cast<I>(E(e,Head));
            
            // lv[e] = y[v_1] - y[v_0] >= 1
            agg.Push( v_1 + v_offset, e + e_offset, S( 1) );
            agg.Push( v_0 + v_offset, e + e_offset, S(-1) );
        }
    }
    
    // Constraints from vertical edges.
    {
        const auto & E   = Dh().Edges();
        const I v_offset = dim.DhV_offset;
        const I e_offset = dim.DhE_offset;
        const I e_count  = int_cast<I>(E.Dim(0));
        
        for( I e = 0; e < e_count; ++e )
        {
            const I v_0 = static_cast<I>(E(e,Tail));
            const I v_1 = static_cast<I>(E(e,Head));
            
            // lv[e] = y[v_1] - y[v_0] >= 1
            agg.Push( v_1 + v_offset, e + e_offset, S( 1) );
            agg.Push( v_0 + v_offset, e + e_offset, S(-1) );
        }
    }
    
    // Constraints from virtual edges.
    {
        const I v_offset   = dim.DhV_offset;
        const I con_offset = dim.virtual_edge_offset;
        
        const Int e_begin = int_cast<Int>(E_V.Dim(0) - virtual_edge_count);
        const Int e_end   = int_cast<Int>(E_V.Dim(0));
        
        for( Int e = e_begin; e < e_end; ++e )
        {
            const Int de = ToDedge<Head>(e);
            
            // DEBUGGING
            if( !DedgeVirtualQ(de) )
            {
                wprint("Nonvirtual dedge " + ToString(de) + " detected where only virtual edges should be.");
            }
            
            const Int v_0 = E_V(e,Tail);
            const Int v_1 = E_V(e,Head);
            
            // TODO: Finish this!
            
            Dir_T direction = NoDir;
            
            const Dir_T d = E_dir[e];
            
            // BEWARE: This always works if corners have valence e
            
            auto is_voidQ = [this]( const Int de, const Int df )
            {
                return df == Uninitialized || (DedgeVirtualQ(df) && de != df);
            };
            
            
            switch( d )
            {
                case East:
                {
                    if( is_voidQ(de,V_dE(v_0,North)) )
                    {
                        direction = NorthEast;
                    }
                    else
                    {
                        direction = SouthEast;
                    }
                    break;
                }
                case North:
                {
                    if( is_voidQ(de,V_dE(v_0,West)) )
                    {
                        direction = NorthWest;
                    }
                    else
                    {
                        direction = NorthEast;
                    }
                    break;
                }
                case West:
                {
                    if( is_voidQ(de,V_dE(v_0,South)) )
                    {
                        direction = SouthWest;
                    }
                    else
                    {
                        direction = NorthWest;
                    }
                    break;
                }
                case South:
                {
                    if( is_voidQ(de,V_dE(v_0,East)) )
                    {
                        direction = SouthEast;
                    }
                    else
                    {
                        direction = NorthWest;
                    }
                    break;
                }
            }
            
//            print("virtual edge " + ToString(e) + " = { " + ToString(v_0) + ", " + ToString(v_1) + " } classified with direction " + DirectionString(direction));
            
            const I con = int_cast<I>(e - e_begin) + con_offset;
            
            // Constraint is.
            // a_0 * x[v_0] + b_0 * y[v_0] + a_1 * x[v_1] + b_1 * y[v_1] >= 1
            I a_0;
            I b_0;
            I a_1;
            I b_1;

            switch( direction )
            {
                case NorthEast:
                {
//
//                         ^
//                         |
//                         |
//                         |
//                         +<--------
//                      v_1
//
//
//                v_0
//      -------->+
//               |
//               |
//               |
//               v
//
// - x[v_0] - y[v_0] + x[v_1] + y[v_1] >= 1
                    
                    a_0 = -1; b_0 = -1; a_1 =  1; b_1 =  1;
                    break;
                }
                case SouthWest:
                {
                    // This is the NorthEast case with swapped roles for v_0 and v_1.
                    a_0 =  1; b_0 =  1; a_1 = -1; b_1 = -1;
                    break;
                }
                case NorthWest:
                {
//
//               ^
//               |
//               |
//               |
//      <--------+
//                v_1
//
//
//                     v_0
//                        +-------->
//                        |
//                        |
//                        |
//                        v
//
// + x[v_0] - y[v_0] - x[v_1] + y[v_1] >= 1
                    
                    a_0 =  1; b_0 = -1; a_1 = -1; b_1 =  1;
                    break;
                }
                case SouthEast:
                {
                    // This is the NorthWest case with swapped roles for v_0 and v_1.
                    a_0 = -1; b_0 =  1; a_1 =  1; b_1 = -1;
                    break;
                }
                    
                default:
                {
                    a_0 = 0;
                    a_1 = 0;
                    b_0 = 0;
                    b_1 = 0;
                    eprint(MethodName("Lengths_ConstraintMatrix_Variant4")+": direction of virtual dedge + " + ToString(de) + "cannot be detected.");
                    break;
                }
            }
            
            // Positions of x[v_0], y[v_0], x[v_1], y[v_1] in the vector of variables.
            const I Dv_s_0 = int_cast<I>(V_DvV[v_0]);            // x[v_0]
            const I Dh_s_0 = int_cast<I>(V_DhV[v_0]) + v_offset; // y[v_0]
            const I Dv_s_1 = int_cast<I>(V_DvV[v_1]);            // x[v_1]
            const I Dh_s_1 = int_cast<I>(V_DhV[v_1]) + v_offset; // y[v_1]
            
//            print("push {" + ToString(con) + ", " + ToString(Dv_s_0) + ", " + ToString(a_0) + " }");
//            print("push {" + ToString(con) + ", " + ToString(Dh_s_0) + ", " + ToString(b_0) + " }");
//            print("push {" + ToString(con) + ", " + ToString(Dv_s_1) + ", " + ToString(a_1) + " }");
//            print("push {" + ToString(con) + ", " + ToString(Dh_s_1) + ", " + ToString(b_1) + " }");
            
            agg.Push( Dv_s_0, con, a_0 );
            agg.Push( Dh_s_0, con, b_0 );
            agg.Push( Dv_s_1, con, a_1 );
            agg.Push( Dh_s_1, con, b_1 );
        }
    }
    
    Sparse::MatrixCSR<S,I,J> A (agg,dim.var_count,dim.con_count,true,false);
    
    return A;
}

Tensor1<COIN_Real,COIN_Int> Lengths_LowerBoundsOnVariables_Variant4()
{
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();

    return Tensor1<COIN_Real,COIN_Int>(dim.var_count,COIN_Real(0));
}

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnVariables_Variant4(
    bool minimize_areaQ = false
)
{
    TOOLS_MAKE_FP_STRICT();
    
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
    
    Tensor1<COIN_Real,COIN_Int> v (dim.var_count);
    
    
    COIN_Real w  = minimize_areaQ
                 ? Dv().TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;
    
    COIN_Real h  = minimize_areaQ
                 ? Dh().TopologicalNumbering().Max()
                 : Scalar::Infty<COIN_Real>;
    
    
    fill_buffer( v.data(dim.DvV_offset), w, dim.DvV_count );
    fill_buffer( v.data(dim.DhV_offset), h, dim.DhV_count );
    
    return v;
}

Tensor1<COIN_Real,COIN_Int> Lengths_LowerBoundsOnConstraints_Variant4()
{
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
    
    // Each edge must have length >= 1.
    return Tensor1<COIN_Real,COIN_Int>(dim.con_count,COIN_Real(1));
}

Tensor1<COIN_Real,COIN_Int> Lengths_UpperBoundsOnConstraints_Variant4()
{
    TOOLS_MAKE_FP_STRICT();
    
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
    
    // No upper bound for edge length.
    
    return Tensor1<COIN_Real,COIN_Int>(dim.con_count,Scalar::Infty<COIN_Real>);
}

void ComputeVertexCoordinates_ByLengths_Variant4( bool minimize_areaQ = false )
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_PTIMER(timer, MethodName("ComputeVertexCoordinates_ByLengths_Variant4"));
    
    ClpWrapper<double,Int> clp(
        Lengths_ObjectiveVector_Variant4(),
        Lengths_LowerBoundsOnVariables_Variant4(),
        Lengths_UpperBoundsOnVariables_Variant4(minimize_areaQ),
        Lengths_ConstraintMatrix_Variant4(),
        Lengths_LowerBoundsOnConstraints_Variant4(),
        Lengths_UpperBoundsOnConstraints_Variant4(),
        { .dualQ = settings.use_dual_simplexQ }
    );
    
    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
    
//    auto s = clp.IntegralPrimalSolution();
//    
//    cref<Variant4_Dimensions_T> dim = Variant4_Dimensions();
//
//    // The integer conversion is safe as we have rounded s correctly already.
//    Tensor1<Int,Int> x ( s.data(dim.DvV_offset), dim.DvV_count );
//    Tensor1<Int,Int> y ( s.data(dim.DhV_offset), dim.DhV_count );
        
    auto s = clp.PrimalSolution();
    
//    TOOLS_DUMP(s);
    
//    s *= COIN_Real(4);
    
//    TOOLS_DUMP(s);
    


    // The integer conversion is safe as we have rounded s correctly already.
    Tensor1<Int,Int> x ( dim.DvV_count );
    Tensor1<Int,Int> y ( dim.DhV_count );

    
    for( Int i = 0; i < dim.DvV_count; ++i )
    {
        x[i] = static_cast<Int>(std::round(COIN_Real(4) * s[i + dim.DvV_offset]));
    }
    
    for( Int i = 0; i < dim.DhV_count; ++i )
    {
        y[i] = static_cast<Int>(std::round(COIN_Real(4) * s[i + dim.DhV_offset]));
    }
    
    ComputeVertexCoordinates(x,y);
}

