#pragma  once

namespace Knoodle
{
    // TODO: Add color information!
    template<FloatQ Real_ = double, IntQ Int_ = Int64, FloatQ BReal_ = float>
    class alignas( ObjectAlignment ) LinkEmbedding : public Link<Int_>
    {
        // This data type is mostly intended to read in 3D vertex coordinates, to apply a planar projection and compute the crossings. Then it can be handed over to class PlanarDiagram. Hence, this class' main routine is RequireIntersections (using a static binary tree).
        
        
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        // TODO: Read  GeomView .vect files.
        // TODO: Write GeomView .vect files.
        
        // TODO: Add value semantics.
        
    public:
        
        using Real  = Real_;
        using Int   = Int_;
        using BReal = BReal_;
        
        using Base_T          = Link<Int>;
        using LinkEmbedding_T = LinkEmbedding<Real,Int,BReal>;
        
        using Tree2_T         = AABBTree<2,Real,Int,BReal,false>;
        using Tree3_T         = AABBTree<3,Real,Int,BReal,false>;
        
        using Vector2_T       = Tiny::Vector<2,Real,Int>;
        using Vector3_T       = Tiny::Vector<3,Real,Int>;
        using E_T             = Tiny::Matrix<2,3,Real,Int>;
        
        using EContainer_T    = typename Tree3_T::EContainer_T;
        using BContainer_T    = typename Tree2_T::BContainer_T;
         
        using Intersection_T  = Intersection<Real,Int>;
        
        using Intersector_T   = PlanarLineSegmentIntersector<Real,Int>;
        using IntersectionFlagCounts_T = Tiny::Vector<9,Size_T,Int>;
        
        static constexpr Int AmbDim = 3;
        
        using Matrix3x3_T = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        
    protected:
        
        static_assert(std::in_range<Int>(4 * 64 + 1),"");
        
        static constexpr Int max_depth = 64;
        
        static constexpr Real one     = 1;
        static constexpr Real eps     = std::numeric_limits<Real>::epsilon();
        static constexpr Real big_one = 1 + eps;
        
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
        
        Tensor1<Int,Int> edge_ctr;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        EContainer_T     edge_coords;
        
        Matrix3x3_T R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        Tree2_T T;
        
        BContainer_T box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        Tensor1<Int ,Int> edge_intersections;
        Tensor1<Real,Int> edge_times;
        Tensor1<bool,Int> edge_overQ;
        
        Vector3_T Sterbenz_shift {0};
        
        Intersector_T S;
        IntersectionFlagCounts_T intersection_flag_counts = {};

        Int intersection_count_3D = 0;
        
        bool intersections_computedQ  = false;
        bool bounding_boxes_computedQ = false;
        
    public:
        
        // Default constructor
        LinkEmbedding() = default;
        // Destructor (virtual because of inheritance)
        virtual ~LinkEmbedding() = default;
        // Copy constructor
        LinkEmbedding( const LinkEmbedding & other ) = default;
        // Copy assignment operator
        LinkEmbedding & operator=( const LinkEmbedding & other ) = default;
        // Move constructor
        LinkEmbedding( LinkEmbedding && other ) = default;
        // Move assignment operator
        LinkEmbedding & operator=( LinkEmbedding && other ) = default;
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<IntQ I>
        explicit LinkEmbedding( const I edge_count_ )
        :   Base_T      { int_cast<Int>(edge_count_) }
        ,   edge_coords { edge_count                 }
        {}
        
