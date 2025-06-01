#pragma once

namespace Knoodle
{
    template<
        typename VInt_ = Int64, typename EInt_ = VInt_, typename SInt_ = Int8
    >
    class alignas( ObjectAlignment ) MultiGraphBase : public CachedObject
    {
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        static_assert(SignedIntQ<VInt_>,"");
        static_assert(SignedIntQ<EInt_>,"");
        static_assert(SignedIntQ<SInt_>,"");
        
    public:
        
        using Base_T            = CachedObject;
        
        using VInt              = VInt_;
        using EInt              = EInt_;
        using SInt              = SInt_;
        using Edge_T            = Tiny::Vector<2,VInt,EInt>;
        using EdgeContainer_T   = Tiny::VectorList_AoS<2,VInt,EInt>;
        
        enum class InOut : SInt
        {
            Undirected =  0,
            In         =  1,
            Out        = -1
        };
        
        enum class Direction : bool
        {
            Forward  = 1,
            Backward = 0
        };
        
        template<typename Int, bool nonbinaryQ>
        using SignedMatrix_T = std::conditional_t<
                nonbinaryQ,
                Sparse::MatrixCSR<SInt,Int,Int>,
                Sparse::BinaryMatrixCSR<Int,Int>
        >;
        
        using IncidenceMatrix_T = SignedMatrix_T<EInt,1>;
        
        using VV_Vector_T       = Tensor1<VInt,VInt>;
        using EE_Vector_T       = Tensor1<EInt,EInt>;
        using EV_Vector_T       = Tensor1<EInt,VInt>;
        using VE_Vector_T       = Tensor1<VInt,EInt>;
        
        static constexpr bool Tail = 0;
        static constexpr bool Head = 1;
        
    protected:
        
        VInt vertex_count;
        EdgeContainer_T edges;
        
        // Multiple purpose helper buffer, e.g., for marking visited vertices or edges or for storing labels.
        mutable VV_Vector_T V_scratch;
        mutable EE_Vector_T E_scratch;

    public:
        
        MultiGraphBase() = default;
        
        virtual ~MultiGraphBase() override = default;

        
        // Provide a list of edges in interleaved form.
        template<typename I_0, typename I_1, typename I_2>
        MultiGraphBase(
            const I_0 vertex_count_, cptr<I_1> edges_, const I_2 edge_count_
        )
        :   vertex_count ( int_cast<VInt>(vertex_count_)        )
        ,   edges        ( edges_, int_cast<EInt>(edge_count_)  )
        ,   V_scratch    ( vertex_count                         )
        ,   E_scratch    ( edges.Dimension(0)                   )
        {
            CheckInputs();
        }
        
        
        // Provide a list of edges in interleaved form.
        template<typename I_0>
        MultiGraphBase( const I_0 vertex_count_, EdgeContainer_T && edges_ )
        :   vertex_count ( vertex_count_        )
        ,   edges        ( std::move(edges_)    )
        ,   V_scratch    ( vertex_count         )
        ,   E_scratch    ( edges.Dimension(0)   )
        {
            if( edges.Dimension(1) != 2 )
            {
                wprint( this->ClassName()+"(): Second dimension of input tensor is not equal to 0." );
            }
            
            CheckInputs();
        }
        
        
        // Copy constructor
        MultiGraphBase( const MultiGraphBase & other ) = default;
//        MultiGraphBase( const MultiGraphBase & other )
//        :   Base_T( static_cast<const Base_T &>(other) )
//        {}
        
        friend void swap( MultiGraphBase & A, MultiGraphBase & B ) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( static_cast<Base_T &>(A), static_cast<Base_T &>(B) );
            swap( A.vertex_count, B.vertex_count );
            swap( A.edges       , B.edges        );
            swap( A.V_scratch   , B.V_scratch    );
            swap( A.E_scratch   , B.E_scratch    );
        }
        
        // Move constructor
        MultiGraphBase( MultiGraphBase && other ) noexcept
        :   MultiGraphBase()
        {
            print("MultiGraphBase move constructor");
            swap(*this, other);
        }

//        /* Copy assignment operator */
//        MultiGraphBase & operator=( MultiGraphBase other ) noexcept
//        {   //                                   ^
//            //                                   |
//            // Use the copy constructor   -------+
//            swap( *this, other );
//            return *this;
//        }
        
    protected:
        
        void CheckInputs() const
        {
            TOOLS_PTIMER( timer, ClassName() + "::CheckInputs" );
            
            const EInt edge_count = edges.Dimension(0);
            
            for( EInt e = 0; e < edge_count; ++e )
            {
                const VInt e_0 = edges(e,Tail);
                const VInt e_1 = edges(e,Head);


                if( e_0 < VInt(0) )
                {
                    eprint("MultiGraphBase:  first entry " + ToString(e_0) + " of edge " + ToString(e) + " is negative.");

                    return;
                }
                
                if( e_0 >= vertex_count )
                {
                    eprint("MultiGraphBase:  first entry " + ToString(e_0) + " of edge " + ToString(e) + " is greater or equal to vertex_count = " + ToString(vertex_count) + ".");

                    return;
                }

                if( e_1 < VInt(0) )
                {
                    eprint("MultiGraphBase: second entry " + ToString(e_1) + " of edge " + ToString(e) + " is negative.");
                    return;
                }
                
                if( e_1 >= vertex_count )
                {
                    eprint("MultiGraphBase: second entry " + ToString(e_1) + " of edge " + ToString(e) + " is greater or equal to vertex_count = " + ToString(vertex_count) + ".");

                    return;
                }
            }
        }
        
    public:
        
        VInt VertexCount() const
        {
            return vertex_count;
        }
        
        EInt EdgeCount() const
        {
            return edges.Dimension(0);
        }
    
        cref<EdgeContainer_T> Edges() const
        {
            return edges;
        }
        
    protected:
        
#include "MultiGraphBase/IncidenceMatrices.hpp"
#include "MultiGraphBase/VertexDegree.hpp"
#include "MultiGraphBase/DepthFirstSearch.hpp"

    public:
                
        static std::string ClassName()
        {
            return ct_string("MultiGraphBase")
                + "<" + TypeName<VInt>
                + "," + TypeName<EInt>
                + "," + TypeName<SInt>
                + ">";
        }
        
    };
    
} // namespace Knoodle


