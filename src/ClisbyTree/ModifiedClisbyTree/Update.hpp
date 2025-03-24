void Update()
{
    UpdateSubtree(Root());
}

void UpdateSubtree( mref<ClisbyNode> N )
{
    if( N.all_movedQ )
    {
        UpdateNode(transform,N);
    }
    else if( N.all_unmovedQ )
    {
        return;
    }
    else
    {
        auto [L,R] = GetChildren(N);
        
        UpdateSubtree(L);
        UpdateSubtree(R);
        MergeBalls(L,R,N);
    }
}

void UpdateNode( cref<Transform_T> f, mref<ClisbyNode> N )
{
    N.center = f(N.center);
    
    // Transformation of a leaf node never needs a change.
    if( !N.interiorQ )
    {
        return;
    }
    
    N.f = f(N.f);
}


void MergeBalls( cref<ClisbyNode> L, cref<ClisbyNode> R, mref<ClisbyNode> N )
{
    const Real d = Distance(L.center,R.center);
    
    if( d + R.radius <= L.radius ) // [[unlikely]]
    {
        N.center = L.center;
        N.radius = L.radius;
        return;
    }
    
    if( d + L.radius <= R.radius ) // [[unlikely]]
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
