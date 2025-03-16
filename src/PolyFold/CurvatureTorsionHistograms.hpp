std::pair<Tensor1<LInt,Int>,Tensor1<LInt,Int>> CurvatureTorsionHistograms(
    cptr<Real> X
)
{
    if( (bin_count <= Int(0)) || n < 4 )
    {
        return { Tensor1<LInt,Int>(0), Tensor1<LInt,Int>(0) };
    }
    
    Tensor1<LInt,Int> curvature_hist ( bin_count, 0 );
    Tensor1<LInt,Int> torsion_hist ( Int(2) * bin_count, 0 );
    
    const Real scale = Frac<Real>(bin_count,Scalar::Pi<Real>);

    auto update = [&curvature_hist,&torsion_hist,X,scale,this](
        Int i_A, Int i_B, Int i_C, Int i_D
    )
    {
        auto [curvature,torsion] = CurvatureTorsion(X, i_A, i_B, i_C, i_D );
        
        const Int curvature_bin = Clip(
            static_cast<Int>( curvature * scale ),
            Int(0), bin_count - Int(1)
        );
        
        ++curvature_hist[curvature_bin];
        
        const Int torsion_bin = Clip(
            bin_count + static_cast<Int>( torsion * scale ),
            Int(0), Int(2) * bin_count - Int(1)
        );
        
        ++torsion_hist[torsion_bin];
    };
    
    update( n - 3, n - 2, n - 1, 0 );   // i = 0;
    update( n - 2, n - 1, 0    , 1 );   // i = 1;
    update( n - 1, 0,     1    , 2 );   // i = 2;
    
    for( Int i = 3; i < n; ++i )
    {
        update( i - 3, i - 2, i - 1, i - 0 );
    }
    
    return std::pair( curvature_hist, torsion_hist );
}

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
    
    const Real norm_nu = nu.Norm();
    const Real norm_mu = mu.Norm();
    
    if( (norm_nu > Real(0)) && (norm_mu > Real(0)) )
    {
        torsion = std::asin( Det(v,nu,mu) / (norm_nu * norm_mu )  );
    }
    
    return { curvature, torsion };
}


