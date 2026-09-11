public:

bool AdmissableEdgeQ( const Int p, const bool side, const Int x )
{
    if( p==NIL ) { return (x == root); }
    
    return (GetChild(p,side) == x);
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

public: // TODO: Make this private after testing.

/*!@brief Rotate tree at node `x` in direction `dir`.
 *
 * @param p The parent node of `x`. It must be `NIL` if `x` is the root of the tree.
 *
 * @param side If `p` is not `NIL`, then we expect that `x == GetChild(p,side)`. Otherwise, it does not matter. This variable is to reduce branching in the code.
 *
 * @param x The node at which we rotate the tree.
 *
 * @param dir Direction into which we rotate the tree.
 */
int RotateTree( const Int p, const bool side, const Int x, const bool dir )
{
    if constexpr ( debugQ )
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
    Int y = GetChild(x,!dir);
    Int z = GetChild(y, dir);

    SetChild(x,!dir,z);
    SetChild(y, dir,x);
    
    if( p != NIL )
    {
        SetChild(p,side,y);
    }
    else
    {
        // `x` is root.
        root = y;   
    }

    return 0;
}
