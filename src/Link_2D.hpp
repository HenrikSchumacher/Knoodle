#pragma  once

namespace KnotTools
{

    template<typename Real_ = double, typename Int_ = Int32, typename SInt_ = Int32, typename BReal_ = Real_>
    class alignas( ObjectAlignment ) Link_2D : public Link<Int_>
    {
        // This data type is mostly intended to read in 3D vertex coordinates, to apply a planar projection and compute the crossings. Then it can be handed over to class PlanarDiagram. Hence this class' main routine is FindIntersections (using a static binary tree).
        
        
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        // TODO: Read  GeomView .vect files.
        // TODO: Write GeomView .vect files.
        
        // TODO: Add value semantics.
        
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(SignedIntQ<SInt_>,"");
        
    public:
        
        using Real  = Real_;
        using Int   = Int_;
        using SInt  = SInt_;
//        using LInt  = Int64;
        using BReal = BReal_;
        
        using Base_T         = Link<Int>;
        
        using Tree2_T        = AABBTree<2,Real,Int,BReal,false>;
        using Tree3_T        = AABBTree<3,Real,Int,BReal,false>;
        
        using Vector2_T      = Tiny::Vector<2,Real,Int>;
        using Vector3_T      = Tiny::Vector<3,Real,Int>;
        
        using EContainer_T   = typename Tree3_T::EContainer_T;
        
        using BContainer_T   = typename Tree2_T::BContainer_T;
        
        using Intersection_T = Intersection<Real,Int,Int>;
        
        using Intersector_T  = PlanarLineSegmentIntersector<Real,Int,Int>;
        using IntersectionFlagCounts_T = Tiny::Vector<8,Size_T,Int>;
        
        //        using BinaryMatrix_T = Sparse::BinaryMatrixCSR<Int,std::size_t>;
        
        static constexpr Int AmbDim = 3;
        
        template<typename Int>
        friend class PlanarDiagram;
        
        template<typename Real, typename Int, typename LInt, typename BReal>
        friend class PolyFold;
        
    protected:
        
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
        using Base_T::cyclicQ;
        using Base_T::preorderedQ;
        
    public:
        
        using Base_T::ComponentCount;
        using Base_T::ComponentPointers;
        using Base_T::VertexCount;
        using Base_T::EdgeCount;
        using Base_T::Edges;
        
    protected:
        
        Tensor1<Int,Int> edge_ctr;
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        EContainer_T edge_coords;
        
        Tiny::Matrix<3,3,Real,Int> R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)
        
        Tree2_T T;
        
        BContainer_T  box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        Tensor1<Int ,Int> edge_intersections;
        Tensor1<Real,Int> edge_times;
        Tensor1<bool,Int> edge_overQ;
        
        Vector3_T Sterbenz_shift {0};
        
        Intersector_T S;
        IntersectionFlagCounts_T intersection_flag_counts = {};
        
        Int degenerate_edge_count = 0;
        Int intersection_count_3D = 0;
        
    public:
        
        Link_2D() = default;
        
        Link_2D( const Link_2D & other) = default;
        
        virtual ~Link_2D() override = default;
        
        
        /*! @brief Calling this constructor makes the object assume that it represents a cyclic polyline.
         */
        template<typename I>
        explicit Link_2D( const I edge_count_ )
        :   Base_T      { int_cast<Int>(edge_count_) }
        ,   edge_coords { edge_count, Int(2), Int(3) }
        ,   T           { edge_count                 }
        ,   box_coords  { T.AllocateBoxes()          }
        {}
        
