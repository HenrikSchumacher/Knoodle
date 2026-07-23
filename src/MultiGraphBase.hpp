#pragma once

namespace Knoodle
{
    // TODO: Make this ready for unsigned integers.
    
    // TODO: Adjacency matrix and graph Laplacian.
    
    // TODO: Create a constructor that eliminates all duplicated edges. (Use the adjacency matrix for that.)
    
    /*!@brief A base class for various (multi-)graph classes.
     *
     * At the moment, this implementation is single-threaded only.
     *
     * @param VInt_ Integral type for vertex indices.
     *
     * @param EInt_ Integral type for edge indices.
     *
     * @param Sign_T_ Singed integral type to store signedness information.
     */
    template<
        IntQ VInt_   = Int64,
        IntQ EInt_   = VInt_,
        SignedIntQ Sign_T_ = Int8,
        Parallel_T parQ_ = Sequential
    >
    class MultiGraphBase : public CachedObject<1,0,0,0>
    {
        
    public:
        
        using Base_T          = CachedObject<1,0,0,0>;

        /*!@brief Integral type for vertex indices.*/
        using VInt            = VInt_;
        /*!@brief Integral type for edge indices.*/
        using EInt            = EInt_;
        /*!@brief Singed integral type to store signedness information.*/
        using Sign_T          = Sign_T_;
        /*!@brief Container used for a single edge.*/
        using Edge_T          = Tiny::Vector<2,VInt,EInt>;
        /*!@brief Container used for storing many edges.*/
        using EdgeContainer_T = Tiny::VectorList_AoS<2,VInt,EInt>;
        
        static constexpr Parallel_T parQ = parQ_;
        
        enum class InOut : Sign_T
        {
            Undirected =  0,
            In         =  1,
            Out        = -1
        };

//        enum class Direction : bool
//        {
//            Forward  = 1,
//            Backward = 0
//        };
        
        using HeadTail_T = bool;
        static constexpr HeadTail_T Tail = 0;
        static constexpr HeadTail_T Head = 1;

        template<IntQ Int, bool nonbinaryQ>
        using SignedMatrix_T = std::conditional_t<
                nonbinaryQ,
                Sparse::MatrixCSR<Sign_T,Int,Int,parQ>,
                Sparse::BinaryMatrixCSR<Int,Int,parQ>
        >;

        using IncidenceMatrix_T = SignedMatrix_T<EInt,1>;

        /*!@brief Container for vertex indices indexed by vertex indices.*/
        using VV_Vector_T       = Tensor1<VInt,VInt>;
        /*!@brief Container for edge indices indexed by edge indices.*/
        using EE_Vector_T       = Tensor1<EInt,EInt>;
        /*!@brief Container for edge indices indexed by vertex indices.*/
        using EV_Vector_T       = Tensor1<EInt,VInt>;
        /*!@brief Container for vertex indices indexed by edge indices.*/
        using VE_Vector_T       = Tensor1<VInt,EInt>;
        
        /*!@brief Value reserved for signalling an unitinialized vertex index.*/
        static constexpr VInt UninitializedVertex = SignedIntQ<VInt> ? VInt(-1) : std::numeric_limits<VInt>::max();
        
        /*!@brief Value reserved for signalling an unitinialized edge index.*/
        static constexpr EInt UninitializedEdge = SignedIntQ<EInt> ? EInt(-1) : std::numeric_limits<EInt>::max();

    public:
        
        // Default constructor
        MultiGraphBase() = default;
        // Destructor (virtual because of inheritance)
        virtual ~MultiGraphBase() = default;
        // Copy constructor
        MultiGraphBase( const MultiGraphBase & other ) = default;
        // Copy assignment operator
        MultiGraphBase & operator=( const MultiGraphBase & other ) = default;
        // Move constructor
        MultiGraphBase( MultiGraphBase && other ) = default;
        // Move assignment operator
        MultiGraphBase & operator=( MultiGraphBase && other ) = default;

        
    public:
        
        /*!@brief Initialize from list of edges in interleaved form.*/
        template<typename I_0, typename I_1, typename I_2>
        MultiGraphBase(
            const I_0 vertex_count_, cptr<I_1> edges_, const I_2 edge_count_
        )
        :   vertex_count ( int_cast<VInt>(vertex_count_)        )
        ,   edges        ( edges_, int_cast<EInt>(edge_count_)  )
        ,   V_scratch    ( vertex_count                         )
        ,   E_scratch    ( edges.Dim(0)                         )
        {
            CheckInputs();
        }
        
