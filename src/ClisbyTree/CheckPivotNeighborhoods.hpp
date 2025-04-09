// Apparently, this does not make anything faster. 

template<Int k>
int CheckPivotNeighborhoods()
{
    constexpr Int K = Int(2) * k + Int(1);
    
    const Int n = VertexCount();
    
    if( (p < k) || (p + k + Int(1) > n) )
    {
        wprint("Edge case: p too close to boundary.");
        return 0;
    }
    
    if( (q < k) || (q + k + Int(1) > n) )
    {
        wprint("Edge case: q too close to boundary.");
        return 0;
    }
    
    
    if( ModDistance(n, p, q) < Int(2) * k )
    {
        wprint("Edge case: p and q too close.");
        return 0;
    }
    
    Vector_T X [K];
    Vector_T Y [K];

    const Int i_begin = p - k;
    const Int i_mid   = p + mid_changedQ;
    const Int i_end   = p + k + 1;
    
//    dump(i_begin);
//    dump(i_mid);
//    dump(i_end);
    
    if( mid_changedQ )
    {
        for( Int i = i_begin; i < i_mid; ++i )
        {
            X[i-i_begin] = VertexCoordinates(i);
        }
        for( Int i = i_mid  ; i < i_end; ++i )
        {
            X[i-i_begin] = transform(VertexCoordinates(i));
        }
    }
    else
    {
        for( Int i = i_begin; i < i_mid; ++i )
        {
            X[i-i_begin] = transform(VertexCoordinates(i));
        }
        for( Int i = i_mid  ; i < i_end; ++i )
        {
            X[i-i_begin] = VertexCoordinates(i);
        }
    }
    
    
    for( Int i = 0; i < K; ++i )
    {
        for( Int j = i + 2; j < K; ++j )
        {
            bool collision_foundQ = (SquaredDistance(X[i],X[j]) < hard_sphere_squared_diam);
            
            if( collision_foundQ )
            {
                return 1;
            }
        }
    }
    
    const Int j_begin = q - k;
    const Int j_mid   = q + !mid_changedQ;
    const Int j_end   = q + k + 1;
    
//    dump(j_begin);
//    dump(j_mid);
//    dump(j_end);
    
    if( mid_changedQ )
    {
        for( Int j = j_begin; j < j_mid; ++j )
        {
            Y[j-j_begin] = transform(VertexCoordinates(j));
        }
        for( Int j = j_mid  ; j < j_end; ++j )
        {
            Y[j-j_begin] = VertexCoordinates(j);
        }
    }
    else
    {
        for( Int j = j_begin; j < j_mid; ++j )
        {
            Y[j-j_begin] = VertexCoordinates(j);
        }
        for( Int j = j_mid  ; j < j_end; ++j )
        {
            Y[j-j_begin] = transform(VertexCoordinates(j));
        }
    }
    
    for( Int i = 0; i < K; ++i )
    {
        for( Int j = i + 2; j < K; ++j )
        {
            bool collision_foundQ = (SquaredDistance(Y[i],Y[j]) < hard_sphere_squared_diam);
            
            if( collision_foundQ )
            {
                return 2;
            }
        }
    }
    
    for( Int i = 0; i < K; ++i )
    {
        for( Int j = 0; j < K; ++j )
        {
            bool collision_foundQ = (SquaredDistance(X[i],Y[j]) < hard_sphere_squared_diam);
            
            if( collision_foundQ )
            {
                return 3;
            }
        }
    }
    
    return 0;
}
