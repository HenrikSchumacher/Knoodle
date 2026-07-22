#pragma  once

#include "Prosector.hpp"

namespace Knoodle
{
    
    /*!@brief EXPERIMENTAL. This class is mostly intended for reading in 3D vertex coordinates, applying a planar projection, and computing the crossings. Then it can be handed over to class `PlanarDiagram` or `PlanarDiagramComplex`.
     *
     *  This class's main routine is `RequireIntersections`. It uses a static binary tree, exact integer computations, and _symbolic_ perturbation techniques to compute the planar diagram as exactly as possible. It can deal with many geometric degeneracies: line segments that have length 0, line segments that project to a point, line segments whose endpoints project to the projections of other line segments, multiple intersections at a single point, intersecting line segments that a parallel. In particular, this class can deal with lattice links.
     *
     *  There are really only two cases in which this can go wrong:
     *
     *      (i) If floating-point inputs are used, then the initial rounding (see `ComputeEdgeCoordinates`) to an integer grid can induce a bit of rounding error. Caution is used, though to mediate this: we try to use a relatively big scaling factor and we scale only by powers of 2 (which does not lead to rouding errors on its own, unless some inputs are really tiny or really so that we have overflow in the exponent). We intentionally do not translate the inputs to avoid catastrophic cancellation. Note that this means that the range of the employed integer type may be used in an optimal way. It is in the user's discretion to apply appropriate measures to "center" the inputs around 0. If `Real_ = double` and `IReal_ = int64_t`, then often the rounding error is 0. You can check with `RoundingError()` after the computations have finished.
     *
     *      (ii) If after, the rounding procedire, two non-neigboring line segments intersect nontrivially in 3-space, there is no way to perturb it in a topologically meaningful way. Then `RequireIntersections` aborts and returns a nonzero error flag.
     *
     * This implementation is single-threaded only so that many instances of this object can be used in parallel.
     *
     * This class is EXPERIMENTAL at the moment, but once it has withstood the test of time, it is supposed to replace `LinkEmbedding`, which currently uses the less accurate floating-point backend. Moreover, it is planned to replace the tree-based intersection computations by a sweep line algorithm that should be more suitable for tightly confined links and will probably require less memory.
     *
     * @tparam Real_ The scalar type used for the coordinates of the link embedding. This is the format for loading and storing these curves. Allowed are `float`, `double`, and signed integral types.
     *
     * @tparam Int_ Integral type used for indices. Unsigned integers should work, too, but we give no guarantees. CAUTION: It must be big enough to hold the number of crossings that emerge after projecting the link to the x-y-plane. So `Int64` is probably the safest bet.
     *
     * @tparam IReal_ Internal scalar type used for computation geometry routines. Must be an signed integral type. If `Real_` is a floating-point type, the coordinates will be scaled appropriately and then rounded to the integer grid. If `Real_` is `double`, then `IReal_ = int64_t` can often do the job without any rounding. Should be the same for `Real_ = float` and `IReal_ = int32_t`. If `Real_` is a integral type, then `IReal_` must be identical to `Real_`.
     */
    
    template<
        typename   Real_  = Real64,
        IntQ       Int_   = Int64,
        SignedIntQ IReal_ = std::conditional_t<SameQ<Real_,Real64>, Int64,
                                std::conditional_t<SameQ<Real_,Real32>, Int32, Real_>
                            >
    >
    class alignas( ObjectAlignment ) LinkEmbedding2 : public Link<Int_>
    {
        static_assert( FloatQ<Real_> || SignedIntQ<Real_>, "");
        
    public:
        
        using Real  = Real_;
        using Int   = Int_;
        using IReal = IReal_;
        
        static constexpr Int AmbDim = 3;
        
        using Base_T          = Link<Int>;
        using LinkEmbedding_T = LinkEmbedding2;

        using Tree2_T         = AABBTree<2,IReal,Int,IReal,false>;
        using Tree3_T         = AABBTree<3,IReal,Int,IReal,false>;

        using Vector3_T       = Tiny::Vector<3,  Real ,Int>;
        using Matrix3x3_T     = Tiny::Matrix<3,3,Real ,Int>;
        using IRealVector3_T  = Tiny::Vector<3,  IReal,Int>;
        
        using VContainer_T    = Tiny::VectorList_AoS<3,Real,Int>;
        using EContainer_T    = typename Tree3_T::EContainer_T;
        using BContainer_T    = typename Tree2_T::BContainer_T;
         
        using Prosector_T     = Prosector<IReal,Int>;
        using Intersection_T  = Prosector_T::Intersection;
        using Time_T          = Prosector_T::IntersectionTime;
        
    protected:
        
        static_assert(std::in_range<Int>(4 * 64 + 1),"");
        
        static constexpr Int max_depth = 64;
        
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
        
        Tree2_T T;
        
