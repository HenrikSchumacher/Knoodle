#pragma  once

namespace KnotTools
{
    template<typename Real_ = double, typename Int_ = long long, typename SInt_ = int>
    struct Intersection
    {
        using Real = Real_;
        using Int  = Int_;
        using SInt = SInt_;
        
        const Int  edges [2] = {-2}; // First edge goes over, second edge goes under.
        const Real times [2] = {-2};
        
        const SInt sign;
        
        Intersection(
            const Int over_edge_,
            const Int under_edge_,
            const Real over_edge_time_,
            const Real under_edge_time_,
            const SInt sign_
        )
        :   edges { over_edge_,      under_edge_      }
        ,   times { over_edge_time_, under_edge_time_ }
        ,   sign  ( sign_ )
        {}
        
        ~Intersection() = default;
    };

    template<typename Real_ = double, typename Int_ = long long, typename SInt_ = short int>
    class alignas( ObjectAlignment ) Link_2D : public Link<Int_>
    {
        // This data type is mostly intended to read in 3D vertex coordinates, to apply a planar projection and then to generate an object of type PlanarDiagram or some other representation of planar diagrams. Hence this class' main routines are FindIntersections (using a static binary tree) and CreatePlanarDiagram.
        
        
        // This implementation is single-threaded only so that many instances of this object can be used in parallel.
        
        
        // TODO: Read  GeomView .vect files.
        // TODO: Write GeomView .vect files.
        
        // TODO: Add value semantics.
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        using SInt = SInt_;
        
        using Base_T = Link<Int>;
        
        using Tree2_T        = AABBTree<2,Real,Int>;
        using Tree3_T        = AABBTree<3,Real,Int>;
        
        using Vector2_T      = Tiny::Vector<2,Real,Int>;
        using Vector3_T      = Tiny::Vector<3,Real,Int>;
        
        using EContainer_T   = Tree3_T::EContainer_T;

        using BContainer_T   = Tree2_T::BContainer_T;
        
        using Intersection_T = Intersection<Real,Int,SInt>;
        
        using BinaryMatrix_T = Sparse::BinaryMatrixCSR<Int,std::size_t>;
        
    protected:
        
        ASSERT_FLOAT(Real);
        ASSERT_INT(Int);
        
        static constexpr Int max_depth = 128;
        
        static constexpr Real one     = 1;
        static constexpr Real eps     = std::numeric_limits<Real>::epsilon();
        static constexpr Real big_one = 1 + eps;
        
        
        using Base_T::edges;
        using Base_T::next_edge;
        using Base_T::edge_ctr;
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
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        EContainer_T edge_coords;
        
        Tiny::Matrix<3,3,Real,Int> R { { {1,0,0}, {0,1,0}, {0,0,1} } }; // a rotation matrix (later to be randomized)

        Tree2_T T;
        
        BContainer_T  box_coords;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        Tensor1<Int,Int>  edge_intersections;
        Tensor1<Real,Int> edge_times;
        Tensor1<bool,Int> edge_overQ;
                 
        Int intersections_3D = 0;
        Int intersections_nontransversal = 0;

        Vector3_T Sterbenz_shift {0};

    public:
        
        Link_2D() = default;
        
        ~Link_2D() = default;
        
  
        // TODO: Make this constructor work.
//        // Calling this constructor makes the object assume that it represents a cyclic polyline.
//        template<typename I>
//        explicit Link_2D( const I edge_count_ )
//        :   Base_T      ( static_cast<Int>(edge_count_)         )
//        ,   edge_coords ( static_cast<Int>(edge_count_), 2, 3   )
//        ,   T           ( static_cast<Int>(edge_count_)         )
//        ,   box_coords  ( T.NodeCount()                , 2, 2   )
//        {
//            ptic(ClassName()+"() (cyclic)");
//            
////            intersections.reserve( static_cast<size_t>(2 * edge_count_) );
//                        
//            ptoc(ClassName()+"() (cyclic)");
//        }
        
