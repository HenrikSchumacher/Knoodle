#pragma  once

namespace Knoodle
{
    /*!@brief  **INEXACT.** This data type is mostly intended for reading in 3D vertex coordinates of a _link_, applying a planar projection, and computing the crossings. Then it can be handed over to class `PlanarDiagram` or `PlanarDiagramComplex`.
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
    
    template<FloatQ Real_ = double, IntQ Int_ = Int64, FloatQ BReal_ = float>
    class alignas( ObjectAlignment ) LinkEmbedding : public Link<Int_>
    {
        
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
        using Matrix3x3_T     = Tiny::Matrix<3,3,Real,Int>;
        
        using E_T             = Tiny::Matrix<2,3,Real,Int>;
        
        using EContainer_T    = Tree3_T::EContainer_T;
        using BContainer_T    = Tree2_T::BContainer_T;
         
        using Intersection_T  = Intersection<Real,Int>;
        
        using Intersector_T   = Prosector_Float<Real,Int>;
        using IntersectionFlagCounts_T = Tiny::Vector<9,Size_T,Int>;

        
        static constexpr Int AmbDim = 3;
        static constexpr Int InvalidColor = PlanarDiagram<Int>::InvalidColor;
        
        static constexpr bool countersQ = false;
        
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
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        EContainer_T     edge_coords;
        
        Matrix3x3_T R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        Tree2_T T;
        
        BContainer_T box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        Tensor1<Int ,Int> edge_intersections;
        Tensor1<Real,Int> edge_times;
        Tensor1<Int8,Int> edge_state;
        
        Vector3_T Sterbenz_shift {0};
        
        Intersector_T S;
        IntersectionFlagCounts_T intersection_flag_counts = {};

        Size_T intersection_count_3D  = 0;
        Size_T box_box_counter        = 0;
        Size_T edge_edge_counter      = 0;
        Int intersection_count        = 0;
        
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
        
        /*!@brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<IntQ I>
        explicit LinkEmbedding( const I edge_count_ )
        :   Base_T      { int_cast<Int>(edge_count_) }
        ,   edge_coords { edge_count                 }
        {}
        
        /*!@brief Construction from a list of component pointers and a list of component colors. The inputs will be consumed.
         */
        
        LinkEmbedding( Tensor1<Int,Int> && component_ptr_, Tensor1<Int,Int> && component_color_ )
        :   Base_T      { std::move(component_ptr_), std::move(component_color_)  }
        ,   edge_coords { edge_count                                              }
        {}
        
        
        /*!@brief Construction from a list of edges in interleaved form.
         */
        template<IntQ I_0, IntQ I_1>
        LinkEmbedding(
            cptr<I_0> edges_, cptr<I_0> edges_colors_, const I_1 edge_count_
        )
        :   Base_T      { edges_, edges_colors_, int_cast<Int>(edge_count_) }
        ,   edge_coords { edge_count                                        }
        {}
        
        
        // TODO: Make this available again. For that we have to make sure that the corresponding constructor of the Link class is intact.
        
//        // Provide lists of edge tails and edge tips to make the object figure out its topology.
//        template<IntQ I_0, IntQ I_1>
//        LinkEmbedding(
//            cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, cptr<I_0> edges_colors_, const I_1 edge_count_
//        )
//        :   Base_T      { edge_tails_, edge_tips_, edges_colors_, edge_count_ }
//        ,   edge_coords { edge_count                                          }
//        {}
        
    public:

#include "LinkEmbedding/Helpers.hpp"
#include "LinkEmbedding/VertexCoordinates.hpp"
#include "LinkEmbedding/BoundingBoxes.hpp"
#include "LinkEmbedding/FindIntersections.hpp"
#include "LinkEmbedding/ToFile.hpp"
#include "LinkEmbedding/FromFile.hpp"

    public:
        
        bool ValidQ() const
        {
            return (component_ptr.Size() >= Int{2});
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
            return Vector3_T( edge_coords.data(e,k) );
        }
        
        // This function must be here because KnotEmbedding needs another definition.
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
            
            // Strictly speaking, this is not part of the tree, but it is not necessary anymore, once the intersections are computed (and sorted).
            
            edge_times = Tensor1<Real,Int>();
        }

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
                + edge_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_intersections.AllocatedByteCount()
                + edge_times.AllocatedByteCount()
                + edge_state.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(LinkEmbedding) + AllocatedByteCount();
        }
        
        template<int t0 = 0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                ct_string("<|")
                + ( "\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edges)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::next_edge)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edge_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_color)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(box_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_intersections)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_times)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_state)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("LinkEmbedding")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace Knoodle
