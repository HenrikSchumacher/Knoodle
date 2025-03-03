private:

void UpdateSubtree_ManualStack( const Int start_node )
{
    Int  stack [max_depth];
    SInt stack_ptr = -1;
    
    switch( NodeNeedsUpdateQ( start_node ) )
    {
        case UpdateFlag_T::DoNothing:
        {
            // Cannot happen?
            return;
        }
        case UpdateFlag_T::Update:
        {
            UpdateNode( transform, start_node );
            return;
        }
        case UpdateFlag_T::Split:
        {
            // Push this node as unvisited; it will be split.
            stack[++stack_ptr] = ( start_node << 1 );
        }
    }
    
    while( (0 <= stack_ptr) && (stack_ptr < max_depth - 2) )
    {
        const Int  code     = stack[stack_ptr];
        const Int  node     = (code >> 1);
        const bool visitedQ = (code & 1);
        
        // Only nodes with flag == 2 land on the stack.
        // Thus, this node must be a splitting node.
        
        if( !visitedQ )
        {
            // Remember that we have been here.
            stack[stack_ptr] |= 1;
            
            // If node is on the stack, then it contains changed and unchanged vertices.
            // So, in particular, it cannot contain any leaf nodes.
            
            auto [L,R] = Children(node);
            
            PushTransform( node, L, R );
            
            // We never update both nodes; otherwise, this would not be a split node.
            
            switch( NodeNeedsUpdateQ( R ) )
            {
                case UpdateFlag_T::DoNothing:
                {
                    break;
                }
                case UpdateFlag_T::Update:
                {
                    UpdateNode( transform, R );
                    break;
                }
                case UpdateFlag_T::Split:
                {
                    // Push this node as unvisited; it will be split.
                    stack[++stack_ptr] = (R << 1);
                    break;
                }
            }
            
            switch( NodeNeedsUpdateQ( L ) )
            {
                case UpdateFlag_T::DoNothing:
                {
                    break;
                }
                case UpdateFlag_T::Update:
                {
                    UpdateNode( transform, L );
                    break;
                }
                case UpdateFlag_T::Split:
                {
                    // Push this node as unvisited; it will be split.
                    stack[++stack_ptr] = (L << 1);
                    break;
                }
            }
        }
        else
        {
            ComputeBall( node );
            
            // Pop this node.
            --stack_ptr;
        }
    }
}
