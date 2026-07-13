// Included inside class LatticeLink. Conversion to/from LinkEmbedding.
//
//   LinkEmbedding -> LatticeLink  (BuildFromEmbedding): validate integer + axis-aligned,
//        reject zero-length edges, subdivide every edge to unit steps, populate the
//        EdgeList + OccupiedSet (rejecting non-self-avoiding input), init the accumulators.
//
//   LatticeLink -> LinkEmbedding  (ToLinkEmbedding): visited-bitmap walk over the cycles,
//        re-weld maximal collinear unit runs back into single long edges, then build via
//        the component-pointer ctor + ReadVertexCoordinates.

private:

// Read vertex `row` from the AoS coordinate buffer, snapping to the nearest integer and
// verifying it really is (near-)integral. Returns false on a non-integer coordinate.
bool ReadLatticeVertex(
    cptr<Real> coord, const Int row, LCoord & X, LCoord & Y, LCoord & Z
) const
{
    const Real rx = coord[3*row + 0];
    const Real ry = coord[3*row + 1];
    const Real rz = coord[3*row + 2];

    const long long ix = std::llround(rx);
    const long long iy = std::llround(ry);
    const long long iz = std::llround(rz);

    if( std::fabs(rx - static_cast<Real>(ix)) > coord_tol ) { return false; }
    if( std::fabs(ry - static_cast<Real>(iy)) > coord_tol ) { return false; }
    if( std::fabs(rz - static_cast<Real>(iz)) > coord_tol ) { return false; }

    X = static_cast<LCoord>(ix);
    Y = static_cast<LCoord>(iy);
    Z = static_cast<LCoord>(iz);
    return true;
}

void BuildFromEmbedding( cref<LinkEmbedding_T> emb )
{
    Clear();

    const Int V = emb.VertexCount();
    if( V <= Int(0) )
    {
        eprint(ClassName() + ": empty LinkEmbedding.");
        return;
    }

    Tensor1<Real,Int> coord( Int(3) * V );
    emb.WriteVertexCoordinates( coord.data() );

    cref<Tensor1<Int,Int>> comp_ptr = emb.ComponentPointers();
    cref<Tensor1<Int,Int>> comp_col = emb.ComponentColors();
    const Int ncomp = emb.ComponentCount();

    edges_.reserve( static_cast<std::size_t>(V) );
    color_.reserve( static_cast<std::size_t>(V) );

    for( Int c = 0; c < ncomp; ++c )
    {
        const Int vb     = comp_ptr[c         ];
        const Int ve     = comp_ptr[c + Int(1)];
        const Int vcount = ve - vb;

        if( vcount < Int(3) )
        {
            eprint(ClassName() + ": component " + ToString(c) + " has < 3 vertices.");
            Clear();
            return;
        }

        const Int color = ( c < comp_col.Size() ) ? comp_col[c] : c;

        const EIdx comp_first = static_cast<EIdx>(edges_.size());

        // Walk the component's vertices cyclically; each consecutive pair is one
        // axis-aligned edge, which we subdivide into unit edges.
        for( Int k = 0; k < vcount; ++k )
        {
            const Int i0 = vb + k;
            const Int i1 = vb + ((k + Int(1)) % vcount);

            LCoord ax, ay, az, bx, by, bz;
            if( !ReadLatticeVertex(coord.data(), i0, ax, ay, az)
             || !ReadLatticeVertex(coord.data(), i1, bx, by, bz) )
            {
                eprint(ClassName() + ": non-integer vertex coordinate.");
                Clear();
                return;
            }

            int    ndiff = 0;
            int    axis  = -1;
            LCoord delta = 0;
            if( ax != bx ) { ++ndiff; axis = 0; delta = bx - ax; }
            if( ay != by ) { ++ndiff; axis = 1; delta = by - ay; }
            if( az != bz ) { ++ndiff; axis = 2; delta = bz - az; }

            if( ndiff == 0 )
            {
                eprint(ClassName() + ": zero-length edge (coincident vertices).");
                Clear();
                return;
            }
            if( ndiff > 1 )
            {
                eprint(ClassName() + ": non-axis-aligned edge.");
                Clear();
                return;
            }

            const std::uint8_t dir    = static_cast<std::uint8_t>(2*axis + (delta > 0 ? 0 : 1));
            const LCoord       n_unit  = (delta > 0) ? delta : -delta;

            LCoord cx = ax, cy = ay, cz = az;
            for( LCoord s = 0; s < n_unit; ++s )
            {
                edges_.push_back( Edge{ cx, cy, cz, dir, EIdx(0), EIdx(0) } );
                color_.push_back( color );
                cx += dir_dx[dir];
                cy += dir_dy[dir];
                cz += dir_dz[dir];
            }
        }

        const EIdx comp_last = static_cast<EIdx>(edges_.size()) - EIdx(1);

        // Link the component's edges into one directed cycle.
        for( EIdx idx = comp_first; idx <= comp_last; ++idx )
        {
            edges_[idx].next = (idx == comp_last ) ? comp_first : (idx + EIdx(1));
            edges_[idx].prev = (idx == comp_first) ? comp_last  : (idx - EIdx(1));
        }
    }

    // Populate OccupiedSet (both vertices and midpoints) and the accumulators. A duplicate
    // insertion means the input is not self-avoiding, which we reject.
    for( const Edge & e : edges_ )
    {
        if( !occupied_.insert( VertexKey(e.tx, e.ty, e.tz) ).second )
        {
            eprint(ClassName() + ": input polygon is not self-avoiding (vertex collision).");
            Clear();
            return;
        }
        if( !occupied_.insert( MidpointKey(e) ).second )
        {
            eprint(ClassName() + ": input polygon is not self-avoiding (edge collision).");
            Clear();
            return;
        }
        AccAddVertex(e.tx, e.ty, e.tz);
    }

    validQ_ = true;
}

