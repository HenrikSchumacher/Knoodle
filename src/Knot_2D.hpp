#pragma  once

namespace Knoodle
{
    template<typename Real_ = double, typename Int_ = Int64, typename BReal_ = Real_>
    class alignas( ObjectAlignment ) Knot_2D final
    {
        // This data type is mostly intended to read in 3D vertex coordinates, to apply a planar projection and compute the crossings. Then it can be handed over to class PlanarDiagram. Hence, this class' main routine is FindIntersections (using a static binary tree).
        
        
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        // TODO: Read  GeomView .vect files.
        // TODO: Write GeomView .vect files.
        
        // TODO: Add value semantics.
        
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(FloatQ<BReal_>,"");
        
    public:
        
        using Real  = Real_;
        using Int   = Int_;
//        using LInt  = Int64;
        using BReal = BReal_;
        
        static constexpr Int AmbDim = 3;
        
        
        using Tree2_T        = AABBTree<2,Real,Int,BReal,false>;
//        using Tree3_T        = AABBTree<3,Real,Int,BReal,false>;
        
        using Vector2_T      = Tiny::Vector<2,Real,Int>;
        using Vector3_T      = Tiny::Vector<AmbDim,Real,Int>;
        using E_T            = Tiny::Matrix<2,AmbDim,Real,Int>;
        
        using VContainer_T   = Tiny::VectorList_AoS<AmbDim,Real,Int>;
        using BContainer_T   = typename Tree2_T::BContainer_T;
        
        using Intersection_T = Intersection<Real,Int>;
        
        using Intersector_T  = PlanarLineSegmentIntersector<Real,Int>;
        using IntersectionFlagCounts_T = Tiny::Vector<9,Size_T,Int>;
        
        template<typename Int>
        friend class PlanarDiagram;
        
        template<typename Real, typename Int, typename LInt, typename BReal>
        friend class PolyFold;
        
    protected:
        
        static_assert(std::in_range<Int>(4 * 64 + 1),"");
        
        static constexpr Int max_depth = 64;
        
        static constexpr Real one     = 1;
        static constexpr Real eps     = std::numeric_limits<Real>::epsilon();
        static constexpr Real big_one = 1 + eps;
        
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
        std::vector<Intersection_T> intersections;
        Tensor1<Int ,Int> edge_intersections;
        Tensor1<Real,Int> edge_times;
        Tensor1<bool,Int> edge_overQ;
        Tensor1<Int,Int>  edge_ctr;
        
        Vector3_T Sterbenz_shift {0};
        
        Intersector_T S;
        IntersectionFlagCounts_T intersection_flag_counts = {};
        
        Int intersection_count_3D = 0;
        
    public:
        
        // Default constructor
        Knot_2D() = default;
        // Destructor
        ~Knot_2D() = default;
        // Copy constructor
        Knot_2D( const Knot_2D & other ) = default;
        // Copy assignment operator
        Knot_2D & operator=( const Knot_2D & other ) = default;
        // Move constructor
        Knot_2D( Knot_2D && other ) = default;
        // Move assignment operator
        Knot_2D & operator=( Knot_2D && other ) = default;
        
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I>
        explicit Knot_2D( const I edge_count_ )
        :   edge_count      { int_cast<Int>(edge_count_) }
        ,   vertex_coords   { edge_count + 1             }
        ,   edge_ptr        { edge_count + 1             }
        ,   component_ptr   { 1                          }
        ,   T               { edge_count                 }
        ,   box_coords      { T.NodeCount()              }
        {
            component_ptr[0] = 0;
            component_ptr[1] = edge_count;
        }

#include "Link_2D/Helpers.hpp"
#include "Link_2D/FindIntersections.hpp"

    
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
        
        void ReadVertexCoordinates( cptr<Real> v )
        {
            TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates"));
            
            Vector3_T lo;
            Vector3_T hi;

            ComputeBoundingBox( v, lo, hi );
            
            constexpr Real margin = static_cast<Real>(1.01);
            constexpr Real two = 2;

            Sterbenz_shift[0] = margin * ( hi[0] - two * lo[0] );
            Sterbenz_shift[1] = margin * ( hi[1] - two * lo[1] );
            Sterbenz_shift[2] = margin * ( hi[2] - two * lo[2] );
            
            for( Int edge = 0; edge < edge_count; ++edge )
            {
                cptr<Real> source = &v[AmbDim * edge];
                mptr<Real> target = vertex_coords.data(edge);
                
                target[0] = source[0] + Sterbenz_shift[0];
                target[1] = source[1] + Sterbenz_shift[1];
                target[2] = source[2] + Sterbenz_shift[2];
            }

            copy_buffer<AmbDim>(vertex_coords.data(),vertex_coords.data(edge_count));
        }
        

        void ComputeBoundingBoxes()
        {
            TOOLS_PTIMER(timer,MethodName("ComputeBoundingBoxes"));
            T.template ComputeBoundingBoxes<2,AmbDim,AmbDim>(
                vertex_coords.data(), box_coords.data()
            );
        }
        
    private:

        // Caution: Only meant to be called by a constructor of PlanarDiagram to make room for the new diagram.
        void DeleteTree()
        {
            T             = Tree2_T();
            vertex_coords = VContainer_T();
            box_coords    = BContainer_T();
        }

    public:
        
        Size_T AllocatedByteCount() const
        {
            return
                  T.AllocatedByteCount()
//                + Base_T::edges.AllocatedByteCount()
//                + Base_T::next_edge.AllocatedByteCount()
                + edge_ptr.AllocatedByteCount()
//                + Base_T::component_ptr.AllocatedByteCount()
//                + Base_T::component_lookup.AllocatedByteCount();
                + edge_ctr.AllocatedByteCount()
                + vertex_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_intersections.AllocatedByteCount()
                + edge_times.AllocatedByteCount()
                + edge_overQ.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(Knot_2D) + AllocatedByteCount();
        }
        
        template<int t0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                std::string("<|")
                + ( "\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(vertex_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(box_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_ptr)
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
            return ct_string("Knot_2D")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace Knoodle
