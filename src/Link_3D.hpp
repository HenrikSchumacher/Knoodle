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
        
        // Default constructor
        Link_3D() = default;
        // Destructor (virtual because of inheritance)
        virtual ~Link_3D() = default;
        // Copy constructor
        Link_3D( const Link_3D & other ) = default;
        // Copy assignment operator
        Link_3D & operator=( const Link_3D & other ) = default;
        // Move constructor
        Link_3D( Link_3D && other ) = default;
        // Move assignment operator
        Link_3D & operator=( Link_3D && other ) = default;
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I>
        explicit Link_3D( const I edge_count_ )
        :   Base_T   { int_cast<Int>(edge_count_) }
        ,   E_coords { this->EdgeCount()          }
        ,   T        { this->EdgeCount()          }
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template< typename I>
        Link_3D( cptr<Int> edges_, const I edge_count_ )
        :   Base_T   { edges_, edge_count_     }
        ,   E_coords { this->EdgeCount()       }
        ,   T        { this->EdgeCount()       }
        {
            static_assert(IntQ<I>,"");
        }
        
        template< typename I>
        Link_3D( cptr<Real> V_coords_, cptr<Int> edges_, const I edge_count_ )
        :   Base_T   { edges_, edge_count_     }
        ,   E_coords { this->EdgeCount()       }
        ,   T        { this->EdgeCount()       }
        {
            static_assert(IntQ<I>,"");
            
            ReadVertexCoordinates<false>(V_coords_);
        }
        
        
        // This constructor makes the link assume to be a simple cycle and that the vertices are sorted accordingly.
        template< typename I>
        Link_3D( cptr<Real> V_coords_, const I edge_count_ )
        :   Base_T   { edge_count_             }
        ,   E_coords { this->EdgeCount()       }
        ,   T        { this->EdgeCount()       }
        {
            static_assert(IntQ<I>,"");
            
            ReadVertexCoordinates<false>(V_coords_);
        }
        
        Link_3D( cref<Base_T> link )
        :   Base_T   { link                    }
        ,   E_coords { this->EdgeCount()       }
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
            if( inputSortedQ || preorderedQ )
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = e;
                    const Int j = next_edge[e];

                    copy_buffer<3>( &V[3*i] , E.data(e,0) );
                    copy_buffer<3>( &V[3*j] , E.data(e,1) );
                }
            }
            else
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = edges(e,0);
                    const Int j = edges(e,1);

                    copy_buffer<3>( &V[3*i] , E.data(e,0) );
                    copy_buffer<3>( &V[3*j] , E.data(e,1) );
                }
            }
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
                    copy_buffer<3>( E.data(e,0), &V[Int(3) * e] );
                }
            }
            else
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = edges(e,0);
                    
                    copy_buffer<3>( E.data(e,0), &V[Int(3) * i] );
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
            const Int m = EdgeCount();
            
            for( Int e = 0; e < m; ++e )
            {
//                copy_buffer<3>( E_coords_.data(e,0,0), &E[6 * e + 0] );
//                copy_buffer<3>( E_coords_.data(e,1,0), &E[6 * e + 3] );
                
                copy_buffer<6>( E_coords_.data(e,0), &E[6 * e] );
            }
        }
        
        template<typename R>
        void WriteEdgeCoordinates( mptr<R> E ) const
        {
            WriteEdgeCoordinates( E_coords, E );
        }
        
        static void ProjectPointToEdge(
            cptr<Real> x, cptr<Real> E, mref<Real> t, cptr<Real> y, mref<Real> dist
        )
        {
            Vector3_T P_0 ( &E[0] );
            Vector3_T P_1 ( &E[3] );
            Vector3_T X   ( x );
            
            Vector3_T u = P_1 - P_0;
            Vector3_T v = X  - P_0;
            
            Real uu = u.SquaredNorm();
            
            if( uu <= Real(0) )
            {
                t = Real(0);
                P_0.Write(y);
                dist = v.Norm();
            }
         
            t = Dot(u,v) / uu;
            
            if( ( Real(0) <= t) && (t <= Real(1)) )
            {
                Vector3_T Y = P_0 + t * u;
                Y.Write(y);
                Vector3_T w = Y - X;
                dist = w.Norm();
            }
            else
            {
                const Real d2_0 = v.SquaredNorm();
                
                Vector3_T w = X - P_1;
                const Real d2_1 = w.SquaredNorm();
                
                if( d2_0 <= d2_1 )
                {
                    t = 0;
                    P_0.Write(y);
                    dist = Sqrt(d2_0);
                }
                else
                {
                    t = 1;
                    P_1.Write(y);
                    dist = Sqrt(d2_1);
                }
            }
        }
        
        void ProjectPointToEdge(
            cptr<Real> x, Int i, mref<Real> t, cptr<Real> y, mref<Real> dist
        )
        {
            ProjectPointToEdge(x,E_coords.data(i),t,y,dist);
        }
        
//        void EdgeEdgeClosestPoints(
//            cptr<Real> E_0,
//            cptr<Real> E_1,
//            mref<Real> s, cptr<Real> x,
//            mref<Real> t, cptr<Real> y,
//            mref<Real> dist
//        )
//        {
//            Vector3_T P_0 ( &E_0[0] );
//            Vector3_T P_1 ( &E_0[3] );
//            Vector3_T Q_0 ( &E_1[0] );
//            Vector3_T Q_1 ( &E_1[3] );
//            
//            Vector3_T u = P_1 - P_0;
//            Vector3_T v = Q_0 - Q_1;
//            Vector3_T w = Q_0 - P_0;
//            
////            f(s,t) = Dot( (P_0 + s * u) - (Q_0 - t * v), (P_0 + s * u) - (Q_0 - t * v) );
//            
////            df/fs = 2 * Dot( u, P_0 - Q_0 + s * u + t * v );
////            df/ft = 2 * Dot( v, P_0 - Q_0 + s * u + t * v );
//            
////            Dot( u, P_0 - Q_0 ) + Dot( u, u ) * s + Dot( u, v ) * t == 0
////            Dot( v, P_0 - Q_0 ) + Dot( v, u ) * s + Dot( v, v ) * t == 0
//            
////            Dot( u, u ) * s + Dot( u, v ) * t == Dot( u, Q_0 - P_0 )
////            Dot( v, u ) * s + Dot( v, v ) * t == Dot( v, Q_0 - P_0 )
//            
//            const Real uu = Dot(u,u);
//            const Real uv = Dot(u,v);
//            const Real vv = Dot(v,v);
////            Tiny::Matrix<2,2,Real,Int> A ( { {Dot(u,u), uv}, {uv, Dot(v,v)} } );
//            Real det = Det2D_Kahan( uu, uv, uv, vv );
//            // TODO: Check whether det == 0.
//            
//            Real det_inv = Inv(det);
//            
//            Tiny::Vector<2,  Real,Int> b ( { Dot(u,w), Dot(v,w) } );
//            s = (vv * b[0] - uv * b[1]) * det_inv;
//            t = (uv * b[0] - uu * b[1]) * det_inv;
//            
//            
//        }
        

        
    public:

        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
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

