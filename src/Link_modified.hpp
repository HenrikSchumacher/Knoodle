#pragma once

namespace Knoodle
{
    template<typename Int>
    void WriteCircleEdges( mref<Int> edges, const Int first_edge, const Int n )
    {
        if( n < Int(1) ) { return; }
            
        const Int last_edge = first_edge + n - Int(1);
        
        
        for( Int i = 0; i < n; ++i )
        {
            edges( Int(2) * i + Int(0) ) = first_edge + i         ;
            edges( Int(2) * i + Int(1) ) = first_edge + i + Int(1);
        }
        
        edges( Int(2) * n - Int(2) ) = last_edge;
        edges( Int(2) * n - Int(1) ) = first_edge;
    }
    
    template<typename Int>
    Tensor2<Int,Int> CircleEdges( const Int n )
    {
        if( n < Int(1) ) { return Tensor2<Int,Int>(); }
        
        Tensor2<Int,Int> edges ( n, Int(2) );
        
        WriteCircleEdges( edges.data(), Int(0), n );
        
        return edges;
    }
    
    template<typename Int_ = Int64>
    class alignas( ObjectAlignment ) Link
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        static_assert(IntQ<Int_>,"");

    protected:
        
        using Int = Int_;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.

        Int edge_count = 0;

        Tiny::VectorList<2,Int,Int> edges;
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
            
            if( n <= Int(0) ) { return; }
            
            for( Int i = 0; i < n-1; ++i )
            {
                edges     [0][i] = i;
                edges     [1][i] = i+1;
                next_edge [i]    = i+1;
            }
            
            edges     [0][n-1] = n-1;
            edges     [1][n-1] = 0;
            
            next_edge [n-1] = 0;
        }
        
        template<typename J, typename K>
        explicit Link( Tensor1<J,K> & component_ptr_ )
        :   Link(
                (component_ptr_.Size() == J(0))
                ? Int(0)
                : int_cast<Int>(component_ptr_.Last())
            )
        ,   component_ptr( component_ptr_ )
        {
            static_assert(IntQ<J>,"");
            static_assert(IntQ<K>,"");
        
            // AoS = array of structures, i.e., loading data in interleaved form
            
            cyclicQ     = (component_count == Int(1));
            preorderedQ = true;
            
        }
        
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
        :   Link( int_cast<Int>(edge_count_), true )
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
        :   Link( int_cast<Int>(edge_count_), true )
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
        
        template<typename ExtInt>
        void ReadEdges( cptr<ExtInt> edges_ )
        {
            static_assert(IntQ<ExtInt>,"");
            
            // Finding for each e its next e.
            // Caution: Assuming here that link is correctly oriented and that it has no boundaries.
            
            bool in_rangeQ = true;
            
            // using edges.data(0) temporarily as scratch space.
            mptr<Int> tail_of_edge = edges.data(0);

            for( Int e = 0; e < edge_count; ++e )
            {
                const ExtInt tail = edges_[2*e];
                
                in_rangeQ = in_rangeQ && std::in_range<Int>(tail);

                tail_of_edge[tail] = static_cast<Int>(tail);
            }
            
            if( !in_rangeQ )
            {
                eprint(ClassName()+"::ReadEdges: input edges are out of range for type " + TypeName<Int> + "." );
            }

            for( Int e = 0; e < edge_count; ++e )
            {
                const Int tip = edges_[2*e+1];

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
                const Int from = Int(2) * perm[e];

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
            
            Aggregator<Int,Int> agg (2);
            
            agg.Push(0);

            Int visited_edge_counter = 0;

            Tensor1<bool,Int> e_visitedQ( edge_count, false );
            
            for( Int e_0 = 0; e_0 < edge_count; ++e_0 )
            {
                if( e_visitedQ[e_0] ) { continue; }
                
                Int e = e_0;
                do
                {
                    e_visitedQ[e] = true;
                    perm[visited_edge_counter] = e;
                    ++visited_edge_counter;
                    e = next_edge[e];
                }
                while( e != e_0 );

                agg.Push( visited_edge_counter );
                
            }
            
            component_count = Max( Int(0), agg.Size() - Int(1) );
            
            component_ptr = agg.Disband();
        }
        
        void FinishPreparations()
        {
            cyclicQ = (component_count == Int(1));
            
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
        
        cref<Tiny::VectorList<2,Int,Int>> Edges() const
        {
            return edges;
        }
        
        
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

