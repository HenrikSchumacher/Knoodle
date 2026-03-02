public:

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

/*!@brief Writes the current vertex coordinates to buffer `q`.
 *
 * Suppose that `n = this->EdgeCount()` is the number of edges and `d = this->AmbientDimension()` is the dimension of the ambient space.
 *
 * @param q Output buffer; size must be at least `(n + wrap_aroundQ) * d`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th vertex is stored in `q[d * i + j]`.
 *
 * @param wrap_aroundQ If set to yes, then the output polygon `q` will have `EdgeCount()` + 1 vertex positions. (If the polygon is already closed then the first one is reapeated.) The difference between first and last vertex positions indicates the numerical error.
 *
 * @param mode Specify whether the output polygon `q` is to be centered to its center of mass and in which sense "mass" is operationalized.
 */
void WriteVertexCoordinates( Real * restrict const q, CentralizationMode_T mode, const bool wrap_aroundQ )
{
    writeCoordinates(y_,q,mode,wrap_aroundQ);
}