        Vector3_T global_lo { Scalar::Max<Real> };
        Vector3_T global_hi { Scalar::Min<Real> };
        Matrix3x3_T R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        VContainer_T vertex_coords;
        EContainer_T edge_coords;
        Tensor1<bool,Int> edge_degenerateQ;        
        BContainer_T box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        
        Tensor1<Int   ,Int> edge_intersections;
        Tensor1<Time_T,Int> edge_times;
        Tensor1<Int8  ,Int> edge_state;

        // Other data.
        
        Prosector_T S;
        
        Real scaling_factor           = 1;
        Real rounding_error           = 0;
        Int  intersection_count       = 0;
        Int  intersection_count_3D    = 0;
        
        int  scaling_exponent         = 0;
        
        bool vertex_coords_loadedQ    = false;
        bool edge_coords_computedQ    = false;
        bool bounding_boxes_computedQ = false;
        bool intersections_computedQ  = false;
        bool inputs_integralQ         = IntQ<Real>;
        
    public:
        
        // Default constructor
        LinkEmbedding2() = default;
        // Destructor (virtual because of inheritance)
        virtual ~LinkEmbedding2() = default;
        // Copy constructor
        LinkEmbedding2( const LinkEmbedding2 & other ) = default;
        // Copy assignment operator
        LinkEmbedding2 & operator=( const LinkEmbedding2 & other ) = default;
        // Move constructor
        LinkEmbedding2( LinkEmbedding2 && other ) = default;
        // Move assignment operator
        LinkEmbedding2 & operator=( LinkEmbedding2 && other ) = default;
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<IntQ I>
        explicit LinkEmbedding2( const I edge_count_ )
        :   Base_T        { int_cast<Int>(edge_count_) }
        ,   vertex_coords { edge_count                 }
        {}
        
        LinkEmbedding2( Tensor1<Int,Int> && component_ptr_, Tensor1<Int,Int> && component_color_ )
        :   Base_T        { std::move(component_ptr_), std::move(component_color_)  }
        ,   vertex_coords { edge_count                                              }
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding2(
            cptr<I_0> edges_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T        { edges_, edges_colors_, int_cast<Int>(edge_count_) }
        ,   vertex_coords { edge_count                                        }
        {}
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding2(
            cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T        { edge_tails_, edge_tips_, edges_colors_, edge_count_ }
        ,   vertex_coords { edge_count                                          }
        {}
        
    public:

#include "LinkEmbedding2/WriteToFile.hpp"
#include "LinkEmbedding2/ReadFromFile.hpp"
#include "LinkEmbedding2/VertexCoordinates.hpp"
#include "LinkEmbedding2/EdgeCoordinates.hpp"
#include "LinkEmbedding2/BoundingBoxes.hpp"
#include "LinkEmbedding2/Intersections.hpp"
        
//#include "LinkEmbedding2/FindIntersections.hpp"


    public:
        
        static constexpr Int AmbientDimension()
        {
            return 3;
        }

        Int CrossingCount() const
        {
            return int_cast<Int>( intersections.size() );
        }

        cref<Tensor1<Int,Int>> EdgePointers() const
        {
            return edge_ptr;
        }

        Int IntersectionCount() const
        {
            return intersection_count;
        }
        
        Int IntersectionCount3D() const
        {
            return intersection_count_3D;
        }
        
        cref<Tensor1<Time_T,Int>> EdgeIntersectionTimes() const
        {
            return edge_times;
        }
        
        Tensor1<double,Int> EdgeIntersectionTimesAsDouble() const
        {
            Tensor1<double,Int> result ( edge_times.Size() );
            
            for( Int i = 0; i < edge_times.Size(); ++i )
            {
                result[i] = edge_times[i].ToDouble();
            }
                
            return result;
        }

        cref<Tensor1<Int,Int>> EdgeIntersections() const
        {
            return edge_intersections;
        }

        cref<Tensor1<Int8,Int>> EdgeStates() const
        {
            return edge_state;
        }
        
        bool ValidQ() const
        {
            return (component_ptr.Size() >= Int(2));
        }
        
        cref<Tree2_T> Tree() const
        {
            return T;
        }

        Real ScalingFactor() const
        {
            return scaling_factor;
        }
        
        int ScalingExponent() const
        {
            return scaling_exponent;
        }
        
        Real RoundingError() const
        {
            return rounding_error;
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
        
    public:

        void DeleteTree()
        {
            T           = Tree2_T();
            edge_coords = EContainer_T();
            box_coords  = BContainer_T();
            edge_coords_computedQ    = false;
            bounding_boxes_computedQ = false;
            
            // Strictly speaking, this is not part of the tree, but it is not necessary anymore, once the intersections are computed (and sorted).
            
            edge_times = Tensor1<Time_T,Int>();
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
                + edge_ctr.AllocatedByteCount()
                + edge_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_intersections.AllocatedByteCount()
                + edge_times.AllocatedByteCount()
                + edge_state.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(LinkEmbedding2) + AllocatedByteCount();
        }
        
        template<int t0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                ct_string("<|")
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edges)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::next_edge)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edge_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_color)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_ctr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_intersections)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_times)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_state)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("LinkEmbedding2")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }
        
    }; // LinkEmbedding2
    
} // namespace Knoodle


