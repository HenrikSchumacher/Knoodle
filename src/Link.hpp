#pragma once

namespace KnotTools
{
    template<typename Int>
    Tensor2<Int,Int> CircleEdges( const Int n )
    {
        Tensor2<Int,Int> e ( n, 2 );
        
        if( n >= 1 )
        {
            for( Int i = 0; i < n-1; ++i )
            {
                e(i,0) = i  ;
                e(i,1) = i+1;
            }
            
            e(n-1,0) = n-1;
            e(n-1,1) = 0;
        }
        
        return e;
    }
    
    template<typename Int_ = long long>
    class alignas( ObjectAlignment ) Link
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        static_assert(IntQ<Int_>,"");

    protected:
        
        using Int = Int_;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.

        const Int edge_count = 0;
        
        Tiny::VectorList<2,Int,Int> edges;
        Tensor1<Int,Int> next_edge;
        Tensor1<Int,Int> edge_ptr;

        Int component_count = 0;
        
        Tensor1<Int,Int> component_ptr;
        
        
        Tensor1<Int,Int> component_lookup;
        
        bool cyclicQ      = false;
        bool preorderedQ  = false;

    public:
        
        Link() = default;
        
        virtual ~Link() = default;
        
        
    private:
        
        /*! @brief This constructor only allocates the internal arrays. Only for internal use.
         */
        
        template<typename I_0 >
        explicit Link( const I_0 edge_count_, bool dummy )
        :   edge_count      { static_cast<Int>(edge_count_) }
        ,   edges           { edge_count     }
        ,   next_edge       { edge_count     }
        ,   edge_ptr        { edge_count + 1 }
        
        ,   component_count { 1              }
        ,   component_ptr   { 2              }
        ,   component_lookup{ edge_count     }
        {}
        
    public:
        
        // TODO: Make this constructor work correctly!
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I_0 >
        explicit Link( const I_0 edge_count_ )
        :   edge_count      { static_cast<Int>(edge_count_) }
        ,   edges           { edge_count     }
        ,   next_edge       { edge_count     }
        ,   edge_ptr        { edge_count + 1 }
        
        ,   component_count { 1              }
        ,   component_ptr   { 2              }
        ,   component_lookup{ edge_count, 0  }
        ,   cyclicQ         { true           }
        ,   preorderedQ     { true           }
        {
//            ptic(ClassName()+"( " + ToString(edge_count_) + " ) (cyclic)");
            
            static_assert(IntQ<I_0>,"");
            
            const Int n = edge_count;
            
            component_ptr[0] = 0;
            
            component_ptr[1] = n;
            
            for( Int i = 0; i < n-1; ++i )
            {
                edges     [0][i] = i;
                edges     [1][i] = i+1;
                next_edge [i]    = i+1;
            }
            
            edges     [0][n-1] = n-1;
            edges     [1][n-1] = 0;
            
            next_edge [n-1] = 0;
            
//            ptoc(ClassName()+"( " + ToString(edge_count_) + " ) (cyclic)");
        }
        
//        template<typename J, typename K>
//        explicit Link( Tensor1<J,K> & component_ptr_ )
//        :   Link(
//                (component_ptr_.Size() == 0) ? 0 : component_ptr_.Last()
//            )
//        ,   component_ptr( component_ptr_.data(), component_ptr_.Size() )
//        {
//            static_assert(IntQ<J>,"");
//            static_assert(IntQ<K>,"");
        
//            // AoS = array of structures, i.e., loading data in interleaved form
////            ptic(ClassName()+"() (preordered)");
//            
//            cyclicQ     = (component_count == static_cast<Int>(1));
//            preorderedQ = true;
//            
////            ptoc(ClassName()+"() (preordered)");
//        }
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        
        /*! @brief Construct oriented `Link` from a list of oriented edges.
         *
         *  @param edges_ Array of integers of length `2 * edge_count_`. Entries at odd positions are treated as tails; entries at even positions are treated as tips.
         *
         *  @param edge_count_ Number of edges.
         *
         */
        
        template<typename I_0, typename I_1>
        Link( cptr<I_0> edges_, const I_1 edge_count_ )
        :   Link( static_cast<Int>(edge_count_), true )
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
            
