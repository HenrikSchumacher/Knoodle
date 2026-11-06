public:


template<bool verboseQ = false>
int Delete( const Data_T & data )
{
    if constexpr ( verboseQ ) { logprint(MethodName("Delete")); }
    
    if( !Find(data) ) { return -1; };

    return DeleteCurrentNode();
}

private:

template<bool verboseQ = false>
int DeleteCurrentNode()
{
    Int node = Current();
    
    if constexpr ( verboseQ )
    {
        logprint("DeleteCurrentNode");
        
        logvalprint("Node to delete",node);
    }
    
    const bool leftQ  = (GetChild(node,Left ) != NIL);
    const bool rightQ = (GetChild(node,Right) != NIL);
    
    int child_count = leftQ + rightQ;
    
    switch ( child_count )
    {
        case 2:
        {
            if constexpr ( verboseQ )
            {
                logprint("Deleting node with 2 children.");
            }
            
            // Find leftmost descendant of right subtree
            PushPath(Right);
            WalkToEnd(Left);
            const Int descendant = Current();
            
            // Swap data.
            // TODO: This moves data, which is a bit unpleasant.
            // TODO: It would be nicer if we could swap the nodes and leave the data where it is.
            SetData(node, GetData(descendant));

            // Delete `descendant`.
            // Note that `descendant` is guaranteed to have either no children or a single  child to the _right_. So calling DeleteCurrentNode() takes us definitely to another branch and does not cycle indefinitely.
            // The only
            return DeleteCurrentNode();
        }
        case 1:
        {
            // Node must be black.
            // Single child must be red.
            
            const Int child = GetChild(node,rightQ);
            
            // We could move data of `C` to `N` and delete `C` or replace `N` by `C` and paint it black.
            // We do the latter because we want to avoid copying.
            
            if( node == root )
            {
                if constexpr ( verboseQ )
                {
                    logprint("Deleting root node with 1 child.");
                }
                root = child;
            }
            else
            {
                if constexpr ( verboseQ )
                {
                    logprint("Deleting interior node with 1 child.");
                }
                auto [parent,parent_side] = ParentData();
                SetChild(parent, parent_side, child);
            }
            
            SetState(child, State_T::Black);
            DeactivateNode(node);
            return 0;
        }
        default:
        {
            // child_count == 0
            
            if( node == root )
            {
                if constexpr ( verboseQ )
                {
                    logprint("Deleting root node with 0 children.");
                }
                root = NIL;
                DeactivateNode(node);
                return 0;
            }
            
            if( GetState(node) == State_T::Red )
            {
                if constexpr ( verboseQ )
                {
                    logprint("Deleting red node with 0 children.");
                }
                auto [parent,parent_side] = ParentData();
                SetChild(parent, parent_side, NIL);
                DeactivateNode(node);
                return 0;
            }
            
            if constexpr ( verboseQ )
            {
                logprint("Deleting black node with 0 children.");
            }
            
            auto [parent,parent_side] = ParentData();
            SetChild(parent, parent_side, NIL);
            DeactivateNode(node);
            
            return RebalanceAfterDelete();
        }
    }
}

