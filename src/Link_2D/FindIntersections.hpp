public:
    
    template<bool verboseQ = true> // whether to print errors and warnings
    [[nodiscard]] int FindIntersections()
    {
//        TOOLS_PTIC(ClassName()+"FindIntersections");
        TOOLS_PTIMER(timer,ClassName()+"FindIntersections");
        
        // Here we do something strange:
        // We hand over edge_coords, a Tensor3 of size edge_count x 2 x 3
        // to a T which is a Tree2_T.
        // The latter expects a Tensor3 of size edge_count x 2 x 2, but it accesses the
        // enties only via operator(i,j,k), so this is safe!

        ComputeBoundingBoxes();
        
        const Int degenerate_edge_count = DegenerateEdgeCount();
        
        if( degenerate_edge_count > Int(0) )
        {
            if constexpr ( verboseQ )
            {
                eprint(ClassName() + "::FindIntersections: Detected " + ToString(degenerate_edge_count) + " degenerate edges.");
            }
            return 9;
        }
          
        // TODO: Randomly rotate until not degenerate.
        
        TOOLS_PTIC("FindIntersectingEdges_DFS");
        FindIntersectingEdges_DFS();
        TOOLS_PTOC("FindIntersectingEdges_DFS");
        
        // Check for bad intersections.

        {
            const Size_T count = intersection_flag_counts[7];
            if( count > Size_T(0) )
            {
                if constexpr ( verboseQ )
                {
                    eprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where line segments intersection times were out of bounds.");
                }
                return 7;
            }
        }
        
        {
            const Size_T count = intersection_flag_counts[6];
            if( count > Size_T(0) )
            {
                if constexpr ( verboseQ )
                {
                    eprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where line segments intersected in 3D.");
                }
                return 6;
            }
        }
        
        {
            const Size_T count = intersection_flag_counts[5];
            if( count > Size_T(0) )
            {
                if constexpr ( verboseQ )
                {
                    eprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where the line-line intersection was degenerate (the intersection set was an interval). Try to randomly rotate the input coordinates.");
                }
                return 5;
            }
        }
        
        {
            const Size_T count = intersection_flag_counts[4];
            
            if( count > Size_T(0) )
            {
                if constexpr ( verboseQ )
                {
                    wprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where the line-line intersection was a point in the corners of two line segments. Try to randomly rotate the input coordinates.");
                }
                return 4;
            }
        }
        
        {
            const Size_T count =
                  intersection_flag_counts[2]
                + intersection_flag_counts[3];
            
            if( count > Size_T(0) )
            {
                if constexpr ( verboseQ )
                {
                    wprint(ClassName() + "::FindIntersections: Detected " + ToString(count) + " cases where the line-line intersection was a point in a corner of a line segment. Try to randomly rotate the input coordinates.");
                }
                return 3;
            }
        }
        
        // Check for integer overflow.
        if( std::cmp_greater(
                Size_T(4) * intersections.size(),
                std::numeric_limits<Int>::max()
            )
        )
        {
            eprint(ClassName() + "::FindIntersections: More intersections found than can be handled by integer type " + TypeName<Int> + "." );
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
        
        if( intersection_count <= Int(0) )
        {
            return 0;
        }
        
        TOOLS_PTIC("Counting sort");
        for( Int k = intersection_count; k --> Int(0);  )
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
        
        Size_T close_counter = 0;
        
        for( Int i = 0; i < edge_count; ++i )
        {
            // This is the range of data in edge_intersections/edge_times that belongs to edge i.
            const Int k_begin = edge_ptr[i  ];
            const Int k_end   = edge_ptr[i+1];
                 
            // We need to sort only if there are at least two intersections on that edge.
            if( k_begin + Int(1) < k_end )
            {
                sort(
                    &edge_times[k_begin],
                    &edge_intersections[k_begin],
                    &edge_overQ[k_begin],
                    k_end - k_begin
                );
                
                constexpr Real intersection_time_tolerance = 0.000000000001;
                
                for( Int l = k_begin + Int(1); l < k_end; ++l )
                {
                    const Real delta = edge_times[l] - edge_times[l-1];
                    
                    if( delta < intersection_time_tolerance )
                    {
                        ++close_counter;
                        
                        if constexpr ( verboseQ )
                        {
                            auto inter_0 = intersections[
                                static_cast<Size_T>(edge_intersections[l-1])
                            ];
                            auto inter_1 = intersections[
                                static_cast<Size_T>(edge_intersections[l  ])
                            ];
                            
                            const Int j_0 = (inter_0.edges[0] == i) ? inter_0.edges[1] : inter_0.edges[0];
                            
                            const Int j_1 = (inter_1.edges[0] == i) ? inter_1.edges[1] : inter_1.edges[0];
                            
                            wprint(ClassName()+"::FindIntersections: Detected tiny difference of intersection times = " + ToStringFPGeneral(delta) + " < " + ToStringFPGeneral(intersection_time_tolerance)+ " = intersection_time_tolerance for intersections of line segment " + ToString(i) + " with line segments " + ToString(j_0) + " (" + (edge_overQ[l-1] ? "over" : "under") + ") and " + ToString(j_1) + " (" + (edge_overQ[l] ? "over" : "under") + ")." );
                        }
                    }
                }
            }
        }
        
        intersection_flag_counts[8] = close_counter;
        
        TOOLS_PTOC("Counting sort");
        
        
        if( intersection_flag_counts[8] )
        {
            if constexpr ( verboseQ )
            {
                wprint(ClassName()+"::FindIntersections: Detected " + ToString(close_counter) + " case(s) of tiny difference between intersection times." );
            }
            return 8;
        }
        
        // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
        
        return 0;
    }

private:

    void FindIntersectingEdges_DFS()
    {
        TOOLS_PTIC(ClassName()+"::FindIntersectingEdges_DFS");
        
        intersections.clear();
        
        intersections.reserve( ToSize_T(2 * EdgeCount()) );
        
        S = Intersector_T();
        
        intersection_count_3D = 0;
        
        edge_ptr.Fill(0);
        
        // Last time I checked the _ManualStack version was 5% faster.
        
        FindIntersectingEdges_DFS_ManualStack();

//        FindIntersectingEdges_DFS_Recursive(T.Root(),T.Root());
        
        edge_ptr.Accumulate();

        TOOLS_PTOC(ClassName()+"::FindIntersectingEdges_DFS");
        
    } // FindIntersectingClusters_DFS


    // Improved version of FindIntersectingEdges_DFS_impl_0; we do the box-box checks of all the children at once; this saves us a couple of cache misses.
    void FindIntersectingEdges_DFS_ManualStack()
    {
        const Int int_node_count = T.InternalNodeCount();

        Int stack [4 * max_depth][2];
        Int stack_ptr = -1;

        // Helper routine to manage the pair_stack.
        auto push = [&stack,&stack_ptr]( const Int i, const Int j )
        {
            ++stack_ptr;
            stack[stack_ptr][0] = i;
            stack[stack_ptr][1] = j;
        };
        
        // Helper routine to manage the pair_stack.
        auto conditional_push = [this,push]( const Int i, const Int j )
        {
            if( this->BoxesIntersectQ(i,j) )
            {
                push(i,j);
            }
        };

        // Helper routine to manage the pair_stack.
        auto pop = [&stack,&stack_ptr]()
        {
            const std::pair result ( stack[stack_ptr][0], stack[stack_ptr][1] );
            stack_ptr--;
            return result;
        };
        
        auto continueQ = [&stack_ptr,this]()
        {
            const bool overflowQ = (stack_ptr >= Int(4) * max_depth - Int(4));
            
            if( (Int(0) <= stack_ptr) && (!overflowQ) ) [[likely]]
            {
                return true;
            }
            else
            {
                if ( overflowQ ) [[unlikely]]
                {
                    eprint(this->ClassName()+"::FindIntersectingEdges_DFS_impl_1: Stack overflow.");
                }
                return false;
            }
        };
        
        
        push(0,0);
        
    //        Size_T box_call_count  = 0;
    //        Size_T edge_call_count = 0;
    //
    //        double box_time  = 0;
    //        double edge_time = 0;
        
        while( continueQ() )
        {
            // Pop from stack.

            auto [i,j] = pop();

    //            Time box_start_time = Clock::now();
    //            ++box_call_count;
            
            
    //            Time box_end_time = Clock::now();
    //            box_time += Tools::Duration( box_start_time, box_end_time );
            
            
            const bool i_internalQ = (i < int_node_count);
            const bool j_internalQ = (j < int_node_count);
            
            // Warning: This assumes that both children in a cluster tree are either defined or empty.
            
            if( i_internalQ || j_internalQ ) // [[likely]]
            {
                auto [L_i,R_i] = Tree2_T::Children(i);
                auto [L_j,R_j] = Tree2_T::Children(j);
                
                // T is a balanced binary tree.

                if( i_internalQ == j_internalQ )
                {
                    if( i == j )
                    {
                        //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                        
                        conditional_push(L_i,R_j);
                        push(R_i,R_j);
                        push(L_i,L_j);
                    }
                    else
                    {
                        // tie breaker: split both clusters
                        conditional_push(R_i,R_j);
                        conditional_push(L_i,R_j);
                        conditional_push(R_i,L_j);
                        conditional_push(L_i,L_j);
                    }
                }
                else
                {
                    // split only larger cluster
                    if( i_internalQ ) // !j_internalQ follows from this.
                    {
                        //split cluster i
                        conditional_push(R_i,j);
                        conditional_push(L_i,j);
                    }
                    else
                    {
                        //split cluster j
                        conditional_push(i,R_j);
                        conditional_push(i,L_j);
                    }
                }
            }
            else
            {
    //                    Time edge_start_time = Clock::now();
    //                    ++edge_call_count;
                ComputeEdgeEdgeIntersection( T.NodeBegin(i), T.NodeBegin(j) );

    //                    Time edge_end_time = Clock::now();
    //                    edge_time += Tools::Duration( edge_start_time, edge_end_time );
            }
        }
        
    //        TOOLS_DUMP(box_call_count);
    //        TOOLS_DUMP(box_time);
    //        TOOLS_DUMP(edge_call_count);
    //        TOOLS_DUMP(edge_time);
        
    } // FindIntersectingEdges_DFS_ManualStack


    void FindIntersectingEdges_DFS_Recursive( const Int i, const Int j )
    {
        const bool i_internalQ = T.InternalNodeQ(i);
        const bool j_internalQ = T.InternalNodeQ(j);
        
        // Warning: This assumes that both children in a cluster tree are either defined or empty.
        
        if( i_internalQ || j_internalQ ) // [[likely]]
        {
            auto [L_i,R_i] = Tree2_T::Children(i);
            auto [L_j,R_j] = Tree2_T::Children(j);
            
            // T is a balanced bindary tree.

            if( i_internalQ == j_internalQ )
            {
                if( i == j )
                {
                    //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                    
                    const bool subdQ = BoxesIntersectQ(L_i,R_i);
                    
                    FindIntersectingEdges_DFS_Recursive(L_i,L_i);
                    FindIntersectingEdges_DFS_Recursive(R_i,R_i);
                    
                    if( subdQ )
                    {
                        FindIntersectingEdges_DFS_Recursive(L_i,R_i);
                    }
                }
                else
                {
                    const bool subdQ [2][2] = {
                        { BoxesIntersectQ(L_i,L_j), BoxesIntersectQ(L_i,R_j) },
                        { BoxesIntersectQ(R_i,L_j), BoxesIntersectQ(R_i,R_j) },
                    };
                    
                    if( subdQ[0][1] )
                    {
                        FindIntersectingEdges_DFS_Recursive(L_i,R_j);
                    }
                    if( subdQ[1][0] )
                    {
                        FindIntersectingEdges_DFS_Recursive(R_i,L_j);
                    }
                    if( subdQ[0][0] )
                    {
                        FindIntersectingEdges_DFS_Recursive(L_i,L_j);
                    }
                    if( subdQ[1][1] )
                    {
                        FindIntersectingEdges_DFS_Recursive(R_i,R_j);
                    }
                }
            }
            else
            {
                // split only larger cluster
                if( i_internalQ ) // !j_internalQ follows from this.
                {
                    //split cluster i
                    
                    const bool subdQ [2] = {
                        BoxesIntersectQ(L_i,j), BoxesIntersectQ(R_i,j)
                    };
                    
                    if( subdQ[0] )
                    {
                        FindIntersectingEdges_DFS_Recursive(L_i,j);
                    }
                    if( subdQ[1] )
                    {
                        FindIntersectingEdges_DFS_Recursive(R_i,j);
                    }
                }
                else
                {
                    //split cluster j
                    const bool subdQ [2] = {
                        BoxesIntersectQ(i,L_j), BoxesIntersectQ(i,R_j)
                    };
                    
                    if( subdQ[0] )
                    {
                        FindIntersectingEdges_DFS_Recursive(i,L_j);
                    }
                    if( subdQ[1] )
                    {
                        FindIntersectingEdges_DFS_Recursive(i,R_j);
                    }
                }
            }
        }
        else
        {
            ComputeEdgeEdgeIntersection( T.NodeBegin(i), T.NodeBegin(j) );
        }
    }

public:

    bool BoxesIntersectQ( const Int i, const Int j ) const
    {
        return T.BoxesIntersectQ( box_coords.data(i), box_coords.data(j) );
    }

protected:

    void ComputeEdgeEdgeIntersection( const Int k, const Int l )
{
        // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
        if( (l != k) && (l != NextEdge(k)) && (k != NextEdge(l)) )
        {
//            constexpr Int k0 = 4453;
//            constexpr Int l0 = 7619;
//
//            const bool verboseQ = (k == k0) && (l == l0);
//
//            if( verboseQ )
//            {
//                this->template ComputeEdgeEdgeIntersection_impl<true>(k,l);
//            }
//            else
//            {
//                this->template ComputeEdgeEdgeIntersection_impl<false>(k,l);
//            }
            
            this->template ComputeEdgeEdgeIntersection_impl<false>(k,l);
        }
    }


    template<bool verboseQ>
    void ComputeEdgeEdgeIntersection_impl( const Int k, const Int l )
    {
        if constexpr ( verboseQ )
        {
            print(ClassName() + "::ComputeEdgeEdgeIntersection in verbose mode.");
            TOOLS_DUMP(k);
            TOOLS_DUMP(l);
        }
        
        // At this point we assume that `k != l` and that they are also not direct neighbors.

        const E_T x = EdgeData(k);
        const E_T y = EdgeData(l);
        
        if constexpr ( verboseQ )
        {
            TOOLS_DUMP(ToString(x));
            TOOLS_DUMP(ToString(y));
        }
        
        LineSegmentsIntersectionFlag flag
            = S.template IntersectionType<verboseQ>( x[0], x[1], y[0], y[1] );
        
        if constexpr ( verboseQ )
        {
            TOOLS_DUMP(flag);
        }
        
        if( IntersectingQ(flag) )
        {
            auto [t,sign] = S.IntersectionTimesAndSign();
            
            
            if( (t[0]<Real(0)) || (t[0]>=Real(1)) || (t[1]<Real(0)) || (t[1]>=Real(1)) )
            {
                flag = LineSegmentsIntersectionFlag::OOBounds;
            }
            
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
                
                /*      If det > 0, then this looks like this (left-handed crossing):
                 *
                 *        v       u
                 *         ^     ^
                 *          \   /
                 *           \ /
                 *            \
                 *           / \
                 *          /   \
                 *         /     \
                 *        k       l
                 *
                 *      If det < 0, then this looks like this (right-handed crossing):
                 *
                 *        u       v
                 *         ^     ^
                 *          \   /
                 *           \ /
                 *            /
                 *           / \
                 *          /   \
                 *         /     \
                 *        l       k
                 */
            }
            else if ( h[0] > h[1] )
            {
                intersections.push_back( Intersection_T(k,l,t[0],t[1],sign) );
                // edge k goes OVER l
                
                /*      If det > 0, then this looks like this (positive crossing):
                 *
                 *        v       u
                 *         ^     ^
                 *          \   /
                 *           \ /
                 *            /
                 *           / \
                 *          /   \
                 *         /     \
                 *        k       l
                 *
                 *      If det < 0, then this looks like this (positive crossing):
                 *
                 *        u       v
                 *         ^     ^
                 *          \   /
                 *           \ /
                 *            \
                 *           / \
                 *          /   \
                 *         /     \
                 *        l       k
                 */
            }
            else
            {
                flag = LineSegmentsIntersectionFlag::Spatial;
            }
            
        } // if( IntersectingQ(flag) )
        
        ++intersection_flag_counts[ ToUnderlying(flag) ];
        
        switch(flag)
        {
            case LineSegmentsIntersectionFlag::AtCorner0:
            {
                wprint(ClassName() + "::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in first corner of edge " + ToString(k) + ".");
                break;
            }
            case LineSegmentsIntersectionFlag::AtCorner1:
            {
                wprint(ClassName() + "::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in first corner of edge " + ToString(l) + ".");
                break;
            }
            case LineSegmentsIntersectionFlag::CornerCorner:
            {
                wprint(ClassName() + "::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " have common first corners.");
                break;
            }
            case LineSegmentsIntersectionFlag::Interval:
            {
                wprint(ClassName() + "::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in an interval.");
                break;
            }
            case LineSegmentsIntersectionFlag::Spatial:
            {
                wprint(ClassName() + "::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in 3D.");
                
                // DEBUGGING
                
                logvalprint( "edge " + ToString(k), x );
                logvalprint( "edge " + ToString(l), y );
                logprint("full polygon");
                for( Int e = 0; e < edge_count; ++e )
                {
                    logvalprint( "edge " + ToString(e), EdgeData(e) );
                }
                break;
            }
            case LineSegmentsIntersectionFlag::OOBounds:
            {
                wprint(ClassName() + "::ComputeEdgeIntersection: Intersection times of intersection between edges " + ToString(k) + " and " + ToString(l) + " are out of bounds.");
                break;
            }
            default:
            {
                break;
            }
        }
        
//        if( t[0] < Real(0) )
//        {
//        }
    }


