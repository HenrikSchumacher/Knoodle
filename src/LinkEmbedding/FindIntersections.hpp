public:
    
    template<bool verboseQ = true> // whether to print errors and warnings
    [[nodiscard]] int RequireIntersections()
    {
        if( intersections_computedQ ) { return 0; }
        
        return FindIntersections();
    }
    
    template<bool verboseQ = true> // whether to print errors and warnings
    [[nodiscard]] int FindIntersections()
    {
        [[maybe_unused]] auto tag = [](){ return MethodName("FindIntersections"); };
        
        TOOLS_PTIMER(timer,tag());
        
        // Here we do something strange:
        // We hand over edge_coords, a Tensor3 of size edge_count x 2 x 3
        // to a T which is a Tree2_T.
        // The latter expects a Tensor3 of size edge_count x 2 x 2, but it accesses the
        // enties only via operator(i,j,k), so this is safe!
        
        if constexpr ( countersQ )
        {
            box_box_counter = 0;
            edge_edge_counter = 0;
        }
        
        RequireBoundingBoxes();
        
        const Int degenerate_edge_count = DegenerateEdgeCount();
        
        if( degenerate_edge_count > Int(0) )
        {
            if constexpr ( verboseQ )
            {
                eprint(tag()+": Detected " + ToString(degenerate_edge_count) + " degenerate edges.");
            }
            return 9;
        }

        intersections.Clear();
        intersections.RequireCapacity( Int{2} * EdgeCount() );

        FindIntersectingEdges_DFS();
        
        
        if constexpr ( countersQ )
        {
            logvalprint("number of edges", EdgeCount());
            logvalprint("number of box-box checks", box_box_counter);
            logvalprint("number of edge-edge checks", edge_edge_counter);
            logvalprint("number of intersections", intersections.size());
        }
                
        intersections_computedQ = true;
        
        // Check for bad intersections.
        {
            const Size_T count = prosector_flag_counts[7];
            if( count > Size_T{0} )
            {
                if constexpr ( verboseQ )
                {
                    eprint(tag() + ": Detected " + ToString(count) + " cases where line segments intersection times were out of bounds.");
                }
                return 7;
            }
        }

        {
            const Size_T count = prosector_flag_counts[6];
            if( count > Size_T{0} )
            {
                if constexpr ( verboseQ )
                {
                    eprint(tag() + ": Detected " + ToString(count) + " cases where line segments intersected in 3D.");
                }
                return 6;
            }
        }
        
        {
            const Size_T count = prosector_flag_counts[5];
            if( count > Size_T{0} )
            {
                if constexpr ( verboseQ )
                {
                    wprint(tag() + ": Detected " + ToString(count) + " cases where the line-line intersection was degenerate (the intersection set was an interval). Try to randomly rotate the input coordinates.");
                }
                return 5;
            }
        }
        
        {
            const Size_T count = prosector_flag_counts[4];
            
            if( count > Size_T{0} )
            {
                if constexpr ( verboseQ )
                {
                    wprint(tag() + ": Detected " + ToString(count) + " cases where the line-line intersection was a point in the corners of two line segments. Try to randomly rotate the input coordinates.");
                }
                return 4;
            }
        }
        
        {
            const Size_T count =
                  prosector_flag_counts[2]
                + prosector_flag_counts[3];
            
            if( count > Size_T{0} )
            {
                if constexpr ( verboseQ )
                {
                    wprint(tag() + ": Detected " + ToString(count) + " cases where the line-line intersection was a point in a corner of a line segment. Try to randomly rotate the input coordinates.");
                }
                return 3;
            }
        }
        
        // Check for integer overflow.
        if( std::cmp_greater(
                Int(4) * ToSize_T(intersections.Size()),
                std::numeric_limits<Int>::max()
            )
        )
        {
            eprint(tag() + ": More intersections found than can be handled by integer type " + TypeName<Int> + "." );
        }
        
        intersection_count = intersections.Size();
        
        {
            TOOLS_PTIMER(sort_timer, tag() + ": coarse sorting.");
            
            // We are going to use edge_ptr for the assembly; because we are going to modify it, we need a copy.
            Tensor1<Int,Int> edge_ctr { edge_ptr };
            
            edge_cross.template Resize<false>(edge_ptr.Last());
            edge_times.template Resize<false>(edge_ptr.Last());
            
            // We are going to fill edge_cross so that data of the i-th edge lies in edge_cross[edge_ptr[i]],..,edge_cross[edge_ptr[i+1]].
            // To this end, we use (and modify!) edge_ctr so that edge_ctr[i] points AFTER the position to insert.
            
            if( intersection_count <= Int(0) ) { return 0; }
            
            for( Int idx = intersection_count; idx --> Int(0);  )
            {
                Intersection_T & isec = intersections[idx];
                
                // We have to write BEFORE the positions specified by edge_ctr (and decrease it for the next write;
                
                const Int pos_0 = --edge_ctr[isec.edges[0]+1];
                const Int pos_1 = --edge_ctr[isec.edges[1]+1];
                
                const bool right_handedQ = PositiveQ(isec.handedness);
                
                edge_cross[pos_0] = EdgeCrossing_T(idx,right_handedQ,true );
                edge_times        [pos_0] = isec.times[0];
                
                edge_cross[pos_1] = EdgeCrossing_T(idx,right_handedQ,false);
                edge_times        [pos_1] = isec.times[1];
               
            }
        }
        
        Size_T close_counter = 0;
        
        {
            TOOLS_PTIMER(sort_timer, tag() + ": fine sorting.");
            
            // Sort intersections edgewise w.r.t. edge_times.
            TwoArraySort<Real,EdgeCrossing_T,Int> sort (intersection_count);
            
            for( Int i = 0; i < edge_count; ++i )
            {
                // This is the range of data in edge_cross that belongs to edge i.
                const Int k_begin = edge_ptr[i  ];
                const Int k_end   = edge_ptr[i+1];
                     
                // We need to sort only if there are at least two intersections on that edge.
                if( k_begin + Int(1) < k_end )
                {
                    sort(
                        &edge_times[k_begin],
                        &edge_cross[k_begin],
                        k_end - k_begin
                    );

                    constexpr Real intersection_time_tolerance = 0.000000000001;
                    
                    for( Int l = k_begin + Int(1); l < k_end; ++l )
                    {
                        const Real delta = Abs(edge_times[l] - edge_times[l-1]);

                        if( delta < intersection_time_tolerance )
                        {
                            ++close_counter;
                            
                            // TODO: For the moment we _want_ to see this warning.
                            // TODO: On the long run we need a more precise detector for the ordering of the intersection times.
                            
    //                        if constexpr ( verboseQ )
    //                        {
                                auto ec_0   = edge_cross[l-1];
                                auto isec_0 = intersections[ec_0.Index()];
                                auto ec_1   = edge_cross[l  ];
                                auto isec_1 = intersections[ec_1.Index()];
                                
                                const Int j_0 = (isec_0.edges[0] == i) ? isec_0.edges[1] : isec_0.edges[0];
                                
                                const Int j_1 = (isec_1.edges[0] == i) ? isec_1.edges[1] : isec_1.edges[0];
                            
                                wprint(tag() + ": Detected tiny difference of intersection times = " + ToString(delta) + " < " + ToString(intersection_time_tolerance)+ " = intersection_time_tolerance for intersections of line segment " + ToString(i) + " with line segments " + ToString(j_0) + " (" + (ec_0.OverQ() ? "over" : "under") + ") and " + ToString(j_1) + " (" + (ec_1.OverQ() ? "over" : "under") + ")." );
    //                        }
                        }
                    }
                }
            }
        }
        
        // We don't need this anymore.
        intersections = Aggregator<Intersection_T,Int>();
        
        prosector_flag_counts[8] = close_counter;
        
        if( prosector_flag_counts[8] )
        {
            // TODO: For the moment we _want_ to see this warning.
            // TODO: On the long run we need a more precise detector for the ordering of the intersection times.
//            if constexpr ( verboseQ )
//            {
                wprint(tag() + ": Detected " + ToString(close_counter) + " case(s) of tiny difference between intersection times." );
//            }
            return 8;
        }

        // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
        
        return 0;
    }

