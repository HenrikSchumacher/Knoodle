private:

template<bool mQ, bool full_checkQ = false>
bool CollisionQ_ManualStack()
{
    static_assert(SignedIntQ<Int>,"");
    Int stack [Int(4) * max_depth][2];
    Int stack_ptr = -1;

    // Helper routine to manage stack.
    auto push = [&stack,&stack_ptr]( const Int i, const Int j )
    {
        ++stack_ptr;
        stack[stack_ptr][0] = i;
        stack[stack_ptr][1] = j;
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
                eprint(this->ClassName()+"::CollideQ_Reference: Stack overflow.");
            }
            return false;
        }
    };
    
//    auto continueQ = [&stack_ptr]()
//    {
//        return stack_ptr >= Int(0);
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
        
        // "Internal node" means "not a leaf node".
        
        const bool i_internalQ = InternalNodeQ(i);
        const bool j_internalQ = InternalNodeQ(j);
        
        if ( i_internalQ && j_internalQ )
        {
            if( i == j )
            {
                const Int c [2] = {LeftChild(i),RightChild(i)};
                
                PushTransform(i,c[0],c[1]);
                
                NodeSplitFlagMatrix_T F = NodeSplitFlagMatrix<mQ>(c[0],c[1]);
                
                if constexpr ( full_checkQ )
                {
                    F[0][0] = true; F[0][1] = true; F[1][0] = true; F[1][1] = true;
                }
                else
                {
                    F = NodeSplitFlagMatrix<mQ>(c[0],c[1]);
                }
                
                auto cond_push = [&c,&F,this,push]( bool k, bool l )
                {
                    if(
                        (( F[k][0] && F[l][1] ) || ( F[k][1] && F[l][0] ))
                        &&
                        ( (k==l) || this->BallsCollideQ(c[k],c[l]))
                    )
                    {
                        push(c[k],c[l]);
                    }
                };

                cond_push(0,1);
                cond_push(1,1); // Overlaps are likely to be here...
                cond_push(0,0); // ... or here.
            }
            else // (i != j )
            {
                // Split both nodes.
                const Int c_i [2] = {LeftChild(i),RightChild(i)};
                const Int c_j [2] = {LeftChild(j),RightChild(j)};
                
                PushTransform(i,c_i[0],c_i[1]);
                PushTransform(j,c_j[0],c_j[1]);

                NodeSplitFlagMatrix_T F_i;
                NodeSplitFlagMatrix_T F_j;
  
                if constexpr ( full_checkQ )
                {
                    F_i[0][0] = true; F_i[0][1] = true;
                    F_i[1][0] = true; F_i[1][1] = true;
                    F_j[0][0] = true; F_j[0][1] = true;
                    F_j[1][0] = true; F_j[1][1] = true;
                }
                else
                {
                    F_i = NodeSplitFlagMatrix<mQ>(c_i[0],c_i[1]);
                    F_j = NodeSplitFlagMatrix<mQ>(c_j[0],c_j[1]);
                }
                
                auto cond_push = [&c_i,&c_j,&F_i,&F_j,this,push]( bool k, bool l )
                {
                    if(
                        ( (F_i[k][0] && F_j[l][1]) || (F_i[k][1] && F_j[l][0]) )
                        &&
                        (this->BallsCollideQ(c_i[k],c_j[l]))
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
        else if ( !i_internalQ && j_internalQ )
        {
            // Split node j
            Int c_j [2] = {LeftChild(j),RightChild(j)};
            
            PushTransform(j,c_j[0],c_j[1]);
            
            NodeSplitFlagVector_T f_i;
            NodeSplitFlagMatrix_T F_j;
            
            if constexpr ( full_checkQ )
            {
                f_i[0] = true; f_i[1] = true;
                F_j[0][0] = true; F_j[0][1] = true;
                F_j[1][0] = true; F_j[1][1] = true;
            }
            else
            {
                f_i = NodeSplitFlagVector<mQ>(i);
                F_j = NodeSplitFlagMatrix<mQ>(c_j[0],c_j[1]);
            }
            
            auto cond_push = [i,&c_j,&f_i,&F_j,this,push]( bool l )
            {
                if( ( (f_i[0] && F_j[l][1]) || (f_i[1] && F_j[l][0]) )
                    &&
                   (this->BallsCollideQ(i,c_j[l]) )
                )
                {
                    push(i,c_j[l]);
                }
            };

            cond_push(0);
            cond_push(1);
        }
        else if ( i_internalQ && !j_internalQ )
        {
            // Split node i
            Int c_i [3] = {LeftChild(i),RightChild(i)};
            
            PushTransform(i,c_i[0],c_i[1]);
            
            NodeSplitFlagVector_T f_j;
            NodeSplitFlagMatrix_T F_i;
            
            if constexpr ( full_checkQ )
            {
                f_j[0]    = true; f_j[1]    = true;
                F_i[0][0] = true; F_i[0][1] = true;
                F_i[1][0] = true; F_i[1][1] = true;
            }
            else
            {
                f_j = NodeSplitFlagVector<mQ>(j);
                F_i = NodeSplitFlagMatrix<mQ>(c_i[0],c_i[1]);
            }
            
            auto cond_push = [j,&c_i,&f_j,&F_i,this,push]( bool k )
            {
                if(
                    ( (F_i[k][0] && f_j[1]) || (F_i[k][1] && f_j[0]) )
                    &&
                   (this->BallsCollideQ(c_i[k],j))
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
            
            if( Min( delta, VertexCount() - delta ) > Int(1) )
            {
                witness[0] = k;
                witness[1] = l;
                
                return true;
            }
        }
    }
    return false;
    
} // CollisionQ_ManualStack
