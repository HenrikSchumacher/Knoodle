private:

void UpdateSubtree_Recursive( const Int start_node = Root() )
{
    if( InternalNodeQ(start_node) )
    {
        updateSubtree_Recursive(start_node);
    }
}

void updateSubtree_Recursive( const Int node )
{
    switch( NodeNeedsUpdateQ(node) )
    {
        case UpdateFlag_T::DoNothing:
        {
            break;
        }
        case UpdateFlag_T::Update:
        {
            UpdateNode( transform, node );
            break;
        }
        case UpdateFlag_T::Split:
        {
            // If we land here, then `node` is an internal node.
            auto [L,R] = Children(node);
            PushTransform(node,L,R);
            updateSubtree_Recursive(L);
            updateSubtree_Recursive(R);
            ComputeBall(node);
        }
    }
}
