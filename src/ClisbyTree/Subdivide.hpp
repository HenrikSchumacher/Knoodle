public:

ClisbyTree Subdivide()
{
    const Int n = VertexCount();
    
    if( n <= Int(3) )
    {
        return *this;
    }
    
    using V_T = Tiny::Vector<AmbDim,Real,Int>;
    
    ClisbyTree T ( Int(2) * n, HardSphereDiameter() );
    
    auto X = this->VertexCoordinates();
    
    Tensor2<Real,Int> X_new ( Int(2) * n, AmbDim );
    
    const Int last = n - Int(1);
    
    V_T x;
    V_T y;
    V_T z;
    {
        x.Read( X, last   );
        y.Read( X, Int(0) );
        z = x + y;
        x *= Real(2);
        x.Write( X_new, Int(2) * last          );
        z.Write( X_new, Int(2) * last + Int(1) );
    }
    
    for( Int i = 0; i < last; ++i )
    {
        x.Read( X, i          );
        y.Read( X, i + Int(1) );
        x *= Real(2);
        x.Write( X_new, Int(2) * i          );
        z.Write( X_new, Int(2) * i + Int(1) );
    }
    
    T->ReadVertexCoordinates( X_new.data() );
    
    return T;
}
