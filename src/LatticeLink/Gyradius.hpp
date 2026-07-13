// Included inside class LatticeLink. Radius-of-gyration accumulators.
//
//   R_g^2 = (1/N) * sum_i |x_i - xbar|^2  =  S2/N - |S1/N|^2
//         = ( N*S2 - |S1|^2 ) / N^2
//
// where S1 = sum_i x_i (a vec3), S2 = sum_i |x_i|^2, N = #vertices. Every BFACF move
// touches O(1) vertices, so these update in O(1). We keep S1,S2 as exact Int64 (lattice
// coords are integers) and only convert to floating point when reporting R_g^2.

private:

// Fold one lattice vertex into the accumulators.
void AccAddVertex( const LCoord x, const LCoord y, const LCoord z )
{
    S1x += x;
    S1y += y;
    S1z += z;
    S2  += std::int64_t(x)*std::int64_t(x)
         + std::int64_t(y)*std::int64_t(y)
         + std::int64_t(z)*std::int64_t(z);
    ++Nv;
}

// Remove one lattice vertex from the accumulators (for death / moved vertices).
void AccRemoveVertex( const LCoord x, const LCoord y, const LCoord z )
{
    S1x -= x;
    S1y -= y;
    S1z -= z;
    S2  -= std::int64_t(x)*std::int64_t(x)
         + std::int64_t(y)*std::int64_t(y)
         + std::int64_t(z)*std::int64_t(z);
    --Nv;
}

// Change in R_g^2 that WOULD result from adding the given (dS1,dS2,dN) to the accumulators,
// without mutating them. O(1); used to score a proposed move before it is applied.
Real GyradiusDelta( const std::int64_t dS1x, const std::int64_t dS1y, const std::int64_t dS1z,
                    const std::int64_t dS2,  const std::int64_t dNv ) const
{
    const long double n1 = static_cast<long double>(Nv + dNv);
    if( n1 <= 0 ) { return -SquaredGyradius(); }

    auto rg2 = []( long double sx, long double sy, long double sz, long double s2, long double n )
    {
        return s2 / n - (sx*sx + sy*sy + sz*sz) / (n*n);
    };

    const long double before = (Nv <= 0) ? 0.0L
        : rg2( S1x, S1y, S1z, S2, static_cast<long double>(Nv) );
    const long double after = rg2( S1x + dS1x, S1y + dS1y, S1z + dS1z, S2 + dS2, n1 );

    return static_cast<Real>( after - before );
}

public:

// Squared radius of gyration from the running accumulators (O(1)). Uses long double
// intermediates so the N*S2 / |S1|^2 combination does not lose precision for large N.
Real SquaredGyradius() const
{
    if( Nv <= 0 ) { return Real(0); }

    const long double n  = static_cast<long double>(Nv);
    const long double s2 = static_cast<long double>(S2);
    const long double sx = static_cast<long double>(S1x);
    const long double sy = static_cast<long double>(S1y);
    const long double sz = static_cast<long double>(S1z);

    const long double rg2 = s2 / n - (sx*sx + sy*sy + sz*sz) / (n*n);

    return static_cast<Real>(rg2);
}

// From-scratch recomputation by walking the EdgeList. O(N). Used only by the debug
// cross-check; the hot path never calls this.
Real RecomputeSquaredGyradius() const
{
    std::int64_t s1x = 0, s1y = 0, s1z = 0, s2 = 0, n = 0;

    for( const Edge & e : edges_ )
    {
        s1x += e.tx;
        s1y += e.ty;
        s1z += e.tz;
        s2  += std::int64_t(e.tx)*std::int64_t(e.tx)
             + std::int64_t(e.ty)*std::int64_t(e.ty)
             + std::int64_t(e.tz)*std::int64_t(e.tz);
        ++n;
    }

    if( n <= 0 ) { return Real(0); }

    const long double N  = static_cast<long double>(n);
    const long double rg2 = static_cast<long double>(s2) / N
        - ( static_cast<long double>(s1x)*static_cast<long double>(s1x)
          + static_cast<long double>(s1y)*static_cast<long double>(s1y)
          + static_cast<long double>(s1z)*static_cast<long double>(s1z) ) / (N*N);

    return static_cast<Real>(rg2);
}

// True iff the incremental accumulators agree with a from-scratch walk and N matches the
// edge count. Debug invariant; cheap enough to assert in tests.
bool AccumulatorsConsistentQ() const
{
    if( Nv != static_cast<std::int64_t>(edges_.size()) ) { return false; }

    const Real a = SquaredGyradius();
    const Real b = RecomputeSquaredGyradius();
    const Real d = (a > b) ? (a - b) : (b - a);

    return d <= static_cast<Real>(1e-6) * (Real(1) + (a > b ? a : b));
}
