#pragma once

namespace KnotTools
{
    
    template<typename Int_ = long long>
    class alignas( ObjectAlignment ) Multigraph : public CachedObject
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        ASSERT_SIGNED_INT(Int_);
                
    public:
        
        
        using Int  = Int_;
        using SInt = signed char;
        
        using IncidenceMatrix_T  = Sparse::MatrixCSR<SInt,Int,Int>;
        
        using CycleMatrix_T      = Sparse::MatrixCSR<SInt,Int,Int>;
        
        using ComponentMatrix_T  = Sparse::BinaryMatrixCSR<Int,Int>;
    protected:
        
        Tensor2<Int,Int> edges;
        
        Int max_vertex;
        Int vertex_count;
        
        IncidenceMatrix_T B ; // oriented incidence matrix.
        IncidenceMatrix_T BT; // transpose of oriented incidence matrix.

    public:
        
        Multigraph() = default;
        
        ~Multigraph() = default;
        
        
        
        // Provide a list of edges in interleaved form.
        template<typename I_0, typename I_1, typename I_2>
        Multigraph( cptr<I_0> edges_, const I_1 edge_count_ )
        :   edges( edges_, edge_count_ )
        {
            const Int edge_count = static_cast<Int>( edge_count_ );
            
            max_vertex = 0;
            
            for( Int e = 0; e < edge_count; ++e )
            {
                const Int e_0 = edges[e][0];
                const Int e_1 = edges[e][1];
                
                if( e_0 < 0 )
                {
                    eprint("Multigraph:  first entry of edge " + ToString(e) " is negative.");
                }
                
                if( e_1 < 0 )
                {
                    eprint("Multigraph: second entry of edge " + ToString(e) " is negative.");
                }
                
                max_vertex = Max( max_vertex, Max( e_0, e_1 ) );
            }
            
            Tensor1< Int,Int> rp  ( edge_count + 1          );
            Tensor1< Int,Int> ci  ( 2 * edge_count          );
            Tensor1<SInt,Int> val ( 2 * edge_count, SInt(0) );
            
            rp[0] = 0;
            
            Int nnz = 0
            
            for( Int e = 0; e < edge_count; ++e )
            {
                rp[e+1] = 2 * e;
                
                Int e_0 = edges[ 2 * e + 0 ];
                Int e_1 = edges[ 2 * e + 1 ];
                
                if( e_0 < e_1 )
                {
                    ci [ nnz + 0 ] = e_0;
                    ci [ nnz + 1 ] = e_1;
                    val[ nnz + 0 ] = SInt(-1);
                    val[ nnz + 1 ] = SInt( 1);
                    
                    nnz += 2;
                }
                else if( e_0 > e_1 )
                {
                    ci [ nnz + 0 ] = e_1;
                    ci [ nnz + 1 ] = e_0;
                    val[ nnz + 0 ] = SInt( 1);
                    val[ nnz + 1 ] = SInt(-1);
                    
                    nnz += 2;
                }
                else // if( e_0 == e_1 )
                {
                    ci [ nnz + 0 ] = e_0;
                    val[ nnz + 0 ] = SInt( 0); // Putting an explicit zero for the loop.
                    
                    nnz += 1;
                }
            }
            
            ci.Resize(nnz);
            val.Resize(nnz);
            
            Sparse::MatrixCSR<SInt,Int,Int> B (
                std::move(rp), std::move(ci), std::move(val), edge_count, max_vertex, Int(1)
            );
            
            BT = B.Transpose();
            
        }
        
        
    public:
        
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        Int MaxVertexLabel() const
        {
            return max_vertex;
        }
        
        Int EdgeCount() const
        {
            return edges.Dimension(0);
        }
        
    
        cref<Tensor2<Int,Int>> Edges() const
        {
            return edges;
        }
        
        cref<IncidenceMatrix_T> IncidenceMatrix() const
        {
            return B;
        }
        
