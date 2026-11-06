public:

/*!@brief Insert a node with field data equal to `data`.
 *
 * CAUTION: There is currently no guarantee on the state of the internal path stack after the call to `Insert`: in particular, `Current()` need _not_ point to the inserted node and `Parent()` need not be its parent.
 *
 * @return `0` if the insertion was successful and if the internal path stack traces the path to the inserted node. `1` if the insertion was successful, but the path to the inserted node is not traced. `-1` if a node with that data field already existed.
 */
template<bool verboseQ = false>
int Insert( const Data_T & data, const bool overwriteQ = false )
{
    if constexpr ( verboseQ )
    {
        logprint(MethodName("Insert(" + ToString(data)+ ")"));
    }
    
    // First we do a standard BST insertion.
    
    // The call to Find is necessary to initialize that path stack correctly.
    if( Find(data) )
    {
        if( overwriteQ ) { SetData(Current(),data); }
        return -1;
    };
    
    const Int new_node = CreateNode(data);
    
    if( Parent() == NIL )
    {
        root = new_node;
        SetState(root, State_T::Black);
        return 0;
    }
    
    SetChild(Parent(), ParentSide(), new_node);
    // `Current()` is still a `NIL`. We inserted `node` there, so we have to tell the path about it, too.
    current = new_node;

    // Rebalance the tree.
    do
    {
        auto [P,P_side] = ParentData();
        if( GetState(P) == State_T::Black )
        {
//            logprint("Case 1");
            return 0;
        }
        
        // Parent() is red.
        
        auto [G,G_side] = GrandParentData();
        if( G == NIL )
        {
//            logprint("Case 4");
            SetState(P, State_T::Black);
            return 0;
        }
        
        Int U = GetChild(G, !G_side);
        
        // CAUTION: The path will be invalidated by this. But there is a guaranteed `return` in this if-branch, so messing around with the path does not harm the do loop.
        if( (U == NIL) || (GetState(U) == State_T::Black) )
        {
            // parent is red but uncle is black.
//            logprint("Case 5 or 6");
            
            Int N = Current();
            
//            AssertParent(G,G_side,P);
//            AssertParent(P,P_side,N);
            
            if( G_side != P_side )
            {
//                logprint("Case 5");
                /*! The goals is to transform this to case 6.
                 *
                 *  GrandParentSide() == Left; (color in parentheses)
                 *  .
                 *            GG(r)                       GG(r)
                 *             |                           |
                 *             |                           |
                 *            G(b)                        G(b)
                 *            / \                         / \
                 *           /   \                       /   \
                 *          /     \         ==>         /     \
                 *         /       \                   /       \
                 *       P(r)      U(b)              N(r)      U(b)
                 *       / \       / \               / \       / \
                 *      /   \     /   \             /   \     /   \
                 *    Z(b)  N(r)                  P(r)  Y(b)
                 *          / \                   / \
                 *         /   \                 /   \
                 *       X(b) Y(b)             Z(b) X(b)
                 */
                
                /*! This transforms this to case #6 */
                RotateTree(G, G_side, P, G_side );
                std::swap(P,N);
            }
            
//            logprint("Case 6");

            /*! GrandParentSide() == ParentSide == Left; (color in parentheses)
             *  .
             *           GG(r)                       GG(r)
             *             |                           |
             *             |                           |
             *            G(b)                        P(b)
             *            / \                         / \
             *           /   \                       /   \
             *          /     \          ==>        /     \
             *         /       \                   /       \
             *       P(r)      U(b)              N(r)      G(r)
             *       / \       / \               / \       / \
             *      /   \     /   \             /   \     /   \
             *    N(r) Z(b) A(r)  B(r)        X(b) Y(b) Z(b)  U(b)
             *    / \                                         / \
             *   /   \                                       /   \
             * X(b)  Y(b)                                  A(r) B(r)
             */

            SetState(P, State_T::Black);
            SetState(G, State_T::Red  );
            
            auto [GG,GG_side] = GreatGrandParentData();
            RotateTree( GG, GG_side, G, !G_side );
            
            // TODO: path is in inconsistent state now. Shall we repair it?
            return 1;
        }
        
//        logprint("Case 2");
        // Case 2: parent and uncle are both red.
        // We make both black, and turn the grandparent red.
        SetState(P, State_T::Black);
        SetState(U, State_T::Black);
        SetState(G, State_T::Red  );
        // This may cause a red violation at the grandparent, so we walk two steps up and check again.
        PopPath();
        PopPath();

    } while ( Parent() != NIL );
    
//    logprint("Case 3");
    // Now Current() should be the root and have color red.
    return 0;
}


private:

Int NewNodeId()
{
    ++node_count;
    
    Int node;
    if( !deleted_nodes.empty() )
    {
        node = deleted_nodes.back();
        deleted_nodes.pop_back();
    }
    else
    {
        if( node_end >= NodeCapacity() )
        {
            Reserve( Int(2) * Max(NodeCapacity(),Int(1)) );
        }
        node = node_end;
        ++node_end;
    }
    return node;
}

Int CreateNode( cref<Data_T> data )
{
    const Int node    = NewNodeId();
    SetChild(node, Left , NIL);
    SetChild(node, Right, NIL);
    SetData (node, data);
    SetState(node, State_T::Red);
    return node;
}

Int CreateNode( Data_T && data )
{
    const Int node = NewNodeId();
    SetChild(node, Left , NIL);
    SetChild(node, Right, NIL);
    SetData (node, std::move(data));
    SetState(node, State_T::Red);
    
    return node;
}
