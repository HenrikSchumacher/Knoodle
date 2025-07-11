#pragma once

namespace Knoodle
{
    // TODO: Make this ready for unsigned integers.

    
    template<
        typename VInt_   = Int64,
        typename EInt_   = VInt_,
        typename Sign_T_ = Int8
    >
    class alignas( ObjectAlignment ) MultiGraph : public MultiGraphBase<VInt_,EInt_,Sign_T_>
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
                
    public:
        
        using Base_T            = MultiGraphBase<VInt_,EInt_,Sign_T_>;
        using VInt              = Base_T::VInt;
        using EInt              = Base_T::EInt;
        using Sign_T            = Base_T::Sign_T;
        using HeadTail_T        = Base_T::HeadTail_T;
        using Edge_T            = Base_T::Edge_T;
        using EdgeContainer_T   = Base_T::EdgeContainer_T;
        using InOut             = Base_T::InOut;
        using IncidenceMatrix_T = Base_T::IncidenceMatrix_T;
        
        using VV_Vector_T       = Base_T::VV_Vector_T;
        using EE_Vector_T       = Base_T::EE_Vector_T;
        using EV_Vector_T       = Base_T::EV_Vector_T;
        using VE_Vector_T       = Base_T::VE_Vector_T;
        
        using CycleMatrix_T     = Sparse::MatrixCSR<Sign_T,EInt,EInt>;
        
        using ComponentMatrix_T = Sparse::BinaryMatrixCSR<VInt,VInt>;

        using Base_T::Tail;
        using Base_T::Head;
        using Base_T::TrivialEdgeFunction;
        using Base_T::UninitializedVertex;
        using Base_T::UninitializedEdge;
        
        template<typename Int,bool nonbinQ>
        using SignedMatrix_T = Base_T::template SignedMatrix_T<Int,nonbinQ>;
        
    protected:
        
        using Base_T::vertex_count;
        using Base_T::edges;
        using Base_T::V_scratch;
        using Base_T::E_scratch;

    public:
        
        // Default constructor
        MultiGraph() = default;
        // Destructor (virtual because of inheritance)
        virtual ~MultiGraph() override  = default;
        // Copy constructor
        MultiGraph( const MultiGraph & other ) = default;
        // Copy assignment operator
        MultiGraph & operator=( const MultiGraph & other ) = default;
        // Move constructor
        MultiGraph( MultiGraph && other ) = default;
        // Move assignment operator
        MultiGraph & operator=( MultiGraph && other ) = default;

        
        // Provide a list of edges in interleaved form.
        template<typename I_0, typename I_1, typename I_2>
        MultiGraph(
           const I_0 vertex_count_, cptr<I_1> edges_, const I_2 edge_count_
        )
        :   Base_T( vertex_count_, edges_, edge_count_ )
        {}
        
        // Provide list of edges in noninterleaved form.
        template<typename I_0, typename I_1>
        MultiGraph(
            const I_0 vertex_count_,
            cref<I_0> tails, cref<I_0> heads, const I_1 edge_count_
        )
        :   Base_T( vertex_count_, tails, heads, edge_count_ )
        {}
        

        // Copy an EdgeContainer_T.
        template<typename I_0>
        MultiGraph( const I_0 vertex_count_, cref<EdgeContainer_T> edges_ )
        :   Base_T( vertex_count_, edges_ )
        {}
        
        // Provide an EdgeContainer_T. Caution: this destroys the container.
        template<typename I_0>
        MultiGraph( const I_0 vertex_count_, EdgeContainer_T && edges_ )
        :   Base_T( vertex_count_, std::move(edges_) )
        {}
        
        // Provide a list of edges by a PairAggregator.
        template<typename I_0, typename I_1>
        MultiGraph(
            const I_0 vertex_count_, mref<PairAggregator<I_0,I_0,I_1>> pairs
        )
        :   Base_T( vertex_count_, pairs )
        {}

        
    protected:
        
    public:
        
        using Base_T::VertexCount;
        using Base_T::EdgeCount;
        using Base_T::Edges;
        using Base_T::OrientedIncidenceMatrix;
        using Base_T::InIncidenceMatrix;
        using Base_T::OutIncidenceMatrix;
        
        using Base_T::VertexDegree;
        using Base_T::VertexInDegree;
        using Base_T::VertexOutDegree;
        
        using Base_T::VertexDegrees;
        using Base_T::VertexInDegrees;
        using Base_T::VertexOutDegrees;
        
    public:
        
        template<typename Scal = std::make_signed_t<EInt>>
        cref<Sparse::MatrixCSR<Scal,VInt,EInt>> AdjacencyMatrix() const
        {
            return this->template UndirectedAdjacencyMatrix<Scal>();
        }
        
