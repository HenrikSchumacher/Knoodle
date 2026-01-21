#pragma once

namespace Knoodle
{
    template<typename Int_ = Int64>
    class alignas( ObjectAlignment ) Link
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        static_assert(IntQ<Int_>,"");

    public:
        
        using Int = Int_;
        
        using EdgeContainer_T = Tiny::VectorList_AoS<2,Int,Int>;
        
    protected:
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.

        Int edge_count = 0;

        EdgeContainer_T edges;
        Tensor1<Int,Int> next_edge;
        Tensor1<Int,Int> edge_ptr;

        Int component_count = 0;
        
        Tensor1<Int,Int> component_ptr;
        Tensor1<Int,Int> component_lookup;
        
        bool cyclicQ      = false;
        bool preorderedQ  = false;

    public:
        
        // Default constructor
        Link() = default;
        // Destructor (virtual because of inheritance)
        virtual ~Link() = default;
        // Copy constructor
        Link( const Link & other ) = default;
        // Copy assignment operator
        Link & operator=( const Link & other ) = default;
        // Move constructor
        Link( Link && other ) = default;
        // Move assignment operator
        Link & operator=( Link && other ) = default;
        
    private:
        
        /*! @brief This constructor only allocates the internal arrays. Only for internal use.
         */
        
        template<typename I_0 >
        explicit Link( const I_0 edge_count_, bool dummy )
        :   edge_count      { int_cast<Int>(edge_count_) }
        ,   edges           { edge_count     }
        ,   next_edge       { edge_count     }
        ,   edge_ptr        { edge_count + 1 }
        
        ,   component_count { 1              }
        ,   component_ptr   { 2              }
        ,   component_lookup{ edge_count     }
        {
            (void)dummy;
        }
        
    public:
        
        // TODO: Make this constructor work correctly!
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I_0 >
        explicit Link( const I_0 edge_count_ )
        :   edge_count      { int_cast<Int>(edge_count_) }
        ,   edges           { edge_count            }
        ,   next_edge       { edge_count            }
        ,   edge_ptr        { edge_count + Int(1)   }
        
        ,   component_count { Int(1)                }
        ,   component_ptr   { Int(2)                }
        ,   component_lookup{ edge_count, Int(0)    }
        ,   cyclicQ         { true                  }
        ,   preorderedQ     { true                  }
        {
//            TOOLS_PTIMER(timer,ClassName()+"( " + ToString(edge_count_) + " ) (cyclic)");
            
            static_assert(IntQ<I_0>,"");
            
            const Int n = edge_count;
            
            component_ptr[0] = 0;
            
            component_ptr[1] = n;
            
            for( Int i = 0; i < n-1; ++i )
            {
                edges(i,0)   = i;
                edges(i,1)   = i+1;
                next_edge[i] = i+1;
            }
            
            edges(n-1,0)   = n-1;
            edges(n-1,1)   = 0;
            next_edge[n-1] = 0;
        }
        
        /*! @brief Initialize from component pointers.
         *
         * @param component_ptr_ An array that indicates how many connected components there are which vertices are contained in each component. In the case of k components the array must have size `k+1`. The entries must be increasing integers starting at 0. Then the vertices of the i-th component are `component_ptr_[i],...,component_ptr_[i-1]-1`.
         */
        
        template<typename J, typename K>
        explicit Link( cref<Tensor1<J,K>> component_ptr_ )
        :   Link{
                (component_ptr_.Size() < Int(2)) ? 0 : component_ptr_.Last(),
                false // Just allocate buffers. We fill them manually.
            }
        {
            static_assert(IntQ<J>,"");
            static_assert(IntQ<K>,"");
            
            component_ptr = component_ptr_;
            
            if( component_ptr.Size() < Int(2) ) { return; }
            
            component_count = component_ptr.Size() - Int(1);
            cyclicQ         = (component_count == Int(1));
            preorderedQ     = true;
            
            for( Int comp = 0; comp < component_count; ++comp )
            {
                const Int v_begin = component_ptr[comp         ];
                const Int v_end   = component_ptr[comp + Int(1)];
                
                const Int comp_size = v_end - v_begin;
                
                WriteCircleEdges(&edges(v_begin,0),v_begin,comp_size);
                
                for( Int v = v_begin; v < v_end; ++v )
                {
                    component_lookup[v] = comp;
                    
//                    // DEBUGGING
//                    if( v != edges(v,0) )
//                    {
//                        eprint("!!!");
//                        TOOLS_DDUMP(v);
//                        TOOLS_DDUMP(edges(v,0));
//                        TOOLS_DDUMP(edges(v,1));
//                        
//                        logvalprint("edges", ArrayToString(&edges(v_begin,0),{comp_size,Int(2)})
//                        );
//                    }
                    next_edge[v] = edges(v,1);
                }
            }
            
        }
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        
        /*! @brief Construct oriented `Link` from a list of oriented edges.
         *
         *  @param edges_ Array of integers of length `2 * edge_count_`. Entries at odd positions are treated as tails; entries at even positions are treated as heads.
         *
         *  @param edge_count_ Number of edges.
         *
         */
        
