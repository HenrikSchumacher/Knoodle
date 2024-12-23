protected:
    
    
    void FindIntersectingEdges_DFS()
    {
        ptic(ClassName()+"::FindIntersectingEdges_DFS");
        
        FindIntersectingEdges_DFS_impl_0();
        
//        FindIntersectingEdges_DFS_impl_1();
        
//        FindIntersectingEdges_DFS_impl_2();

        ptoc(ClassName()+"::FindIntersectingEdges_DFS");
        
    } // FindIntersectingClusters_DFS

    void FindIntersectingEdges_DFS_impl_0()
    {
        const Int int_node_count = T.InteriorNodeCount();
        
        intersections.clear();
        
        intersections.reserve( 2 * edge_coords.Dimension(0) );
        
        S = Intersector_T();
        
        intersection_count_3D = 0;
        
        Int stack[max_depth][2];
        Int stack_ptr = 0;
        stack[0][0] = 0;
        stack[0][1] = 0;
        
        edge_ptr.Fill(0);
        
        while( (0 <= stack_ptr) && (stack_ptr < max_depth - 4) )
        {
            // Pop from stack.

            const Int i = stack[stack_ptr][0];
            const Int j = stack[stack_ptr][1];
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
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = R_j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = R_i;
                            stack[stack_ptr][1] = R_j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = L_j;
                        }
                        else
                        {
                            // tie breaker: split both clusters

                            ++stack_ptr;
                            stack[stack_ptr][0] = R_i;
                            stack[stack_ptr][1] = R_j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = R_j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = R_i;
                            stack[stack_ptr][1] = L_j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = L_j;
                        }
                    }
                    else
                    {
                        // split only larger cluster
                        if( is_interior_i ) // !is_interior_j follows from this.
                        {
                            //split cluster i

                            ++stack_ptr;
                            stack[stack_ptr][0] = R_i;
                            stack[stack_ptr][1] = j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = j;
                        }
                        else //score_i < score_j
                        {
                            //split cluster j

                            ++stack_ptr;
                            stack[stack_ptr][0] = i;
                            stack[stack_ptr][1] = R_j;

                            ++stack_ptr;
                            stack[stack_ptr][0] = i;
                            stack[stack_ptr][1] = L_j;
                        }
                    }
                }
                else
                {
                    ComputeEdgeIntersection( T.NodeBegin(i), T.NodeBegin(j) );
                }
            }
        }
        
        edge_ptr.Accumulate();
        
    } // FindIntersectingClusters_DFS_impl_1

public:

    bool BoxesIntersectQ( const Int i, const Int j ) const
    {
        return T.BoxesIntersectQ( box_coords.data(i), box_coords.data(j) );
    }

protected:

    void ComputeEdgeIntersection( const Int k, const Int l )
    {
        // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
        if( (l != k) && (l != next_edge[k]) && (k != next_edge[l]) )
        {
//            Time start_time = Clock::now();
            
            // Get the edge lengths in order to decide what's a "small" determinant.
            
            const Tiny::Matrix<2,3,Real,Int> x { edge_coords.data(k) };
            const Tiny::Matrix<2,3,Real,Int> y { edge_coords.data(l) };
            
            const LineSegmentsIntersectionFlag flag
                = S.IntersectionType( x[0], x[1], y[0], y[1] );
            
//            ++intersection_counts[ ToUnderlying(flag) ];
            
            if( IntersectingQ(flag) )
            {
                auto [t,sign] = S.IntersectionTimesAndSign();
                
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
                    
                    intersections.push_back( Intersection_T(l,k,t[1],t[0],-sign) );
                    
                    //      If det > 0, then this looks like this (left-handed crossing):
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
                    //      If det < 0, then this looks like this (right-handed crossing):
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
                    intersections.push_back( Intersection_T(k,l,t[0],t[1],sign) );
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
                    ++intersection_count_3D;
                }
            }
            
//            Time stop_time = Clock::now();
//            
//            line_intersection_time += Tools::Duration(start_time,stop_time);
        }
    }
