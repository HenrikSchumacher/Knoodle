#pragma  once

namespace Knoodle
{
//    template<FloatQ Real_ = double, IntQ Idx_ = Int64 /*, IntQ BReal_ = float*/>
    class alignas( ObjectAlignment ) LinkEmbedding_Int : public Link<Idx_>
    {
        // This data type is mostly intended to read in 3D vertex coordinates, to apply a planar projection and compute the crossings. Then it can be handed over to class PlanarDiagram. Hence, this class' main routine is RequireIntersections (using a static binary tree).
        
        
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
    public:
        
//        using Real  = Real_;
//        using Idx   = Idx_;
//        using BReal = Real_;
//        using BReal = BReal_;
        
        // TODO: Allow also integer types for Real -> lattice knots.
        // TODO: When Real coincides with double or float,  loading coordinates in
  
        using Real = double;
        using Idx  = Int64;
        using Int  = Int64;
        using SInt = Int64;
        
        using Base_T          = Link<Idx>;
        using LinkEmbedding_T = LinkEmbedding_Int;

        
        using Tree2_T         = AABBTree_Int<2,Int,Idx,SInt,false>;
        using Tree3_T         = AABBTree_Int<3,Int,Idx,SInt,false>;

//        using RealVector2_T   = Tiny::Vector<2,Real,Idx>;
        using RealVector3_T   = Tiny::Vector<3,Real,Idx>;
        
//        using IntVector2_T    = Tiny::Vector<2,Int,Idx>;
//        using IntVector3_T    = Tiny::Vector<3,Int,Idx>;
        using E_T             = Tiny::Matrix<2,3,Int,Idx>;
        
        
        using VContainer_T    = Tiny::VectorList_AoS<AmbDim,Real,Idx>;
        using EContainer_T    = typename Tree3_T::EContainer_T;
        using BContainer_T    = typename Tree2_T::BContainer_T;
         
        using Prosector_T     = Prosector<Idx>;
        using Intersection_T  = Prosector_T::Intersection;
//        using FlagCounts_T  = Tiny::Vector<9,Size_T,Idx>;
        
        static constexpr Idx AmbDim = 3;
        
        using Matrix3x3_T = Tiny::Matrix<AmbDim,AmbDim,Real,Idx>;
        
    protected:
        
        static_assert(std::in_range<Idx>(4 * 64 + 1),"");
        
        static constexpr Idx max_depth = 64;
        
        using Base_T::edges;
        using Base_T::next_edge;
        using Base_T::edge_ptr;
        using Base_T::edge_count;
        using Base_T::component_count;
        using Base_T::component_ptr;
        using Base_T::component_color;
        using Base_T::cyclicQ;
        using Base_T::preorderedQ;
        
    public:
        
        using Base_T::ComponentCount;
        using Base_T::ComponentPointers;
        using Base_T::VertexCount;
        using Base_T::EdgeCount;
        using Base_T::Edges;
        using Base_T::NextEdge;
        using Base_T::EdgeNextEdge;
        
    protected:
        
        Tensor1<Idx,Idx> edge_ctr;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        
        VContainer_T vertex_coords;
        Matrix3x3_T R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        Tree2_T T;
        
        EContainer_T edge_coords;
        BContainer_T box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
//        std::vector<Intersection_T> intersections;
//        Tensor1<Idx ,Size_T> edge_intersections;
//        Tensor1<Real,Size_T> edge_times;
//        Tensor1<bool,Size_T> edge_overQ;
        
//        Vector3_T Sterbenz_shift {0};
        
//        Intersector_T S;
        
//        IntersectionFlagCounts_T intersection_flag_counts = {};

        Size_T intersection_count_3D = 0;
        
        bool intersections_computedQ  = false;
        bool bounding_boxes_computedQ = false;
        
    public:
        
        // Default constructor
        LinkEmbedding_Int() = default;
        // Destructor (virtual because of inheritance)
        virtual ~LinkEmbedding_Int() = default;
        // Copy constructor
        LinkEmbedding_Int( const LinkEmbedding_Int & other ) = default;
        // Copy assignment operator
        LinkEmbedding_Int & operator=( const LinkEmbedding_Int & other ) = default;
        // Move constructor
        LinkEmbedding_Int( LinkEmbedding_Int && other ) = default;
        // Move assignment operator
        LinkEmbedding_Int & operator=( LinkEmbedding_Int && other ) = default;
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<IntQ I>
        explicit LinkEmbedding_Int( const I edge_count_ )
        :   Base_T        { int_cast<Idx>(edge_count_) }
        ,   vertex_coords { edge_count                 }
//        ,   edge_coords   { edge_count                 }
        {}
        
