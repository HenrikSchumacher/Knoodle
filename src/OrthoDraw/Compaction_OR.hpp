public:

void ComputeVertexCoordinates_Compaction_OR()
{
    TOOLS_PTIMER(timer,MethodName("ComputeVertexCoordinates_Compaction_OR"));
    
    Tensor1<Int,Int> x;
    Tensor1<Int,Int> y;
    
    ParallelDo(
        [&x,&y,this](const Int thread)
        {
            if( thread == Int(0) )
            {
                x = Compaction_OR( Dv(), DvEdgeCosts() );
            }
            else if( thread == Int(1) )
            {
                y = Compaction_OR( Dh(), DhEdgeCosts() );
            }
        },
        Int(2),
        (settings.parallelizeQ ? Int(2) : (Int(1)))
    );
    
    ComputeVertexCoordinates(x,y);
}


private:

Tensor1<Int,Int> Compaction_OR( cref<DiGraph_T> D, cref<Tensor1<Cost_T,Int>> edge_costs )
{
    TOOLS_MAKE_FP_STRICT();
    
    using SimpleMinCostFlow_T = operations_research::SimpleMinCostFlow;
    
    using Node_T = SimpleMinCostFlow_T::NodeIndex;
    using Arc_T  = SimpleMinCostFlow_T::ArcIndex;
    using Flow_T = SimpleMinCostFlow_T::FlowQuantity;
    using Cost_T = SimpleMinCostFlow_T::CostValue;
    
    TOOLS_PTIMER(timer,MethodName("Compaction_OR"));

    {
        Size_T max_idx = Max(static_cast<Size_T>(D.VertexCount()),static_cast<Size_T>(D.EdgeCount()));
        
        if( std::cmp_greater( max_idx, std::numeric_limits<Arc_T>::max() ) )
        {
            eprint(MethodName("Compaction_MCF") + ": Too many arcs to fit into type " + TypeName<Arc_T> + ".");
            
            return Tensor1<Int,Int>();
        }
    }
    
    const Node_T n = static_cast<Node_T>(D.VertexCount());
    const Arc_T  m = static_cast<Arc_T >(D.EdgeCount());
    
    // The length minimization problem we want to solve is a minimum cost tension problem (MCT).
    // Its dual problem (in the LP sense) is a minimum cost flow problem (MCF).
    // We use MCFSimplex_T to solve this MCF problem; then MCFSimplex_T::MCFGetPi returns the dual solution.
    
//    Tensor1<Flow_T,I> capacities(m,Flow_T();
    Tensor1<Node_T,Arc_T > tails     (m);
    Tensor1<Node_T,Arc_T > heads     (m);
    Tensor1<Flow_T,Arc_T > costs     (m);
    Tensor1<Flow_T,Node_T> demands   (n,Flow_T(0));
    
    const auto & edges = D.Edges();
    
    for( Arc_T e = 0; e < m; ++e )
    {
        const Node_T t = static_cast<Node_T>(edges(e,0));
        const Node_T h = static_cast<Node_T>(edges(e,1));
        
        tails[e]  = t;
        heads[e]  = h;
        costs[e] = -1;
        
        const Cost_T primal_cost = static_cast<Cost_T>(edge_costs[e]);
        
        demands[t] -= primal_cost;
        demands[h] += primal_cost;
    }

    MCFSimplex_T mcf (n,m);
    
    mcf.LoadNet(
        n, // maximal number of vertices
        m, // maximal number of edges
        n, // current number of vertices
        m, // current number of edges
        capacities.data(),
        costs.data(),
        demands.data(),
        tails.data(),
        heads.data()
    );
    
    mcf.SolveMCF();

    Tensor1<Flow_T,Int> potentials (D.VertexCount());

    mcf.MCFGetPi(potentials.data());
    
    const R max_potential = potentials.Max();
    
    Tensor1<Int,Int> z (D.VertexCount());
    
    for( Int v = 0; v < D.VertexCount(); ++v )
    {
        z[v] = static_cast<Int>( std::round(max_potential - potentials[v]) );
    }
    
    return z;
}
