Real SquaredGyradius( cptr<Real> X, cref<Vector_T> mu )
{
    Real r2 = 0;
    
    for( Int i = 0; i < n; ++i )
    {
        r2 += SquaredDistance(Vector_T( &X[AmbDim * i] ), mu);
    }
    
//    dump(r2);
    
    r2 *= Frac<Real>(1,n);
    
    return r2;
}

Real SquaredGyradius( cptr<Real> X )
{
    return SquaredGyradius( X, Barycenter(X) );
}
