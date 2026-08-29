#pragma  once

namespace Knoodle
{
    
    /*!@brief **EXPERIMENTAL.** This class is mostly intended for reading in 3D vertex coordinates, applying a planar projection, and computing the crossings. Then it can be handed over to class `PlanarDiagram` or `PlanarDiagramComplex`.
     *
     *  This class's main routines are `ReadVertexCoordinates` and `RequireIntersections`.
     *  `ReadVertexCoordinates` loads vertex coordinates from a raw buffer; the intrinsic topology of the link and the ordering in which individual vertices are loaded depends on which constructor was used.
     *  `RequireIntersections` uses a static binary tree, exact integer computations, and _symbolic_ perturbation techniques to compute the planar diagram as exactly as possible. It can deal with many geometric degeneracies: line segments that have length 0, line segments that project to a point, line segments whose endpoints project to the projections of other line segments, multiple intersections at a single point, intersecting line segments that a parallel. In particular, this class can deal with lattice links.
     *
     *
     *  There are really only two cases in which this can go wrong:
     *
     *  -# If floating-point inputs are used, then the initial rounding (see `ComputeEdgeCoordinates`) to an integer grid can induce a bit of rounding error. Caution is used, though to mediate this: we try to use a relatively big scaling factor and we scale only by powers of 2 (which does not lead to rouding errors on its own, unless some inputs are really tiny or really so that we have overflow in the exponent). We intentionally do not translate the inputs to avoid catastrophic cancellation. Note that this means that the range of the employed integer type may be used in an optimal way. It is in the user's discretion to apply appropriate measures to "center" the inputs around 0. If `Real_ = double` and `IReal_ = int64_t`, then often the rounding error is 0. You can check with `RoundingError()` after the computations have finished.
     *
     *  -# If, after the rounding procedure, two non-neigboring line segments intersect nontrivially in 3-space, there is no way to perturb it in a topologically meaningful way. Then `RequireIntersections` aborts and returns a nonzero error flag.
     *
     * This implementation is single-threaded only so that many instances of this object can be used in parallel.
     *
     * This class is EXPERIMENTAL at the moment, but once it has withstood the test of time, it is supposed to replace `LinkEmbedding`, which currently uses the less accurate floating-point backend. Moreover, it is planned to replace the tree-based intersection computations by a sweep line algorithm that should be more suitable for tightly confined links and will probably require less memory.
     *
     * @tparam Real_ The scalar type used for the coordinates of the link embedding. This is the format for loading and storing these curves. Allowed are `float`, `double`, and signed integral types.
     *
     * @tparam Prosector_T_ What is expected here is one of the `Prosector*` classes that the backend for projecting the coordinates to a plane and for computing the intersections. This also specified which integer classes to use for indexing and for coordinate computations. CAUTION: The type `Prosector_T_::Idx` must be an integral type big enough to hold the number of crossings that emerge after projecting the link to the x-y-plane. So `Int64` is probably the safest bet here.
     */
    
    template<
        typename Real_,
        typename Prosector_T_
    >
    class alignas( ObjectAlignment ) LinkEmbedding_Int : public Link<typename Prosector_T_::Idx>
    {
        static_assert( FloatQ<Real_> || SignedIntQ<Real_>, "");
        
    public:
        
        using Real            = Real_;
        using Prosector_T     = Prosector_T_;
        
        using Int             = Prosector_T::Idx;
        using IReal           = Prosector_T::Int;
        
        using Intersection_T  = Prosector_T::Intersection;
        using Time_T          = Prosector_T::Time_T;
        
        static constexpr Int AmbDim = 3;
        static constexpr Int InvalidColor = PlanarDiagram<Int>::InvalidColor;
        
        using Base_T          = Link<Int>;
        using LinkEmbedding_T = LinkEmbedding_Int;

        using Tree2_T         = AABBTree<2,IReal,Int,IReal,false>;
        using Tree3_T         = AABBTree<3,IReal,Int,IReal,false>;