        template<typename Scal = std::make_signed_t<EInt>>
        cref<Sparse::MatrixCSR<Scal,VInt,EInt>> Laplacian() const
        {
            const std::string tag = std::string("Laplacian<") + TypeName<Scal> + ">";
            
            if(!this->InCacheQ(tag))
            {
                this->SetCache(tag,this->template CreateLaplacian<true,Scal>());
            }
            
            return this->template GetCache<cref<Sparse::MatrixCSR<Scal,VInt,EInt>>>(tag);
        }
        
        
        // TODO: This function is not correct, I think. It does not account for single-vertex components!
        
        // TODO: Rewrite this with `DepthFirstSearch`.
        
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
            this->SetCache( std::string("SpanningForest"), std::move(v_parents) );
            
            
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
        
        cref<ComponentMatrix_T> ComponentVertexMatrix() const
        {
            // Instead of merging this with the spanning tree and the cycle code, we prefer a simpler algorithm here.
            
            std::string tag ("ComponentVertexMatrix");
            
            if( !this->InCacheQ(tag) )
            {
                const EInt edge_count = edges.Dim(0);
                
                if( edge_count <= 0 )
                {
                    this->SetCache(
                        std::string(tag),
                        ComponentMatrix_T::IdentityMatrix(vertex_count)
                    );

                    return this->template GetCache<ComponentMatrix_T>(tag);
                }
                
                TOOLS_PTIMER(timer,ClassName()+"::ComponentVertexMatrix");
                
                // Stores the graph's components.
                // c_v_ptr contains a counter for the number of components.
                Aggregator<VInt,EInt> c_v_ptr ( edge_count + 1 );
                Aggregator<VInt,EInt> c_v_idx ( edge_count     );

                mptr<bool> V_visitedQ = reinterpret_cast<bool *>(V_scratch.data());
                fill_buffer( V_visitedQ, false, vertex_count );
                
                mptr<bool> E_visitedQ = reinterpret_cast<bool *>(E_scratch.data());
                fill_buffer( E_visitedQ, false, edge_count );
                
                // TODO: I can keep e_stack, path, and d_stack uninitialized.
                // Keeps track of the edges we have to process.
                // Every edge can be only explored in two ways: from its two vertices.
                // Thus, the stack does not have to be bigger than 2 * edge_count.
                Stack<EInt,EInt> e_stack ( edge_count );

                cref<SignedMatrix_T<EInt,1>> B = OrientedIncidenceMatrix();
                
                cptr<EInt> B_outer = B.Outer().data();
                cptr<VInt> B_inner = B.Inner().data();
                
                VInt v_ptr = 0; // Guarantees that every component will be visited.

                while( v_ptr < vertex_count )
                {
                    while( (v_ptr < vertex_count) && (V_visitedQ[v_ptr]) )
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
                        const VInt v = v_ptr;
                        
                        c_v_ptr.Push(c_v_idx.Size());
                        c_v_idx.Push(v);
                        
                        // Tell vertex v that it is visited.
                        V_visitedQ[v] = true;
                        
                        const EInt k_begin = B_outer[v    ];
                        const EInt k_end   = B_outer[v + 1];
                        
                        // Push all edges incident to `v` onto the stack.
                        // (They must be unvisited because we have not visited this component, yet.)
                        for( EInt k = k_end; k --> k_begin; )
                        {
                            const EInt n_e = B_inner[k]; // new edge
                            
                            e_stack.Push(n_e);
                        }
                    }
                    
                    while( !e_stack.EmptyQ() )
                    {
                        const EInt e = e_stack.Pop();
                        
                        V_visitedQ[e] = true;
                        
                        const VInt v = V_visitedQ[edges(e,Tail)]
                                      ? edges(e,Head)
                                      : edges(e,Tail);
                        
                        V_visitedQ[v] = true;
                        
                        c_v_idx.Push(v);

                        const EInt k_begin = B_outer[v    ];
                        const EInt k_end   = B_outer[v + 1];
                        
                        // Push all unvisited edges incident to v onto the stack.
                        for( EInt k = k_end; k --> k_begin; )
                        {
                            const EInt n_e = B_inner[k]; // new edge
                            
                            if( !E_visitedQ[n_e] )
                            {
                                e_stack.Push(n_e);
                            }
                        }
                        
                    }
                    
                } // while( v_ptr < vertex_count )

                c_v_ptr.Push(c_v_idx.Size());
                
                ComponentMatrix_T C (
                    c_v_ptr.Disband(),
                    c_v_idx.Disband(),
                    c_v_ptr.Size()-VInt(1), vertex_count, VInt(1)
                );

                C.SortInner();
                
                this->SetCache( std::string(tag), std::move(C) );
            }

            return this->template GetCache<ComponentMatrix_T>(tag);
        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
                
        static std::string ClassName()
        {
            return ct_string("MultiGraph")
                + "<" + TypeName<VInt>
                + "," + TypeName<EInt>
                + "," + TypeName<Sign_T>
                + ">";
        }
        
    };
    
} // namespace Knoodle


