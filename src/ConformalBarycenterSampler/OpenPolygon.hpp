// Edge vectors:

Vector_T InitialEdgeVector( const Int i ) const
{
    return Vector_T (x_,i);
}

/*!@brief Read the edge vectors of the open polygon from buffer `x`.
 *
 * Suppose that `n = this->EdgeCount()` is the number of edges and `d = this->AmbientDimension()` is the dimension of the ambient space.
 *
 * @param x Target array; assumed to be of size of at least `n * d`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th edge vector of the polygon is stored in `x[d * i + j].
 *
 * @param normalizeQ Only if `normalizeQ` is set to true, the vectors get normalized. Otherwise, they are assumed to be already normalized.
 */

void ReadInitialEdgeVectors(
    const Real * restrict const x, bool normalizeQ = true
)
{
    if( normalizeQ )
    {
        for( Int i = 0; i < edge_count_; ++i )
        {
            Vector_T x_i (x,i);
            
            x_i.Normalize();
            
            x_i.Write(x_,i);
        }
    }
    else
    {
        x_.Read(x);
    }
}


void RandomizeInitialEdgeVectors()
{
    for( Int i = 0; i < edge_count_; ++i )
    {
        Vector_T x_i;

        for( Int j = 0; j < AmbDim; ++j )
        {
            x_i[j] = normal_dist( random_engine );
        }
        
        x_i.Normalize();
        
        x_i.Write(x_,i);
    }
}

/*!@brief Writes the unit edge vectors of the open polygon to buffer `x`.
 *
 * Suppose that `n = this->EdgeCount()` is the number of edges and `d = this->AmbientDimension()` is the dimension of the ambient space.
 *
 * @param x Target array; assumed to be of size of at least `n * d`. The coordinates are stored in interleaved form`, i.e., the `j`-th coordinate of the `i`-th unit edge vector of polygon number `offset` is stored in `x[d * i + j].
 */

void WriteInitialEdgeVectors( Real * restrict const x )
{
    x_.Write(x);
}

/*!@brief Reads the vertex positions of the open polygon from buffer `p`.
 *
 * Suppose that `n = this->EdgeCount()` is the number of edges and `d = this->AmbientDimension()` is the dimension of the ambient space.
 *
 * @param p Source array; assumed to be of size of at least `(n + 1) * d`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th edge of polygon number `offset` is stored in `p[d * i + j]`.
 */


// Vertex positions
void ReadInitialVertexPositions( const Real * restrict const p )
{
    for( Int i = 0; i < edge_count_; ++i )
    {
        cptr<Real> u = &p[AmbDim * (i + 0) ];
        cptr<Real> v = &p[AmbDim * (i + 1) ];
        
        Vector_T x_i;
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            x_i[j] =  v[j] - u[j];
        }

        x_i.Normalize();
        
        x_i.Write( x_, i );
    }
}

/*!@brief Writes the vertex positions of the open polygon to buffer `p`.
 *
 * Suppose that `n = this->EdgeCount()` is the number of edges and `d = this->AmbientDimension()` is the dimension of the ambient space.
 *
 * @param p Target array; assumed to be of size of at least `(n + 1) * d * (offset + 1)`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th edge of polygon number `offset` is stored in `p[d * i + j]`.
 */

void WriteInitialVertexPositions( Real * restrict const p )
{
    // We treat the edges as massless.
    // All mass is concentrated in the vertices, and each vertex carries the same mass.
    Vector_T barycenter        (zero);
    Vector_T point_accumulator (zero);
    
    for( Int i = 0; i < edge_count_; ++i )
    {
        const Vector_T x_i ( x_, i );
        
        const Real r_i = r_[i];
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            const Real delta = r_i * x_i[j];
            
            barycenter[j] += (point_accumulator[j] + delta);
            
            point_accumulator[j] += delta;
        }
    }
    
    barycenter *= Inv<Real>( edge_count_ + Int(1) );
    
    point_accumulator = barycenter;
    
    point_accumulator *= -one;
    

    point_accumulator.Write(p);
    
    for( Int i = 0; i < edge_count_; ++i )
    {
        const Vector_T x_i ( x_, i );
        
        const Real r_i = r_[i];
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            point_accumulator[j] += r_i * x_i[j];
        }
        
        point_accumulator.Write( &p[AmbDim * (i+1)] );
    }
}