        template<typename I_0, typename I_1>
        Link( cptr<I_0> edges_, const I_1 edge_count_ )
        :   Link( int_cast<Int>(edge_count_), true )
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
            
            ReadEdges( edges_ );
        }
        
        /*! @brief Construct oriented `Link` from a list of tails and from a list of heads.
         *
         *  @param edge_tails_ Array of integers of length `edge_count_`. Entries are treated as tails of edges.
         *
         *  @param edge_heads_ Array of integers of length `edge_count_`. Entries are treated as heads of edges.
         *
         *  @param edge_count_ Number of edges.
         */
        
        template<typename I_0, typename I_1>
        Link( cptr<I_0> edge_tails_, cptr<I_0> edge_heads_, const I_1 edge_count_ )
        :   Link( int_cast<Int>(edge_count_), true )
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
            
            ReadEdges( edge_tails_, edge_heads_ );
        }

    public:
        
        
        bool PreorderedQ() const
        {
            return preorderedQ;
        }
        
        bool CyclicQ() const
        {
            return cyclicQ;
        }
        
        
        /*! @brief Reads edges from the array `edges_`.
         *
         *  @param edges_ Integer array of length `2 * EdgeCount()`.
         */
        
        template<typename ExtInt>
        void ReadEdges( cptr<ExtInt> edges_ )
        {
            static_assert(IntQ<ExtInt>,"");
            
            [[maybe_unused]] auto tag = [](){ return MethodName("ReadEdges");};
            
            TOOLS_PTIMER(timer,tag());
        
            // Finding for each e its next e.
            // Caution: Assuming here that link is correctly oriented and that it has no boundaries.
            
            // using edges(...,0) temporarily as scratch space.
//            mptr<Int> tail_of_edge = edges.data(0);

            for( Int e = 0; e < edge_count; ++e )
            {
                const ExtInt tail = edges_[Int(2) * e];

                if( !std::in_range<Int>(tail) )
                {
                    error(tag()+": index tail is out of range for type " + TypeName<Int> + " (tail = " + ToString(tail) + ").");
                }
                if( std::cmp_less(tail, ExtInt(0)) )
                {
                    error(tag()+": tail < 0 (tail = " + ToString(tail) + ").");
                }
                if( std::cmp_greater_equal(tail,edge_count) )
                {
                    error(tag()+": tail >= edge_count (tail = " + ToString(tail) + ", edge_count = " + ToString(edge_count) + ").");
                }
                
                edges(tail,0) = static_cast<Int>(tail);
            }

            for( Int e = 0; e < edge_count; ++e )
            {
                const ExtInt head = edges_[Int(2) * e + Int(1)];
                
                if( !std::in_range<Int>(head) )
                {
                    error(tag()+": index head is out of range for type " + TypeName<Int> + " (head = " + ToString(head) + ").");
                }
                if( std::cmp_less(head, ExtInt(0)) )
                {
                    error(tag()+": head < 0 (head = " + ToString(head) + ").");
                }
                if( std::cmp_greater_equal(head,edge_count) )
                {
                    error(tag()+": head >= edge_count (head = " + ToString(head) + ", edge_count = " + ToString(edge_count) + ").");
                }
                
                next_edge[e] = edges(static_cast<Int>(head),0);
            }
            
            FindComponents();

            // using edge_ptr temporarily as scratch space.
            cptr<Int> perm = edge_ptr.data();
            
            // Reordering edges.
            for( Int e = 0; e < edge_count; ++e )
            {
                const Int from = Int(2) * perm[e];

                edges(e,0) = edges_[from  ];
                edges(e,1) = edges_[from+1];
            }
            
            FinishPreparations();
        }

        /*! @brief Reads edges from the arrays `edge_tails_` and `edge_heads_`.
         *
         *  @param edge_tails_ Integer array of length `EdgeCount()` that contains the list of tails.
         *
         *  @param edge_heads_ Integer array of length `EdgeCount()` that contains the list of heads.
         */
        
        template<typename ExtInt>
        void ReadEdges( cptr<ExtInt> edge_tails_, cptr<ExtInt> edge_heads_ )
        {
            static_assert(IntQ<ExtInt>,"");
            
            [[maybe_unused]] auto tag = [](){ return MethodName("ReadEdges");};
            
            TOOLS_PTIMER(timer,tag());
            
            // Finding for each e its next e.
            // Caution: Assuming here that link is correctly oriented and that it has no boundaries.
            
            // using edges.data(0) temporarily as scratch space.
            mptr<ExtInt> tail_of_edge = edges.data(0);
            
            for( Int e = 0; e < edge_count; ++e )
            {
                const ExtInt tail = edge_tails_[e];

                if( !std::in_range<Int>(tail) )
                {
                    error(tag()+": index tail is out of range for type " + TypeName<Int> + " (tail = " + ToString(tail) + ").");
                }
                if( std::cmp_less(tail, ExtInt(0)) )
                {
                    error(tag()+": tail < 0 (tail = " + ToString(tail) + ").");
                }
                if( std::cmp_greater_equal(tail,edge_count) )
                {
                    error(tag()+": tail >= edge_count (tail = " + ToString(tail) + ", edge_count = " + ToString(edge_count) + ").");
                }
                
                tail_of_edge[static_cast<Int>(tail)] = e;
            }

            for( Int e = 0; e < edge_count; ++e )
            {
                const ExtInt head = edge_heads_[e];
                
                if( !std::in_range<Int>(head) )
                {
                    error(tag()+": head tail is out of range for type " + TypeName<Int> + " (head = " + ToString(head) + ").");
                }
                if( std::cmp_less(head, ExtInt(0)) )
                {
                    error(tag()+": tail < 0 (head = " + ToString(head) + ").");
                }
                if( std::cmp_greater_equal(head,edge_count) )
                {
                    error(tag()+": head >= edge_count (head = " + ToString(head) + ", edge_count = " + ToString(edge_count) + ").");
                }

                next_edge[e] = tail_of_edge[static_cast<Int>(head)];
            }
            
            FindComponents();
            
            // using edge_ptr temporarily as scratch space.
            cptr<Int> perm       = edge_ptr.data();
            mptr<Int> edge_tails = edges.data(0);
            mptr<Int> edge_heads = edges.data(1);
            
            // Reordering edges.
            for( Int e = 0; e < edge_count; ++e )
            {
                const Int from = perm[e];

                edge_tails[e] = edge_tails_[from];
                edge_heads[e] = edge_heads_[from];
            }
            
            FinishPreparations();
        }
        
    protected:
        
        void FindComponents()
        {
            TOOLS_PTIMER(timer,MethodName("FindComponents"));
            
            // TODO: FindComponents goes nuts if we supply a 1D simplicial complex that is not closed. Add some tests and error messages here!
            
            // using edge_ptr temporarily as scratch space.
            mptr<Int> perm = edge_ptr.data();
            
            Aggregator<Int,Int> agg ( Int(2) );
            agg.Push(0);

            Int visited_edge_counter = 0;

            Tensor1<bool,Int> edge_visited( edge_count, false );
            
            for( Int e = 0; e < edge_count; ++e )
            {
                if( edge_visited[e] ) { continue; }

                const Int start_edge = e;

                Int current_edge = e;
                
                do
                {
                    edge_visited[current_edge] = true;
                    perm[visited_edge_counter] = current_edge;
                    ++visited_edge_counter;
                    current_edge = next_edge[current_edge];
                }
                while( current_edge != start_edge );
                    
                agg.Push( visited_edge_counter );
            }

            component_ptr = agg.Disband();

            component_count = component_ptr.Size() > Int(0)
                            ? component_ptr.Size() - Int(1)
                            : Int(0);
        }
        
        void FinishPreparations()
        {
            TOOLS_PTIMER(timer,MethodName("FinishPreparations"));
            
            cyclicQ = (component_count == Int(1));
            
            bool b = true;
            
//            cptr<Int> edge_tails = edges.data(0);
            
            for( Int i = 0; i < edge_count; ++i )
            {
                b = b && (edges(i,0) == i);
            }
            
            preorderedQ = b;
            
            for( Int c = 0; c < component_count; ++ c )
            {
                const Int i_begin = component_ptr[c  ];
                const Int i_end   = component_ptr[c+1];
                
                for( Int i = i_begin; i < i_end-1; ++i )
                {
                    next_edge       [i  ] = i+1;
                    edge_ptr        [i+1] = i  ;
                    component_lookup[i  ] = c;
                }
                
                next_edge       [i_end-1] = i_begin;
                component_lookup[i_end-1] = c;
            }
        }
        
        void ComputeComponentLookup()
        {
            for( Int c = 0; c < component_count; ++ c )
            {
                const Int begin = component_ptr[c  ];
                const Int end   = component_ptr[c+1];
                
                std::fill( &component_lookup[begin], &component_lookup[end], c );
            }
        }
        
    public:
        
        /*! @brief Returns the number of components of the link.
         */
        
        Int ComponentCount() const
        {
            return static_cast<Int>(component_ptr.Size()-1);
        }
        
        /*! @brief This returns the component in which vertex `i` lies.
         */
        
        Int ComponentLookup( const Int i ) const
        {
            return (cyclicQ) ? Int(0) : component_lookup[i];
        }
        
        /*! @brief Returns the first vertex in component `c`.
         */
        
        Int ComponentBegin( const Int c ) const
        {
            return component_ptr[c];
        }
                
        /*! @brief Returns the first vertex in component `c`.
         */
        
        Int ComponentEnd( const Int c ) const
        {
            return component_ptr[c+1];
        }
        
        cref<Tensor1<Int,Int>> ComponentPointers() const
        {
            return component_ptr;
        }
        
        /*! @brief Returns the number of vertices in component `c`.
         */
        
        Int ComponentSize( const Int c ) const
        {
            return component_ptr[c+1] - component_ptr[c];
        }
        
        /*! @brief Returns the total number of vertices.
         */
        
        Int VertexCount() const
        {
            return edge_count;
        }
        
        /*! @brief Returns the number of vertices in component `c`.
         */
        
        Int VertexCount( const Int c ) const
        {
            return component_ptr[c+1] - component_ptr[c];
        }
        
        /*! @brief Returns the total number of edges.
         */
        
        Int EdgeCount() const
        {
            return edge_count;
        }
        
        /*! @brief Returns the number of edges in component `c`.
         */
        
        Int EdgeCount( const Int c ) const
        {
            return component_ptr[c+1] - component_ptr[c];
        }
        
        
        /*! @brief Checks whether edges `i` and `j` are neighbors of each other.
         */
        
        bool EdgesAreNeighborsQ( const Int i, const Int j ) const
        {
            return (i == j) || (i == next_edge[j]) || (j == next_edge[i]);
        }

        
        cref<Tensor1<Int,Int>> NextEdge() const
        {
            return next_edge;
        }
        
        
        /*! @brief Returns the edge that follows `i` when traversing `i`'s link component in positive orientation.
         */
        
        Int NextEdge( const Int i ) const
        {
            return next_edge[i];
        }

