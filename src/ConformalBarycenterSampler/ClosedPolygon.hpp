// Edge vectors:

/*!@brief Returns the coordinate of the `i`-th unit edge vector of the _closed_ polygon.
 */

Vector_T EdgeVector( const Int i ) const
{
    return Vector_T (y_,i);
}

/*!@brief Writes the unit edge vectors of the closed polygon to buffer `y`.
 *
 * Suppose that `n = this->EdgeCount()` is the number of edges and `d = this->AmbientDimension()` is the dimension of the ambient space.
 *
 * @param y Target array; assumed to be of size of at least `n * d`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th edge vector of the polygon is stored in `y[d * i + j].
 */

void WriteEdgeVectors( Real * restrict const y ) const
{
    y_.Write(y);
}



// Vertex positions


/*!@brief Returns the position of `k`-th vertex of the closed polygon.
 */

Vector_T VertexPosition( const Int k ) const
{
    return Vector_T(p_,k);
}

/*!@brief Writes the vertex positions of the closed polygon to buffer `q`.
 *
 * @param q Target array; assumed to be of size of at least `(n + 1) * d `, where `n = this->EdgeCount()` is the number of edges and `d = AmbientDimension()` is the dimension of the ambient space. The `j`-th coordinate of the `i`-th vertex of the polygon is stored in `q[d * i + j]`.
 */

void WriteVertexPositions( Real * restrict const q) const
{
    p_.Write(q);
}


private:


/*!@brief Makes sure that the vertex coordinates of the closed polygon are computed.
*/

void ComputeVertexPositions()
{
    //Caution: This gives only half the weight to the end vertices of the chain.
    //Thus this is only really the barycenter, if the chain is closed!
    
    // We treat the edges as massless.
    // All mass is concentrated in the vertices, and each vertex carries the same mass.
    Vector_T barycenter        (zero);
    Vector_T point_accumulator (zero);
    
    for( Int i = 0; i < edge_count_; ++i )
    {
        const Vector_T y_i ( y_, i );
        
        const Real r_i = r_[i];
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            const Real delta = r_i * y_i[j];
            
            barycenter[j] += (point_accumulator[j] + half * delta);
            
            point_accumulator[j] += delta;
        }
    }
    
    barycenter *= Inv<Real>( edge_count_ );
    
    point_accumulator = barycenter;
    
    point_accumulator *= -one;
    
    point_accumulator.Write( p_, Int(0) );
    
    for( Int i = 0; i < edge_count_; ++i )
    {
        const Vector_T y_i ( y_, i );
        
        const Real r_i = r_[i];
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            point_accumulator[j] += r_i * y_i[j];
        }
        
        point_accumulator.Write( p_, i + Int(1) );
    }
}
