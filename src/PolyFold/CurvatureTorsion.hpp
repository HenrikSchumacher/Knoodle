std::pair<Real,Real> CurvatureTorsion(
    cptr<Real> X, Int i_A, Int i_B, Int i_C, Int i_D
)
{
    // Assuming that A, B, C, D form an equilateral polyline.
    
    Vector_T A ( X, i_A );
    Vector_T B ( X, i_B );
    Vector_T C ( X, i_C );
    Vector_T D ( X, i_D );
    
    Vector_T u = B - A;
    Vector_T v = C - B;
    Vector_T w = D - C;
                    
//    u.Normalize();
    v.Normalize();
    w.Normalize();
    
    const Real curvature = AngleBetweenUnitVectors(v,w);
    
    Real torsion = 0;
    
    const Vector_T nu = Cross(u,v);
    const Vector_T mu = Cross(w,v);
    
    if( (nu.SquaredNorm() > Real(0)) && (mu.SquaredNorm() > Real(0)) )
    {
        torsion = std::atan2( Det(v,nu,mu), Dot(nu,mu) );
    }
    
    return { curvature, torsion };
}


