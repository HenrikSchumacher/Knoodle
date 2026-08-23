bool AdmissableEdgeQ( const Int p, const bool side, const Int x )
{
    if( p==NIL ) { return (x == root); }
    
    return (Child(p,side) == x);
}

bool AssertAdmissableEdge( const Int p, const bool side, const Int x )
{
    if( !AdmissableEdgeQ(p,side,x) )
    {
        eprint(MethodName("AssertAdmissableEdge") + ": node x = " + ToString(x) + " is not the child of node p = " + ToString(p) + " on side " + (side ? "Right" : "Left") + ". Aborting.");
        return false;
    }
    
    return true;
}

/*!@brief Rotate tree at node `x` in direction `dir`.
 *
 * @param p The parent node of `x`. It must be `NIL` if `x` is the root of the tree.
 *
 * @param side If `p` is not `NIL`, then we expect that `x == Child(p,side)`. Otherwise, it does not matter. This variable is to reduce branching in the code.
 *
 * @param x The node at which we rotate the tree.
 *
 * @param dir Direction into which we rotate the tree.
 */
int RotateTree( const Int p, const bool side, const Int x, const bool dir )
{
    if constexpr ( debug )
    {
        TOOLS_PTIMER(timer,MethodName("RotateTree(") + (dir ? "right)" : "left)") );
    }
//    print(std::string("== RotateTree ") + (dir ? "right" : "left"));
    
//    // DEBUGGING
//    if( !AdmissableEdgeQ(p,side,x) )
//    {
//        eprint(MethodName("RotateTree") + ": node x = " + ToString(x) + " is not the child of node p = " + ToString(p) + " on side " + (side ? "Right" : "Left") + ". Aborting.");
//        return 1;
//    }
    
//    TOOLS_DUMP(p);
//    TOOLS_DUMP(x);
    /*! For side == Left and dir == Right
     *           p                 p
     *          /                 /
     *         /                 /
     *        x                 y
     *       / \               / \
     *      /   \             /   \
     *     y     a     ==>   b     x
     *    / \                     / \
     *   /   \                   /   \
     *  b     z                 z     a
     *
     */
    Int y = Child(x,!dir);
    Int z = Child(y, dir);
    
    Child(x,!dir) = z;
    Child(y, dir) = x;
    
    if( p != NIL )
    {
        Child(p,side) = y;
    }
    else
    {
        // `x` is root.
        root = y;   
    }

    return 0;
}