        /*!@brief Initialize from lists of edge tails and edge heads. */
        template<IntQ I_0, IntQ I_1>
        MultiGraphBase(
            const I_0 vertex_count_,
            cref<I_0> tails, cref<I_0> heads, const I_1 edge_count_
        )
        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
        ,   edges        ( int_cast<EInt>(edge_count_)      )
        ,   V_scratch    ( vertex_count                     )
        ,   E_scratch    ( edges.Dim(0)                     )
        {
            const EInt m = edges.Dim(0);
            
            for( EInt e = 0; e < m; ++e )
            {
                edges(e,Tail) = tails[e];
                edges(e,Head) = heads[e];
            }

            CheckInputs();
        }

        /*!@brief Initialize from list of edges in interleaved form.*/
        template<IntQ I_0>
        MultiGraphBase( const I_0 vertex_count_, cref<EdgeContainer_T> edges_ )
        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
        ,   edges        ( edges_                           )
        ,   V_scratch    ( vertex_count                     )
        ,   E_scratch    ( edges.Dim(0)                     )
        {
            CheckInputs();
        }
        
        /*!@brief Initialize from list of edges in interleaved form.*/
        template<IntQ I_0>
        MultiGraphBase( const I_0 vertex_count_, EdgeContainer_T && edges_ )
        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
        ,   edges        ( std::move(edges_)                )
        ,   V_scratch    ( vertex_count                     )
        ,   E_scratch    ( edges.Dim(0)                     )
        {
            CheckInputs();
        }
        
        
        /*!@brief Initialize from a `PairAggregator`.*/
        template<IntQ I_0, IntQ I_1>
        MultiGraphBase(
            const I_0 vertex_count_, mref<PairAggregator<I_0,I_0,I_1>> pairs
        )
        :   MultiGraphBase(
               vertex_count_,
               pairs.data_0(), pairs.data_1(), pairs.Size()
            )
        {}
//        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
//        ,   edges        ( int_cast<EInt>(pairs.Size())     )
//        ,   V_scratch    ( vertex_count                     )
//        ,   E_scratch    ( edges.Dim(0)                     )
//        {
//            
//            cptr<I_0> i = pairs.data_0();
//            cptr<I_0> j = pairs.data_1();
//
//            EInt e_count = edges.Dim(0);
//
//            for( EInt e = 0; e < e_count; ++e )
//            {
//                edges(e,Tail) = i[e];
//                edges(e,Head) = j[e];
//            }
//
//            CheckInputs();
//        }



    protected:
        
        VInt vertex_count = 0;
        EdgeContainer_T edges;
        
        // Multiple purpose helper buffer, e.g., for marking visited vertices or edges or for storing labels.
        mutable VV_Vector_T V_scratch;
        mutable EE_Vector_T E_scratch;
        
    public:
        
        /*!@brief Return number of vertices.*/
        VInt VertexCount() const
        {
            return vertex_count;
        }
        
        /*!@brief Return number of edges.*/
        EInt EdgeCount() const
        {
            return edges.Dim(0);
        }
    
        /*!@brief Return the container holding the edges.*/
        cref<EdgeContainer_T> Edges() const
        {
            return edges;
        }
        
        /*!@brief Convert a directed edge index into an undirected one.*/
        static constexpr std::pair<EInt,HeadTail_T> FromDedge( EInt de )
        {
            return std::pair( de / EInt(2), HeadTail_T(de % EInt(2)) );
        }
        
        /*!@brief Convert an undirected edge index to a directed  one.*/
        static constexpr EInt ToDedge( const EInt e, const HeadTail_T d )
        {
            return EInt(2) * e + d;
        }
        
        /*!@brief Convert an undirected edge index to a directed one.*/
        template<HeadTail_T d>
        static constexpr EInt ToDedge( const EInt e )
        {
            return EInt(2) * e + d;
        }
        
        /*!@brief Revert the direction of an undirected edge index.*/
        static constexpr EInt FlipDiEdge( const EInt de )
        {
            return de ^ EInt(1);
        }
        
    protected:

#include "MultiGraphBase/CheckInputs.hpp"
#include "MultiGraphBase/IncidenceMatrices.hpp"
#include "MultiGraphBase/VertexDegree.hpp"
#include "MultiGraphBase/DepthFirstSearch.hpp"
#include "MultiGraphBase/SpanningForest.hpp"
        
#include "MultiGraphBase/AdjacencyMatrix.hpp"
#include "MultiGraphBase/Laplacian.hpp"

    public:
                
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("MultiGraphBase")
                + "<" + TypeName<VInt>
                + "," + TypeName<EInt>
                + "," + TypeName<Sign_T>
                + ">";
        }
        
    };
    
} // namespace Knoodle
