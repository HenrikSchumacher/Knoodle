public:

class ModifiedClisbyTree
{
    
    friend class ClisbyTree;
    
    cref<ClisbyTree> T;
    
    Stack<ClisbyNode,Int> mod_nodes;
    Tensor1<Int,Int> node_lookup;
    
    Int  p;
    Int  q;
    Real theta;
    Real hard_sphere_diam;
    Real hard_sphere_squared_diam;
    
    Vector_T X_p;
    Vector_T X_q;
    Transform_T transform;
    
    WitnessVector_T witness {{-1,-1}};

    bool reflectQ;
    bool mid_changedQ;
    
    // TODO: Needs also random engine.
    
public:
    
    ModifiedClisbyTree( cref<ClisbyTree> T_ )
    :   T                           ( T_                         )
    ,   mod_nodes                   ( T.NodeCount()              )
    ,   node_lookup                 ( T.NodeCount(), Int(-1)     )
    ,   hard_sphere_diam            ( T.HardSphereDiameter()     )
    ,   hard_sphere_squared_diam    ( T.hard_sphere_squared_diam )
    {}
    
    void Clear()
    {
        witness[0] = -1;
        witness[1] = -1;
        
        if( mod_nodes.Size() > 0 )
        {
            for( ClisbyNode & N : mod_nodes )
            {
                node_lookup[N.local_id] = -1;
            }
            
            mod_nodes.Reset();
        }
    }
    
    Int ModifiedNodeCount() const
    {
        return mod_nodes.Size();
    }
    
    cref<Stack<ClisbyNode,Int>> ModifiedNodes() const
    {
        return mod_nodes;
    }
    
    mref<ClisbyNode> GetNode( Int id )
    {
//        print("GetNode");
//        TOOLS_DUMP(id);
        Int local_id = node_lookup[id];

        if( local_id < 0 )
        {
            local_id = ModifiedNodeCount();
            
            node_lookup[id] = local_id;
            
            ClisbyNode N ( id, local_id, T, p, q, mid_changedQ );
            mod_nodes.Push( std::move(N) );
        }
        
//        TOOLS_DUMP(local_id);
        
        return mod_nodes[local_id];
    }
    
    mref<ClisbyNode> Root()
    {
        return GetNode( T.Root() );
    }
    
//    mref<ClisbyNode> GetLeftChild( cref<ClisbyNode> N )
//    {
//        return GetNode( T.LeftChild(N.id) );
//    }
//    
//    mref<ClisbyNode> GetRightChild( cref<ClisbyNode> N )
//    {
//        return GetNode( T.RightChild(N.id) );
//    }
    
    std::pair<mref<ClisbyNode>,mref<ClisbyNode>> GetChildren( mref<ClisbyNode> N )
    {
//        print("GetChildren");
//        TOOLS_DUMP(N.id);
        
        if( !N.interiorQ )
        {
            eprint("!!!");
        }
        
//        Int N_id = N.id;
//        Int L_id = T.LeftChild (N.id);
//        TOOLS_DUMP(N.id);
//        TOOLS_DUMP(L_id);
//        Int R_id = T.RightChild(N.id);
//        TOOLS_DUMP(N.id);
//        TOOLS_DUMP(R_id);
        
        mref<ClisbyNode> L = GetNode( T.LeftChild (N.id) );
        mref<ClisbyNode> R = GetNode( T.RightChild(N.id) );
        
        
//        print("---------before---------");
//        
//        TOOLS_DUMP(N.id);
//        TOOLS_DUMP(L.id);
//        TOOLS_DUMP(R.id);
//        
//        TOOLS_DUMP(N.f.Matrix());
//        TOOLS_DUMP(N.f.Vector());
//        TOOLS_DUMP(N.f.NontrivialQ());
//        
//        TOOLS_DUMP(L.f.Matrix());
//        TOOLS_DUMP(L.f.Vector());
//        TOOLS_DUMP(L.f.NontrivialQ());
//        
//        TOOLS_DUMP(R.f.Matrix());
//        TOOLS_DUMP(R.f.Vector());
//        TOOLS_DUMP(R.f.NontrivialQ());
        
        PushTransform( N, L, R );
        
//        print("---------after----------");
//        TOOLS_DUMP(N.id);
//        TOOLS_DUMP(L.id);
//        TOOLS_DUMP(R.id);
//        
//        TOOLS_DUMP(N.f.Matrix());
//        TOOLS_DUMP(N.f.Vector());
//        TOOLS_DUMP(N.f.NontrivialQ());
//        
//        TOOLS_DUMP(L.f.Matrix());
//        TOOLS_DUMP(L.f.Vector());
//        TOOLS_DUMP(L.f.NontrivialQ());
//        
//        TOOLS_DUMP(R.f.Matrix());
//        TOOLS_DUMP(R.f.Vector());
//        TOOLS_DUMP(R.f.NontrivialQ());
        
        return std::pair<mref<ClisbyNode>,mref<ClisbyNode>>(L,R);
    }
    
