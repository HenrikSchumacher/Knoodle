// TODO: This function is not correct, I think. It does not account for single-vertex components!

// TODO: Rewrite this with `DepthFirstSearch`.

private:

void RequireTopology() const
{
    TOOLS_PTIMER(timer,ClassName()+"::RequireTopology");
    // v stands for "vertex"
    // e stands for "edge"
    // d stands for "direction"
    // c stands for "component"
    // z stands for "cycle" (German "Zykel")
    
    const EInt edge_count = edges.Dim(0);
    
    // TODO: Make it work with unsigned integers.
    // TODO: Make this consistent with Sparse::Tree?
    // v_parent[v] stores the parent vertex in the spanning forest.
    // v_parent[v] == -2 means that vertex v does not exist.
    // v_parent[v] == -1 means that vertex v is a root vertex.
    // v_parent[v] >=  0 means that vertex v_parent[v] is v's parent.
    Tensor1<VInt,VInt> v_parents ( vertex_count, -2 );
    
    
    // Stores the graph's components.
    // c_ptr contains a counter for the number of components.
    Aggregator<VInt,EInt> c_ptr ( edge_count + 1 );
    Aggregator<VInt,EInt> c_idx ( edge_count     );
    
    c_ptr.Push(0);
    
    // Stores the graph's cycles.
    // z_ptr contains a counter for the number of cycles.
    Aggregator<EInt  ,EInt> z_ptr ( edge_count );
    Aggregator<EInt  ,EInt> z_idx ( edge_count );
    Aggregator<Sign_T,EInt> z_val ( edge_count );
    
    z_ptr.Push(0);
    
    // TODO: Make it work with unsigned integers.
    // Tracks which vertices have been explored.
    // v_flags[v] == 0 means vertex v is unvisited.
    // v_flags[v] == 1 means vertex v is explored.
    
    Tensor1<UInt8,VInt> v_flags ( vertex_count, 0 );
    
    // Tracks which edges have been visited.
    // e_flags[e] == 0 means edge e is unvisited.
    // e_flags[e] == 1 means edge e is explored.
    // e_flags[e] == 2 means edge e is visited (and completed).
    Tensor1<Sign_T,EInt> e_flags ( edge_count, Sign_T(0) );
    
    static_assert(SignedIntQ<EInt>,"");
    
    // TODO: I can keep e_stack, path, and d_stack uninitialized.
    // Keeps track of the edges we have to process.
    // Every edge can be only explored in two ways: from its two vertices.
    // Thus, the stack does not have to be bigger than 2 * edge_count.
    Tensor1<EInt,EInt> e_stack ( EInt(2) * edge_count, EInt(-2) );
    // Keeps track of the edges we travelled through.
    Tensor1<EInt,EInt> e_path ( edge_count + EInt(2), EInt(-2) );
    // Keeps track of the vertices we travelled through.
    Tensor1<VInt,EInt> v_path ( edge_count + EInt(2), VInt(-2) );
    // Keeps track of the directions we travelled through the edges.
    Tensor1<Sign_T,EInt> d_path ( edge_count + EInt(2), Sign_T(-2) );
    
    EInt e_ptr = 0; // Guarantees that every component will be visited.
    
    auto & B = OrientedIncidenceMatrix();
    
    while( e_ptr < edge_count )
    {
        while( (e_ptr < edge_count) && (e_flags[e_ptr] > Sign_T(0)) )
        {
            ++e_ptr;
        }
        
        if( e_ptr == edge_count )
        {
            break;
        }
        
        // Now e_ptr is an unvisited edge.
        // We can start a new component.
        
        const VInt root = edges(e_ptr,Tail);
        
        v_parents[root] = UninitializedVertex;
        
        static_assert(SignedIntQ<EInt>,"");
        EInt stack_ptr = -1;
        EInt path_ptr  = -1;
        
        // Tell vertex root that it is explored and put it onto the path.
        v_flags[root] = 1;
        v_path[0] = root;
        
        // Push all edges incident to v onto the stack.
        for( EInt k = B.Outer(root+1); k --> B.Outer(root);  )
        {
            const EInt n_e = B.Inner(k); // new edge
            
            e_stack[++stack_ptr] = n_e;
        }
        
        
        // Start spanning tree.
        static_assert(SignedIntQ<EInt>,"");
        while( stack_ptr > EInt(-1) )
        {
            // Top
            const EInt e = e_stack[stack_ptr];
            
            if( e_flags[e] == 1 )
            {
                // Mark as visited and track back.
                e_flags[e] = 2;
                
                // Pop from stack.
                --stack_ptr;

                // Backtrack.
                --path_ptr;
                
                // Tell the current component that e is its member.
                c_idx.Push(e);
                
                continue;
            }
            else if( e_flags[e] == 2 )
            {
                // Pop from stack.
                --stack_ptr;
                
                continue;
            }
            
            Tiny::Vector<2,VInt,EInt> E ( edges.data(e) );
            
            const VInt w = v_path[path_ptr+1];
            
            
            if( (E[0] != w) && (E[1] != w) )
            {
                eprint( "(E[0] != w) && (E[1] != w)" );
            }
            
            const Sign_T d = (E[0] == w) ? Sign_T(1) : Sign_T(-1);
            const VInt v = E[d > Sign_T(0)];
            
            // Mark edge e as explored and put it onto the path.
            e_flags[e] = 1;
            
            ++path_ptr;
            e_path[path_ptr  ] = e;
            d_path[path_ptr  ] = d;
            v_path[path_ptr+1] = v;
            
            const UInt8 f = v_flags[v];
            
            if( f < UInt8(1) )
            {
                // Visit vertex.
                v_flags[v]   = 1;
                v_parents[v] = w;
                
                const EInt k_begin = B.Outer(v  );
                const EInt k_end   = B.Outer(v+1);
                
                // TODO: I could pop the stack if k_end == k_begin + 1.
                
                // Push all edges incident to v except e onto the stack.
                for( EInt k = k_end; k --> k_begin;  )
                {
                    const EInt n_e = B.Inner(k); // new edge
                    
                    if( n_e != e )
                    {
                        e_stack[++stack_ptr] = n_e;
                    }
                }
            }
            else
            {
                // Vertex rediscovered.
                // Create a new cycle.
                
                EInt pos = path_ptr;

                while( (pos >= EInt(0)) && (v_path[pos] != v) )
                {
                    --pos;
                }
                
                if( pos < EInt(0) )
                {
                    eprint("pos < 0");
                }
                
                const EInt z_length = path_ptr - pos + 1;
                
                z_idx.Push( &e_path[pos], z_length );
                z_val.Push( &d_path[pos], z_length );
                z_ptr.Push( z_idx.Size() );
                
                // TODO: I could pop the stack right now.
            }
            
        } // while( stack_ptr > -1 )
        
        c_ptr.Push( c_idx.Size() );
        
    } // while( e_ptr < edge_count )

    
    // Now all components and cycles have been explored.
    
    // Store spanning forest.
    this->SetCache( std::string("SpanningTree"), std::move(v_parents) );
    
    
    // Store cycle matrix.
    
    CycleMatrix_T Z (
        z_ptr.Disband(),
        z_idx.Disband(),
        z_val.Disband(),
        z_ptr.Size()-EInt(1), edge_count, EInt(1)
    );
    
    // TODO: I don't like the idea to lose the information on the ordering of edges in the cycles. But we have to conform to CSR format, right?
//            Z.SortInner();
    
    this->SetCache( std::string("CyleMatrix"), std::move(Z) );
    
    // Store component matrix.
    
    ComponentMatrix_T C (
        c_ptr.Disband(),
        c_idx.Disband(),
        c_ptr.Size()-1, edge_count, VInt(1)
    );

    C.SortInner();
    
    this->SetCache( std::string("ComponentEdgeMatrix"), std::move(C) );
}

public:
    
cref<CycleMatrix_T> CycleMatrix( ) const
{
    std::string tag ("CyleMatrix");
    if(!this->InCacheQ(tag)) { RequireTopology(); }
    return this->template GetCache<CycleMatrix_T>(tag);
}

cref<ComponentMatrix_T> ComponentEdgeMatrix() const
{
    std::string tag ("ComponentEdgeMatrix");
    if(!this->InCacheQ(tag)) { RequireTopology(); }
    return this->template GetCache<ComponentMatrix_T>(tag);
}

cref<Tensor1<VInt,VInt>> SpanningTree() const
{
    std::string tag ("SpanningTree");
    if(!this->InCacheQ(tag)) { RequireTopology(); }
    return this->template GetCache<Tensor1<VInt,VInt>>(tag);
}
