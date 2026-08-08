public:

void FindCollisions_AllPairs()
{
    TOOLS_MAKE_FP_STRICT()
    
    TOOLS_PTIMER(timer,MethodName("FindCollisions_AllPairs"));
    
    ClearCollisionData();
    
    const Int e_count = EdgeCount();
    
    for( Int k = 0; k < e_count; ++k )
    {
        for( Int l = k+1; l < e_count; ++l )
        {
            MovingEdgeCollisions(k,l);
        }
    }
    
    collisions_computedQ = true;
    
} // FindCollisions_AllPairs


void RequireCollisions()
{
    if( !collisions_computedQ ) { FindCollisions(); }
}

void FindCollisions()
{
    TOOLS_PTIMER(timer,MethodName("FindCollisions"));
    
    FindCollisions_impl();
}

void FindCollisions_impl()
{
    TOOLS_MAKE_FP_STRICT()

    ClearCollisionData();

    const Int int_node_count = T.InternalNodeCount();
    
    static_assert(SignedIntQ<Int>,"");
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
    auto conditional_push = [push,this]( const Int i, const Int j )
    {
        if( MovingBoxesCollidingQ(i,j) ) { push(i,j); };
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
        return (Int(0) <= stack_ptr) && (stack_ptr < Int(4) * max_depth - Int(4) );
    };

    push(0,0);

    while( continueQ() )
    {
        auto [i,j] = pop();
        
        const bool internalQ_i = (i < int_node_count);
        const bool internalQ_j = (j < int_node_count);
        
        // Warning: This assumes that both children in a cluster tree are either defined or empty.
        if( internalQ_i || internalQ_j )
        {
            auto [L_i,R_i] = Tree_T::Children(i);
            auto [L_j,R_j] = Tree_T::Children(j);
            
            if( internalQ_i && internalQ_j )
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
                    // split both clusters
                    conditional_push(R_i,R_j);
                    conditional_push(L_i,R_j);
                    conditional_push(R_i,L_j);
                    conditional_push(L_i,L_j);
                }
            }
            else
            {
                // Only one cluster can be split.
                
                if( internalQ_i )
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
            MovingEdgeCollisions( T.NodeBegin(i), T.NodeBegin(j) );
        }
    }
    
    collisions_computedQ = true;
    
} // FindCollisions_impl