    template<bool update_centerQ = true, bool update_transformQ = true>
    void Update( cref<Transform_T> f, mref<ClisbyNode> N )
    {
        if constexpr ( update_centerQ )
        {
            N.center = f(N.center);
            
//            if constexpr ( countersQ )
//            {
//                ++call_counters.mv;
//            }
        }
        
        if constexpr ( update_transformQ )
        {
            // Transformation of a leaf node never needs a change.
            if( !N.interiorQ )
            {
                return;
            }
            
            if (N.state == NodeFlag_T::Id )
            {
                N.f = f;
            }
            else
            {
                N.f = f(N.f);
                
//                if constexpr ( countersQ )
//                {
//                    ++call_counters.load_transform;
//                    ++call_counters.mv;
//                    ++call_counters.mm;
//                }
            }
            
            N.state = NodeFlag_T::NonId;
        }
    }
    
    void PushTransform( mref<ClisbyNode> N, mref<ClisbyNode> L, mref<ClisbyNode> R )
    {
        if( N.state == NodeFlag_T::Id )
        {
            return;
        }
        
        Update(N.f,L);
        Update(N.f,R);
        N.state = NodeFlag_T::Id;
    }
    
    void MergeBalls( cref<ClisbyNode> L, cref<ClisbyNode> R, mref<ClisbyNode> N )
    {
        const Real d = Distance(L.center,R.center);
        
        if( d + R.radius <= L.radius ) [[unlikely]]
        {
            N.center = L.center;
            N.radius = L.radius;
            return;
        }
        
        if( d + L.radius <= R.radius ) [[unlikely]]
        {
            N.center = R.center;
            N.radius = R.radius;
            return;
        }
        
        const Real s = Frac<Real>( R.radius - L.radius, Scalar::Two<Real> * d );
        
        const Real L_alpha = Scalar::Half<Real> - s;
        const Real R_alpha = Scalar::Half<Real> + s;
        
        N.center = L_alpha * L.center + R_alpha * R.center;
        N.radius = Scalar::Half<Real> * ( d + L.radius + R.radius );
    }
    
    bool BallsOverlapQ( cref<ClisbyNode> M, cref<ClisbyNode> N ) const
    {
        const Real d2 = SquaredDistance(M.center,N.center);
        
        const Real threshold = hard_sphere_diam + M.radius + N.radius;
        
        return (d2 < threshold * threshold);
    }
    
    void UpdateSubtree( mref<ClisbyNode> N )
    {
        // TODO: Careful, N could be a leaf node.
        
        print("UpdateSubtree");
        TOOLS_DUMP(N.id);
        
        if( N.all_movedQ )
        {
            print("UpdateSubtree - case A");
            
            
            Update(transform,N);
        }
        else if( (!N.all_movedQ) && (!N.all_unmovedQ)  )
        {
            print("UpdateSubtree - case B");
            
            if( !N.interiorQ )
            {
                eprint("AAA!!!!");
            }
            auto [L,R] = GetChildren(N);
            
//            print("UpdateSubtree");
//            TOOLS_DUMP(L.id);
//            TOOLS_DUMP(L.center);
//            TOOLS_DUMP(R.id);
//            TOOLS_DUMP(R.center);
            
            UpdateSubtree(L);
            UpdateSubtree(R);
            MergeBalls(L,R,N);
        }
    }
    
    bool NodesMightOverlapQ( cref<ClisbyNode> M, cref<ClisbyNode> N ) const
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
    
    bool SubtreesCollideQ( cref<ClisbyNode> N )
    {
        if( N.all_movedQ || N.all_unmovedQ )
        {
            return false;
        }
        
        if( !N.interiorQ )
        {
            return false;
        }
        
        auto [L,R] = GetChildren(N);
        
        return SubtreesCollideQ(L)
            || SubtreesCollideQ(R)
            || SubtreesCollideQ(L,R);
    }
    
