private:

bool OverlapQ_Reference()
{
    Int stack [4 * max_depth][2];
    SInt stack_ptr = -1;

    // Helper routine to manage stack.
    auto push = [&stack,&stack_ptr]( const Int i, const Int j )
    {
        ++stack_ptr;
        stack[stack_ptr][0] = i;
        stack[stack_ptr][1] = j;
    };
    
    // Helper routine to manage stack.
    auto conditional_push = [this,push]( const Int i, const Int j )
    {
        const bool overlappingQ = ( (i==j) || this->BallsOverlapQ(i,j,this->r) );
        
        if( overlappingQ)
        {
            push(i,j);
        }
    };
    
    // Helper routine to manage stack.
    auto pop = [&stack,&stack_ptr]()
    {
        auto result = MinMax( stack[stack_ptr][0], stack[stack_ptr][1] );
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
                eprint(this->ClassName()+"::OverlapQ_Reference: Stack overflow.");
            }
            return false;
        }
    };
    
//    auto continueQ = [&stack_ptr]()
//    {
//        return (0 <= stack_ptr);
//    };
    
    auto [b_root_0,b_root_1] = NodeSplitFlags(0);
    
    if( b_root_0 && b_root_1 )
    {
        push(Root(),Root());
    }
    
    while( continueQ() )
    {
        auto [i,j] = pop();
        
        // "Interior node" means "not a leaf node".
        const bool i_interiorQ = InteriorNodeQ(i);
        const bool j_interiorQ = InteriorNodeQ(j);
        
        if( i_interiorQ || j_interiorQ )
        {
            if( i_interiorQ && j_interiorQ )
            {
                // Split both nodes.
                
                auto [L_i,R_i] = Children(i);
                auto [L_j,R_j] = Children(j);

                PushTransform( i, L_i, R_i );
                PushTransform( j, L_j, R_j );
                
                auto [b_L_i_0,b_L_i_1] = NodeSplitFlags(L_i);
                auto [b_R_i_0,b_R_i_1] = NodeSplitFlags(R_i);
                auto [b_L_j_0,b_L_j_1] = NodeSplitFlags(L_j);
                auto [b_R_j_0,b_R_j_1] = NodeSplitFlags(R_j);
                
                if( (b_R_i_0 && b_R_j_1) || (b_R_i_1 && b_R_j_0) )
                {
                    conditional_push(R_i,R_j);
                }
                
                if( (b_L_i_0 && b_L_j_1) || (b_L_i_1 && b_L_j_0) )
                {
                    conditional_push(L_i,L_j);
                }
                
                
                // TODO: Optimize the order of the following two calls to conditional_push.
                // TODO: We should encourage to first visit the neighborhood of the pivots.
                
                if( (b_L_i_0 && b_R_j_1) || (b_L_i_1 && b_R_j_0) )
                {
                    conditional_push(L_i,R_j);
                }
                
                // If i == j, we can skip (R_i,L_j) as (R_j,L_i) above is identical.
                if( (i != j) && ((b_R_i_0 && b_L_j_1) || (b_R_i_1 && b_L_j_0)) )
                {
                    conditional_push(R_i,L_j);
                }
            }
            else
            {
                // split only the interior node
                if ( i_interiorQ ) // !j_interiorQ follows from this.
                {
                    // Split node i.
                    
                    auto [L_i,R_i] = Children(i);
                    
                    PushTransform( i, L_i, R_i );
                    
                    auto [b_j_0  ,b_j_1  ] = NodeSplitFlags(j  );
                    auto [b_R_i_0,b_R_i_1] = NodeSplitFlags(R_i);
                    auto [b_L_i_0,b_L_i_1] = NodeSplitFlags(L_i);
                    
                    if( (b_R_i_0 && b_j_1) || (b_R_i_1 && b_j_0) )
                    {
                        conditional_push(R_i,j);
                    }
                    
                    if( (b_L_i_0 && b_j_1) || (b_L_i_1 && b_j_0) )
                    {
                        conditional_push(L_i,j);
                    }
                }
                else
                {
                    // Split node j.
                    
                    auto [L_j,R_j] = Children(j);
                    
                    PushTransform( j, L_j, R_j );

                    auto [b_i_0  ,b_i_1  ] = NodeSplitFlags(i  );
                    auto [b_R_j_0,b_R_j_1] = NodeSplitFlags(R_j);
                    auto [b_L_j_0,b_L_j_1] = NodeSplitFlags(L_j);
                    
                    if( (b_i_0 && b_R_j_1) || (b_i_1 && b_R_j_0) )
                    {
                        conditional_push(i,R_j);
                    }
                    
                    if( (b_i_0 && b_L_j_1) || (b_i_1 && b_L_j_0) )
                    {
                        conditional_push(i,L_j);
                    }
                }
            }
        }
        else // [[unlikely]]
        {
            // Nodes i and j are overlapping leaf nodes.
            
            // Rule out that tiny distance errors of neighboring vertices cause problems.
            const Int k = NodeBegin(i);
            const Int l = NodeBegin(j);
            
            const Int delta = Abs(k-l);
            
            if( Min( delta, VertexCount() - delta ) > 1 )
            {
                witness_0 = k;
                witness_1 = l;
                
                return true;
            }
        }
    }
    
    return false;
    
} // OverlapQ_Reference
