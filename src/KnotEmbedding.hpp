#pragma  once

namespace Knoodle
{
    /*!@brief This data type is mostly intended for reading in 3D vertex coordinates of a _knot_, applying a planar projection, and computing the crossings. Then it can be handed over to class `PlanarDiagram` or `PlanarDiagramComplex`. This class is very similar to `LinkEmbedding`, but with a few performance tweaks specifically for knots.
     *
     *  This class's main routine is `RequireIntersections`. It uses a static binary tree, high precision floating-point computations to compute the resulting planar diagram as exactly as possible.
     *
     * This implementation is single-threaded only so that many instances of this object can be used in parallel.
     *
     * Since computations are performed in finite floating-point arithmetic, this is not exact. But at least for random inputs, significant rounding errors (i.e., those that change topology) should be very, very seldom.
     *
     * @tparam Real_ The scalar type used for the coordinates of the link embedding. Best to use `double` here; `float` is not accurate enough.
     *
     * @tparam Int_ Integral type used for indices. Unsigned integers should work, too, but we give no guarantees. CAUTION: It must be big enough to hold the number of crossings that emerge after projecting the link to the x-y-plane. So `Int64` is probably the safest bet.
     *
     * @tparam BReal_ A floating-point type to store the boundaries of the bonding boxes of the internal tree. Relatively low precision does not harm here, so using `BReal_ = float` will save a lot of memory.
     */
    
    template<FloatQ Real_ = double, IntQ Int_ = Int64, FloatQ BReal_ = Real_>
    class alignas( ObjectAlignment ) KnotEmbedding final
    {
        // This data type is mostly intended to read in 3D vertex coordinates, to apply a planar projection and compute the crossings. Then it can be handed over to class PlanarDiagram. Hence, this class' main routine is FindIntersections (using a static binary tree).
        
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        // This differs from LinkEmbedding in that the edge coordinates are not stored separated. This is to save memory when very large polygons ought to be handled.
        
        // TODO: Read  GeomView .vect files.
        // TODO: Write GeomView .vect files.
        
        // TODO: Add value semantics.
        
    public:
        
        using Real  = Real_;
        using Int   = Int_;
//        using LInt  = Int64;
        using BReal = BReal_;
        
        static constexpr Int AmbDim = 3;
        
        
        using Tree2_T        = AABBTree<2,Real,Int,BReal,false>;
//        using Tree3_T        = AABBTree<3,Real,Int,BReal,false>;
        
        using Vector2_T      = Tiny::Vector<2,Real,Int>;
        using Vector3_T      = Tiny::Vector<3,Real,Int>;
        using Matrix3x3_T    = Tiny::Matrix<3,3,Real,Int>;
        using E_T            = Tiny::Matrix<2,3,Real,Int>;
        
        using VContainer_T   = Tiny::VectorList_AoS<AmbDim,Real,Int>;
        using BContainer_T   = typename Tree2_T::BContainer_T;
        
        using Prosector_T     = Prosector_Float<Real,Int>;
        using Intersection_T  = Prosector_T::Intersection_T;
        using EdgeCrossing_T  = EdgeCrossing<Int>;
        
        using ProsectorFlagCounts_T = Tiny::Vector<9,Size_T,        Underlying_T<ProsectorFlag>>;
        
        
        static constexpr bool countersQ = false;
        
    protected:
        
        static_assert(std::in_range<Int>(4 * 64 + 1),"");
        
        static constexpr Int max_depth = 64;
        
    protected:
        
        Int edge_count;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        VContainer_T vertex_coords;
        
        Tensor1<Int,Int> edge_ptr;
        Tensor1<Int,Int> component_ptr;
        
        Tiny::Matrix<3,3,Real,Int> R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        Tree2_T T;
        
        BContainer_T box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        Aggregator<Intersection_T,Int> intersections;
        Tensor1<EdgeCrossing_T,Int> edge_cross;
        Tensor1<Real,Int> edge_times;
        
