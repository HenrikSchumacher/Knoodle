public:

        void FindCollisions_AllPairs()
        {
            ClearCollisionData();
            
            for( Int k = 0; k < EdgeCount(); ++k )
            {
                for( Int l = k+1; l < EdgeCount(); ++l )
                {
                    MovingEdgeCollisions(k,l);
                }
            }
        } // FindCollisions_AllPairs

        void FindCollisions()
        {
            ptic(ClassName()+"::FindCollisions");
            
            FindCollisions_implementation_0()
            
            ptoc(ClassName()+"::FindCollisions");
        }

        void FindCollisions_implementation_0()
        {
            ClearCollisionData();

            const Int int_node_count = T.InteriorNodeCount();
            
            Int stack[4 * max_depth][2];
            Int stack_ptr = -1;
            
            // Helper routine to manage the pair_stack.
            auto push = [&stack,&stack_ptr]( const Int i, const Int j )
            {
                ++stack_ptr;
                stack[stack_ptr][0] = i;
                stack[stack_ptr][1] = j;
            };
        
            // Helper routine to manage the pair_stack.
            auto pop = [&stack,&stack_ptr]()
            {
                const std::pair result ( stack[stack_ptr][0], stack[stack_ptr][1] );
                stack_ptr--;
                return result;
            };
            
            auto continueQ = [&stack_ptr]()
            {
                return (0 <= stack_ptr) && (stack_ptr < 4 * max_depth - 4 );
            };
            
            push(0,0);

            while( continueQ() )
            {
                auto [i,j] = pop();
                
                const bool boxes_collidingQ = (i==j) ? true : MovingBoxesCollidingQ(
                    B_0.data(i), B_1.data(i), B_0.data(j), B_1.data(j)
                );
                
                if( boxes_collidingQ )
                {
                    const bool is_interior_i = (i < int_node_count);
                    const bool is_interior_j = (j < int_node_count);
                    
                    // Warning: This assumes that both children in a cluster tree are either defined or empty.
                    if( is_interior_i || is_interior_j )
                    {
                        auto [L_i,R_i] = Tree_T::Children(i);
                        auto [L_j,R_j] = Tree_T::Children(j);
                        
                        // TODO: We probably should only split the larger node (larger by diameter?).
                                                
                        if( is_interior_i && is_interior_j )
                        {
                            if( i == j )
                            {
                                //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                                push(L_i,R_j);
                                push(R_i,R_j);
                                push(L_i,L_j);
                            }
                            else
                            {
                                // split both clusters
                                push(R_i,R_j);
                                push(L_i,R_j);
                                push(R_i,L_j);
                                push(L_i,L_j);
                            }
                        }
                        else
                        {
                            // Only one cluster can be split.
                            
                            if( is_interior_i )
                            {
                                //split cluster i
                                push(R_i,j);
                                push(L_i,j);
                            }
                            else
                            {
                                //split cluster j
                                push(i,R_j);
                                push(i,L_j);
                            }
                        }
                    }
                    else
                    {
                        MovingEdgeCollisions( T.NodeBegin(i), T.NodeBegin(j) );
                    }
                }
            }
            
        } // FindCollisions_implementation_0



        void FindCollisions_implementation_1()
        {
            ClearCollisionData();

            const Int int_node_count = T.InteriorNodeCount();
            
            Int stack[4 * max_depth][2];
            Int stack_ptr = -1;
            
            // Helper routine to manage the pair_stack.
            auto push = [&stack,&stack_ptr]( const Int i, const Int j )
            {
                ++stack_ptr;
                stack[stack_ptr][0] = i;
                stack[stack_ptr][1] = j;
            };
            
            // Helper routine to manage the pair_stack.
            auto check_push = [&stack,&stack_ptr,this]( const Int i, const Int j )
            {
                const bool boxes_collidingQ = (i==j) ? true : MovingBoxesCollidingQ(
                    B_0.data(i), B_1.data(i), B_0.data(j), B_1.data(j)
                );
                
                if( boxes_collidingQ )
                {
                    ++stack_ptr;
                    stack[stack_ptr][0] = i;
                    stack[stack_ptr][1] = j;
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
                const bool overflowQ = (stack_ptr >= 4 * max_depth - 4);
                
                if( (0 <= stack_ptr) && (!overflowQ) ) [[likely]]
                {
                    return true;
                }
                else
                {
                    if ( overflowQ ) [[unlikely]]
                    {
                        eprint(this->ClassName()+"::FindCollisions_implementation_1: Stack overflow.");
                    }
                    return false;
                }
            };
            
            push(0,0);

            while( continueQ() )
            {
                auto [i,j] = pop();
                
                const bool is_interior_i = (i < int_node_count);
                const bool is_interior_j = (j < int_node_count);
                
                // Warning: This assumes that both children in a cluster tree are either defined or empty.
                if( is_interior_i || is_interior_j )
                {
                    auto [L_i,R_i] = Tree_T::Children(i);
                    auto [L_j,R_j] = Tree_T::Children(j);
                    
                    // TODO: We probably should only split the larger node (larger by diameter?).
                                            
                    if( is_interior_i && is_interior_j )
                    {
                        if( i == j )
                        {
                            //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                            check_push(L_i,R_j);
                            check_push(R_i,R_j);
                            check_push(L_i,L_j);
                        }
                        else
                        {
                            // split both clusters
                            check_push(R_i,R_j);
                            check_push(L_i,R_j);
                            check_push(R_i,L_j);
                            check_push(L_i,L_j);
                        }
                    }
                    else
                    {
                        // Only one cluster can be split.
                        
                        if( is_interior_i )
                        {
                            //split cluster i
                            check_push(R_i,j);
                            check_push(L_i,j);
                        }
                        else
                        {
                            //split cluster j
                            check_push(i,R_j);
                            check_push(i,L_j);
                        }
                    }
                }
                else
                {
                    MovingEdgeCollisions( T.NodeBegin(i), T.NodeBegin(j) );
                }
            }
            
        } // FindCollisions_implementation_1