        template<typename J, typename K, IS_INT(J), IS_INT(K)>
        explicit Link_2D( Tensor1<J,K> & component_ptr_ )
        :   Base_T      ( component_ptr_       )
        ,   edge_coords ( component_ptr.Last(), 2 ,3 )
        ,   T           ( component_ptr.Last() )
        ,   box_coords  ( T.NodeCount(), 2, 2 )
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        template<typename I_0, typename I_1>
        Link_2D( cptr<I_0> edges_, const I_1 edge_count_ )
        :   Base_T      ( edges_, edge_count_  )
        ,   edge_coords ( edge_count_, 2, 3    )
        ,   T           ( edge_count_          )
        ,   box_coords  ( T.NodeCount(), 2 , 2 )
        {}
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        template<typename I_0, typename I_1>
        Link_2D( cptr<I_0> edge_tails_, cptr<I_0> edge_tips_, const I_1 edge_count_ )
        :   Base_T      ( edge_tails_, edge_tips_, edge_count_ )
        ,   edge_coords ( edge_count_, 2, 3                    )
        ,   T           ( edge_count_                          )
        ,   box_coords  ( T.NodeCount(), 2, 3                  )
        {}
        
    public:
        
        void ReadVertexCoordinates( cptr<Real> v, const bool update = false )
        {
            ptic(ClassName()+"::ReadVertexCoordinates (AoS)");
            
            // TODO: Translate the data so that everything lies in a box [L,2*L] x [L,2*L] z[L,2*L]. ->Subtraction without precision loss.
            if( preorderedQ )
            {
                for( Int c = 0; c < component_count; ++c )
                {
                    const Int i_begin = component_ptr[c  ];
                    const Int i_end   = component_ptr[c+1];

                    for( Int i = i_begin; i < i_end-1; ++i )
                    {
                        const Int j = i+1;
                      
                        copy_buffer<3>( &v[3*i], edge_coords.data(i,0) );
                        copy_buffer<3>( &v[3*j], edge_coords.data(i,1) );
                    }

                    {
                        const Int i = i_end-1;
                        const Int j = i_begin;

                        copy_buffer<3>( &v[3*i], edge_coords.data(i,0) );
                        copy_buffer<3>( &v[3*j], edge_coords.data(i,1) );
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
                    
                    copy_buffer<3>( &v[3*i], edge_coords.data(edge,0) );
                    copy_buffer<3>( &v[3*j], edge_coords.data(edge,1) );
                }
            }
            
            ptoc(ClassName()+"::ReadVertexCoordinates (AoS)");
        }
        
        void ReadVertexCoordinates( cptr<Real> x, cptr<Real> y, cptr<Real> z )
        {
            ptic(ClassName()+"::ReadVertexCoordinates (SoA)");
            
            if( preorderedQ )
            {
                for( Int c = 0; c < component_count; ++c )
                {
                    const Int i_begin = component_ptr[c  ];
                    const Int i_end   = component_ptr[c+1];

                    for( Int i = i_begin; i < i_end-1; ++i )
                    {
                        const int j = i+1;

                        edge_coords(i,0,0) = x[i];
                        edge_coords(i,0,1) = y[i];
                        edge_coords(i,0,2) = z[i];
                        
                        edge_coords(i,1,0) = x[j];
                        edge_coords(i,1,1) = y[j];
                        edge_coords(i,1,2) = z[j];
                    }

                    {
                        const Int i = i_end-1;
                        const Int j = i_begin;

                        edge_coords(i,0,0) = x[i];
                        edge_coords(i,0,1) = y[i];
                        edge_coords(i,0,2) = z[i];
                        
                        edge_coords(i,1,0) = x[j];
                        edge_coords(i,1,1) = y[j];
                        edge_coords(i,1,2) = z[j];
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
                    
                    edge_coords(edge,0,0) = x[i];
                    edge_coords(edge,0,1) = y[i];
                    edge_coords(edge,0,2) = z[i];
                    
                    edge_coords(edge,1,0) = x[j];
                    edge_coords(edge,1,1) = y[j];
                    edge_coords(edge,1,2) = z[j];
                }
            }
            
            ptoc(ClassName()+"::ReadVertexCoordinates (SoA)");
        }
        
        
        void Rotate()
        {
            cptr<Int> edge_tails = edges.data(0);
            cptr<Int> edge_tips  = edges.data(1);
            
            for( Int edge = 0; edge < edge_count; ++edge )
            {
                const Int i = edge_tails[edge];
                const Int j = edge_tips [edge];
                
                const Vector3_T x ( edge_coords.data(edge,0) );
                const Vector3_T y ( edge_coords.data(edge,1) );
                
                const Vector3_T Rx = Dot( R, x );
                const Vector3_T Ry = Dot( R, y );
                
                Rx.Write( edge_coords.data(edge,0) );
                Ry.Write( edge_coords.data(edge,1) );
            }
        }
        
        void ApplySterbenzShift()
        {
            // Add 1% security margin.
            const Real margin = static_cast<Real>(1.01);
            
            
            Real lo = edge_coords(0,0,2);
            Real hi = edge_coords(0,1,2);
            
            for( Int e = 1; e < edge_count; ++e )
            {
                lo = Min ( lo, edge_coords(e,0,2) );
                hi = Max ( hi, edge_coords(e,0,2) );
            }
            
            constexpr Real two = 2;

            Sterbenz_shift[0] = margin * ( box_coords(0,0,1) - two * box_coords(0,0,0) );
            Sterbenz_shift[1] = margin * ( box_coords(0,1,1) - two * box_coords(0,1,0) );
            Sterbenz_shift[2] = margin * ( hi                - two * lo                );

            
            for( Int C = 0; C < T.NodeCount(); ++C )
            {
                box_coords(C,0,0) += Sterbenz_shift[0];
                box_coords(C,0,1) += Sterbenz_shift[0];
                
                box_coords(C,1,0) += Sterbenz_shift[1];
                box_coords(C,1,1) += Sterbenz_shift[1];
            }
            
            const Int edge_count = edge_coords.Dimension(0);
            
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
        
    public:
        
        static constexpr Int AmbientDimension()
        {
            return 3;
        }
        
        
        
#include "Link_2D/FindIntersections_DFS.hpp"
        
    public:
        
        void FindIntersections()
        {
            ptic(ClassName()+"FindIntersections");
            
            // TODO: Randomly rotate until not degenerate.
            
            // Here we do something strange:
            // We hand over edge_coords, a Tensor3 of size edge_count x 2 x 3
            // to a T which is a Tree2_T.
            // The latter expects a Tensor3 of size edge_count x 2 x 2, but it accesses the
            // enties only via operator(i,j,k), so this is safe!
            
            T.ComputeBoundingBoxes( edge_coords, box_coords );
            
            ApplySterbenzShift();
            
            FindIntersectingEdges_DFS();
            
            // We are going to use edge_ptr for the assembly; because we are going to modify it, we need a copy.
            edge_ctr.Read( edge_ptr.data() );
            
            if( edge_intersections.Size() != edge_ptr.Last() )
            {
                edge_intersections = Tensor1<Int, Int>( edge_ptr.Last() );
                edge_times         = Tensor1<Real,Int>( edge_ptr.Last() );
                edge_overQ         = Tensor1<bool,Int>( edge_ptr.Last() );
            }

            // We are going to fill edge_intersections so that data of the i-th edge lies in edge_intersections[edge_ptr[i]],..,edge_intersections[edge_ptr[i+1]].
            // To this end, we use (and modify!) edge_ctr so that edge_ctr[i] points AFTER the position to insert.
            
            
            const Int intersection_count = static_cast<Int>(intersections.size());
            
            // Counting sort.
            for( Int k = intersection_count-1; k > -1; --k )
            {
                Intersection_T & inter = intersections[k];
                
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
            ThreeArraySort<Real,Int,bool,Size_T> S ( intersection_count );

            for( Int i = 0; i < edge_count; ++i )
            {
                // This is the range of data in edge_intersections/edge_times that belongs to edge i.
                const Int k_begin = edge_ptr[i  ];
                const Int k_end   = edge_ptr[i+1];
                     
                S(
                    &edge_times[k_begin],
                    &edge_intersections[k_begin],
                    &edge_overQ[k_begin],
                    k_end - k_begin
                );
            }
            
            // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
            
            ptoc(ClassName()+"FindIntersections");
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
        
        static inline SInt Sign( const Real x )
        {
            return Tools::Sign<SInt>(x);
        }
        
        Int CrossingCount() const
        {
            return static_cast<Int>( intersections.size() );
        }
        
        Int DegenerateIntersectionCount() const
        {
            return intersections_nontransversal;
        }
        
        Int InvalidIntersectionCount() const
        {
            return intersections_3D;
        }
        
        cref<EContainer_T> EdgeCoordinates() const
        {
            return edge_coords;
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
        
        static std::string ClassName()
        {
            return std::string("Link_2D")+"<"+TypeName<Real>+","+TypeName<Int>+">";
        }
    };
    
} // namespace KnotTools
