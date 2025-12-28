public:

void ComputeVertexCoordinates_Compaction_MCF()
{
    TOOLS_PTIMER(timer,MethodName("ComputeVertexCoordinates_Compaction_MCF"));
    
    Tensor1<Int,Int> x;
    Tensor1<Int,Int> y;
    
    ParallelDo(
        [&x,&y,this](const Int thread)
        {
            if( thread == Int(0) )
            {
                x = Compaction_MCF( Dv(), DvEdgeCosts() );
            }
            else if( thread == Int(1) )
            {
                y = Compaction_MCF( Dh(), DhEdgeCosts() );
            }
        },
        Int(2),
        (settings.parallelizeQ ? Int(2) : (Int(1)))
    );
    
    ComputeVertexCoordinates(x,y);
}


private:

Tensor1<Int,Int> Compaction_MCF( cref<DiGraph_T> D, cref<Tensor1<Cost_T,Int>> edge_costs )
{
    TOOLS_MAKE_FP_STRICT();
    
    using MCFSimplex_T = MCF::MCFSimplex;
    using R = MCFSimplex_T::FNumber;
    using I = MCFSimplex_T::Index;
    
    TOOLS_PTIMER(timer,MethodName("Compaction_MCF"));

    {
        Size_T max_idx = Max(static_cast<Size_T>(D.VertexCount()),static_cast<Size_T>(D.EdgeCount()));
        
        if( std::cmp_greater( max_idx, std::numeric_limits<I>::max() ) )
        {
            eprint(MethodName("Compaction_MCF") + ": Too many arcs to fit into type " + TypeName<I> + ".");
            
            return Tensor1<Int,Int>();
        }
    }
    
    const I n = static_cast<I>(D.VertexCount());
    const I m = static_cast<I>(D.EdgeCount());
    
    // The length minimization problem we want to solve is a minimum cost tension problem (MCT).
    // Its dual problem (in the LP sense) is a minimum cost flow problem (MCF).
    // We use MCFSimplex_T to solve this MCF problem; then MCFSimplex_T::MCFGetPi returns the dual solution.
    
    Tensor1<R,I> capacities(m,Scalar::Infty<R>);
    Tensor1<I,I> tails     (m);
    Tensor1<I,I> heads     (m);
    Tensor1<R,I> costs     (m);
    Tensor1<R,I> demands   (n,R(0));
    
    const auto & edges = D.Edges();
    
    for( I e = 0; e < m; ++e )
    {
        const I t = static_cast<I>(edges(e,I(0)));
        const I h = static_cast<I>(edges(e,I(1)));
        
        // MCFClass uses 1-based indexing.
        tails[e]  = t + I(1);
        heads[e]  = h + I(1);
        costs[e] = -1;
        
        const R primal_cost = static_cast<R>(edge_costs[e]);
        
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

    Tensor1<R,Int> potentials (D.VertexCount());

    mcf.MCFGetPi(potentials.data());
    
    const R max_potential = potentials.Max();
    
    Tensor1<Int,Int> z (D.VertexCount());
    
    for( Int v = 0; v < D.VertexCount(); ++v )
    {
        z[v] = static_cast<Int>( std::round(max_potential - potentials[v]) );
    }
    
    return z;
}
