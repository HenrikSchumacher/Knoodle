public:

bool CollideQ()
{
    TOOLS_PTIMER(timer,MethodName("CollideQ"));
    return SubtreesCollideQ(Root());
}

private:

// TODO: This is almost literally copied from ClisbyTree.
int CheckJoints()
{
    const Int n = T.VertexCount();
    
    const Int p_prev = (p == 0)     ? (n - 1) : (p - 1);
    const Int p_next = (p + 1 == n) ? Int(0)  : (p + 1);

    Vector_T X_p_prev = T.VertexCoordinates(p_prev);
    Vector_T X_p_next = T.VertexCoordinates(p_next);
    
    if( mid_changedQ )
    {
        X_p_next = transform(X_p_next);
    }
    else
    {
        X_p_prev = transform(X_p_prev);
    }
    
    if( SquaredDistance(X_p_next,X_p_prev) <= hard_sphere_squared_diam )
    {
        witness[0] = p_prev;
        witness[1] = p_next;
        return 2;
    }
    
    const Int q_prev = (q == 0)     ? (n - 1) : (q - 1);
    const Int q_next = (q + 1 == n) ? Int(0)  : (q + 1);
    
    Vector_T X_q_prev = T.VertexCoordinates(q_prev);
    Vector_T X_q_next = T.VertexCoordinates(q_next);
    
    if( mid_changedQ )
    {
        X_q_prev = transform(X_q_prev);
    }
    else
    {
        X_q_next = transform(X_q_next);
    }
    
    if( SquaredDistance(X_q_next,X_q_prev) <= hard_sphere_squared_diam )
    {
        witness[0] = q_prev;
        witness[1] = q_next;
        return 3;
    }
    
    return 0;
}

//bool BallsCollideQ( cref<ClisbyNode> M, cref<ClisbyNode> N ) const
//{
//    const Real threshold = hard_sphere_diam + M.radius + N.radius;
//    
//    return (SquaredDistance(M.center,N.center) < threshold * threshold);
//}

bool NodesMightCollideQ( mref<ClisbyNode> M, mref<ClisbyNode> N ) const
{
    if( ( M.all_unmovedQ && N.all_unmovedQ )
        ||
        ( M.all_movedQ && N.all_movedQ )
    )
    {
        return false;
    }
    else
    {
        Real threshold = hard_sphere_diam + M.radius + N.radius;
        
        return SquaredDistance(M.center,N.center) < threshold * threshold;
    }
}

bool SubtreesCollideQ( mref<ClisbyNode> N )
{
    if( N.all_movedQ || N.all_unmovedQ )
    {
        return false;
    }
    
    if( !N.internalQ )
    {
        return false;
    }
    
    auto [L,R] = GetChildren(N);
    
    return SubtreesCollideQ(L)
        || SubtreesCollideQ(R)
        || SubtreesCollideQ(L,R);
}

bool SubtreesCollideQ( mref<ClisbyNode> M, mref<ClisbyNode> N )
{
    if( !NodesMightCollideQ(M,N) )
    {
        return false;
    }
    
    if( M.internalQ && N.internalQ )
    {
        // Split both nodes.

        auto [L_N,R_N] = GetChildren(N);
        auto [L_M,R_M] = GetChildren(M);
        
        return SubtreesCollideQ(L_M,L_N)
            || SubtreesCollideQ(R_M,R_N)
            || SubtreesCollideQ(L_M,R_N)
            || SubtreesCollideQ(R_M,L_N);
    }
    else if( M.internalQ && !N.internalQ )
    {
        // Split M only.
        
        auto[L_M,R_M] = GetChildren(M);
        
        return SubtreesCollideQ(L_M,N) || SubtreesCollideQ(R_M,N);
    }
    else if( !M.internalQ && N.internalQ )
    {
        // Split N only.
        
        auto [L_N,R_N] = GetChildren(N);
        
        return SubtreesCollideQ(M,L_N) || SubtreesCollideQ(M,R_N);
    }
    else
    {
        // Nodes M and N are overlapping leaf nodes.
        
        // Rule out that tiny distance errors of neighboring vertices cause problems.
        const Int k = M.begin;
        const Int l = N.begin;
        
        const Int delta = Abs(k-l);
        
        if( Min( delta, T.VertexCount() - delta ) > Int(1) )
        {
            witness[0] = k;
            witness[1] = l;
            
            return true;
        }
        else
        {
            return false;
        }
    }
    
    return false;
}