        using Vector3_T       = Tiny::Vector<3,  Real ,Int>;
        using Matrix3x3_T     = Tiny::Matrix<3,3,Real ,Int>;
        using IRealVector3_T  = Tiny::Vector<3,  IReal,Int>;
        
        using VContainer_T    = Tiny::VectorList_AoS<3,Real,Int>;
        using EContainer_T    = typename Tree3_T::EContainer_T;
        using BContainer_T    = typename Tree2_T::BContainer_T;
        
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
        
        Tree2_T T;
        
        Vector3_T global_lo { Scalar::Max<Real> };
        Vector3_T global_hi { Scalar::Min<Real> };
        Matrix3x3_T R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        
        VContainer_T vertex_coords;
        EContainer_T edge_coords;    
        BContainer_T box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        
        Tensor1<Int   ,Int> edge_intersections;
        Tensor1<Time_T,Int> edge_times;
        Tensor1<Int8  ,Int> edge_state;

        // Other data.
        
        Prosector_T S;
        
        Real   scaling_factor           = 1;
        Real   rounding_error           = 0;
        Real   max_modulus              = 0;
        Size_T intersection_count_3D    = 0;
        Int    intersection_count       = 0;
        
        int    scaling_exponent         = 0;
        
        bool   vertex_coords_loadedQ    = false;
        bool   edge_coords_computedQ    = false;
        bool   bounding_boxes_computedQ = false;
        bool   intersections_computedQ  = false;
        bool   input_integralQ          = IntQ<Real>;
        
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
        
        /*!@brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<IntQ I>
        explicit LinkEmbedding_Int( const I edge_count_ )
        :   Base_T        { int_cast<Int>(edge_count_) }
        ,   vertex_coords { edge_count                 }
        {}
        
        LinkEmbedding_Int( Tensor1<Int,Int> && comp_ptr_, Tensor1<Int,Int> && comp_color_ )
        :   Base_T { std::move(comp_ptr_), std::move(comp_color_)  }
        ,   vertex_coords { edge_count }
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding_Int(
            cptr<I_0> edges_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T { edges_, edges_colors_, int_cast<Int>(edge_count_) }
        ,   vertex_coords { edge_count }
        {}
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding_Int(
            cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T { edge_tails_, edge_tips_, edges_colors_, edge_count_ }
        ,   vertex_coords { edge_count }
        {}

#include "LinkEmbedding_Int/Helpers.hpp"
#include "LinkEmbedding_Int/ToFile.hpp"
#include "LinkEmbedding_Int/FromFile.hpp"
#include "LinkEmbedding_Int/VertexCoordinates.hpp"
#include "LinkEmbedding_Int/EdgeCoordinates.hpp"
#include "LinkEmbedding_Int/BoundingBoxes.hpp"
#include "LinkEmbedding_Int/Intersections.hpp"

    public:

        Size_T AllocatedByteCount() const
        {
            return
                  T.AllocatedByteCount()
                + Base_T::edges.AllocatedByteCount()
                + Base_T::next_edge.AllocatedByteCount()
                + Base_T::edge_ptr.AllocatedByteCount()
                + Base_T::component_ptr.AllocatedByteCount()
                + Base_T::component_color.AllocatedByteCount()
                + vertex_coords.AllocatedByteCount()
                + edge_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_intersections.AllocatedByteCount()
                + edge_times.AllocatedByteCount()
                + edge_state.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(LinkEmbedding_Int) + AllocatedByteCount();
        }
        
        template<int t0 = 0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                std::string("<|")
                + ( "\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edges)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::next_edge)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edge_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_color)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(vertex_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(box_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_intersections)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_times)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_state)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName().append("::").append(tag);
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("LinkEmbedding_Int")
                .append("<").append(TypeName<Real>)
                .append(",").append(Prosector_T::ClassName())
                .append(">");
        }
        
    }; // LinkEmbedding_Int

} // namespace Knoodle
