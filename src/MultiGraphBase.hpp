#pragma once

namespace Knoodle
{
    // TODO: Make this ready for unsigned integers.
    
    // TODO: Adjacency matrix and graph Laplacian.
    
    // TODO: Create a constructor that eliminates all duplicated edges. (Use the adjacency matrix for that.)
    
    template<
        typename VInt_   = Int64,
        typename EInt_   = VInt_,
        typename Sign_T_ = Int8
    >
    class alignas( ObjectAlignment ) MultiGraphBase : public CachedObject
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        static_assert(IntQ<VInt_>,"");
        static_assert(IntQ<EInt_>,"");
        static_assert(SignedIntQ<Sign_T_>,"");
        
    public:
        
        using Base_T          = CachedObject;

        using VInt            = VInt_;
        using EInt            = EInt_;
        using Sign_T          = Sign_T_;
        using Edge_T          = Tiny::Vector<2,VInt,EInt>;
        using EdgeContainer_T = Tiny::VectorList_AoS<2,VInt,EInt>;
        
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

        template<typename Int, bool nonbinaryQ>
        using SignedMatrix_T = std::conditional_t<
                nonbinaryQ,
                Sparse::MatrixCSR<Sign_T,Int,Int>,
                Sparse::BinaryMatrixCSR<Int,Int>
        >;

        using IncidenceMatrix_T = SignedMatrix_T<EInt,1>;

        using VV_Vector_T       = Tensor1<VInt,VInt>;
        using EE_Vector_T       = Tensor1<EInt,EInt>;
        using EV_Vector_T       = Tensor1<EInt,VInt>;
        using VE_Vector_T       = Tensor1<VInt,EInt>;
         
        static constexpr VInt UninitializedVertex = SignedIntQ<VInt> ? VInt(-1) : std::numeric_limits<VInt>::max();
        
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
        
        // Provide a list of edges in interleaved form.
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
        
        // Provide list of edges in noninterleaved form.
        template<typename I_0, typename I_1>
        MultiGraphBase(
            const I_0 vertex_count_,
            cref<I_0> tails, cref<I_0> heads, const I_1 edge_count_
        )
        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
        ,   edges        ( int_cast<EInt>(edge_count_)      )
        ,   V_scratch    ( vertex_count                     )
        ,   E_scratch    ( edges.Dim(0)                     )
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
            
            const EInt m = edges.Dim(0);
            
            for( EInt e = 0; e < m; ++e )
            {
                edges(e,Tail) = tails[e];
                edges(e,Head) = heads[e];
            }

            CheckInputs();
        }

        // Provide a list of edges in interleaved form.
        template<typename I_0>
        MultiGraphBase( const I_0 vertex_count_, cref<EdgeContainer_T> edges_ )
        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
        ,   edges        ( edges_                           )
        ,   V_scratch    ( vertex_count                     )
        ,   E_scratch    ( edges.Dim(0)                     )
        {
            CheckInputs();
        }
        
        // Provide a list of edges in interleaved form.
        template<typename I_0>
        MultiGraphBase( const I_0 vertex_count_, EdgeContainer_T && edges_ )
        :   vertex_count ( int_cast<VInt>(vertex_count_)    )
        ,   edges        ( std::move(edges_)                )
        ,   V_scratch    ( vertex_count                     )
        ,   E_scratch    ( edges.Dim(0)                     )
        {
            CheckInputs();
        }
        
        
        // Provide a list of edges by a PairAggregator.
        template<typename I_0, typename I_1>
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
//            static_assert(IntQ<I_0>,"");
//            static_assert(IntQ<I_1>,"");
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
        
        VInt VertexCount() const
        {
            return vertex_count;
        }
        
        EInt EdgeCount() const
        {
            return edges.Dim(0);
        }
    
        cref<EdgeContainer_T> Edges() const
        {
            return edges;
        }
        
        // TODO: These things would be way faster if EInt where unsigned.
        static constexpr std::pair<EInt,HeadTail_T> FromDedge( EInt de )
        {
            return std::pair( de / EInt(2), HeadTail_T(de % EInt(2)) );
        }
        
        static constexpr EInt ToDedge( const EInt e, const HeadTail_T d )
        {
            return EInt(2) * e + d;
        }
        
        template<HeadTail_T d>
        static constexpr EInt ToDedge( const EInt e )
        {
            return EInt(2) * e + d;
        }
        
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
