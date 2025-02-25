Tiny::Vector<AmbDim,Real,Int> Barycenter( cptr<Real> X )
{
    Vector_T mu;
    
    mu.SetZero();
    
    for( Int i = 0; i < n; ++i )
    {
        mu += Vector_T( &X[AmbDim * i] );
    }
    
    mu *= Frac<Real>(1,n);
    
//    dump(mu);
    
    return mu;
}