public:

// Rebuild a LinkEmbedding, re-welding maximal collinear unit runs into single edges so
// downstream code never sees the subdivided vertices. Coordinates are exact (shiftQ=false).
LinkEmbedding_T ToLinkEmbedding() const
{
    if( !validQ_ || edges_.empty() ) { return LinkEmbedding_T(); }

    const EIdx m = static_cast<EIdx>(edges_.size());
    std::vector<char> visited( static_cast<std::size_t>(m), 0 );

    std::vector<std::vector<IVec3>> comps;    // welded corner vertices, per component
    std::vector<Int>                comp_colors;

    for( EIdx start = 0; start < m; ++start )
    {
        if( visited[start] ) { continue; }

        std::vector<IVec3> corners;
        EIdx e = start;
        do
        {
            visited[e] = 1;
            const Edge & cur = edges_[e];
            const Edge & prv = edges_[cur.prev];

            // A corner is a direction change at cur's tail: the welded polygon keeps only
            // these vertices; collinear pass-through vertices are dropped.
            if( cur.dir != prv.dir )
            {
                corners.push_back( IVec3{ cur.tx, cur.ty, cur.tz } );
            }
            e = edges_[e].next;
        }
        while( e != start );

        if( corners.size() < 3 )
        {
            eprint(ClassName() + "::ToLinkEmbedding: degenerate component (< 3 corners).");
            return LinkEmbedding_T();
        }

        comp_colors.push_back( color_[start] );
        comps.push_back( std::move(corners) );
    }

    const Int ncomp = static_cast<Int>(comps.size());

    Tensor1<Int,Int> out_ptr( ncomp + Int(1) );
    Tensor1<Int,Int> out_col( ncomp );

    Int total = 0;
    out_ptr[0] = 0;
    for( Int c = 0; c < ncomp; ++c )
    {
        total     += static_cast<Int>(comps[c].size());
        out_ptr[c + Int(1)] = total;
        out_col[c] = comp_colors[c];
    }

    Tensor1<Real,Int> out_coord( Int(3) * total );
    Int row = 0;
    for( Int c = 0; c < ncomp; ++c )
    {
        for( const IVec3 & v : comps[c] )
        {
            out_coord[3*row + 0] = static_cast<Real>(v.x);
            out_coord[3*row + 1] = static_cast<Real>(v.y);
            out_coord[3*row + 2] = static_cast<Real>(v.z);
            ++row;
        }
    }

    LinkEmbedding_T emb( std::move(out_ptr), std::move(out_col) );
    emb.template ReadVertexCoordinates<false,false>( out_coord.data() );
    return emb;
}