        Vector3_T Sterbenz_shift {0};
        
        Prosector_T S;
        ProsectorFlagCounts_T prosector_flag_counts = {};
        
        Int    intersection_count     = 0;
        Size_T intersection_count_3D  = 0;
        Size_T box_box_counter        = 0;
        Size_T edge_edge_counter      = 0;
        
        bool intersections_computedQ  = false;
        bool bounding_boxes_computedQ = false;
        
    public:
        
        // Default constructor
        KnotEmbedding() = default;
        // Destructor
        ~KnotEmbedding() = default;
        // Copy constructor
        KnotEmbedding( const KnotEmbedding & other ) = default;
        // Copy assignment operator
        KnotEmbedding & operator=( const KnotEmbedding & other ) = default;
        // Move constructor
        KnotEmbedding( KnotEmbedding && other ) = default;
        // Move assignment operator
        KnotEmbedding & operator=( KnotEmbedding && other ) = default;
        
        
        /*!@brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I>
        explicit KnotEmbedding( const I edge_count_ )
        :   edge_count      { int_cast<Int>(edge_count_)         }
        ,   vertex_coords   { int_cast<Int>(edge_count + Int(1)) }
        ,   edge_ptr        { int_cast<Int>(edge_count + Int(1)) }
        ,   component_ptr   { Int(1)                             }
        {
            if(
                std::cmp_greater_equal(edge_count_, Scalar::Max<Int> - Int(1))
                ||
                std::cmp_less(edge_count_, Int(0))
            )
            {
                edge_count = 0;
            }
            component_ptr[0] = 0;
            component_ptr[1] = edge_count;
        }

#include "KnotEmbedding/VertexCoordinates.hpp"
#include "LinkEmbedding/Helpers.hpp"
#include "LinkEmbedding/BoundingBoxes.hpp"
#include "LinkEmbedding/FindIntersections.hpp"

    
    public:
        
        Int VertexCount() const
        {
            return edge_count;
        }
        
        Int EdgeCount() const
        {
            return edge_count;
        }
        
        cref<VContainer_T> VertexCoordinates() const
        {
            return vertex_coords;
        }
        
        Int ComponentCount() const
        {
            return 1;
        }
        
        cref<Tensor1<Int,Int>> ComponentPointers() const
        {
            return component_ptr;
        }
        
        Int NextEdge( const Int edge) const
        {
            return (edge < edge_count) ? edge + 1 : 0;
        }
        
        E_T EdgeData( const Int edge) const
        {
            return E_T( vertex_coords.data(edge) );
        }
        
        Vector2_T EdgeVector2( const Int edge, const bool k ) const
        {
            return Vector2_T( vertex_coords.data(edge + k) );
        }
        
        Vector3_T EdgeVector3( const Int edge, const bool k ) const
        {
            return Vector3_T( vertex_coords.data(edge + k) );
        }
        
        void ComputeBoundingBoxes()
        {
//            TOOLS_TIMER(timer,MethodName("ComputeBoundingBoxes"));
            
            T.template ComputeBoundingBoxes<2,AmbDim,AmbDim>( vertex_coords.data(), box_coords.data() );
            bounding_boxes_computedQ = true;
        }
        
    public:

        // Caution: Only meant to be called by a constructor of PlanarDiagram to make room for the new diagram.
        void DeleteTree()
        {
            T             = Tree2_T();
            vertex_coords = VContainer_T();
            box_coords    = BContainer_T();
            bounding_boxes_computedQ = false;
        }

    public:
        
        Size_T AllocatedByteCount() const
        {
            return
                  T.AllocatedByteCount()
                + edge_ptr.AllocatedByteCount()
                + vertex_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_cross.AllocatedByteCount()
                + edge_times.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(KnotEmbedding) + AllocatedByteCount();
        }
        
        template<int t0 = 0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                std::string("<|")
                + ( "\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(vertex_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(box_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_cross)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_times)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("KnotEmbedding")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace Knoodle