        cref<IncidenceMatrix_T> TransposedIncidenceMatrix() const
        {
            return BT;
        }
        
        
    private:
        
        
        void RequireTopology()
        {
            ptic( ClassName()+ "::RequireTopology" );
            // v stands for "vertex"
            // e stands for "edge"
            // d stands for "direction"
            // c stands for "component"
            // z stands for "cycle" (German "Zykel")
            
            // TODO: Make it work with unsigned integers.
            // v_parent[v] stores the parent vertex in the spanning forest.
            // v_parent[v] == -2 means that vertex v does not exist.
            // v_parent[v] == -1 means that vertex v is a root vertex.
            // v_parent[v] >=  0 means that vertex v_parent[v] is v's parent.
            Tensor1< Int,Int> v_parent ( max_vertex, -2 );
            
            // Stores the graph's components.
            Int c = 0;
            Tensor1< Int,Int> c_ptr ( edge_count );
            Tensor1< Int,Int> c_idx ( edge_count );
            Tensor1<SInt,Int> c_val ( edge_count );
            c_ptr[0] = 0;
            
            // Stores the graph's cycles.
            Int z = 0;
            Tensor1< Int,Int> z_ptr ( edge_count );
            Tensor1< Int,Int> z_idx ( edge_count );
            Tensor1<SInt,Int> z_val ( edge_count );
            zycle_ptr[0] = 0;
            
            // TODO: Make it work with unsigned integers.
            // Stores position of vertex on the stack and tracks whether vertex is visited:
            // v is unvisited if v_flag[v] == -2.
            // If v_flag[v] > -2, then at some time it was pointed to by the edge at  position v_flag[v] on the stack (in downward direction of the generated spanning tree. v_flag[v] == -1 means that v is the root vertex in the spanning tree of its connected component.
            //
            Tensor1< Int,Int> v_flags ( max_vertex, -2 );
            
            // Tracks which edges have been visited.
            // e_flags[e] == 0 means unvisited.
            // e_flags[e] == 1 means explored.
            // e_flags[e] == 2 means visited (and completed).
            Tensor1<SInt,Int> e_flags ( edge_count, 0 );
            
            // Keeps track on the edges we travelled through.
            Tensor1< Int,Int> e_stack ( edge_count );
            
            // Keeps track on the directions we travelled through the edges.
            Tensor1<SInt,Int> d_stack ( edge_count );
            
            Int e_ptr = 0; // Guarantees that every component will be visited.
            
            Int e_ctr = 0; // counts the edges already visited.
            
            while( e_ptr < edge_count )
            {
                while( (e_ptr < edge_count) && (e_flags[e_ptr] > 0) )
                {
                    ++e_ptr;
                }
                
                // Now e_ptr is an unvisited edge.
                // We can start a new component.
                
                Int c_e_counter = 0;
                
                const Int root = edges(e_ptr,0);
                
                v_parent[root] = -1;
                
                Int stack_ptr = -1;
                
                // Tell vertex root that it is the tip of edge at position -1 on the stack.
                v_flags[root] = -1;
                
                // Push all edges incident to v onto the stack.
                for( Int k = BT.Outer(root); k < BT.Outer(root+1); ++k )
                {
                    // Push edge onto stack.
                    e_stack[stack_ptr] = BT.Inner(k); // edge
                    d_stack[stack_ptr] = BT.Value(k); // direction (0 for loop)
                    
                    ++stack_ptr;
                }
                
                while( stack_ptr > -1 )
                {
                    // Top
                    const Int e = e_stack[stack_ptr];
                    
                    if( e_flags[e] == 1 )
                    {
                        // Pop
                        --stack_ptr;
                        
                        // Tell the currect component that e is its member.
                        c_idx[e_ctr] = e;
                        
                        ++e_ctr;
                    }
                    
                    // Mark edge e as visited.
                    e_flags[e] = 1;
                    
                    const Int d = d_stack[stack_ptr];
                    const Int v = (d>0) ? edges(e,1) : edges(e,0);
                    const Int w = (d>0) ? edges(e,0) : edges(e,1);
                    const Int f = f_flags[v];
                    
                    if( f < 0 )
                    {
                        // Vertex v is undiscovered.
                        
                        v_flags[v] = stack_ptr;
                        v_parent[v] = w;
                        
                        const Int k_begin = BT.Outer(v  );
                        const Int k_end   = BT.Outer(v+1);
                        
                        // TODO: I could pop the stack if k_end == k_begin + 1.
                        
                        // Push all edges incident to v except e onto the stack.
                        for( Int k = k_begin; k < k_end; ++k )
                        {
                            const Int n_e = BT.Inner(k); // new edge
                            
                            if( n_e != e )
                            {
                                // Push new edge onto stack.
                                e_stack[stack_ptr] = n_e;
                                d_stack[stack_ptr] = BT.Value(k);
                                
                                ++stack_ptr;
                            }
                        }
                    }
                    else
                    {
                        // Vertex v is already visited.
                        
                        // Create a new cycle.
                        
                        const Int pos = f + 1; // position of cycle's first edge on stack.
                        
                        const Int z_length = stack_ptr - f );
                        
                        const Int z_begin  = z_ptr[z]
                        
                        z_ptr[c+1] = z_begin + z_length;
                        
                        copy_buffer( &e_stack[pos], &z_idx[z_begin], z_length );
                        copy_buffer( &d_stack[pos], &z_val[z_begin], z_length );
                        
                        ++z;
                        
                        // TODO: I could pop the stack right now.
                    }
                    
                } // while( stack_ptr > -1 )
                
                // Increase component counter by 1.
                ++c;
                c_ptr[c] = e_ctr;
                
                
            } // while( e_ptr < edge_count )
            
