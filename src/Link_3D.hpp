#pragma  once

// TODO: Optimize tree for general links?

// TODO: Track the number of edges that cross the boundary of a box.
// TODO: This will allow us to find connected sums.

namespace Knoodle
{
    template<typename Real_ = double, typename Int_ = Int64, typename BReal_ = Real_>
    class alignas( ObjectAlignment ) Link_3D : public Link<Int_>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(FloatQ<BReal_>,"");
        
    public:
        
        using Real  = Real_;
        using Int   = Int_;
        using BReal = BReal_;
        
        // DEBUGGING
        static constexpr Int i_0 = - 1;
        static constexpr Int j_0 = - 1;
        
        static constexpr Scalar::Flag Plus    = Scalar::Flag::Plus;
        static constexpr Scalar::Flag Minus   = Scalar::Flag::Minus;
        static constexpr Scalar::Flag Generic = Scalar::Flag::Generic;
        static constexpr Scalar::Flag Zero    = Scalar::Flag::Zero;
    
        using Base_T         = Link<Int>;
        using Tree_T         = AABBTree<3,Real,Int,BReal,false>;
        using Vector3_T      = Tiny::Vector<3,Real,Int>;
      
//        using Vector3_T      = typename Tree_T::Vector_T;
        using EContainer_T   = typename Tree_T::EContainer_T;
        using BContainer_T   = typename Tree_T::BContainer_T;
        
//        using EContainer_T = std::vector<std::array<Vector3_T,2>>;
        
        using BinaryMatrix_T   = Sparse::BinaryMatrixCSR<Int,Size_T>;
        
    protected:
        
        static constexpr Real zero    = 0;
        static constexpr Real one     = 1;
        static constexpr Real two     = 2;
        static constexpr Real three   = 3;
        
        using Base_T::edges;
        using Base_T::next_edge;
        using Base_T::edge_count;
        using Base_T::component_count;
        using Base_T::component_ptr;
        using Base_T::cyclicQ;
        using Base_T::preorderedQ;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        
//        VContainer_T V_coords;
        EContainer_T E_coords;
        
        Tree_T T;
        
    public:
        
        using Base_T::ComponentCount;
        using Base_T::VertexCount;
        using Base_T::EdgeCount;
        using Base_T::Edges;
        
        using Base_T::ComponentBegin;
        using Base_T::ComponentEnd;
        
    public:
        
        Link_3D() = default;
        
        virtual ~Link_3D() override = default;
                
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I>
        explicit Link_3D( const I edge_count_ )
        :   Base_T   { int_cast<Int>(edge_count_) }
        ,   E_coords { this->EdgeCount(), 2, 3    }
        ,   T        { this->EdgeCount()          }
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template< typename I>
        Link_3D( cptr<Int> edges_, const I edge_count_ )
        :   Base_T   { edges_, edge_count_     }
        ,   E_coords { this->EdgeCount(), 2, 3 }
        ,   T        { this->EdgeCount()       }
        {
            static_assert(IntQ<I>,"");
        }
        
        template< typename I>
        Link_3D( cptr<Real> V_coords_, cptr<Int> edges_, const I edge_count_ )
        :   Base_T   { edges_, edge_count_     }
        ,   E_coords { this->EdgeCount(), 2, 3 }
        ,   T        { this->EdgeCount()       }
        {
            static_assert(IntQ<I>,"");
            
            ReadVertexCoordinates<false>(V_coords_);
        }
        
        
        // This constructor makes the link assume to be a simply cycle and that the vertices are sorted accordingly.
        template< typename I>
        Link_3D( cptr<Real> V_coords_, const I edge_count_ )
        :   Base_T   { edge_count_             }
        ,   E_coords { this->EdgeCount(), 2, 3 }
        ,   T        { this->EdgeCount()       }
        {
            static_assert(IntQ<I>,"");
            
            ReadVertexCoordinates<false>(V_coords_);
        }
        
        Link_3D( cref<Base_T> link )
        :   Base_T   { link                    }
        ,   E_coords { this->EdgeCount(), 2, 3 }
        ,   T        { this->EdgeCount()       }
        {}

    public:
        
        
        static constexpr Int AmbientDimension()
        {
            return 3;
        }
        
        cref<Tree_T> Tree() const
        {
            return T;
        }
        
        cref<EContainer_T> EdgeCoordinates() const
        {
            return E_coords;
        }
        
        template<bool inputSortedQ, typename R>
        void ReadVertexCoordinates( cptr<R> V, mref<EContainer_T> E ) const
        {
//            TOOLS_PTIC(ClassName()+"::ReadVertexCoordinates (AoS)");

            if( inputSortedQ || preorderedQ )
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = e;
                    const Int j = next_edge[e];

                    copy_buffer<3>( &V[3*i] , E.data(e,0,0) );
                    copy_buffer<3>( &V[3*j] , E.data(e,1,0) );
                }
            }
            else
            {
                cptr<Int> tails =  edges[0].data();
                cptr<Int> heads =  edges[1].data();
                
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = tails[e];
                    const Int j = heads[e];

                    copy_buffer<3>( &V[3*i] , E.data(e,0,0) );
                    copy_buffer<3>( &V[3*j] , E.data(e,1,0) );
                }
            }

//            TOOLS_PTOC(ClassName()+"::ReadVertexCoordinates (AoS)");
        }
        
        template<bool inputSortedQ, typename R>
        void ReadVertexCoordinates( cptr<R> V )
        {
            ReadVertexCoordinates<inputSortedQ>( V, E_coords );
        }

        template<bool outputSortedQ = true, typename R>
        void WriteVertexCoordinates( cref<EContainer_T> E, mptr<R> V ) const
        {
            if ( outputSortedQ || preorderedQ )
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    copy_buffer<3>( E.data(e,0,0), &V[3*e] );
                }
            }
            else
            {
                cptr<Int> tails =  edges[0].data();
                
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = tails[e];
                    
                    copy_buffer<3>( E.data(e,0,0), &V[3*i] );
                }
            }
        }
        
        template<typename R, bool outputSortedQ = true >
        void WriteVertexCoordinates( mptr<R> V ) const
        {
            WriteVertexCoordinates<outputSortedQ>( E_coords, V );
        }
        
        
        template<typename R>
        void WriteEdgeCoordinates( cref<EContainer_T> E_coords_, mptr<R> E ) const
        {
            for( Int e = 0; e < EdgeCount(); ++e )
            {
//                copy_buffer<3>( E_coords_.data(e,0,0), &E[6 * e + 0] );
//                copy_buffer<3>( E_coords_.data(e,1,0), &E[6 * e + 3] );
                
                copy_buffer<6>( E_coords_.data(e,0,0), &E[6 * e] );
            }
        }
        
        template<typename R>
        void WriteEdgeCoordinates( mptr<R> E ) const
        {
            WriteEdgeCoordinates( E_coords, E );
        }
        
    public:

        static std::string ClassName()
        {
            return ct_string("Link_3D")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace Knoodle

