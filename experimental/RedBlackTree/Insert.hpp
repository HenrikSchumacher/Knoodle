public:

/*!@brief Insert a node with field data equal to `data`.
 *
 * CAUTION: There is currently no guarantee on the state of the internal path stack after the call to `Insert`: in particular, `Current()` need _not_ point to the inserted node and `Parent()` need not be its parent.
 *
 * @param f A function. It is assumed that the nodes `N` in the tree are sorted by `f(N.data)`
 *
 * @param cmp A user-defined comparison function for the type returned by `f`. `cmp(a,b)` should return true if `a` is considered less than `b`.
 *
 * @return `0` if the insertion was successful and if the internal path stack traces the path to the inserted node. `1` if the insertion was successful, but the path to the inserted node is not traced. `-1` if a node with that data field already existed.
 */
template<
    bool rebalanceQ = true, typename F = std::identity, typename C = std::less<>
>
int Insert( const Data_T & data, F && f = F(), C && cmp = C() )
{
    if constexpr ( debug )
    {
        TOOLS_PTIMER(timer,MethodName("Insert"));
    }
    
    // First we do a standard BST insertion.
    
    // The call to Find is necessary to initialize that path stack correctly.
    if( Find(data, std::forward<F>(f), std::forward<C>(cmp)) )
    {
        return -1;
    };

    const Int node = CreateNode(data);

    if( Parent() == NIL )
    {
        root = node;
        State(root) = State_T::Black;
        return 0;
    }
    
    Child(Parent(),ParentSide()) = node;
    // `Current()` is still a `NIL`. We inserted `node` there, so we have to tell the path about it, too.
    current = node;
    
    if constexpr ( !rebalanceQ ) { return 0; }
    
    // Rebalance the tree.
    do
    {
        auto [P,P_side] = ParentData();
        if( State(P) == State_T::Black )
        {
//            print("Case 1");
            return 0;
        }
        
        // Parent() is red.
        
        auto [G,G_side] = GrandParentData();
        if( G == NIL )
        {
//            print("Case 4");
            State(P) = State_T::Black;
            return 0;
        }
        
        Int U = Child(G, !G_side);
        
        // CAUTION: The path will be invalidated by this. But there is a guaranteed `return` in this if-branch, so messing around with the path does not harm the do loop.
        if( (U == NIL) || (State(U) == State_T::Black) )
        {
            // parent is red but uncle is black.
//            print("Case 5 or 6");
            
            Int N = Current();
            
//            AssertParent(G,G_side,P);
//            AssertParent(P,P_side,N);
            
            if( G_side != P_side )
            {
//                print("Case 5");
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
            
//            print("Case 6");

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

            State(P) = State_T::Black;
            State(G) = State_T::Red;
            
            auto [GG,GG_side] = GreatGrandParentData();
            RotateTree( GG, GG_side, G, !G_side );
            
            // TODO: path is in inconsistent state now. Shall we repair it?
            return 1;
        }
        
//        print("Case 2");
        // Case 2: parent and uncle are both red.
        // We make both black, and turn the grandparent red.
        State(P) = State_T::Black;
        State(U) = State_T::Black;
        State(G) = State_T::Red;
        // This may cause a red violation at the grandparent, so we walk two steps up and check again.
        PopPath();
        PopPath();

    } while ( Parent() != NIL );
    
//    print("Case 3");
    // Now Current() should be the root and have color red.
    return 0;
}


private:

Int NewNodeId()
{
    ++node_count;
    
    Int node;
    if( !deleted_nodes.EmptyQ() )
    {
        node = deleted_nodes.Pop();
    }
    else
    {
        if( node_end >= node_buffer.Size() )
        {
            // TODO: Add a check whether this size can be stored in the integer type.
            node_buffer.template Resize<true>(
                Int(2) * Max(node_buffer.Size(),Int(1))
            );
        }
        node = node_end;
        ++node_end;
    }
    return node;
}

Int CreateNode( cref<Data_T> data )
{
    const Int node = NewNodeId();
    Node_T & N     = node_buffer[node];
    N.child[Left ] = NIL;
    N.child[Right] = NIL;
    N.data         = data;
    N.state        = State_T::Red;
    return node;
}

Int CreateNode( Data_T && data )
{
    const Int node = NewNodeId();
    Node_T & N     = node_buffer[node];
    N.child[Left ] = NIL;
    N.child[Right] = NIL;
    N.data         = std::move(data);
    N.state        = State_T::Red;
    return node;
}