template<bool verboseQ = false>
int RebalanceAfterDelete()
{
    // Node `node` is black and has no children.
    
    if constexpr ( verboseQ )
    {
        logprint(std::string("RebalanceAfterDelete: node to delete = ") + ToString(Current()) + ", data " + ToString(GetData(Current())) + ".");
    }
    
    Int  P;  // parent;
    Int  G;  // grand parent;
    Int  S;  // sibling
    Int  CN; // close nephew
    Int  DN; // distant nephew
    
    bool P_side;
    bool G_side;

    do
    {
        std::tie(G,G_side) = GrandParentData();
        std::tie(P,P_side) = ParentData();
        
        // N is black, so it must have a sibling.
        S  = GetChild(P,!P_side);
        DN = GetChild(S,!P_side);
        CN = GetChild(S, P_side);
        
        if( GetState(S) == State_T::Red )
        {
            if constexpr ( verboseQ ) { logprint("Case 3"); }
            
            RotateTree(G, G_side, P, P_side);
            SetState(P, State_T::Red  );
            SetState(S, State_T::Black);
            
            /*! For `P_side == Left`.
             *
             *         P(?)                      S(b)
             *         / \                       / \
             *        /   \                     /   \
             *       /     \                   /     \
             *      /       \        ==>      /       \
             *    N(b)     S(r)             P(r)      DN
             *    / \       / \             / \
             *             /   \           /   \
             *            CN   DN        N(b)  CN
             *                           / \
             */
            
            // TODO: Aren't CN and DN black at this point?
            
            G      = S;
            G_side = P_side;
            S      = CN;
            DN     = GetChild(S,!P_side);
            
            if( RedQ(DN) )
            {
                /*!            G(b) = old S(b)
                 *              /
                 *             /
                 *            /
                 *           /
                 *         P(r)
                 *         / \
                 *        /   \
                 *       /     \
                 *      /       \
                 *    N(b)      S(?)
                 *              / \
                 *             /   \
                 *         CN(?)   DN(r)
                 */
                
                goto case_6;
            }
            
            CN = GetChild(S, P_side);
            if( RedQ(CN) )
            {
                /*!            G(b) = old S(b)
                 *              /
                 *             /
                 *            /
                 *           /
                 *         P(r)
                 *         / \
                 *        /   \
                 *       /     \
                 *      /       \
                 *    N(b)      S(?)
                 *              / \
                 *             /   \
                 *          CN(r)  DN(b)
                 */
                
                goto case_5;
            }
            
            if constexpr ( verboseQ ) { logprint("Case 4a"); }
            
            /*!            G(b) = old S(b)
             *              /
             *             /
             *            /
             *           /
             *         P(r) ==> P(b)
             *         / \
             *        /   \
             *       /     \
             *      /       \
             *    N(b)      S(?) ==> S(r)
             *              / \
             *             /   \
             *          CN(b)  DN(b)
             */
            
            SetState(P, State_T::Black);
            SetState(S, State_T::Red  );
            if constexpr ( verboseQ ) { logprint("Exit from case 4a"); }
            return 0;
            
        } // if( GetState(S) == State_T::Red )
        
        // S must be black.
        
        /*!        P(?)
         *         / \
         *        /   \
         *       /     \
         *      /       \
         *    N(b)     S(b)
         *              / \
         *             /   \
         *          CN(?)  DN(?)
         */
        
        if( RedQ(DN) ) { goto case_6; }
        
        if( RedQ(CN) ) { goto case_5; }

        // Both nephews are black.
        /*!        P(?)
         *         / \
         *        /   \
         *       /     \
         *      /       \
         *    N(b)     S(b)
         *              / \
         *             /   \
         *          CN(b)  DN(b)
         */
        
        if( GetState(P) == State_T::Red )
        {
            /*!        P(r) ==> P(b)
             *         / \
             *        /   \
             *       /     \
             *      /       \
             *    N(b)     S(b) == S(r)
             *              / \
             *             /   \
             *          CN(b)  DN(b)
             */
            
            if constexpr ( verboseQ ) { logprint("Case 4b"); }
            
            SetState(S, State_T::Red  );
            SetState(P, State_T::Black);
            if constexpr ( verboseQ ) { logprint("Exit from case 4b"); }
            return 0;
        }
        
        /*!        P(b)
         *         / \
         *        /   \
         *       /     \
         *      /       \
         *    N(b)     S(b)
         *              / \
         *             /   \
         *          CN(b)  DN(b)
         */
        if constexpr ( verboseQ ) { logprint("Case 2"); }
        
        SetState(Sibling(), State_T::Red);
        PopPath();
        
    } while ( Parent() != NIL );
    
    if constexpr ( verboseQ ) { logprint("Exit from case 1"); }
    
    return 0;

case_5:
    
    if constexpr ( verboseQ )
    {
        logprint("Case 5");
        
        if( GetState(CN) != State_T::Red )
        {
            wprint("GetState(CN) != State_T::Red");
        }
    }
    
//    if( GetState(DN) != State_T::Black )
//    {
//        wprint("GetState(DN) != State_T::Black");
//    }
    
    RotateTree( P, !P_side, S, !P_side);
    SetState(S , State_T::Red  );
    SetState(CN, State_T::Black);

    /*!            G(?)                         G(?)
     *              /                            /
     *             /                            /
     *            /                            /
     *           /                            /
     *         P(?)                         P(r)
     *         / \                          / \
     *        /   \           ==>          /   \
     *       /     \                      /     \
     *      /       \                    /       \
     *    N(b)      S(?)               N(b)     CN(b)
     *              / \                          / \
     *             /   \                        /   \
     *          CN(r)  DN(?)                   W    S(r)
     *           / \   / \                          / \
     *          W   X Y   Z                        /   \
     *                                            X    DN(?)
     *                                                  / \
     *                                                 Y   Z
     */

    // Prepare Case 6.
    DN        = S;
    S         = CN;

case_6:
    
    if constexpr ( verboseQ )
    {
        logprint("Case 6");
        
        if( GetState(DN) != State_T::Red )
        {
            wprint("GetState(DN) != State_T::Red");
        }
    }
    
    RotateTree( G, G_side, P, P_side);
    SetState(S , GetState(P)   );
    SetState(P , State_T::Black);
    SetState(DN, State_T::Black);
    
    /*!            G(?)                         G(?)
     *              /                            /
     *             /                            /
     *            /                            /
     *           /                            /
     *         P(?)                         S(r)
     *         / \                          / \
     *        /   \           ==>          /   \
     *       /     \                      /     \
     *      /       \                    /       \
     *    N(b)      S(?)               P(b)     DN(b)
     *              / \                / \
     *             /   \              /   \
     *         CN(?)   DN(r)       N(b)   CN(?)
     */
    if constexpr ( verboseQ ) { logprint("Exit from case 6"); }
    return 0;
}


void DeactivateNode( Int node )
{
    if( node == NIL ) { return; }
    
    deleted_nodes.push_back(node);
    
    if constexpr ( bound_checksQ )
    {
        if ( node_count <= Int{0} )
        {
            eprint(MethodName("DeactivateNode") + ": node_count <= Int{0}.");
        }
    }
    
    --node_count;

}