//        EdgeContainer_T ExportEdges() const
//        {
//            TOOLS_PTIMER(timer,MethodName("ExportEdges"));
//            
//            EdgeContainer_T e ( edge_count, Int(2) );
//
//            for( Int i = 0; i < edge_count; ++i )
//            {
//                e(i,0) = edges[0][i];
//                e(i,1) = edges[1][i];
//            }
//            
//            return e;
//        }
        
//        EdgeContainer_T ExportSortedEdges() const
//        {
//            TOOLS_PTIMER(timer,MethodName("ExportSortedEdges"));
//            
//            EdgeContainer_T e ( edge_count );
//            
//            for( Int c = 0; c < component_count; ++c )
//            {
//                const Int i_begin = component_ptr[c  ];
//                const Int i_end   = component_ptr[c+1];
//                
//                for( Int i = i_begin; i < i_end-1; ++i )
//                {
//                    e(i,0) = i  ;
//                    e(i,1) = i+1;
//                }
//                
//                e(i_end-1,0) = i_end-1;
//                e(i_end-1,1) = i_begin;
//            }
//            
//            return e;
//        }

    
        /*! @brief Returns a reference to the list of edges.
         *
         *  The tail of the edge `i` is given by `Edges()(i,0)`.
         *  The head of the edge `i` is given by `Edges()(i,1)`.
         */
        
        cref<EdgeContainer_T> Edges() const
        {
            return edges;
        }
        
        template<typename Int>
        static void WriteCircleEdges(
            mptr<Int> e_ptr, const Int first_edge, const Int n
        )
        {
            if( n <= Int(0) ) { return; }
            
            const Int last_edge = first_edge + n - Int(1);
            
            for( Int i = 0; i < n - Int(1); ++i )
            {
                e_ptr[Int(2) * i + Int(0)] = first_edge + i         ;
                e_ptr[Int(2) * i + Int(1)] = first_edge + i + Int(1);
            }
            
            e_ptr[Int(2) * n - Int(2)] = last_edge;
            e_ptr[Int(2) * n - Int(1)] = first_edge;
        }
        
        
//        template<typename Int>
//        static EdgeContainer_T CircleEdges( const Int n )
//        {
//            EdgeContainer_T edges ( n );
//            
//            WriteCircleEdges(edges.data(),Int(0),n);
//            
//            return edges;
//        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*! @brief Returns the name of the class, including template parameters.
         *
         *  Used for logging, profiling, and error handling.
         */

        static std::string ClassName()
        {
            return ct_string("Link") + "<" + TypeName<Int> + ">";
        }
    };
    
} // namespace Knoodle