        template<typename J, typename K>
        explicit Link_2D( Tensor1<J,K> & component_ptr_ )
        :   Base_T      { component_ptr_                       }
        ,   edge_coords { component_ptr.Last(), Int(2), Int(3) }
        ,   T           { component_ptr.Last()                 }
        ,   box_coords  { T.AllocateBoxes()                    }
        {
            static_assert(IntQ<J>,"");
            static_assert(IntQ<K>,"");
        }
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template<typename I_0, typename I_1>
        Link_2D( cptr<I_0> edges_, const I_1 edge_count_ )
        :   Base_T      { edges_, int_cast<Int>(edge_count_) }
        ,   edge_coords { edge_count, Int(2), Int(3)         }
        ,   T           { edge_count                         }
        ,   box_coords  { T.AllocateBoxes()                  }
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
        }
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        template<typename I_0, typename I_1>
        Link_2D( cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, const I_1 edge_count_ )
        :   Base_T      { edge_tails_, edge_tips_, edge_count_ }
        ,   edge_coords { edge_count, Int(2), Int(3)           }
        ,   T           { edge_count                           }
        ,   box_coords  { T.AllocateBoxes()                    }
        {
            static_assert(IntQ<I_0>,"");
            static_assert(IntQ<I_1>,"");
        }
        
    
    private:
        
        // Caution: Only meant to be called by a constructor of PlanarDiagram to make room for the new diagram.
        void DeleteTree()
        {
            T           = Tree2_T();
            edge_coords = EContainer_T();
            box_coords  = BContainer_T();
        }
        
    private:
        
        void ComputeBoundingBox( cptr<Real> v, mref<Vector3_T> lo, mref<Vector3_T> hi )
        {
            // We assume that input is a link; thus
            const Int vertex_count = edge_count;
            
            lo.Read( v );
            hi.Read( v );
            
            for( Int i = 1; i < vertex_count; ++i )
            {
                lo[0] = Min( lo[0], v[3 * i + 0] );
                hi[0] = Max( hi[0], v[3 * i + 0] );
                
                lo[1] = Min( lo[1], v[3 * i + 1] );
                hi[1] = Max( hi[1], v[3 * i + 1] );
                
                lo[2] = Min( lo[2], v[3 * i + 2] );
                hi[2] = Max( hi[2], v[3 * i + 2] );
            }
        }
        
    public:
        
        void ReadVertexCoordinates( cptr<Real> v )
        {
            TOOLS_PTIC(ClassName()+"::ReadVertexCoordinates (AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");
            
            Vector3_T lo;
            Vector3_T hi;

            ComputeBoundingBox( v, lo, hi );
            
//            TOOLS_DUMP(lo);
//            TOOLS_DUMP(hi);
            
            constexpr Real margin = static_cast<Real>(1.01);
            constexpr Real two = 2;

            Sterbenz_shift[0] = margin * ( hi[0] - two * lo[0] );
            Sterbenz_shift[1] = margin * ( hi[1] - two * lo[1] );
            Sterbenz_shift[2] = margin * ( hi[2] - two * lo[2] );
            
//            TOOLS_DUMP(Sterbenz_shift);
            
            if( preorderedQ )
            {
                for( Int c = 0; c < component_count; ++c )
                {
                    const Int i_begin = component_ptr[c  ];
                    const Int i_end   = component_ptr[c+1];
                    
                    for( Int i = i_begin; i < i_end-1; ++i )
                    {
                        const Int j = i+1;
                        
                        mptr<Real> target_0 = edge_coords.data(i,1);
                        mptr<Real> target_1 = &target_0[3]; // = edge_coords.data(j,0)
                        
                        target_0[0] = target_1[0] = v[3*j + 0] + Sterbenz_shift[0];
                        target_0[1] = target_1[1] = v[3*j + 1] + Sterbenz_shift[1];
                        target_0[2] = target_1[2] = v[3*j + 2] + Sterbenz_shift[2];
                    }

                    {
                        const Int i = i_end-1;
                        const Int j = i_begin;

                        mptr<Real> target_0 = edge_coords.data(i,1);
                        mptr<Real> target_1 = edge_coords.data(j,0);
                      
                        target_0[0] = target_1[0] = v[3*j + 0] + Sterbenz_shift[0];
                        target_0[1] = target_1[1] = v[3*j + 1] + Sterbenz_shift[1];
                        target_0[2] = target_1[2] = v[3*j + 2] + Sterbenz_shift[2];
                    }
                }
            }
            else
            {
                cptr<Int> edge_tails = edges.data(0);
                cptr<Int> edge_tips  = edges.data(1);
                
                for( Int edge = 0; edge < edge_count; ++edge )
                {
                    const Int i = edge_tails[edge];
                    const Int j = edge_tips [edge];

                    mptr<Real> target_0 = edge_coords.data(edge,0);
                    mptr<Real> target_1 = &target_0[3]; // = edge_coords.data(edge,1);
                  
                    target_0[0] = v[3 * i + 0] + Sterbenz_shift[0];
                    target_0[1] = v[3 * i + 1] + Sterbenz_shift[1];
                    target_0[2] = v[3 * i + 2] + Sterbenz_shift[2];
                    
                    target_1[0] = v[3 * j + 0] + Sterbenz_shift[0];
                    target_1[1] = v[3 * j + 1] + Sterbenz_shift[1];
                    target_1[2] = v[3 * j + 2] + Sterbenz_shift[2];
                }
            }
            
            TOOLS_PTOC(ClassName()+"::ReadVertexCoordinates (AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");
        }
        
        // TODO: Apply Sterbenz shift.
