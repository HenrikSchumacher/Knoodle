/*!@brief This is a very thorough, but also very slow collision checker. Only meant for debugging purposes!
 */

std::tuple<bool,WitnessVector_T> CollisionQ_Debug() const
{
    WitnessVector_T w {-1,-1};
    
    const Int n = VertexCount();
    
    Tensor2<Real,Int> x( n, AmbDim );
    
    for( Int i = 0; i < n; ++i )
    {
        VertexCoordinates(i).Write( x.data(i) );
    }
    
    for( Int i = 0; i < n; ++i )
    {
        Vector_T x_i ( x.data(i) );
        
        const Int j_begin = i + Int(2);
        const Int j_end   = (i == Int(0) ? n - Int(1) : n);
        
        for( Int j = j_begin; j < j_end; ++j )
        {
            Vector_T x_j ( x.data(j) );
            
            
            
            if(
                (ChordDistance(i,j) > gap)
                &&
                (SquaredDistance(x_i,x_j) < hard_sphere_squared_diam)
            )
            {
                w[0] = i;
                w[1] = j;
                return {true,w};
            }
        }
    }
    
    return {false,w};
}