            ReadEdges( edges_ );
        }
        
        /*! @brief Construct oriented `Link` from a list of tails and from a list of tips.
         *
         *  @param edge_tails_ Array of integers of length `edge_count_`. Entries are treated as tails of edges.
         *
         *  @param edge_tips_ Array of integers of length `edge_count_`. Entries are treated as tips of edges.
         *
         *  @param edge_count_ Number of edges.
         */
        
        template<typename I_0, typename I_1>
        Link( cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, const I_1 edge_count_ )
        :   Link( static_cast<Int>(edge_count_), true )
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
            
            ReadEdges( edge_tails_, edge_tips_ );
        }

    public:
        
        /*! @brief Reads edges from the array `edges_`.
         *
         *  @param edges_ Integer array of length `2 * EdgeCount()`.
         */
        
        template<typename I>
        void ReadEdges( cptr<I> edges_ )
        {
            static_assert(IntQ<I>,"");
            
            // Finding for each e its next e.
            // Caution: Assuming here that link is correctly oriented and that it has no boundaries.
            
            // using edges.data(0) temporarily as scratch space.
            mptr<Int> tail_of_edge = edges.data(0);

            for( Int e = 0; e < edge_count; ++e )
            {
                const Int tail = static_cast<Int>(edges_[2*e]);

                tail_of_edge[tail] = e;
            }

            for( Int e = 0; e < edge_count; ++e )
            {
                const Int tip = edges_[2*e+1];

                next_edge[e] = static_cast<Int>(tail_of_edge[tip]);
            }
            
            FindComponents();

            // using edge_ptr temporarily as scratch space.
            cptr<Int> perm       = edge_ptr.data();
            mptr<Int> edge_tails = edges.data(0);
            mptr<Int> edge_tips  = edges.data(1);
            
            // Reordering edges.
            for( Int e = 0; e < edge_count; ++e )
            {
                const Int from = 2*perm[e];

                edge_tails[e] = edges_[from  ];
                edge_tips [e] = edges_[from+1];
            }
            
            FinishPreparations();
        }

        /*! @brief Reads edges from the arrays `edge_tails_` and `edge_tips_`.
         *
         *  @param edge_tails_ Integer array of length `EdgeCount()` that contains the list of tails.
         *
         *  @param edge_tips_ Integer array of length `EdgeCount()` that contains the list of tips.
         */
        
        void ReadEdges( cptr<Int> edge_tails_, cptr<Int> edge_tips_ )
        {
            // Finding for each e its next e.
            // Caution: Assuming here that link is correctly oriented and that it has no boundaries.
            
            // using edges.data(0) temporarily as scratch space.
            mptr<Int> tail_of_edge = edges.data(0);
            
            for( Int e = 0; e < edge_count; ++e )
            {
                const Int tail = edge_tails_[e];

                tail_of_edge[tail] = e;
            }

            for( Int e = 0; e < edge_count; ++e )
            {
                const Int tip = edge_tips_[e];

                next_edge[e] = tail_of_edge[tip];
            }

            FindComponents();
            
            // using edge_ptr temporarily as scratch space.
            cptr<Int> perm       = edge_ptr.data();
            mptr<Int> edge_tails = edges.data(0);
            mptr<Int> edge_tips  = edges.data(1);
            
            // Reordering edges.
            for( Int e = 0; e < edge_count; ++e )
            {
                const Int from = perm[e];

                edge_tails[e] = edge_tails_[from];
                edge_tips [e] = edge_tips_ [from];
            }
            
            FinishPreparations();
        }
        
    protected:
        
        void FindComponents()
        {
            // Finding components.
            
            // using edge_ptr temporarily as scratch space.
            mptr<Int> perm = edge_ptr.data();
            
            std::vector<Int> comp_ptr;
            
            comp_ptr.push_back(0);

            Int visited_edge_counter = 0;

            std::vector<bool> edge_visited( edge_count, false );
            
            for( Int e = 0; e < edge_count; ++e )
            {
                if( edge_visited[e] )
                {
                    continue;
                }
                else
                {
                    const Int start_edge = e;

                    Int current_edge = e;

                    edge_visited[current_edge] = true;
                    perm[visited_edge_counter] = current_edge;
                    ++visited_edge_counter;
                    current_edge = next_edge[current_edge];

                    bool finished = current_edge == start_edge;

                    while( !finished )
                    {
                        edge_visited[current_edge] = true;
                        perm[visited_edge_counter] = current_edge;
                        ++visited_edge_counter;

                        current_edge = next_edge[current_edge];

                        finished = current_edge == start_edge;
                    }

                    comp_ptr.push_back( visited_edge_counter );
                }
            }
            
            component_count = Max(
                static_cast<Int>(0),
                static_cast<Int>(comp_ptr.size()) - static_cast<Int>(1)
            );
            
            component_ptr = Tensor1<Int,Int>( &comp_ptr[0], component_count+1 );
        }
        
        void FinishPreparations()
        {
            cyclicQ = (component_count == static_cast<Int>(1));
            
            bool b = true;
            
            cptr<Int> edge_tails = edges.data(0);
            
            for( Int i = 0; i < edge_count; ++i )
            {
                b = b && (edge_tails[i] == i);
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
            return (cyclicQ) ? static_cast<Int>(0) : component_lookup[i];
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
        
        /*! @brief Returns the first vertex _after_ component `c`.
         */
        
        const Tensor1<Int,Int> & ComponentPointers() const
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
        
        Tensor2<Int,Int> ExportEdges() const
        {
            Tensor2<Int,Int> e ( edge_count, 2 );
            
            for( Int i = 0; i < edge_count; ++i )
            {
                e(i,0) = edges[0][i];
                e(i,1) = edges[1][i];
            }
            
            return e;
        }
        
        Tensor2<Int,Int> ExportSortedEdges() const
        {
            Tensor2<Int,Int> e ( edge_count, 2 );
            
            for( Int c = 0; c < component_count; ++c )
            {
                const Int i_begin = component_ptr[c  ];
                const Int i_end   = component_ptr[c+1];
                
                for( Int i = i_begin; i < i_end-1; ++i )
                {
                    e(i,0) = i  ;
                    e(i,1) = i+1;
                }
                
                e(i_end-1,0) = i_end-1;
                e(i_end-1,1) = i_begin;
            }
            
            return e;
        }

    
        /*! @brief Returns a reference to the list of edges.
         *
         *  The tail of the edge `i` is given by `Edges()[0][i]`.
         *  The tip  of the edge `i` is given by `Edges()[1][i]`.
         */
        
        const Tiny::VectorList<2,Int,Int> & Edges() const
        {
            return edges;
        }
        
        
    public:
        
        /*! @brief Returns the name of the class, including template parameters.
         *
         *  Used for logging, profiling, and error handling.
         */

        static std::string ClassName()
        {
            return std::string("Link")+"<"+TypeName<Int>+">";
        }
    };
    
} // namespace KnotTools