//        void ReadVertexCoordinates( cptr<Real> x, cptr<Real> y, cptr<Real> z )
//        {
//            TOOLS_PTIC(ClassName()+"::ReadVertexCoordinates (SoA, " + (preorderedQ ? "preordered" : "unordered") + ")");
//            
//            if( preorderedQ )
//            {
//                for( Int c = 0; c < component_count; ++c )
//                {
//                    const Int i_begin = component_ptr[c  ];
//                    const Int i_end   = component_ptr[c+1];
//
//                    for( Int i = i_begin; i < i_end-1; ++i )
//                    {
//                        const int j = i+1;
//
//                        edge_coords(i,0,0) = x[i];
//                        edge_coords(i,0,1) = y[i];
//                        edge_coords(i,0,2) = z[i];
//                        
//                        edge_coords(i,1,0) = x[j];
//                        edge_coords(i,1,1) = y[j];
//                        edge_coords(i,1,2) = z[j];
//                    }
//
//                    {
//                        const Int i = i_end-1;
//                        const Int j = i_begin;
//
//                        edge_coords(i,0,0) = x[i];
//                        edge_coords(i,0,1) = y[i];
//                        edge_coords(i,0,2) = z[i];
//                        
//                        edge_coords(i,1,0) = x[j];
//                        edge_coords(i,1,1) = y[j];
//                        edge_coords(i,1,2) = z[j];
//                    }
//                }
//            }
//            else
//            {
//                cptr<Int> edge_tails = edges.data(0);
//                cptr<Int> edge_tips  = edges.data(1);
//                
//                for( Int edge = 0; edge < edge_count; ++edge )
//                {
//                    const Int i = edge_tails[edge];
//                    const Int j = edge_tips [edge];
//                    
//                    edge_coords(edge,0,0) = x[i];
//                    edge_coords(edge,0,1) = y[i];
//                    edge_coords(edge,0,2) = z[i];
//                    
//                    edge_coords(edge,1,0) = x[j];
//                    edge_coords(edge,1,1) = y[j];
//                    edge_coords(edge,1,2) = z[j];
//                }
//            }
//            
//            TOOLS_PTOC(ClassName()+"::ReadVertexCoordinates (SoA, " + (preorderedQ ? "preordered" : "unordered") + ")");
//        }
        
//        
//        void Rotate()
//        {
//            cptr<Int> edge_tails = edges.data(0);
//            cptr<Int> edge_tips  = edges.data(1);
//            
//            for( Int edge = 0; edge < edge_count; ++edge )
//            {
//                const Int i = edge_tails[edge];
//                const Int j = edge_tips [edge];
//                
//                // TODO: There is too much copying here. Remove it.
//                
//                const Vector3_T x ( edge_coords.data(edge,0) );
//                const Vector3_T y ( edge_coords.data(edge,1) );
//                
//                const Vector3_T Rx = Dot( R, x );
//                const Vector3_T Ry = Dot( R, y );
//                
//                Rx.Write( edge_coords.data(edge,0) );
//                Ry.Write( edge_coords.data(edge,1) );
//            }
//        }
        
    public:
        
        static constexpr Int AmbientDimension()
        {
            return 3;
        }
        
        Int DegenerateEdgeCount()
        {
            return degenerate_edge_count;
        }
        
