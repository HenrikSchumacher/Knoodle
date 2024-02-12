#pragma  once

namespace KnotTools
{
    template<typename Real_ = double, typename Int_ = long long, typename SInt_ = signed char>
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
    public:
        
        using Real = Real_;
        using Int  = Int_;
        using Int  = SInt_;
        
        using Base_T = Link<Int>;
        
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
    
        
        using Tree_T   = AABBTree<2,Real,Int>;
        using Intersection_T = Intersection<Real,Int,SInt>;
        
        using BinaryMatrix_T = Sparse::BinaryMatrixCSR<Int,std::size_t>;
        
    public:
        
        using Base_T::ComponentCount;
        using Base_T::VertexCount;
        using Base_T::EdgeCount;
        using Base_T::Edges;
        
    protected:
        
        //Containers and data whose sizes stay constant under ReadVertexCoordinates.
        Tiny::MatrixList<2,3,Real,Int> edge_coords;

        Tiny::Matrix<3,3,Real,Int> { {1,0,0}, {0,1,0}, {0,0,1} }; // a rotation matrix (later to be randomized)

        Tree_T T;
        
        // Containers that might have to be reallocated after calls to ReadVertexCoordinates.
        std::vector<Intersection_T> intersections;
        Tensor1<Int,Int>  edge_intersections;
        Tensor1<Real,Int> edge_times;
        Tensor1<bool,Int> edge_overQ;
                 
        Int intersections_3D = 0;
        Int intersections_nontransversal = 0;


    public:
        
        Link_2D() = default;
        
        ~Link_2D() = default;
        
        
        // Calling this constructor makes the object assume that it represents a cyclic polyline.
        explicit Link_2D(
            const Int edge_count_
        )
        :   Base_T      ( edge_count_)
        ,   edge_coords ( edge_count_)
        ,   T           ( edge_count_)
        {
            ptic(ClassName()+"() (cyclic)");
            
            intersections.reserve( static_cast<size_t>(2 * edge_count_) );
                        
            ptoc(ClassName()+"() (cyclic)");
        }
        
        template<typename J, typename K, IS_INT(J), IS_INT(K)>
        explicit Link_2D(
            Tensor1<J,K> & component_ptr_
        )
        :   Base_T      ( component_ptr_       )
        ,   edge_coords ( component_ptr.Last() )
        ,   T           ( component_ptr.Last() )
        {}
        
        // Provide a list of edges in interleaved form to make the object figure out its topology.
        Link_2D(
            cptr<Int> edges_,
            const Int edge_count_
        )
        :   Base_T      ( edges_, edge_count_ )
        ,   edge_coords ( edge_count_ )
        ,   T           ( edge_count_ )
        {}
        
        // Provide lists of edge tails and edge tips to make the object figure out its topology.
        Link_2D(
            cptr<Int> edge_tails_,
            cptr<Int> edge_tips_,
            const Int edge_count_
        )
        :   Base_T      ( edge_tails_, edge_tips_ )
        ,   edge_coords ( edge_count_ )
        ,   T           ( edge_count_ )
        {}
        
    public:
        
