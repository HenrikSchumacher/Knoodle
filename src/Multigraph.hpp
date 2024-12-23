#pragma once

namespace KnotTools
{
    
    template<typename Int_ = Int64, typename SInt_ = Int8>
    class alignas( ObjectAlignment ) Multigraph : public CachedObject
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        static_assert(SignedIntQ<Int_>,"");
                
    public:
        
        using Int  = Int_;
        
        using SInt = SInt_;
        
        using IncidenceMatrix_T  = Sparse::MatrixCSR<Int,Int,Int>;
        
        using CycleMatrix_T      = Sparse::MatrixCSR<Int,Int,Int>;
        
        using ComponentMatrix_T  = Sparse::BinaryMatrixCSR<Int,Int>;
        
        using Edge_T = Tiny::Vector<2,Int,Int>;
        
        
    protected:
        
        Int vertex_count;
        
        Tensor2<Int,Int> edges;

    public:
        
        Multigraph() = default;
        
        virtual ~Multigraph() override = default;

        
        // Provide a list of edges in interleaved form.
        template<typename I_0, typename I_1, typename I_2>
        Multigraph( const I_0 vertex_count_, cptr<I_1> edges_, const I_2 edge_count_ )
        :   vertex_count ( vertex_count_          )
        ,   edges        ( edges_, edge_count_, 2 )
        {
            CheckInputs();
        }
        
        
        // Provide a list of edges in interleaved form.
        template<typename I_0>
        Multigraph( const I_0 vertex_count_, Tensor2<Int,Int> && edges_ )
        :   vertex_count ( vertex_count_          )
        ,   edges        ( std::move(edges_) )
        {
            if( edges.Dimension(1) != 2 )
            {
                wprint( this->ClassName()+"(): Second dimension of input tensor is not equal to 0." );
            }
            
            CheckInputs();
        }
        
        
        // Copy constructor
        Multigraph( const Multigraph & other ) = default;
        
        friend void swap( Multigraph & A, Multigraph & B ) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( static_cast<CachedObject &>(A), static_cast<CachedObject &>(B) );
            
            swap( A.vertex_count, B.vertex_count );
            swap( A.edges       , B.edges        );
        }
        
        // Move constructor
        Multigraph( Multigraph && other ) noexcept
        :   Multigraph()
        {
            swap(*this, other);
        }

        /* Copy assignment operator */
        Multigraph & operator=( Multigraph other ) noexcept
        {   //                                     ^
            //                                     |
            // Use the copy constructor     -------+
            swap( *this, other );
            return *this;
        }
        
    private:
        
        void CheckInputs() const
        {
            const Int edge_count = edges.Dimension(0);
            
            for( Int e = 0; e < edge_count; ++e )
            {
                Edge_T E ( edges.data(e) );

                if( E[0] < 0 )
                {
                    eprint("Multigraph:  first entry of edge " + ToString(e) + " is negative.");

                    return;
                }

                if( E[1] < 0 )
                {
                    eprint("Multigraph: second entry of edge " + ToString(e) + " is negative.");
                    return;
                }
            }
        }
        
    public:
        
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        Int EdgeCount() const
        {
            return edges.Dimension(0);
        }
        
    
        cref<Tensor2<Int,Int>> Edges() const
        {
            return edges;
        }
        
        cref<IncidenceMatrix_T> TransposedIncidenceMatrix() const
        {
            std::string tag ("TransposedIncidenceMatrix");
            
            if( !this->InCacheQ( tag ) )
            {
                const Int edge_count = edges.Dimension(0);
                
                Tensor1<Int,Int>    rp  ( edge_count + 1 );
                Aggregator<Int,Int> ci  ( 2 * edge_count );
                Aggregator<Int,Int> val ( 2 * edge_count );
                
                // If there are no loops in the graph, then agg has already the correct length.
                
                
                rp[0] = 0;
                
                for( Int e = 0; e < edge_count; ++e )
                {
                    rp[ e + 1 ] = 2 * (e + 1);
                    
                    Edge_T E ( edges.data(e) );
                    
                    if( E[0] < E[1] )
                    {
                        ci.Push( E[0] );
                        ci.Push( E[1] );
                        
                        val.Push( Int(-1) );
                        val.Push( Int( 1) );
                    }
                    else if( E[0] > E[1] )
                    {
                        ci.Push( E[1] );
                        ci.Push( E[0] );
                        
                        val.Push( Int( 1) );
                        val.Push( Int(-1) );
                    }
                    else // if(E[0] == E[1] )
                    {
                        ci.Push( E[0] );
                        
                        val.Push( Int( 0) ); // Putting an explicit zero for the loop.
                    }
                }
                
                this->SetCache(
                    tag,
                    std::make_any<IncidenceMatrix_T>(
                        std::move(rp), std::move(ci.Get()), std::move(val.Get()),
                        edge_count, vertex_count, Int(1)
                    )
                );
            }

            return this->template GetCache<IncidenceMatrix_T>(tag);
        }
        
        cref<IncidenceMatrix_T> IncidenceMatrix() const
        {
            std::string tag ("IncidenceMatrix");
            
            if( !this->InCacheQ( tag ) )
            {
                this->SetCache( tag, std::move(TransposedIncidenceMatrix().Transpose()) );
            }

            return this->template GetCache<IncidenceMatrix_T>(tag);
        }
        
        
