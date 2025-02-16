private:

bool OverlapQ_ManualStack()
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
//        return stack_ptr >= 0;
//    };
    
    auto [b_root_0,b_root_1] = NodeSplitFlags(0);
    
    if( b_root_0 && b_root_1 )
    {
        push(Root(),Root());
    }
    
    while( continueQ() )
    {
        auto [i,j] = MinMax( stack[stack_ptr][0], stack[stack_ptr][1] );
        stack_ptr--;
        
        // "Interior node" means "not a leaf node".
        
        const bool i_interiorQ = InteriorNodeQ(i);
        const bool j_interiorQ = InteriorNodeQ(j);
        
        if ( i_interiorQ && j_interiorQ )
        {
            if( i == j )
            {
                const Int c [2] = {LeftChild(i),RightChild(i)};
                
                PushTransform(i,c[0],c[1]);
                
                const NodeSplitFlagMatrix_T F = NodeSplitFlagMatrix(c[0],c[1]);
                
                auto cond_push = [&c,&F,this,push]( bool k, bool l )
                {
                    if(
                        (( F[k][0] && F[l][1] ) || ( F[k][1] && F[l][0] ))
                        &&
                        (this->NodesOverlapQ(c[k],c[l],this->r))
                    )
                    {
                        push(c[k],c[l]);
                    }
                };

                cond_push(0,1);
                cond_push(1,1); // Overlaps are likely to be hear...
                cond_push(0,0); // ... or here.
            }
            else // (i != j )
            {
                // Split both nodes.
                const Int c_i [2] = {LeftChild(i),RightChild(i)};
                const Int c_j [2] = {LeftChild(j),RightChild(j)};
                
                PushTransform(i,c_i[0],c_i[1]);
                PushTransform(j,c_j[0],c_j[1]);

                const NodeSplitFlagMatrix_T F_i = NodeSplitFlagMatrix(c_i[0],c_i[1]);
                const NodeSplitFlagMatrix_T F_j = NodeSplitFlagMatrix(c_j[0],c_j[1]);
  
                auto cond_push = [&c_i,&c_j,&F_i,&F_j,this,push]( bool k, bool l )
                {
                    if(
                        ( (F_i[k][0] && F_j[l][1]) || (F_i[k][1] && F_j[l][0]) )
                        &&
                        (this->NodesOverlapQ(c_i[k],c_j[l],this->r))
                    )
                    {
                        push(c_i[k],c_j[l]);
                    };
                };
        
                cond_push(1,1);
                cond_push(0,0);
                cond_push(1,0);
                cond_push(0,1); // i < j, so this is most likely to have a collision.
            }
        }
        else if ( !i_interiorQ && j_interiorQ )
        {
            // Split node j
            Int c_j [2] = {LeftChild(j),RightChild(j)};
            
            PushTransform(j,c_j[0],c_j[1]);
            
            const NodeSplitFlagVector_T f_i = NodeSplitFlagVector(i);
            const NodeSplitFlagMatrix_T F_j = NodeSplitFlagMatrix(c_j[0],c_j[1]);
            
            auto cond_push = [i,&c_j,&f_i,&F_j,this,push]( bool l )
            {
                if( ( (f_i[0] && F_j[l][1]) || (f_i[1] && F_j[l][0]) )
                    &&
                   ( this->NodesOverlapQ(i,c_j[l],this->r) )
                )
                {
                    push(i,c_j[l]);
                }
            };

            cond_push(0);
            cond_push(1);
        }
        else if ( i_interiorQ && !j_interiorQ )
        {
            // Split node i
            Int c_i [3] = {LeftChild(i),RightChild(i)};
            
            PushTransform(i,c_i[0],c_i[1]);
            
            const NodeSplitFlagVector_T f_j = NodeSplitFlagVector(j);
            const NodeSplitFlagMatrix_T F_i = NodeSplitFlagMatrix(c_i[0],c_i[1]);
            
            auto cond_push = [j,&c_i,&f_j,&F_i,this,push]( bool k )
            {
                if(
                    ( (F_i[k][0] && f_j[1]) || (F_i[k][1] && f_j[0]) )
                    &&
                   (this->NodesOverlapQ(c_i[k],j,this->r))
                )
                {
                    push(c_i[k],j);
                }
            };

            cond_push(0);
            cond_push(1);
        }
        else // [[unlikely]]
        {
            // Nodes i and j are overlapping leaf nodes.
            
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
    
} // OverlapQ_ManualStack
