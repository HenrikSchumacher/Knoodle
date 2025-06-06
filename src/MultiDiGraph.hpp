#pragma once

namespace Knoodle
{
    // TODO: Make MultiGraphBase ready for unsigned integers.

    template<
        typename VInt_ = Int64, typename EInt_ = VInt_, typename Sign_T_ = Int8
    >
    class alignas( ObjectAlignment ) MultiDiGraph : public MultiGraphBase<VInt_,EInt_,Sign_T_>
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
        
        template<typename Int,bool nonbinQ>
        using SignedMatrix_T = Base_T::template SignedMatrix_T<Int,nonbinQ>;
        
        using Base_T::Tail;
        using Base_T::Head;
        using Base_T::TrivialEdgeFunction;
        using Base_T::UninitializedVertex;
        using Base_T::UninitializedEdge;
        
    protected:
        
        using Base_T::vertex_count;
        using Base_T::edges;
        using Base_T::V_scratch;
        using Base_T::E_scratch;
        
        mutable bool proven_acyclicQ = false;
        mutable bool proven_cyclicQ  = false;
        
    public:
        
        MultiDiGraph() = default;
        
        virtual ~MultiDiGraph() override = default;

        
        // Provide a list of edges in interleaved form.
        template<typename I_0, typename I_1, typename I_2>
        MultiDiGraph(
            const I_0 vertex_count_,
            cptr<I_1> edges_,
            const I_2 edge_count_
        )
        :   Base_T( vertex_count_, edges_, edge_count_ )
        {}
        
        
        // Provide a list of edges in interleaved form.
        template<typename I_0>
        MultiDiGraph(
            const I_0 vertex_count_,
            EdgeContainer_T && edges_
        )
        :   Base_T( vertex_count_, std::move(edges_) )
        {}
        

        // Copy constructor
        MultiDiGraph( const MultiDiGraph & other ) = default;
        
//        MultiDiGraph( const MultiDiGraph & other )
//        :   Base_T( static_cast<const Base_T &>(other) )
//        {}
        
        friend void swap( MultiDiGraph & A, MultiDiGraph & B ) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( static_cast<Base_T &>(A), static_cast<Base_T &>(B) );
        }
        
        // Move constructor
        MultiDiGraph( MultiDiGraph && other ) noexcept
        :   MultiDiGraph()
        {
            swap(*this, other);
        }

        /* Copy assignment operator */
        MultiDiGraph & operator=( MultiDiGraph other ) noexcept
        {   //                                   ^
            //                                   |
            // Use the copy constructor   -------+
            swap( *this, other );
            return *this;
        }
        
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
        
        bool ProvenAcyclicQ() const
        {
            return proven_acyclicQ;
        }
        
        bool ProvenCyclicQ() const
        {
            return proven_cyclicQ;
        }

#include "MultiDiGraph/KahnsAlgorithm.hpp"
        
        
    public:
                
        static std::string ClassName()
        {
            return ct_string("MultiDiGraph")
                + "<" + TypeName<VInt>
                + "," + TypeName<EInt>
                + "," + TypeName<Sign_T>
                + ">";
        }
        
    };
    
} // namespace Knoodle