#include "Link_2D/FindIntersections_DFS.hpp"
#include "Link_2D/CountDegenerateEdges.hpp"
        
    public:
        
        void ComputeBoundingBoxes()
        {
            TOOLS_PTIC(ClassName() + "ComputeBoundingBoxes<2,3>");
            T.template ComputeBoundingBoxes<2,3>( edge_coords, box_coords );
            TOOLS_PTOC(ClassName() + "ComputeBoundingBoxes<2,3>");
        }
        
        template<bool printQ = true> // whether to print errors and warnings
        [[nodiscard]] int FindIntersections()
        {
            TOOLS_PTIC(ClassName()+"FindIntersections");
            
            // TODO: Randomly rotate until not degenerate.
            
            // Here we do something strange:
            // We hand over edge_coords, a Tensor3 of size edge_count x 2 x 3
            // to a T which is a Tree2_T.
            // The latter expects a Tensor3 of size edge_count x 2 x 2, but it accesses the
            // enties only via operator(i,j,k), so this is safe!

            ComputeBoundingBoxes();
            
            
            CountDegenerateEdges();
            
            if( degenerate_edge_count > 0 )
            {
                if constexpr ( printQ )
                {
                    eprint(ClassName() + "::CountDegenerateEdges: Detected " + ToString(degenerate_edge_count) + " degenerate edges.");
                }
                return 7;
            }
              
            TOOLS_PTIC("FindIntersectingEdges_DFS");
            FindIntersectingEdges_DFS();
            TOOLS_PTOC("FindIntersectingEdges_DFS");
            
            // Check for bad intersections.

            {
                const Size_T count = intersection_flag_counts[6];
                if( count > 0 )
                {
                    if constexpr ( printQ )
                    {
                        eprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where line segments intersected in 3D.");
                    }
                    return 6;
                }
            }
            
            {
                const Size_T count = intersection_flag_counts[5];
                if( count > 0 )
                {
                    if constexpr ( printQ )
                    {
                        eprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where the line-line intersection was degenerate (the intersection set was an interval).");
                    }
                    return 5;
                }
            }
            
            {
                const Size_T count =
                      intersection_flag_counts[2]
                    + intersection_flag_counts[3]
                    + intersection_flag_counts[4];
                
                if( count > 0 )
                {
                    if constexpr ( printQ )
                    {
                        wprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where the line-line intersection was a point in a corner of a line segment.");
                    }
                    return 4;
                }
            }
            
            // Check for integer overflow.
            if( Size_T(4) * intersections.size() > static_cast<Size_T>(std::numeric_limits<Int>::max()) )
            {
                eprint(ClassName() + "::FindIntersections: More intersections found than can be handled with integer type " + TypeName<Int> + "." );
            }
            
            const Int intersection_count = static_cast<Int>(intersections.size());
            
            // We are going to use edge_ptr for the assembly; because we are going to modify it, we need a copy.
            edge_ctr.template RequireSize<false>( edge_ptr.Size() );
            edge_ctr.Read( edge_ptr.data() );
            
            if( edge_intersections.Size() != edge_ptr.Last() )
            {
                edge_intersections = Tensor1<Int, Int>( edge_ptr.Last() );
                edge_times         = Tensor1<Real,Int>( edge_ptr.Last() );
                edge_overQ         = Tensor1<bool,Int>( edge_ptr.Last() );
            }

            // We are going to fill edge_intersections so that data of the i-th edge lies in edge_intersections[edge_ptr[i]],..,edge_intersections[edge_ptr[i+1]].
            // To this end, we use (and modify!) edge_ctr so that edge_ctr[i] points AFTER the position to insert.
            
            TOOLS_PTIC("Counting sort");
            // Counting sort.
            
//            for( Int k = intersection_count-1; k > -1; --k )
            for( Int k = intersection_count; k --> 0;  )
            {
                Intersection_T & inter = intersections[static_cast<Size_T>(k)];
                
                // We have to write BEFORE the positions specified by edge_ctr (and decrease it for the next write;

                const Int pos_0 = --edge_ctr[inter.edges[0]+1];
                const Int pos_1 = --edge_ctr[inter.edges[1]+1];

                edge_intersections[pos_0] = k;
                edge_times        [pos_0] = inter.times[0];
                edge_overQ        [pos_0] = true;
                
                edge_intersections[pos_1] = k;
                edge_times        [pos_1] = inter.times[1];
                edge_overQ        [pos_1] = false;
            }
            
            // Sort intersections edgewise w.r.t. edge_times.
            ThreeArraySort<Real,Int,bool,Int> sort ( intersection_count );
            
            for( Int i = 0; i < edge_count; ++i )
            {
                // This is the range of data in edge_intersections/edge_times that belongs to edge i.
                const Int k_begin = edge_ptr[i  ];
                const Int k_end   = edge_ptr[i+1];
                     
                sort(
                    &edge_times[k_begin],
                    &edge_intersections[k_begin],
                    &edge_overQ[k_begin],
                    k_end - k_begin
                );
            }
            TOOLS_PTOC("Counting sort");
            
            // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
            
            TOOLS_PTOC(ClassName()+"FindIntersections");
            
            return 0;
        }
        
    public:
        
        Int UnlinkCount() const
        {
            //TODO: Ensure that edge_ptr is initialized correctly when this is called!
            
            Int unlink_count = 0;
            
            for( Int c = 0; c < component_count; ++c )
            {
                // The range of arcs belonging to this component.
                const Int arc_begin  = edge_ptr[component_ptr[c  ]];
                const Int arc_end    = edge_ptr[component_ptr[c+1]];

                if( arc_begin == arc_end )
                {
                    ++unlink_count;
                }
            }
            
            return unlink_count;
        }
        
    public:
        
        Int CrossingCount() const
        {
            return static_cast<Int>( intersections.size() );
        }
        
        cref<EContainer_T> EdgeCoordinates() const
        {
            return edge_coords;
        }
        
        Vector2_T EdgeVector2( const Int edge, const bool k ) const
        {
            return Vector2_T( &edge_coords.data()[2 * AmbDim * edge + AmbDim * k] );
        }
        
        Vector3_T EdgeVector3( const Int edge, const bool k ) const
        {
            return Vector2_T( &edge_coords.data()[2 * AmbDim * edge + AmbDim * k] );
        }
                             
        cref<BContainer_T> BoundingBoxes() const
        {
            return box_coords;
        }
        
        cref<Tensor1<Int,Int>> EdgePointers() const
        {
            return edge_ptr;
        }
        
        cref<Tensor1<Real,Int>> EdgeIntersectionTimes() const
        {
            return edge_times;
        }
        
        cref<Tensor1<Int,Int>> EdgeIntersections() const
        {
            return edge_intersections;
        }
        
        cref<Tensor1<bool,Int>> EdgeOverQ() const
        {
            return edge_overQ;
        }
        
        cref<std::vector<Intersection_T>> Intersections() const
        {
            return intersections;
        }
        
        cref<Tree2_T> Tree() const
        {
            return T;
        }
        
        cref<Vector3_T> SterbenzShift() const
        {
            return Sterbenz_shift;
        }
        
        cref<IntersectionFlagCounts_T> IntersectionFlagCounts() const
        {
            return intersection_flag_counts;
        }
        

        Size_T AllocatedByteCount() const
        {
            return
                  T.AllocatedByteCount()
                + Base_T::edges.AllocatedByteCount()
                + Base_T::next_edge.AllocatedByteCount()
                + Base_T::edge_ptr.AllocatedByteCount()
                + Base_T::component_ptr.AllocatedByteCount()
                + Base_T::component_lookup.AllocatedByteCount();
                + edge_ctr.AllocatedByteCount()
                + edge_coords.AllocatedByteCount()
                + box_coords.AllocatedByteCount()
                + edge_intersections.AllocatedByteCount()
                + edge_times.AllocatedByteCount()
                + edge_overQ.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(Link_2D) + AllocatedByteCount();
        }
        
        template<int t0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                ct_string("<|")
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_coords)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(box_coords)
                + ( "\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(T)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edges)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::next_edge)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::edge_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_ptr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::component_lookup)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_ctr)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_intersections)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_times)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(edge_overQ)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        static std::string ClassName()
        {
            return ct_string("Link_2D")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<SInt>
                + "," + TypeName<BReal>
                + ">";
        }
    };
    
} // namespace KnotTools