    bool SubtreesCollideQ( cref<ClisbyNode> M, cref<ClisbyNode> N )
    {
        if( !NodesMightOverlapQ(M,N) )
        {
            return false;
        }
        
        if( M.interiorQ && N.interiorQ )
        {
            // Split both nodes.

            auto [L_N,R_N] = GetChildren(N);
            auto [L_M,R_M] = GetChildren(M);
            
            return SubtreesCollideQ(L_M,L_N)
                || SubtreesCollideQ(R_M,R_N)
                || SubtreesCollideQ(L_M,R_N)
                || SubtreesCollideQ(R_M,L_N);
        }
        else if( M.interiorQ && !N.interiorQ )
        {
            // Split M only.
            
            auto[L_M,R_M] = GetChildren(M);
            
            return SubtreesCollideQ(L_M,N) || SubtreesCollideQ(R_M,N);
        }
        else if( !M.interiorQ && N.interiorQ )
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
    
    
    bool OverlapQ()
    {
        TOOLS_PTIC(ClassName()+"::OverlapQ");
        bool result = SubtreesCollideQ( T.Root() );
        TOOLS_PTOC(ClassName()+"::OverlapQ");
        
        return result;
    }
    
    
private:

    // TODO: This is almost literally copied from ClisbyTree.
    template<bool allow_reflectionsQ>
    int LoadPivots( const Int p_, const Int q_, const Real theta_, const bool reflectQ_ )
    {
        print("LoadPivots");
        p = Min(p_,q_);
        q = Max(p_,q_);
        theta = theta_;
        reflectQ = reflectQ_;
        
        TOOLS_DUMP(theta);
        TOOLS_DUMP(reflectQ);
        
        const Int n = T.VertexCount() ;
        const Int mid_size = q - p - 1;
        const Int rem_size = n - mid_size - 2;
        
        if( (mid_size <= 0) || (rem_size <= 0) ) [[unlikely]]
        {
            return 1;
        }
        
        mid_changedQ = (mid_size <= rem_size);
        
        // TODO: There is maybe a more efficient way to compute the pivot vectors.
        X_p = T.VertexCoordinates(p);
        X_q = T.VertexCoordinates(q);
        transform = PivotTransform<allow_reflectionsQ>( X_p, X_q, theta, reflectQ );
        
        TOOLS_DUMP(X_p);
        TOOLS_DUMP(X_q);
        TOOLS_DUMP(transform.Matrix());
        TOOLS_DUMP(transform.Vector());
        
        return 0;
    }
    
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
    
    void Update()
    {
//        Update
        UpdateSubtree(Root());
    }
    
public:
    
    // TODO: This is almost literally copied from ClisbyTree.
    template<bool allow_reflectionsQ, bool check_overlapsQ = true>
    FoldFlag_T Fold( Int p_, Int q_, Real theta_, bool reflectQ_ )
    {
        print("Fold");
        Clear();
        
        int pivot_flag = LoadPivots<allow_reflectionsQ>( p_, q_, theta_, reflectQ_ );
        
        if constexpr ( check_overlapsQ )
        {
            if( pivot_flag != 0 )
            {
                // Folding step aborted because pivots indices are too close.
                return pivot_flag;
            }
            
            int joint_flag = CheckJoints();
            
            if( joint_flag != 0 )
            {
                // Folding step failed because neighbors of pivot touch.
                return joint_flag;
            }
        }
        
        Update();

        if constexpr ( check_overlapsQ )
        {
            if( OverlapQ() )
            {
                // Folding step failed.
                return 4;
            }
            else
            {
                // Folding step succeeded.
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    
    static std::string ClassName()
    {
        return ct_string("ClisbyTree")
            + "<" + ToString(AmbDim)
            + "," + TypeName<Real>
            + "," + TypeName<Int>
            + "," + TypeName<LInt>
            + "," + ToString(clang_matrixQ)
            + "," + ToString(quaternionsQ)
//                + "," + ToString(orientation_preservingQ)
            + "," + ToString(countersQ)
            + "," + ToString(manual_stackQ)
            + "," + ToString(witnessesQ)
            + ">::ModifiedClisbyTree";
    }

    
}; // ClisbyNode

public:

void LoadModifications( mref<ModifiedClisbyTree> S )
{
    // TODO: Needs also state of random engine?
    
    p            = S.p;
    q            = S.q;
    theta        = S.theta;
    reflectQ     = S.reflectQ;
    mid_changedQ = S.mid_changedQ;
    transform    = S.transform;
    witness      = S.witness;
    
    // DEBUGGING
    TOOLS_DUMP( S.mod_nodes.Size() );
    
    TOOLS_DUMP(p);
    TOOLS_DUMP(q);
    TOOLS_DUMP(theta);
    TOOLS_DUMP(reflectQ);
    
    for( ClisbyNode & N : S.mod_nodes )
    {
        N.center.Write( N_ball.data(N.id) );
        N_ball(N.id,AmbDim) = N.radius;
        
        N.f.Write( N_transform.data(N.id), N_state[N.id] );
    }
}

