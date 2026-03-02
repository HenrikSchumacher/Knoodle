public:

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
void ReadInitialVertexCoordinates( const Real * restrict const p )
{
    TOOLS_MAKE_FP_FAST();
    
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
 * @param p Target array; must have size at least `(n + 1) * d`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th vertex is stored in `p[d * i + j]`.
 
 * @param mode Specify whether the output polygon `p` is to be centered to its center of mass and in which sense "mass" is operationalized.
 */
void WriteInitialVertexCoordinates( Real * restrict const p, CentralizationMode_T mode )
{
    writeCoordinates(x_,p,mode,true);
}
