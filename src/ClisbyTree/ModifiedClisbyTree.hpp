public:

class ModifiedClisbyTree
{
    cref<ClisbyTree> T;
    
    std::vector<ClisbyNode> mod_nodes;
    Tensor1<Int,Int> node_lookup;
    
    Int  p;
    Int  q;
    Real theta;
    Real hard_sphere_diam;
    
    Transform_T transform;

    ModifiedClisbyTree( cref<ClisbyTree> T_             )
    :   T                   ( T_                        )
    ,   node_lookup         ( T.NodeCount(), , -1       )
    ,   hard_sphere_diam    ( T.HardSphereDiameter()    )
    {}
    
    void Clear()
    {
        for( ClisbyNode & N : mod_nodes )
        {
            node_lookup[N.local_id] = -1;
        }
        
        mod_nodes.clear();
    }
    
    void Fold( Int p_, Int q_, Int theta_ )
    {}
    
    Int ModifiedNodeCount() const
    {
        return static_cast<Int>(mod_nodes.size());
    }
    
    cref<std::vector<ClisbyNode>> ModifiedNodes() const
    {
        return mod_nodes;
    }
    
    mref<ClisbyNode> GetNode( Int id )
    {
        Int local_id = node_lookup[id];
        
        if( local_id < 0 )
        {
            local_id = ModifiedNodeCount();
            
            node_lookup[id] = local_id;
            
            mod_nodes.emplace_back( id, local_id, T );
        }
        
        return modified_nodes[local_id];
    }
    
    template<bool update_centerQ, bool update_transformQ>
    void Update( cref<Transform_T> f, mref<ClisbyNode> N )
    {
        if constexpr ( update_centerQ )
        {
            N.center = f(N.center)
            
//            if constexpr ( countersQ )
//            {
//                ++call_counters.mv;
//            }
        }
        
        if constexpr ( update_transformQ )
        {
            // Transformation of a leaf node never needs a change.
            if( !N.interior_Q )
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
        
        Update( N.f, L );
        Update( N.f, R );
        
        N.state = NodeFlag_T::Id;
    }
    
    void MergeBalls( cref<ClisbyNode> L, cref<ClisbyNode> R, mref<ClisbyNode> N )
    {
        const Real d = Distance(L.center,R.center);
        
        if( d + R.radius <= L.radius ) [[unlikely]]
        {
            N_center = L.center;
            N.radius = L.radius;
            return;
        }
        
        if( d + L.radius <= R.radius ) [[unlikely]]
        {
            N_center = R.center;
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
        
        if( "N contains only moved" )
        {
            UpdateNode( N );
        }
        else if( "N contains both moved and fixed" )
        {
            mref<ClisbyNode> R = GetNode( 2 * N.id + 2 );
            mref<ClisbyNode> L = GetNode( 2 * N.id + 1 );
            
            PushTransform( N, L, R );
            UpdateSubtree( L );
            UpdateSubtree( R );
            MergeBalls( L, R, N );
        }
    }
    
    bool NodesMightOverlapQ( cref<ClisbyNode> M, cref<ClisbyNode> N ) const
    {
        if( M.contains_no_movedQ && N.contains_no_movedQ )
        {
            return false;
        }
        else if ( M.contains_no_unmovedQ && N.contains_no_unmovedQ )
        {
            return false;
        }
        else
        {
            Real threshold = hard_sphere_diam + M.radius + N.radius;
            
            return SquaredDistance(M.center,N.center) < threshold * threshold;
        }
    }
    
    bool SubtreesCollideQ( cref<ClisbyNode> M, cref<ClisbyNode> N )
    {

        if( M.interiorQ && N.interiorQ )
        {
            mref<ClisbyNode> R_M = GetNode( 2 * M.id + 2 );
            mref<ClisbyNode> L_M = GetNode( 2 * M.id + 1 );
            mref<ClisbyNode> R_N = GetNode( 2 * N.id + 2 );
            mref<ClisbyNode> L_N = GetNode( 2 * N.id + 1 );
            
            PushTransform( M, L_M, R_M );
            PushTransform( N, L_N, R_N );
            
            if( NodesMightOverlapQ(L_M,L_N) && SubtreesCollideQ(L_M,L_N) )
            {
                return true;
            }
            
            if( NodesMightOverlapQ(R_M,R_N) && SubtreesCollideQ(R_M,R_N) )
            {
                return true;
            }
            
            if( NodesMightOverlapQ(L_M,R_N) && SubtreesCollideQ(L_M,R_N) )
            {
                return true;
            }
            
            if( (M.id != N.id) && NodesMightOverlapQ(R_M,L_N) && SubtreesCollideQ(R_M,L_N) )
            {
                return true;
            }
        }
        else if( M.interiorQ && !N.interiorQ )
        {
            mref<ClisbyNode> R_M = GetNode( 2 * M.id + 2 );
            mref<ClisbyNode> L_M = GetNode( 2 * M.id + 1 );
            
            PushTransform( M, L_M, R_M );
            
            if( NodesMightOverlapQ(L_M,N) && SubtreesCollideQ(L_M,N) )
            {
                return true;
            }
            
            if( NodesMightOverlapQ(R_M,N) && SubtreesCollideQ(R_M,N) )
            {
                return true;
            }
        }
        else if( !M.interiorQ && N.interiorQ )
        {
            mref<ClisbyNode> R_N = GetNode( 2 * N.id + 2 );
            mref<ClisbyNode> L_N = GetNode( 2 * N.id + 1 );

            PushTransform( N, L_N, R_N );
            
            if( NodesMightOverlapQ(M,L_N) && SubtreesCollideQ(M,L_N) )
            {
                return true;
            }
            
            if( NodesMightOverlapQ(M,R_N) && SubtreesCollideQ(M,R_N) )
            {
                return true;
            }
        }
        else
        {
        }
    }
    
}; // ClisbyNode