        void ReadVertexCoordinates( cptr<Real> v_, const bool update = false )
        {
            ptic(ClassName()+"::ReadVertexCoordinates (AoS)");
            
            mptr<Real> p [2][3] = {
                { edge_coords.data(0,0), edge_coords.data(0,1), edge_coords.data(0,2) },
                { edge_coords.data(1,0), edge_coords.data(1,1), edge_coords.data(1,2) }
            };
            
            if( preorderedQ )
            {
                for( Int c = 0; c < component_count; ++c )
                {
                    const Int i_begin = component_ptr[c  ];
                    const Int i_end   = component_ptr[c+1];

                    for( Int i = i_begin; i < i_end-1; ++i )
                    {
                        const int j = i+1;
                      
                        p[0][0][i] = v_[3*i+0];
                        p[0][1][i] = v_[3*i+1];
                        p[0][2][i] = v_[3*i+2];
                        p[1][0][i] = v_[3*j+0];
                        p[1][1][i] = v_[3*j+1];
                        p[1][2][i] = v_[3*j+2];
                    }

                    {
                        const Int i = i_end-1;
                        const Int j = i_begin;

                        p[0][0][i] = v_[3*i+0];
                        p[0][1][i] = v_[3*i+1];
                        p[0][2][i] = v_[3*i+2];
                        p[1][0][i] = v_[3*j+0];
                        p[1][1][i] = v_[3*j+1];
                        p[1][2][i] = v_[3*j+2];
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
                    
                    p[0][0][edge] = v_[3*i+0];
                    p[0][1][edge] = v_[3*i+1];
                    p[0][2][edge] = v_[3*i+2];
                    p[1][0][edge] = v_[3*j+0];
                    p[1][1][edge] = v_[3*j+1];
                    p[1][2][edge] = v_[3*j+2];
                }
            }
            ptoc(ClassName()+"::ReadVertexCoordinates (AoS)");
        }
        
        void ReadVertexCoordinates(
            cptr<Real> x_,
            cptr<Real> y_,
            cptr<Real> z_
        )
        {
            mptr<Real> p [2][3] = {
                { edge_coords.data(0,0), edge_coords.data(0,1), edge_coords.data(0,2) },
                { edge_coords.data(1,0), edge_coords.data(1,1), edge_coords.data(1,2) }
            };
            
            if( preorderedQ )
            {
                for( Int c = 0; c < component_count; ++c )
                {
                    const Int i_begin = component_ptr[c  ];
                    const Int i_end   = component_ptr[c+1];

                    for( Int i = i_begin; i < i_end-1; ++i )
                    {
                        const int j = i+1;

                        p[0][0][i] = x_[i];
                        p[0][1][i] = y_[i];
                        p[0][2][i] = z_[i];
                        p[1][0][i] = x_[j];
                        p[1][1][i] = y_[j];
                        p[1][2][i] = z_[j];
                    }

                    {
                        const Int i = i_end-1;
                        const Int j = i_begin;

                        p[0][0][i] = x_[i];
                        p[0][1][i] = y_[i];
                        p[0][2][i] = z_[i];
                        p[1][0][i] = x_[j];
                        p[1][1][i] = y_[j];
                        p[1][2][i] = z_[j];
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
                    
                    p[0][0][edge] = x_[i];
                    p[0][1][edge] = y_[i];
                    p[0][2][edge] = z_[i];
                    p[1][0][edge] = x_[j];
                    p[1][1][edge] = y_[j];
                    p[1][2][edge] = z_[j];
                }
            }
            ptoc(ClassName()+"::ReadVertexCoordinates (SoA)");
        }
        
        
        void Rotate()
        {
            mptr<Real> p [2][3] = {
                { edge_coords.data(0,0), edge_coords.data(0,1), edge_coords.data(0,2) },
                { edge_coords.data(1,0), edge_coords.data(1,1), edge_coords.data(1,2) }
            };
            
            cptr<Int> edge_tails = edges.data(0);
            cptr<Int> edge_tips  = edges.data(1);
            
            for( Int edge = 0; edge < edge_count; ++edge )
            {
                const Int i = edge_tails[edge];
                const Int j = edge_tips [edge];
                
                const Real x [3] = { p[0][0][edge], p[0][1][edge], p[0][2][edge] };
                const Real y [3] = { p[1][0][edge], p[1][1][edge], p[1][2][edge] };
                
                p[0][0][edge] = R[0][0] * x[0] + R[0][1] * x[1] + R[0][2] * x[2];
                p[0][1][edge] = R[1][0] * x[0] + R[1][1] * x[1] + R[1][2] * x[2];
                p[0][2][edge] = R[2][0] * x[0] + R[2][1] * x[1] + R[2][2] * x[2];
                p[1][0][edge] = R[0][0] * y[0] + R[0][1] * y[1] + R[0][2] * y[2];
                p[1][1][edge] = R[1][0] * y[0] + R[1][1] * y[1] + R[1][2] * y[2];
                p[1][2][edge] = R[2][0] * y[0] + R[2][1] * y[1] + R[2][2] * y[2];
            }
        }
        
    public:
        
        static constexpr Int AmbientDimension()
        {
            return 3;
        }
        
    public:
        
        void FindIntersections()
        {
            ptic(ClassName()+"FindIntersections");
            
            T.LoadCoordinates( edge_coords );

            FindIntersectingEdges_DFS();
            
            // We are going to use edge_ptr for the assembly; because we are going to modify it, we need a copy.
            edge_ctr.Read( edge_ptr.data() );
            
            if( edge_intersections.Size() < edge_ptr.Last() )
            {
                edge_intersections = Tensor1<Int,Int> ( edge_ptr.Last() );
                edge_times         = Tensor1<Real,Int>( edge_ptr.Last() );
                edge_overQ         = Tensor1<bool,Int>( edge_ptr.Last() );
            }

            // We are going to fill edge_intersections so that data of the i-th edge lies in edge_intersections[edge_ptr[i]],..,edge_intersections[edge_ptr[i+1]].
            // To this end, we use (and modify!) edge_ctr so that edge_ctr[i] points AFTER the position to insert.
            
            
            const Int intersection_count = static_cast<Int>(intersections.size());
            
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
            ThreeArrayQuickSort<Real,Int,bool> Q;

            for( Int i = 0; i < edge_count; ++i )
            {
                // This is the range of data in edge_intersections/edge_times that belongs to edge i.
                const Int k_begin = edge_ptr[i  ];
                const Int k_end   = edge_ptr[i+1];

                Q.Sort(
                    &edge_times[k_begin],
                    &edge_intersections[k_begin],
                    &edge_overQ[k_begin],
                    k_end - k_begin
                );
            }
            
            // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
            
            
            ptoc(ClassName()+"FindIntersections");
        }
        
        
    protected:
        
        void FindIntersectingEdges_DFS()
        {
            ptic(ClassName()+"::FindIntersectingEdges_DFS");
            
//            const Int expected = ( 2 * edge_count );
            
            const Int int_node_count = T.InteriorNodeCount();
            
            intersections.clear();
//            crossing_edges[0].clear();
//            crossing_edges[1].clear();
//
//            crossing_times[0].clear();
//            crossing_times[1].clear();
//
//            crossing_orient.clear();
            
            intersections_3D = 0;
            intersections_nontransversal = 0;
            
            Int i_stack[max_depth] = {};
            Int j_stack[max_depth] = {};
            
            Int stack_ptr = 0;
            i_stack[0] = 0;
            j_stack[0] = 0;
            
            edge_ptr.Fill(0);

            //Preparing pointers for quick access.
            
            cptr<Int> next = next_edge.data();
            mptr<Int> ctr  = &edge_ptr.data()[1];
            
            cptr<Real> b [2][2] = {
                { T.ClusterBoxes().data(0,0), T.ClusterBoxes().data(0,1) },
                { T.ClusterBoxes().data(1,0), T.ClusterBoxes().data(1,1) }
            };
            
            cptr<Real> p [2][3] = {
                { edge_coords.data(0,0), edge_coords.data(0,1), edge_coords.data(0,2) },
                { edge_coords.data(1,0), edge_coords.data(1,1), edge_coords.data(1,2) }
            };
            
            while( (0 <= stack_ptr) && (stack_ptr < max_depth - 4) )
            {
                const Int i = i_stack[stack_ptr];
                const Int j = j_stack[stack_ptr];
                stack_ptr--;
                    
                bool boxes_intersecting = (i == j)
                    ? true
                    : (
                        ( b[0][0][i] <= b[0][1][j] && b[0][1][i] >= b[0][0][j] ) &&
                        ( b[1][0][i] <= b[1][1][j] && b[1][1][i] >= b[1][0][j] )
                    );
                
                if( boxes_intersecting )
                {
                    const bool is_interior_i = (i < int_node_count);
                    const bool is_interior_j = (j < int_node_count);
                    
                    // Warning: This assumes that both children in a cluster tree are either defined or empty.
                    if( is_interior_i || is_interior_j )
                    {
                        const Int left_i  = Tree_T::LeftChild(i);
                        const Int right_i = left_i+1;
                        
                        const Int left_j  = Tree_T::LeftChild(j);
                        const Int right_j = left_j+1;
                        
                        // TODO: Improve score.

                        if( (is_interior_i == is_interior_j ) /*&& (score_i > static_cast<Real>(0)) && score_j > static_cast<Real>(0)*/ )
                        {
                            if( i == j )
                            {
                                //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = left_i;
                                j_stack[stack_ptr] = right_j;
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = right_i;
                                j_stack[stack_ptr] = right_j;
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = left_i;
                                j_stack[stack_ptr] = left_j;
                            }
                            else
                            {
                                // tie breaker: split both clusters
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = right_i;
                                j_stack[stack_ptr] = right_j;
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = left_i;
                                j_stack[stack_ptr] = right_j;
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = right_i;
                                j_stack[stack_ptr] = left_j;
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = left_i;
                                j_stack[stack_ptr] = left_j;
                            }
                        }
                        else
                        {
                            // split only larger cluster
                            if( is_interior_i ) // !is_interior_j follows from this.
                            {
                                ++stack_ptr;
                                i_stack[stack_ptr] = right_i;
                                j_stack[stack_ptr] = j;
                                
                                //split cluster i
                                ++stack_ptr;
                                i_stack[stack_ptr] = left_i;
                                j_stack[stack_ptr] = j;
                            }
                            else //score_i < score_j
                            {
                                //split cluster j
                                ++stack_ptr;
                                i_stack[stack_ptr] = i;
                                j_stack[stack_ptr] = right_j;
                                
                                ++stack_ptr;
                                i_stack[stack_ptr] = i;
                                j_stack[stack_ptr] = left_j;
                            }
                        }
                    }
                    else
                    {
                        // Translate node indices i and j to edge indices k and l.
                        const Int k = i - int_node_count;
                        const Int l = j - int_node_count;
                    
                        // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
                        if( (l != k) && (l != next[k]) && (k != next[l]) )
                        {
                            // Get the edge lengths in order to decide what's a "small" determinant.

                            const Real x[2][2] = { { p[0][0][k], p[0][1][k] }, { p[1][0][k], p[1][1][k] } };

                            const Real y[2][2] = { { p[0][0][l], p[0][1][l] }, { p[1][0][l], p[1][1][l] } };

                            const Real d[2] = { y[0][0]-x[0][0], y[0][1]-x[0][1] };
                            const Real u[2] = { x[1][0]-x[0][0], x[1][1]-x[0][1] };
                            const Real v[2] = { y[1][0]-y[0][0], y[1][1]-y[0][1] };

                            const Real det = u[0] * v[1] - u[1] * v[0];
                            
                            Real t[2];

                            bool intersecting;

                            const Real u_length_squared = u[0] * u[0] + u[1] * u[1];
                            const Real v_length_squared = v[0] * v[0] + v[1] * v[1];

                            if( std::abs(det*det) > eps * u_length_squared * v_length_squared )
                            {
                                const Real det_inv = static_cast<Real>(1) / det;
                                
                                t[0] = (d[0] * v[1] - d[1] * v[0]) * det_inv;
                                t[1] = (d[0] * u[1] - d[1] * u[0]) * det_inv;

                                intersecting = (t[0] > - eps) && (t[0] < big_one) && (t[1] > - eps) && (t[1] < big_one);
                            }
                            else
                            {
                                intersecting = false;

                                intersections_nontransversal++;
                            }

                            if( intersecting )
                            {
                                // Compute heights at the intersection.
                                const Real h[2] = {
                                    p[0][2][k] * (one - t[0]) + t[0] * p[1][2][k],
                                    p[0][2][l] * (one - t[1]) + t[1] * p[1][2][l]
                                };
                                
                                // Tell edges k and l that they contain an additional crossing.
                                ctr[k]++;
                                ctr[l]++;

                                if( h[0] < h[1] )
                                {
                                    // edge k goes UNDER edge l
                                    
                                    intersections.push_back(
                                        Intersection_T( l, k, t[1], t[0], -Sign(det) )
                                    );
                                    
//      If det > 0, then this looks like this (negative crossing):
//
//        v       u
//         ^     ^
//          \   /
//           \ /
//            \
//           / \
//          /   \
//         /     \
//        k       l
//
//      If det < 0, then this looks like this (positive crossing):
//
//        u       v
//         ^     ^
//          \   /
//           \ /
//            /
//           / \
//          /   \
//         /     \
//        l       k

                                }
                                else if ( h[0] > h[1] )
                                {
                                    intersections.push_back(
                                        Intersection_T( k, l, t[0], t[1], Sign(det) )
                                    );
                                    // edge k goes OVER l
                                    
//      If det > 0, then this looks like this (positive crossing):
//
//        v       u
//         ^     ^
//          \   /
//           \ /
//            /
//           / \
//          /   \
//         /     \
//        k       l
//
//      If det < 0, then this looks like this (positive crossing):
//
//        u       v
//         ^     ^
//          \   /
//           \ /
//            \
//           / \
//          /   \
//         /     \
//        l       k
                                }
                                else
                                {
                                    intersections_3D++;
                                }
                            }
                        }
                    }
                }
            }
            
            edge_ptr.Accumulate();
            
            ptoc(ClassName()+"::FindIntersectingEdges_DFS");
        } // FindIntersectingClusters_DFS
        
        
    public:

        constexpr bool Tip   = true;
        constexpr bool Tail  = false;
        constexpr bool Left  = false;
        constexpr bool Right = true;
        constexpr bool In    = true;
        constexpr bool Out   = false;
        
        PlanarDiagram<Int> CreatePlanarDiagram()
        {
            const Int intersection_count = static_cast<Int>(intersections.size());
            
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
            
            PlanarDiagram<Int> pd ( intersection_count, unlink_count );
            
            //Preparing pointers for quick access.
            
            mptr<Int> C_arcs [2][2] = {
                {pd.Crossings().data(0,0), pd.Crossings().data(0,1)},
                {pd.Crossings().data(1,0), pd.Crossings().data(1,1)}
            };
            
            mptr<Crossing_State> C_state = pd.CrossingStates().data();
            
            mptr<Int> A_crossings [2] = {pd.Arcs().data(0), pd.Arcs().data(1)};
            
            mptr<Arc_State> A_state = pd.ArcStates().data();
            
            // Now we go through all components
            //      then through all edges of the component
            //              then through all intersections of the edge
            // and generate new vertices, edges, crossings, and arcs in one go.
            

            
            PD_print("Begin of Link");
            PD_print("{");
            for( Int comp = 0; comp < component_count; ++comp )
            {
                PD_print("\tBegin of component " + ToString(c));
                PD_print("\t{");
                
                // The range of arcs belonging to this component.
                const Int arc_begin  = edge_ptr[component_ptr[comp  ]];
                const Int arc_end    = edge_ptr[component_ptr[comp+1]];
                
                PD_valprint("\t\tarc_begin", arc_begin);
                PD_valprint("\t\tarc_end"  , arc_end  );

                if( arc_begin == arc_end )
                {
                    // Component is an unlink. Just skip it.
                    continue;
                }
                
                // If we arrive here, then there is definitely a crossing in the first edge.

                for( Int b = arc_begin, a = arc_end-1; b < arc_end; a = (b++) )
                {
                    const Int c = edge_intersections[b];
                    
                    const bool overQ = edge_overQ[b];
                    
                    Intersection_T & inter = intersections[c];
                    
                    A_crossings[Tip ][a] = c; // c is tip  of a
                    A_crossings[Tail][b] = c; // c is tail of b
                    
                    PD_assert( inter.sign > SI(0) || inter.sign < SI(0) );
                    
                    bool positiveQ = inter.sign > SI(0);
                    
                    C_state[c] = positiveQ ? Crossing_State::Positive : Crossing_State::Negative;
                    A_state[a] = Arc_State::Active;
                    
                    /*
                        positiveQ == true and overQ == true:

                          C_arcs[Out][Left][c]  .       .  C_arcs[Out][Right][c] = b
                                                .       .
                                                +       +
                                                 ^     ^
                                                  \   /
                                                   \ /
                                                    /
                                                   / \
                                                  /   \
                                                 /     \
                                                +       +
                                                .       .
                       a = C_arcs[In][Left][c]  .       .  C_arcs[In][Right][c]
                    */
                    const bool over_in_side = (positiveQ == overQ) ? Left : Right ;
                    
                    
                    C_arcs[In ][ over_in_side][c] = a;
                    C_arcs[Out][!over_in_side][c] = b;
                }
        
                
                
                PD_print("\t}");
                PD_print("\tEnd   of component " + ToString(c));
                
                PD_print("");
                
            }
            PD_print("");
            PD_print("}");
            PD_print("End   of Link");
            PD_print("");
            
//            pd.CheckAllCrossings();
//            pd.CheckAllArcs();
            
            return pd;
        }
        
        
    public:
        
        static inline SInt Sign( const Real x )
        {
            return (x>0) ? static_cast<SInt>(1) : ( (x<0) ? static_cast<SInt>(-1) : static_cast<SInt>(0) );
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
        
        static std::string ClassName()
        {
            return "Link_2D<"+TypeName<Real>+","+TypeName<Int>+">";
        }
    };
    
} // namespace KnotTools