        LinkEmbedding_Int( Tensor1<Idx,Idx> && component_ptr_, Tensor1<Idx,Idx> && component_color_ )
        :   Base_T        { std::move(component_ptr_), std::move(component_color_)  }
        ,   vertex_coords { edge_count                                              }
        //        ,   edge_coords   { edge_count                                              }
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding_Int(
            cptr<I_0> edges_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T        { edges_, edges_colors_, int_cast<Idx>(edge_count_) }
        ,   vertex_coords { edge_count                                        }
//        ,   edge_coords   { edge_count                                        }
        {}
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding_Int(
            cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T        { edge_tails_, edge_tips_, edges_colors_, edge_count_ }
        ,   vertex_coords { edge_count                                          }
//        ,   edge_coords   { edge_count                                          }
        {}
        
    public:

#include "LinkEmbedding_Int/Helpers.hpp"
#include "LinkEmbedding_Int/BoundingBoxes.hpp"
#include "LinkEmbedding_Int/FindIntersections.hpp"
#include "LinkEmbedding_Int/WriteToFile.hpp"
#include "LinkEmbedding_Int/ReadFromFile.hpp"

    public:
        
        bool ValidQ() const
        {
            return (component_ptr.Size() >= Idx(2));
        }

        cref<EContainer_T> VertexCoordinates() const
        {
            return edge_coords;
        }
        
        cref<EContainer_T> EdgeCoordinates() const
        {
            return edge_coords;
        }
        
        E_T EdgeData( const Idx e ) const
        {
            return E_T( edge_coords.data(e) );
        }
        
        IntVector2_T EdgeVector2( const Idx e, const bool k ) const
        {
            return Vector2_T( edge_coords.data(e,k) );
        }
        
        IntVector3_T EdgeVector3( const Idx e, const bool k ) const
        {
            return IntVector3_T( edge_coords.data(e,k) );
        }
        
        bool BoundingBoxedComputedQ()
        {
            return bounding_boxes_computedQ;
        }
        
        bool IntersectionsComputedQ()
        {
            return intersections_computedQ;
        }
        
        void SetTransformationMatrix( cref<Matrix3x3_T> A )
        {
            R = A;
        }
        
        void SetTransformationMatrix( Matrix3x3_T && A )
        {
            R = A;
        }
        
        cref<Matrix3x3_T> TransformationMatrix() const
        {
            return R;
        }
        
        template<bool transformQ = false>
        void ReadVertexCoordinates( cptr<Real> v )
        {
            TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates")+"<" + ToString(transformQ) + "," + ToString(shiftQ) + ">(AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");
            
            RealVector3_T lo ( Scalar::Max<Real> );
            RealVector3_T hi ( Scalar::Min<Real> );

            RealVector3_T x;
            RealVector3_T y;
            
            // vertex_coords.data(e), &v[AmbDim * edges(e,0)]
            // After loading we want:
            // vertex_data(e,k) = v[AmbDim * edges(e,0) + k]
            // edge_data(e,0,k) = round(vertex_data(e,k));
            
            auto read = [v,&x,&y]( const Idx e, const Idx i )
            {
                if constexpr ( transformQ )
                {
                    x.Read( &v[3*i] );
                    y = Dot(R,x);
                }
                else
                {
                    y.Read( &v[3*i] );
                }
                
                lo.ElementwiseMin(y);
                hi.ElementwiseMax(y);
                
                y.Write(vertex_coords.data(e));
            }
            
            if( preorderedQ )
            {
                for( Idx e = 0; e < edge_count; ++e )
                {
                    read(e,e);
                }
            }
            else // if( !preorderedQ )
            {
                for( Idx e = 0; e < edge_count; ++e )
                {
                    read(e,edges(e,0));
                }
            }

            ComputeEdgeCoordinates(lo, hi);
        }
        
    private:
        
        void ComputeEdgeCoordinates(cref<RealVector3_T> lo, cref<RealVector3_T> hi)
        {
            TOOLS_PTIMER(timer,MethodName("ComputeEdgeCoordinates"));
            
            intersections_computedQ  = false;
            bounding_boxes_computedQ = false;
            intersections.clear();
            
            if( edge_coords.Dim(0) != edge_count )
            {
                edge_coords = EContainer_T(edge_count);
            }
            
            const Real scale = Frac<Real>(
                long(1) << 53 // 53 or 53?
                ,
                Real(1.01) * Max( lo.Max(), hi.Max() )
            );
            
            RealVector3_T x;
            IntVector3_T  y;
            
            for( Int c = 0; c < component_count; ++c )
            {
                const Int i_begin = component_ptr[c  ];
                const Int i_end   = component_ptr[c+1];
                                    
                for( Int i = i_begin; i < i_end-1; ++i )
                {
                    const Int j = i+1;

                    x.Read(vertex_coords.data(j));
                    y[0] = Round<Int>(x[0] * scale);
                    y[1] = Round<Int>(x[1] * scale);
                    y[2] = Round<Int>(x[2] * scale);
                    
                    mptr<Real> target_i = edge_coords.data(i,1);
                    mptr<Real> target_j = &target_i[3];  // = edge_coords.data(j,0)
                    y.Write(target_i);
                    y.Write(target_j);
                }

                {
                    const Int i = i_end-1;
                    const Int j = i_begin;
                    
                    x.Read(vertex_coords.data(j));
                    y[0] = Round<Int>(x[0] * scale);
                    y[1] = Round<Int>(x[1] * scale);
                    y[2] = Round<Int>(x[2] * scale);

                    mptr<Real> target_i = edge_coords.data(i,1);
                    mptr<Real> target_j = edge_coords.data(j,0);
                    y.Write(target_i);
                    y.Write(target_j);
                }
                
            } // for( Int c = 0; c < component_count; ++c )
            
//            logvalprint("edge_coords",edge_coords);
        }
        
    public:
        
        void WriteVertexCoordinates( mptr<Real> v ) const
        {
            TOOLS_PTIMER(timer,MethodName("WriteVertexCoordinates"));
            
            if( preorderedQ )
            {
                vertex_coords.Write(v);
            }
            else
            {
                for( Idx e = 0; e < edge_count; ++e )
                {
                    copy_buffer<AmbDim>( vertex_coords.data(e), &v[AmbDim * edges(e,0)] );
                }
            }
        }
        
        void Rotate( cref<Matrix3x3_T> A )
        {
            TOOLS_PTIMER(timer,MethodName("Rotate"));
            
            RealVector3_T lo ( Scalar::Max<Real> );
            RealVector3_T hi ( Scalar::Min<Real> );
        
            RealVector3_T x;
            RealVector3_T y;
            
            for( Idx i = 0; i < edge_count; ++i )
            {
                x.Read( vertex_coords.data(i) );
                y = Dot(A,x);
                lo.ElementwiseMin(y);
                hi.ElementwiseMax(y);
                y.Write( vertex_coords.data(i) );
            }
            
            ComputeEdgeCoordinates(lo, hi);
            
            // We make it so that we can restore the original coordinates from R (up to rounding errors).
            // That is: we rotate both the coordinates and R by A; then we set R to the rotated matrix.
            SetTransformationMatrix(Dot(A,R));
        }
        
        void ComputeBoundingBoxes()
        {
        //    TOOLS_PTIMER(timer,MethodName("ComputeBoundingBoxes"));
            
            T.template ComputeBoundingBoxes<2,3>( edge_coords.data(), box_coords.data() );
            bounding_boxes_computedQ = true;
        }
        
    public:

        void DeleteTree()
        {
            T           = Tree2_T();
            edge_coords = EContainer_T();
            box_coords  = BContainer_T();
            bounding_boxes_computedQ = false;
        }

    public:

        Size_T AllocatedByteCount() const
        {
            return
                  T.AllocatedByteCount()
                + edge_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + Base_T::edges.AllocatedByteCount()
                + Base_T::next_edge.AllocatedByteCount()
                + Base_T::edge_ptr.AllocatedByteCount()
                + Base_T::component_ptr.AllocatedByteCount()
                + Base_T::component_color.AllocatedByteCount()
//                + Base_T::component_lookup.AllocatedByteCount();
                + edge_ctr.AllocatedByteCount()
                + edge_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_intersections.AllocatedByteCount()
                + edge_times.AllocatedByteCount()
                + edge_overQ.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(LinkEmbedding_Int) + AllocatedByteCount();
        }
        
        template<int t0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                ct_string("<|")
                + (" \n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(box_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edges)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::next_edge)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edge_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_color)
//                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_lookup)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_ctr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_intersections)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_times)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_overQ)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("LinkEmbedding_Int")
                + "<" + TypeName<Real>
                + "," + TypeName<Idx>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace Knoodle


