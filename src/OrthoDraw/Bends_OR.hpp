template<typename ExtInt, typename ExtInt2>
Tensor1<Turn_T,Int> Bends_OR(
    cref<PlanarDiagram<ExtInt>> pd,
    const ExtInt2 ext_region = -1
)
{
    TOOLS_MAKE_FP_STRICT();

    std::string tag = MethodName("Bends_OR");
    
    TOOLS_PTIMER(timer,tag);
    
    ExtInt ext_region_ = int_cast<ExtInt>(ext_region);
    
    using SimpleMinCostFlow_T = operations_research::SimpleMinCostFlow;
    
    using Node_T = SimpleMinCostFlow_T::NodeIndex;
    using Arc_T  = SimpleMinCostFlow_T::ArcIndex;
    using Flow_T = SimpleMinCostFlow_T::FlowQuantity;
    using Cost_T = SimpleMinCostFlow_T::CostValue;
         
    {
        // TODO: Replace pd.Arcs().Dim(0) by pd.ArcCount().
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dim(0));
        
        if( std::cmp_greater( max_idx, std::numeric_limits<Arc_T>::max() ) )
        {
            eprint(tag+": Too many arcs to fit into type " + TypeName<Arc_T> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    auto A_idx = Bends_ArcIndices(pd);
    
    const Arc_T  m = Bends_VarCount<Arc_T>(pd);
    const Node_T n = Bends_ConCount<Node_T>(pd);
    
    SimpleMinCostFlow_T mcf ( n, m );

    Tensor1<Node_T,Arc_T> tails     (m);
    Tensor1<Node_T,Arc_T> heads     (m);

    const ExtInt a_count = pd.Arcs().Dim(0);
    
    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        // right face of a
        const Node_T f_0  = static_cast<Node_T>( dA_F[pd.template ToDarc<Tail>(a)] );
        // left  face of a
        const Node_T f_1  = static_cast<Node_T>( dA_F[pd.template ToDarc<Head>(a)] );
        
        const Arc_T  di_0 = static_cast<Arc_T>( A_idx(a,0) );
        const Arc_T  di_1 = static_cast<Arc_T>( A_idx(a,1) );

        tails[di_0] = f_1;
        heads[di_0] = f_0;
        
        tails[di_1] = f_0;
        heads[di_1] = f_1;
    }
    
    Tensor1<Flow_T,Arc_T> capacities (m,Flow_T(1'000'000'000));
    
    auto costs      = Bends_ObjectiveVector<Cost_T,Arc_T>(pd);
    auto demands    = Bends_EqualityConstraintVector<Flow_T,Arc_T>(pd,ext_region_);
    
    // Add each arc.
    for( Arc_T a = 0; a < m; ++a )
    {
        Arc_T arc = mcf.AddArcWithCapacityAndUnitCost(tails[a], heads[a], capacities[a], costs[a]);
        
        if( arc != a )
        {
            eprint(tag + ": Something went wrong with the assembly of " + ToString(a) + "-th arc.");
        }
    }
    
    // Add node supplies.
    for( Node_T v = 0; v < n; ++v )
    {
        mcf.SetNodeSupply(v,-demands[v]);
    }
    
    // Find the min cost flow.
    int status = mcf.Solve();

//    TOOLS_LOGDUMP(status);
//    
//    logvalprint("OPTIMAL"           ,int(operations_research::MinCostFlow::OPTIMAL)             );
//    logvalprint("NOT_SOLVED"        ,int(operations_research::MinCostFlow::NOT_SOLVED)          );
//    logvalprint("FEASIBLE"          ,int(operations_research::MinCostFlow::FEASIBLE)            );
//    logvalprint("INFEASIBLE"        ,int(operations_research::MinCostFlow::INFEASIBLE)          );
//    logvalprint("UNBALANCED"        ,int(operations_research::MinCostFlow::UNBALANCED)          );
//    logvalprint("BAD_RESULT"        ,int(operations_research::MinCostFlow::BAD_RESULT)          );
//    logvalprint("BAD_COST_RANGE"    ,int(operations_research::MinCostFlow::BAD_COST_RANGE)      );
//    logvalprint("BAD_CAPACITY_RANGE",int(operations_research::MinCostFlow::BAD_CAPACITY_RANGE)  );
                  
    if (status != operations_research::MinCostFlow::OPTIMAL)
    {
        wprint(tag + ": Failed to find optimal solution.");
    }

    Tensor1<Flow_T,Arc_T> s ( mcf.NumArcs() );
    
    for( Arc_T a = 0; a < m; ++a )
    {
        s[a] = mcf.Flow(a);
    }
    
    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );

    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const ExtInt tail = A_idx(a,0);
            const ExtInt head = A_idx(a,1);
            bends[a] = static_cast<Turn_T>(std::round(s[head] - s[tail]));
        }
        else
        {
            bends[a] = Turn_T(0);
        }
    }

    return bends;
}