//    private:
        
        // TODO: This function is not correct, I think. It does not account for single-vertex components!
        
        void RequireTopology() const
        {
            ptic( ClassName()+ "::RequireTopology" );
            // v stands for "vertex"
            // e stands for "edge"
            // d stands for "direction"
            // c stands for "component"
            // z stands for "cycle" (German "Zykel")
            
            const Int edge_count = edges.Dimension(0);
            
            // TODO: Make it work with unsigned integers.
            // TODO: Make this consistent with Sparse::Tree?
            // v_parent[v] stores the parent vertex in the spanning forest.
            // v_parent[v] == -2 means that vertex v does not exist.
            // v_parent[v] == -1 means that vertex v is a root vertex.
            // v_parent[v] >=  0 means that vertex v_parent[v] is v's parent.
            Tensor1<Int,Int> v_parents ( vertex_count, -2 );
            
            
            // Stores the graph's components.
            // c_ptr contains a counter for the number of components.
            Aggregator<Int,Int> c_ptr ( edge_count + 1 );
            Aggregator<Int,Int> c_idx ( edge_count     );
            
            c_ptr.Push(0);
            
            // Stores the graph's cycles.
            // z_ptr contains a counter for the number of cycles.
            Aggregator<Int,Int> z_ptr ( edge_count );
            Aggregator<Int,Int> z_idx ( edge_count );
            Aggregator<Int,Int> z_val ( edge_count );
            
            z_ptr.Push(0);
            
            // TODO: Make it work with unsigned integers.
            // Tracks which vertices have been explored.
            // v_flags[v] == 0 means vertex v is unvisited.
            // v_flags[v] == 1 means vertex v is explored.
            
            Tensor1<SInt,Int> v_flags ( vertex_count, 0 );
            
            // Tracks which edges have been visited.
            // e_flags[e] == 0 means edge e is unvisited.
            // e_flags[e] == 1 means edge e is explored.
            // e_flags[e] == 2 means edge e is visited (and completed).
            Tensor1<SInt,Int> e_flags ( edge_count, 0 );
            
            // TODO: I can keep e_stack, path, and d_stack uninitialized.
            // Keeps track of the edges we have to process.
            // Every edge can be only explored in two ways: from its two vertices.
            // Thus, the stack does not have to be bigger than 2 * edge_count.
            Tensor1<Int,Int> e_stack ( 2 * edge_count, -2 );
            
            // Keeps track of the edges we travelled through.
            Tensor1<Int,Int> e_path ( edge_count + 2, -2 );
            // Keeps track of the vertices we travelled through.
            Tensor1<Int,Int> v_path ( edge_count + 2, -2 );
            // Keeps track of the directions we travelled through the edges.
            Tensor1<Int,Int> d_path ( edge_count + 2, -2 );
            
            Int e_ptr = 0; // Guarantees that every component will be visited.
            
            cref<IncidenceMatrix_T> B = IncidenceMatrix();
            
            while( e_ptr < edge_count )
            {
                while( (e_ptr < edge_count) && (e_flags[e_ptr] > 0) )
                {
                    ++e_ptr;
                }
                
                if( e_ptr == edge_count )
                {
                    break;
                }
                
                // Now e_ptr is an unvisited edge.
                // We can start a new component.
                
                const Int root = edges[e_ptr][0];
                
                v_parents[root] = -1;
                
                Int stack_ptr = -1;
                Int path_ptr  = -1;
                
                // Tell vertex root that it is explored and put it onto the path.
                v_flags[root] = 1;
                v_path[0] = root;
                
                // Push all edges incident to v onto the stack.
                for( Int k = B.Outer(root+1); k --> B.Outer(root);  )
                {
                    const Int n_e = B.Inner(k); // new edge
                    
//                    logprint("Pushing " + ToString(n_e) + " onto e_stack. ");
                    
                    e_stack[++stack_ptr] = n_e;
                }
                
                
                // Start spanning tree.
                
                while( stack_ptr > -1 )
                {
                    // Top
                    const Int e = e_stack[stack_ptr];
                    
                    if( e_flags[e] == 1 )
                    {
//                        logprint("Edge " + ToString(e) + " is explored.");
                        
                        // Mark as visited and tack back.
                        e_flags[e] = 2;
                        
                        // Pop stack from stack.
                        --stack_ptr;

                        // Backtrack.
                        --path_ptr;
                        
                        // Tell the current component that e is its member.
                        c_idx.Push(e);
                        
                        continue;
                    }
                    else if( e_flags[e] == 2 )
                    {
//                        logprint("Edge " + ToString(e) + " is visited.");
                        
//                        e_stack[stack_ptr] = -2;
                        
                        // Pop stack from stack.
                        --stack_ptr;
                        
                        continue;
                    }
                    
                    
                    Tiny::Vector<2,Int,Int> E ( edges.data(e) );
                    
                    const Int w = v_path[path_ptr+1];
                    
                    
                    if( (E[0] != w) && (E[1] != w) )
                    {
                        eprint( "(E[0] != w) && (E[1] != w)" );
                    }
                    
                    const Int d = (E[0] == w) ? 1 : -1;
                    const Int v = E[d>0];
                    
                    // Mark edge e as explored and put it onto the path.
                    e_flags[e] = 1;
                    
                    ++path_ptr;
                    e_path[path_ptr  ] = e;
                    d_path[path_ptr  ] = d;
                    v_path[path_ptr+1] = v;
                    
                    const Int f = v_flags[v];
                    
                    if( f < 1 )
                    {
//                        logprint("Vertex " + ToString(v) + " is unexplored.");
                        
                        v_flags[v]   = 1;
                        v_parents[v] = w;
                        
                        const Int k_begin = B.Outer(v  );
                        const Int k_end   = B.Outer(v+1);
                        
                        // TODO: I could pop the stack if k_end == k_begin + 1.
                        
                        // Push all edges incident to v except e onto the stack.
                        for( Int k = k_end; k --> k_begin;  )
                        {
                            const Int n_e = B.Inner(k); // new edge
                            
                            if( n_e != e )
                            {
//                                logprint("Pushing " + ToString(n_e) + " onto e_stack. ");
                                
                                e_stack[++stack_ptr] = n_e;
                            }
                        }
                    }
                    else
                    {
//                        logprint("Vertex " + ToString(v) + " is visited.");
                        
                        // Create a new cycle.
                        
                        Int pos = path_ptr;

                        while( (pos >= 0) && (v_path[pos] != v) )
                        {
                            --pos;
                        }
                        
                        if( pos < 0 )
                        {
                            eprint("pos < 0");
                        }
                        
                        const Int z_length = path_ptr - pos + 1;
                        
                        z_idx.Push( &e_path[pos], z_length );
                        z_val.Push( &d_path[pos], z_length );
                        z_ptr.Push( z_idx.Size() );
//
//                        logvalprint( "Cycle " , ArrayToString( &e_path[pos], {z_length} ) );
//                        logvalprint( "Orient" , ArrayToString( &d_path[pos], {z_length} ) );
                        
                        // TODO: I could pop the stack right now.
                    }
                    
                } // while( stack_ptr > -1 )
                
                c_ptr.Push( c_idx.Size() );
                
            } // while( e_ptr < edge_count )
            
            
            
            // Now all components and cycles have been explored.
            
            // Store spanning forest.
            this->SetCache( std::string("SpanningForest"), std::move(v_parents) );
            
            
            // Store cycle matrix.
            
            CycleMatrix_T Z (
                std::move(z_ptr.Get()), 
                std::move(z_idx.Get()),
                std::move(z_val.Get()),
                z_ptr.Size()-1, edge_count, Int(1)
            );
            
            // TODO: I don't like the idea to lose the information on the ordering of edges in the cycles. But we have to conform to CSR format, right?
//            Z.SortInner();
            
            this->SetCache( std::string("CyleMatrix"), std::move(Z) );
            
            // Store component matrix.
            
            ComponentMatrix_T C (
                std::move(c_ptr.Get()), 
                std::move(c_idx.Get()), 
                c_ptr.Size()-1, edge_count, Int(1)
            );

            C.SortInner();
            
            this->SetCache( std::string("ComponentEdgeMatrix"), std::move(C) );
            
            
            ptoc( ClassName()+ "::RequireTopology" );
        }
        
    public:
        
        cref<CycleMatrix_T> CycleMatrix( ) const
        {
            std::string tag ("CyleMatrix");
            
            if( !this->InCacheQ( tag ) )
            {
                RequireTopology();
            }

            return this->template GetCache<CycleMatrix_T>(tag);
        }
        
        cref<ComponentMatrix_T> ComponentEdgeMatrix() const
        {
            std::string tag ("ComponentEdgeMatrix");
            
            if( !this->InCacheQ( tag ) )
            {
                RequireTopology();
            }

            return this->template GetCache<ComponentMatrix_T>(tag);
        }
        
        cref<Tensor1<Int,Int>> SpanningTree() const
        {
            std::string tag ( "SpanningTree" );
            
            if( !this->InCacheQ( tag ) )
            {
                RequireTopology();
            }

            return this->template GetCache<Tensor1<Int,Int>>(tag);
        }
        
        cref<ComponentMatrix_T> ComponentVertexMatrix() const
        {
            // Instead of merging this with the spanning tree and the cycle code, we prefer a simpler algorithm here.
            
            std::string tag ("ComponentVertexMatrix");
            
            if( !this->InCacheQ( tag ) )
            {
                
                const Int edge_count = edges.Dimension(0);
                
                if( edge_count <= 0 )
                {
                    ComponentMatrix_T C ( vertex_count, vertex_count, vertex_count, Int(1) );
                    
                    C.Outer(0) = 0;
                    
                    for( Int v = 0; v < vertex_count; ++v )
                    {
                        C.Outer(v+1) = v;
                        C.Inner(v)   = v;
                    }
                    
                    this->SetCache( std::string(tag), std::move(C) );

                    return this->template GetCache<ComponentMatrix_T>(tag);
                }
                
                ptic( ClassName()+ "::ComponentVertexMatrix" );
                
                // Stores the graph's components.
                // c_v_ptr contains a counter for the number of components.
                Aggregator<Int,Int> c_v_ptr ( edge_count + 1 );
                Aggregator<Int,Int> c_v_idx ( edge_count     );

                Tensor1<bool,Int> v_visitedQ ( vertex_count, false );
                Tensor1<bool,Int> e_visitedQ ( edge_count,   false );
                
                // TODO: I can keep e_stack, path, and d_stack uninitialized.
                // Keeps track of the edges we have to process.
                // Every edge can be only explored in two ways: from its two vertices.
                // Thus, the stack does not have to be bigger than 2 * edge_count.
                Stack<Int,Int> e_stack ( edge_count );

                cref<IncidenceMatrix_T> B = IncidenceMatrix();
                
                cptr<Int> B_outer = B.Outer().data();
                cptr<Int> B_inner = B.Inner().data();
                
                Int v_ptr = 0; // Guarantees that every component will be visited.

                while( v_ptr < vertex_count )
                {
                    while( (v_ptr < vertex_count) && (v_visitedQ[v_ptr]) )
                    {
                        ++v_ptr;
                    }
                    
                    if( v_ptr == vertex_count )
                    {
                        break;
                    }
                    
                    // If we arrive here, then `v_ptr` is an unvisited vertex.
                    // We can start a new component.
                    {
                        const Int v = v_ptr;
                        
                        c_v_ptr.Push(c_v_idx.Size());
                        c_v_idx.Push(v);
                        
                        // Tell vertex v that it is visited.
                        v_visitedQ[v] = true;
                        
                        const Int k_begin = B_outer[v    ];
                        const Int k_end   = B_outer[v + 1];
                        
                        // Push all edges incident to `v` onto the stack.
                        // (They must be unvisited because we have not visited this component, yet.)
                        for( Int k = k_end; k --> k_begin; )
                        {
                            const Int n_e = B_inner[k]; // new edge
                            
                            e_stack.Push(n_e);
                        }
                    }
                    
                    while( !e_stack.EmptyQ() )
                    {
                        const Int e = e_stack.Pop();
                        
                        e_visitedQ[e] = true;
                        
                        const Int v = v_visitedQ[edges(e,0)] ? edges(e,1) : edges(e,0);
                        
                        v_visitedQ[v] = true;
                        
                        c_v_idx.Push(v);

                        const Int k_begin = B_outer[v    ];
                        const Int k_end   = B_outer[v + 1];
                        
                        // Push all invistited edges incident to v onto the stack.
                        for( Int k = k_end; k --> k_begin; )
                        {
                            const Int n_e = B_inner[k]; // new edge
                            
                            if( !e_visitedQ[n_e] )
                            {
                                e_stack.Push(n_e);
                            }
                        }
                        
                    }
                    
                } // while( v_ptr < vertex_count )

                c_v_ptr.Push(c_v_idx.Size());
                
                ComponentMatrix_T C (
                    std::move(c_v_ptr.Get()),
                    std::move(c_v_idx.Get()),
                    c_v_ptr.Size()-1, vertex_count, Int(1)
                );

                C.SortInner();
                
                this->SetCache( std::string(tag), std::move(C) );
                
                ptoc( ClassName()+ "::ComponentVertexMatrix" );
            }

            return this->template GetCache<ComponentMatrix_T>(tag);
        }

    public:
                
        static std::string ClassName()
        {
            return std::string("Multigraph")
                + "<" + TypeName<Int>
                + "," + TypeName<SInt>
                + ">";
        }
        
    };
    
} // namespace KnotTools


