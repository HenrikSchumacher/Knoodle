public:

    bool BoxesIntersectQ( const Int i, const Int j ) const
    {
        if constexpr ( VectorizableQ<Real> )
        {
            // TODO: Not sure whether this makes things faster or slower...
            vec_T<4,Real> a {
                box_coords(i,0,0), box_coords(j,0,0), box_coords(i,1,0), box_coords(j,1,0)
            };
            
            vec_T<4,Real> b {
                box_coords(j,0,1), box_coords(i,0,1), box_coords(j,1,1), box_coords(i,1,1)
            };
            
            return __builtin_reduce_and( a <= b );
        }
        else
        {
            return
                (i == j)
                ? true
                : (
                   ( box_coords(i,0,0) <= box_coords(j,0,1) )
                   &&
                   ( box_coords(i,0,1) >= box_coords(j,0,0) )
                   &&
                   ( box_coords(i,1,0) <= box_coords(j,1,1) )
                   &&
                   ( box_coords(i,1,1) >= box_coords(j,1,0) )
                );
        }
        
        
//        return
//            (i == j)
//            ? true
//            : (
//               ( box_coords(i,0,0) <= box_coords(j,0,1) )
//               &&
//               ( box_coords(i,0,1) >= box_coords(j,0,0) )
//               &&
//               ( box_coords(i,1,0) <= box_coords(j,1,1) )
//               &&
//               ( box_coords(i,1,1) >= box_coords(j,1,0) )
//            );
//        }
    }


    void ComputeEdgeIntersections( const Int k, const Int l )
    {
        // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
        if( (l != k) && (l != next_edge[k]) && (k != next_edge[l]) )
        {
            // Get the edge lengths in order to decide what's a "small" determinant.
            
            const Vector3_T x[2] = { edge_coords.data(k,0), edge_coords.data(k,1) };
            
            const Vector3_T y[2] = { edge_coords.data(l,0), edge_coords.data(l,1) };
            
            // TODO: Need more exact intersection test!
            
            const Vector2_T d { y[0][0] - x[0][0], y[0][1] - x[0][1] };
            const Vector2_T u { x[1][0] - x[0][0], x[1][1] - x[0][1] };
            const Vector2_T v { y[1][0] - y[0][0], y[1][1] - y[0][1] };

            const Real det = Det_Kahan( u, v );
            
            Real t[2];

            bool intersecting;

            if( det * det > eps * Dot_Kahan(u,u) * Dot_Kahan(v,v) )
            {
                const Real det_inv = static_cast<Real>(1) / det;
                
                t[0] = Det_Kahan( d, v ) * det_inv;
                t[1] = Det_Kahan( d, u ) * det_inv;
                
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
                    x[0][2] * (one - t[0]) + t[0] * x[1][2],
                    y[0][2] * (one - t[1]) + t[1] * y[1][2]
                };
                
                // Tell edges k and l that they contain an additional crossing.
                edge_ptr[k+1]++;
                edge_ptr[l+1]++;

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

protected:
    
    

    void FindIntersectingEdges_DFS()
    {
        ptic(ClassName()+"::FindIntersectingEdges_DFS");
        
        const Int int_node_count = T.InteriorNodeCount();
        
        intersections.clear();
        
        intersections.reserve( 2 * edge_coords.Dimension(0) );
        
        intersections_3D = 0;
        intersections_nontransversal = 0;
        
        Int i_stack[max_depth] = {};
        Int j_stack[max_depth] = {};
        
        Int stack_ptr = 0;
        i_stack[0] = 0;
        j_stack[0] = 0;
        
        edge_ptr.Fill(0);
        
        while( (0 <= stack_ptr) && (stack_ptr < max_depth - 4) )
        {
            // Pop from stack.
            
            const Int i = i_stack[stack_ptr];
            const Int j = j_stack[stack_ptr];
            stack_ptr--;
            

            if( BoxesIntersectQ(i,j) )
            {
                const bool is_interior_i = (i < int_node_count);
                const bool is_interior_j = (j < int_node_count);
                
                // Warning: This assumes that both children in a cluster tree are either defined or empty.
                
                if( is_interior_i || is_interior_j )
                {
                    const Int L_i = Tree2_T::LeftChild(i);
                    const Int R_i = L_i+1;
                    
                    const Int L_j = Tree2_T::LeftChild(j);
                    const Int R_j = L_j+1;
                    
                    // T is a balanced bindary tree.

                    if( is_interior_i == is_interior_j )
                    {
                        if( i == j )
                        {
                            //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = L_i;
                            j_stack[stack_ptr] = R_j;
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = R_i;
                            j_stack[stack_ptr] = R_j;
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = L_i;
                            j_stack[stack_ptr] = L_j;
                        }
                        else
                        {
                            // tie breaker: split both clusters
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = R_i;
                            j_stack[stack_ptr] = R_j;
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = L_i;
                            j_stack[stack_ptr] = R_j;
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = R_i;
                            j_stack[stack_ptr] = L_j;
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = L_i;
                            j_stack[stack_ptr] = L_j;
                        }
                    }
                    else
                    {
                        // split only larger cluster
                        if( is_interior_i ) // !is_interior_j follows from this.
                        {
                            ++stack_ptr;
                            i_stack[stack_ptr] = R_i;
                            j_stack[stack_ptr] = j;
                            
                            //split cluster i
                            ++stack_ptr;
                            i_stack[stack_ptr] = L_i;
                            j_stack[stack_ptr] = j;
                        }
                        else //score_i < score_j
                        {
                            //split cluster j
                            ++stack_ptr;
                            i_stack[stack_ptr] = i;
                            j_stack[stack_ptr] = R_j;
                            
                            ++stack_ptr;
                            i_stack[stack_ptr] = i;
                            j_stack[stack_ptr] = L_j;
                        }
                    }
                }
                else
                {
                    ComputeEdgeIntersections( T.Begin(i), T.Begin(j ) );
                }
            }
        }
        
        edge_ptr.Accumulate();
        
        ptoc(ClassName()+"::FindIntersectingEdges_DFS");
    } // FindIntersectingClusters_DFS
