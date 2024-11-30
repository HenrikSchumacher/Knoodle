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
            
            ClearCollisionData();

            const Int int_node_count = T.InteriorNodeCount();
            
            Int stack[max_depth][2];
            Int stack_ptr = 0;
            stack[0][0] = 0;
            stack[0][1] = 0;
            

            while( (0 <= stack_ptr) && (stack_ptr < max_depth - 4) )
            {
                const Int i = stack[stack_ptr][0];
                const Int j = stack[stack_ptr][1];
                
                stack_ptr--;
                
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
                        const Int left_i  = Tree_T::LeftChild(i);
                        const Int right_i = left_i+1;
                        
                        const Int left_j  = Tree_T::LeftChild(j);
                        const Int right_j = left_j+1;
                        
                        // TODO: We probably should only split the larger node (larger by diameter?).
                                                
                        if( is_interior_i && is_interior_j )
                        {
                            if( i == j )
                            {
                                //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = left_i;
                                stack[stack_ptr][1] = right_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = right_i;
                                stack[stack_ptr][1] = right_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = left_i;
                                stack[stack_ptr][1] = left_j;
                            }
                            else
                            {
                                // split both clusters
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = right_i;
                                stack[stack_ptr][1] = right_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = left_i;
                                stack[stack_ptr][1] = right_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = right_i;
                                stack[stack_ptr][1] = left_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = left_i;
                                stack[stack_ptr][1] = left_j;
                            }
                        }
                        else
                        {
                            // Only one cluster can be split.
                            
                            if( is_interior_i )
                            {
                                //split cluster i
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = right_i;
                                stack[stack_ptr][1] = j;

                                ++stack_ptr;
                                stack[stack_ptr][0] = left_i;
                                stack[stack_ptr][1] = j;
                            }
                            else
                            {
                                //split cluster j
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = i;
                                stack[stack_ptr][1] = right_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = i;
                                stack[stack_ptr][1] = left_j;
                            }
                        }
                    }
                    else
                    {
                        MovingEdgeCollisions( T.NodeBegin(i), T.NodeBegin(j) );
                    }
                }
            }
            
            ptoc(ClassName()+"::FindCollisions");
            
        } // FindCollisions