        LinkEmbedding( Tensor1<Int,Int> && component_ptr_, Tensor1<Int,Int> && component_color_ )
        :   Base_T      { std::move(component_ptr_), std::move(component_color_)  }
        ,   edge_coords { edge_count                                              }
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding(
            cptr<I_0> edges_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T      { edges_, edges_colors_, int_cast<Int>(edge_count_) }
        ,   edge_coords { edge_count                                        }
        {}
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding(
            cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T      { edge_tails_, edge_tips_, edges_colors_, edge_count_ }
        ,   edge_coords { edge_count                                          }
        {}
        
    public:

#include "LinkEmbedding/Helpers.hpp"
#include "LinkEmbedding/BoundingBoxes.hpp"
#include "LinkEmbedding/FindIntersections.hpp"
#include "LinkEmbedding/WriteToFile.hpp"
#include "LinkEmbedding/ReadFromFile.hpp"

    public:
        
        bool ValidQ() const
        {
            return (component_ptr.Size() >= Int(2));
        }
        
        cref<EContainer_T> EdgeCoordinates() const
        {
            return edge_coords;
        }
        
        E_T EdgeData( const Int e ) const
        {
            return E_T( edge_coords.data(e) );
        }
        
        Vector2_T EdgeVector2( const Int e, const bool k ) const
        {
            return Vector2_T( edge_coords.data(e,k) );
        }
        
        Vector3_T EdgeVector3( const Int e, const bool k ) const
        {
            return Vector2_T( edge_coords.data(e,k) );
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
        
        template<bool transformQ = false,bool shiftQ = true>
        void ReadVertexCoordinates( cptr<Real> v )
        {
            TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates")+"<" + ToString(transformQ) + "," + ToString(shiftQ) + ">(AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");
        
            intersections_computedQ  = false;
            bounding_boxes_computedQ = false;
            intersections.clear();
            
            Vector3_T lo;
            Vector3_T hi;

            if constexpr ( shiftQ )
            {
                lo.Read( v );
                hi.Read( v );
            }
            else
            {
                (void)lo;
                (void)hi;
            }
            
            Vector3_T x;
            Vector3_T y;
            
            if( preorderedQ )
            {
//                logprint("preordered");
                for( Int c = 0; c < component_count; ++c )
                {
                    const Int i_begin = component_ptr[c  ];
                    const Int i_end   = component_ptr[c+1];
                                        
                    for( Int i = i_begin; i < i_end-1; ++i )
                    {
                        const Int j = i+1;

                        mptr<Real> target_i = edge_coords.data(i,1);
                        mptr<Real> target_j = &target_i[3];  // = edge_coords.data(j,0)
                        
                        if constexpr ( transformQ )
                        {
                            y.Read( &v[3*j] );
                            x = Dot(R,y);
                        }
                        else
                        {
                            x.Read( &v[3*j] );
                        }
                        
                        if constexpr ( shiftQ )
                        {
                            lo.ElementwiseMin(x);
                            hi.ElementwiseMax(x);
                        }

                        x.Write(target_i);
                        x.Write(target_j);
                    }

                    {
                        const Int i = i_end-1;
                        const Int j = i_begin;

                        mptr<Real> target_i = edge_coords.data(i,1);
                        mptr<Real> target_j = edge_coords.data(j,0);
                      
                        if constexpr ( transformQ )
                        {
                            y.Read( &v[3*j] );
                            x = Dot(R,y);
                        }
                        else
                        {
                            x.Read( &v[3*j] );
                        }
                        
                        if constexpr ( shiftQ )
                        {
                            lo.ElementwiseMin(x);
                            hi.ElementwiseMax(x);
                        }
                        
                        x.Write(target_i);
                        x.Write(target_j);
                    }
                }
            }
            else
            {
//                logprint("not preordered");
                
                for( Int e = 0; e < edge_count; ++e )
                {
                    const Int i = edges(e,0);
                    const Int j = edges(e,1);

                    mptr<Real> target_i = edge_coords.data(e,0);
                    mptr<Real> target_j = &target_i[3]; // = edge_coords.data(e,1);
                  
                    if constexpr ( transformQ )
                    {
                        y.Read( &v[3*i] );
                        x = Dot(R,y);
                    }
                    else
                    {
                        x.Read( &v[3*i] );
                    }
                    
                    if constexpr ( shiftQ )
                    {
                        lo.ElementwiseMin(x);
                        hi.ElementwiseMax(x);
                    }
                    
                    x.Write(target_i);
                    
                    if constexpr ( transformQ )
                    {
                        y.Read( &v[3*j] );
                        x = Dot(R,y);
                    }
                    else
                    {
                        x.Read( &v[3*j] );
                    }
                    
                    // We can skip the ElementwiseMin/ElementwiseMax here because every vertex is supposed to appear precisely once as a tail of an edge.
                    
                    x.Write(target_j);
                }
            }
            
            if constexpr ( shiftQ )
            {
                TOOLS_MAKE_FP_STRICT();
                
                // https://en.wikipedia.org/wiki/Sterbenz_lemma
                    
                // Apply Sterbenz shift.
                Sterbenz_shift[0] = std::fma(-Real(2), lo[0], hi[0]);
                Sterbenz_shift[1] = std::fma(-Real(2), lo[1], hi[1]);
                Sterbenz_shift[2] = std::fma(-Real(2), lo[2], hi[2]);

                for( Int e = 0; e < edge_count; ++e )
                {
                    edge_coords(e,0,0) += Sterbenz_shift[0];
                    edge_coords(e,0,1) += Sterbenz_shift[1];
                    edge_coords(e,0,2) += Sterbenz_shift[2];
                    edge_coords(e,1,0) += Sterbenz_shift[0];
                    edge_coords(e,1,1) += Sterbenz_shift[1];
                    edge_coords(e,1,2) += Sterbenz_shift[2];
                }
            }
            else
            {
                Sterbenz_shift[0] = 0;
                Sterbenz_shift[1] = 0;
                Sterbenz_shift[2] = 0;
            }
            
//            logvalprint("edge_coords",edge_coords);
        }
        
        void WriteVertexCoordinates( mptr<Real> v ) const
        {
            TOOLS_PTIMER(timer,MethodName("WriteVertexCoordinates"));
            
            if( preorderedQ )
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    copy_buffer<AmbDim>( edge_coords.data(e), &v[AmbDim * e] );
                }
            }
            else
            {
                for( Int e = 0; e < edge_count; ++e )
                {
                    copy_buffer<AmbDim>( edge_coords.data(e), &v[AmbDim * edges(e,0)] );
                }
            }
        }
        
        template<bool shiftQ = true>
        void Rotate( cref<Matrix3x3_T> A )
        {
            TOOLS_PTIMER(timer,MethodName("Rotate"));
            
            Tensor2<Real,Int> v_coords( edge_count, AmbDim );
            
            WriteVertexCoordinates(v_coords.data());
            
            cref<Matrix3x3_T> R_new = Dot(A,R);
            // We make it so that we can restore the original coordinates up to shift from R.
            // That is: we rotate both the coordinates and R by A; then we set R to the rotated matrix.
            SetTransformationMatrix(A);
            
            this->template ReadVertexCoordinates<true,shiftQ>(v_coords.data());
            
            SetTransformationMatrix(R_new);
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
            return sizeof(LinkEmbedding) + AllocatedByteCount();
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
            return ct_string("LinkEmbedding")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace Knoodle