private:

    void FindIntersectingEdges_DFS()
    {
        TOOLS_PTIMER(timer,MethodName("FindIntersectingEdges_DFS"));
        
        intersection_count_3D = 0;
        edge_ptr.SetZero();
        prosector_flag_counts.SetZero();
        
        // Last time I checked the _ManualStack version was 5% faster.
        FindIntersectingEdges_DFS_ManualStack();
//        FindIntersectingEdges_DFS_Recursive(T.Root(),T.Root());
        
        edge_ptr.Accumulate();
        
    } // FindIntersectingClusters_DFS


    // Improved version of FindIntersectingEdges_DFS_impl_0; we do the box-box checks of all the children at once; this saves us a couple of cache misses.
    void FindIntersectingEdges_DFS_ManualStack()
    {
        constexpr Int stack_max_size = Int(4) * max_depth + Int(1);
        constexpr Int stack_limit    = Int(4) * max_depth - Int(4);
        
        Int stack [stack_max_size][2];
        Int stack_ptr = 0;
        stack[stack_ptr][0] = 0;  // Dummy node.
        stack[stack_ptr][1] = 0;  // Dummy node.
        
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
            if( this->BoxesIntersectQ(i,j) ) { push(i,j); }
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
            const bool overflowQ = (stack_ptr >= stack_limit);
            
            if( (Int(0) < stack_ptr) && (!overflowQ) ) [[likely]]
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
        
        while( continueQ() )
        {
            // Pop from stack.

            auto [i,j] = pop();
            
            const bool i_internalQ = T.InternalNodeQ(i);
            const bool j_internalQ = T.InternalNodeQ(j);
            
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
                ComputeEdgeEdgeIntersection( T.NodeBegin(i), T.NodeBegin(j) );
            }
        }
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
                else // if( i != j )
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
                else // if( i_internalQ )
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

    bool BoxesIntersectQ( const Int i, const Int j )
    {
        if constexpr ( countersQ )
        {
            ++box_box_counter;
        }
        return T.BoxesIntersectQ( box_coords.data(i), box_coords.data(j) );
    }