            // Now all components and cycles have been explored.
            
            // Store spanning forest.
            this->SetCache( std::move(v_parents), std::string("SpanningForest") );
            
            
            // Store cycle matrix.
            
            z_ptr.Resize(z+1);
            z_idx.Resize(z_ptr[z]);
            z_val.Resize(z_ptr[z]);
            
            CycleMatrix_T Z (
                std::move(z_ptr), std::move(z_idx), std::move(z_val), z, edge_count, 1
            );
            
            // TODO: I don't like the idea to lose the information on the ordering of edges in the cycles. But we have to conform to CSR format, right?
            Z.SortInner();
            
            this->SetCache( std::move(Z), std::string("CyleMatrix") );
            
            
            // Store component matrix.
            
            c_ptr.Resize(c+1);
            c_idx.Resize(c_ptr[c]);
            c_val.Resize(c_ptr[c]);
            
            ComponentMatrix_T C (
                std::move(c_ptr), std::move(c_idx), c, edge_count, 1
            );
            
            // TODO: I don't like the idea to lose the information on the ordering of edges in the cycles. But we have to conform to CSR format, right?
            C.SortInner();
            
            this->SetCache( std::move(C), std::string("ComponentMatrix") );
            
            
            ptoc( ClassName()+ "::RequireTopology" );
        }
        
    public:
        
        cref<CycleMatrix_T> CycleMatrix()
        {
            std::string tag ( "CycleMatrix" );
            
            if( !this->InCache( tag ) )
            {
                RequireTopology();
            }

            return this->template GetCache<CycleMatrix_T>(tag)
        }
        
        cref<ComponentMatrix_T> ComponentMatrix()
        {
            std::string tag ( "ComponentMatrix" );
            
            if( !this->InCache( tag ) )
            {
                RequireTopology();
            }

            return this->template GetCache<ComponentMatrix_T>(tag)
        }
        
        cref<Tensor1<Int,Int>> SpanningTree()
        {
            std::string tag ( "SpanningTree" );
            
            if( !this->InCache( tag ) )
            {
                RequireTopology();
            }

            return this->template GetCache<Tensor1<Int,Int>>(tag)
        }
        
    public:
                
        static std::string ClassName()
        {
            return std::string("Multigraph")+"<"+TypeName<Int>+">";
        }
    };
    
} // namespace KnotTools