protected:

    void ComputeEdgeEdgeIntersection( const Int k, const Int l )
    {
        // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
        if( (l != k) && (l != NextEdge(k)) && (k != NextEdge(l)) )
        {
            if constexpr ( countersQ )
            {
                ++edge_edge_counter;
            }
            this->template ComputeEdgeEdgeIntersection_impl<false>(k,l);
        }
    }


    template<bool verboseQ>
    void ComputeEdgeEdgeIntersection_impl( const Int k, const Int l )
    {
        using Sign_T = Intersection_T::Sign_T;
        
        if constexpr ( verboseQ )
        {
            logprint(ClassName()+"::ComputeEdgeEdgeIntersection in verbose mode.");
            TOOLS_LOGDUMP(k);
            TOOLS_LOGDUMP(l);
        }
        
//        ++edge_edge_counter;
        
        // At this point we assume that `k != l` and that they are also not direct neighbors.

        const E_T x = EdgeData(k);
        const E_T y = EdgeData(l);
        
        if constexpr ( verboseQ )
        {
            TOOLS_LOGDUMP(ToString(x));
            TOOLS_LOGDUMP(ToString(y));
        }
        
        ProsectorFlag flag
            = S.template IntersectionType<verboseQ>( x[0], x[1], y[0], y[1] );
        
        if constexpr ( verboseQ )
        {
            TOOLS_LOGDUMP(flag);
        }
        
        if( IntersectingQ(flag) )
        {
            auto [t,sign] = S.IntersectionTimesAndSign();
            
            
            if( (t[0]<Real(0)) || (t[0]>=Real(1)) || (t[1]<Real(0)) || (t[1]>=Real(1)) )
            {
                flag = ProsectorFlag::OOBounds;
            }
            
            // Compute heights at the intersection.
            const Real h[2] = {
                x[0][2] * (Real(1) - t[0]) + t[0] * x[1][2],
                y[0][2] * (Real(1) - t[1]) + t[1] * y[1][2]
            };
            
            // Tell edges k and l that they contain an additional crossing.
            ++edge_ptr[k+1];
            ++edge_ptr[l+1];

            if( h[0] < h[1] )
            {
                // edge k goes UNDER edge l
                
                intersections.Push(Intersection_T(l,k,t[1],t[0],static_cast<Sign_T>(-sign)));
                
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
                intersections.Push( Intersection_T(k,l,t[0],t[1],sign) );
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
                flag = ProsectorFlag::Spatial;
            }
            
        } // if( IntersectingQ(flag) )
        
        ++prosector_flag_counts[ ToUnderlying(flag) ];
        
        switch(flag)
        {
            case ProsectorFlag::AtCorner0:
            {
                wprint(ClassName()+"::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in first corner of edge " + ToString(k) + ".");
//                logvalprint("edge " + ToString(k), x);
//                logvalprint("edge " + ToString(l), y);
                break;
            }
            case ProsectorFlag::AtCorner1:
            {
                wprint(ClassName()+"::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in first corner of edge " + ToString(l) + ".");
//                logvalprint("edge " + ToString(k), x);
//                logvalprint("edge " + ToString(l), y);
                break;
            }
            case ProsectorFlag::CornerCorner:
            {
                wprint(ClassName()+"::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " have common first corners.");
//                logvalprint("edge " + ToString(k), x);
//                logvalprint("edge " + ToString(l), y);
                break;
            }
            case ProsectorFlag::Interval:
            {
                wprint(ClassName()+"::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in an interval.");
//                logvalprint("edge " + ToString(k), x);
//                logvalprint("edge " + ToString(l), y);
                break;
            }
            case ProsectorFlag::Spatial:
            {
                wprint(ClassName()+"::ComputeEdgeIntersection: Edges " + ToString(k) + " and " + ToString(l) + " intersect in 3D.");
                logvalprint("edge " + ToString(k), x);
                logvalprint("edge " + ToString(l), y);
                break;
            }
            case ProsectorFlag::OOBounds:
            {
                wprint(ClassName()+"::ComputeEdgeIntersection: Intersection times of intersection between edges " + ToString(k) + " and " + ToString(l) + " are out of bounds.");
//                logvalprint("edge " + ToString(k), x);
//                logvalprint("edge " + ToString(l), y);
                break;
            }
            default:
            {
                break;
            }
        }
    }
