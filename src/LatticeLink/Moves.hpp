// Included inside class LatticeLink. BFACF elementary moves.
//
// One move acts on a uniformly chosen edge e=(p->q) (direction u) and a uniformly chosen
// unit direction g perpendicular to u (4 choices in 3D). With p'=p+g, q'=q+g, o = tail of
// the previous edge, r = head of the next edge, define
//
//     leftB  = (o == p')      rightD = (r == q')
//
//     !leftB && !rightD  -> BIRTH  (+2): admissible iff p' and q' are free vertices
//      leftB &&  rightD  -> DEATH  (-2): admissible iff midpoint(p'->q') is free
//      exactly one true  -> FLIP    (0): admissible iff the one new vertex is free
//
// Self-avoidance is enforced here on every move (all admissibility tests are OccupiedSet
// lookups), so any admissible move preserves the knot/link type. Each edge owns exactly two
// OccupiedSet keys -- its tail vertex and its midpoint -- which makes every update a local
// "erase old edges' keys, mutate, insert new edges' keys", with the accumulator = sum of
// tails updated in lockstep.

public:

enum class MoveType : std::int8_t { Rejected = 0, Birth = 1, Death = 2, Flip = 3 };

private:

enum class ApplyKind : std::int8_t { Birth, Death, FlipLeft, FlipRight };

// A proposed, already-admissibility-checked BFACF move, not yet applied. Carries everything
// an objective needs to score it (see Objectives.hpp).
struct Proposal
{
    bool         admissibleQ    = false;
    ApplyKind    kind           = ApplyKind::FlipLeft;
    EIdx         ei             = 0;
    std::uint8_t g              = 0;
    Int          dN             = 0;   // +2 birth, -2 death, 0 flip
    Real         dRg2           = 0;   // exact change in R_g^2 if applied
    Real         proposal_ratio = 1;   // BFACF kinematic ratio (detailed balance)
    EIdx         N              = 0;   // pre-move edge count
};

static MoveType PublicType( const ApplyKind k )
{
    switch( k )
    {
        case ApplyKind::Birth: return MoveType::Birth;
        case ApplyKind::Death: return MoveType::Death;
        default:               return MoveType::Flip;
    }
}

// Propose a uniform edge + perpendicular direction, classify the move, and test
// admissibility (self-avoidance + soft edge cap). Does NOT mutate the polygon. On an
// admissible proposal, fills dN, dRg2 and the BFACF proposal ratio.
template<class RNG>
Proposal Propose( RNG & rng, const std::size_t max_edges )
{
    Proposal pr;
    const EIdx N = static_cast<EIdx>(edges_.size());
    pr.N = N;
    if( N < EIdx(4) ) { return pr; }

    const EIdx  ei = static_cast<EIdx>( static_cast<std::uint64_t>(rng()) % static_cast<std::uint64_t>(N) );
    const Edge  E  = edges_[ei];
    const int   a  = E.dir >> 1;
    const std::uint8_t g = perp_table[a][ rng() % 4u ];
    pr.ei = ei;
    pr.g  = g;

    const LCoord px = E.tx,           py = E.ty,           pz = E.tz;            // p
    const LCoord qx = px+dir_dx[E.dir], qy = py+dir_dy[E.dir], qz = pz+dir_dz[E.dir]; // q
    const LCoord ppx = px+dir_dx[g], ppy = py+dir_dy[g], ppz = pz+dir_dz[g];     // p'
    const LCoord qpx = qx+dir_dx[g], qpy = qy+dir_dy[g], qpz = qz+dir_dz[g];     // q'

    const Edge & EP = edges_[E.prev];
    const Edge & EN = edges_[E.next];
    const LCoord ox = EP.tx, oy = EP.ty, oz = EP.tz;                             // o
    const LCoord rx = EN.tx+dir_dx[EN.dir], ry = EN.ty+dir_dy[EN.dir], rz = EN.tz+dir_dz[EN.dir]; // r

    const bool leftB  = (ox==ppx && oy==ppy && oz==ppz);
    const bool rightD = (rx==qpx && ry==qpy && rz==qpz);

    if( !leftB && !rightD )                                    // BIRTH
    {
        if( edges_.size() >= max_edges ) { return pr; }
        if( occupied_.contains( VertexKey(ppx,ppy,ppz) ) ) { return pr; }
        if( occupied_.contains( VertexKey(qpx,qpy,qpz) ) ) { return pr; }
        pr.kind = ApplyKind::Birth;
        pr.dN   = Int(2);
        pr.proposal_ratio = static_cast<Real>(N) / static_cast<Real>(N + EIdx(2));
        pr.dRg2 = GyradiusDeltaAddAdd( ppx,ppy,ppz, qpx,qpy,qpz );   // +p', +q'
        pr.admissibleQ = true;
    }
    else if( leftB && rightD )                                 // DEATH
    {
        if( N < EIdx(6) ) { return pr; }
        if( occupied_.contains( MidpointKeyOf(ppx,ppy,ppz,E.dir) ) ) { return pr; }
        pr.kind = ApplyKind::Death;
        pr.dN   = Int(-2);
        pr.proposal_ratio = static_cast<Real>(N) / static_cast<Real>(N - EIdx(2));
        pr.dRg2 = GyradiusDeltaRemRem( px,py,pz, qx,qy,qz );
        pr.admissibleQ = true;
    }
    else if( !leftB && rightD )                                // FLIP (left)
    {
        if( occupied_.contains( VertexKey(ppx,ppy,ppz) ) ) { return pr; }
        pr.kind = ApplyKind::FlipLeft;
        pr.dN   = Int(0);
        pr.proposal_ratio = Real(1);
        pr.dRg2 = GyradiusDeltaAddRem( ppx,ppy,ppz, qx,qy,qz );   // +p', -q
        pr.admissibleQ = true;
    }
    else                                                       // FLIP (right)
    {
        if( occupied_.contains( VertexKey(qpx,qpy,qpz) ) ) { return pr; }
        pr.kind = ApplyKind::FlipRight;
        pr.dN   = Int(0);
        pr.proposal_ratio = Real(1);
        pr.dRg2 = GyradiusDeltaAddRem( qpx,qpy,qpz, px,py,pz );   // +q', -p
        pr.admissibleQ = true;
    }
    return pr;
}

void ApplyProposal( const Proposal & pr )
{
    switch( pr.kind )
    {
        case ApplyKind::Birth:     ApplyBirth    ( pr.ei, pr.g ); break;
        case ApplyKind::Death:     ApplyDeath    ( pr.ei, pr.g ); break;
        case ApplyKind::FlipLeft:  ApplyFlipLeft ( pr.ei, pr.g ); break;
        case ApplyKind::FlipRight: ApplyFlipRight( pr.ei, pr.g ); break;
    }
}

// dRg2 helpers for the specific add/remove vertex sets of each move.
static std::int64_t Sq( const LCoord v ) { return std::int64_t(v)*std::int64_t(v); }

Real GyradiusDeltaAddAdd( LCoord ax,LCoord ay,LCoord az, LCoord bx,LCoord by,LCoord bz ) const
{
    return GyradiusDelta( std::int64_t(ax)+bx, std::int64_t(ay)+by, std::int64_t(az)+bz,
                          Sq(ax)+Sq(bx)+Sq(ay)+Sq(by)+Sq(az)+Sq(bz), std::int64_t(2) );
}
Real GyradiusDeltaRemRem( LCoord ax,LCoord ay,LCoord az, LCoord bx,LCoord by,LCoord bz ) const
{
    return GyradiusDelta( -(std::int64_t(ax)+bx), -(std::int64_t(ay)+by), -(std::int64_t(az)+bz),
                          -(Sq(ax)+Sq(bx)+Sq(ay)+Sq(by)+Sq(az)+Sq(bz)), std::int64_t(-2) );
}
Real GyradiusDeltaAddRem( LCoord ax,LCoord ay,LCoord az, LCoord bx,LCoord by,LCoord bz ) const
{
    return GyradiusDelta( std::int64_t(ax)-bx, std::int64_t(ay)-by, std::int64_t(az)-bz,
                          Sq(ax)-Sq(bx)+Sq(ay)-Sq(by)+Sq(az)-Sq(bz), std::int64_t(0) );
}

public:

// Attempt one BFACF move, accepting every *admissible* move (the neutral policy used to
// validate knot-type invariance; objective-driven acceptance is Step(), see Objectives.hpp).
// `max_edges` is a soft cap: births are refused at or above it so runs stay bounded.
template<class RNG>
MoveType AttemptMove( RNG & rng, const std::size_t max_edges = ~std::size_t(0) )
{
    const Proposal pr = Propose( rng, max_edges );
    if( !pr.admissibleQ ) { return MoveType::Rejected; }
    ApplyProposal( pr );
    return PublicType( pr.kind );
}

private:

// Perpendicular-direction table: perp_table[axis][0..3] are the 4 unit dirs perpendicular
// to that axis.
static constexpr std::uint8_t perp_table[3][4] = {
    { 2, 3, 4, 5 },   // perpendicular to x
    { 0, 1, 4, 5 },   // perpendicular to y
    { 0, 1, 2, 3 },   // perpendicular to z
};

// dir index of the opposite unit direction.
static constexpr std::uint8_t OppDir[6] = { 1, 0, 3, 2, 5, 4 };

static IVec3 MidpointKeyOf( const LCoord tx, const LCoord ty, const LCoord tz, const std::uint8_t dir )
{
    return IVec3{ static_cast<LCoord>(2*tx + dir_dx[dir]),
                  static_cast<LCoord>(2*ty + dir_dy[dir]),
                  static_cast<LCoord>(2*tz + dir_dz[dir]) };
}

void EraseEdgeKeys( const Edge & e )
{
    occupied_.erase( VertexKey(e.tx, e.ty, e.tz) );
    occupied_.erase( MidpointKey(e) );
}

void InsertEdgeKeys( const Edge & e )
{
    occupied_.insert( VertexKey(e.tx, e.ty, e.tz) );
    occupied_.insert( MidpointKey(e) );
}

// Remove EdgeList slot `idx` by swapping the last element into it (keeping the array dense
// so uniform sampling stays valid) and patching the moved edge's neighbours.
void DeleteSlot( const EIdx idx )
{
    const EIdx last = static_cast<EIdx>(edges_.size()) - EIdx(1);
    if( idx != last )
    {
        edges_[idx] = edges_[last];
        color_[idx] = color_[last];
        const Edge & m = edges_[idx];
        edges_[m.prev].next = idx;
        edges_[m.next].prev = idx;
    }
    edges_.pop_back();
    color_.pop_back();
}

// BIRTH: replace e=(p->q) with (p->p'),(p'->q'),(q'->q). Reuse ei for the first, append two.
void ApplyBirth( const EIdx ei, const std::uint8_t g )
{
    const Edge  E     = edges_[ei];
    const std::uint8_t u = E.dir;
    const EIdx  prevE = E.prev;
    const EIdx  nextE = E.next;
    const Int   col   = color_[ei];

    const LCoord px = E.tx, py = E.ty, pz = E.tz;
    const LCoord ppx = px+dir_dx[g], ppy = py+dir_dy[g], ppz = pz+dir_dz[g];
    const LCoord qpx = ppx+dir_dx[u], qpy = ppy+dir_dy[u], qpz = ppz+dir_dz[u];

    EraseEdgeKeys( E );
    AccRemoveVertex( E.tx, E.ty, E.tz );

    // ei -> c1 = (p -> p'), dir g (tail unchanged).
    edges_[ei].dir = g;

    const EIdx mid = static_cast<EIdx>(edges_.size());
    edges_.push_back( Edge{ ppx, ppy, ppz, u,          EIdx(0), EIdx(0) } );
    color_.push_back( col );
    const EIdx c2  = static_cast<EIdx>(edges_.size());
    edges_.push_back( Edge{ qpx, qpy, qpz, OppDir[g],  EIdx(0), EIdx(0) } );
    color_.push_back( col );

    edges_[ei ].prev = prevE; edges_[ei ].next = mid;
    edges_[mid].prev = ei;    edges_[mid].next = c2;
    edges_[c2 ].prev = mid;   edges_[c2 ].next = nextE;
    edges_[prevE].next = ei;
    edges_[nextE].prev = c2;

    InsertEdgeKeys( edges_[ei ] ); AccAddVertex( px,  py,  pz  );
    InsertEdgeKeys( edges_[mid] ); AccAddVertex( ppx, ppy, ppz );
    InsertEdgeKeys( edges_[c2 ] ); AccAddVertex( qpx, qpy, qpz );
}

// DEATH: the plaquette (o=p' -> p),(p -> q),(q -> r=q') collapses to (p' -> q'). Reuse ei
// for the survivor, delete the two staple sides.
void ApplyDeath( const EIdx ei, const std::uint8_t /*g*/ )
{
    const Edge E   = edges_[ei];
    const std::uint8_t u = E.dir;
    const EIdx epS = E.prev;
    const EIdx enS = E.next;
    const Edge EP  = edges_[epS];
    const Edge EN  = edges_[enS];
    const EIdx pp  = EP.prev;   // edge (? -> o)
    const EIdx nn  = EN.next;   // edge (r -> ?)

    const LCoord ox = EP.tx, oy = EP.ty, oz = EP.tz;   // o = p'

    EraseEdgeKeys( E  ); AccRemoveVertex( E.tx,  E.ty,  E.tz  );
    EraseEdgeKeys( EP ); AccRemoveVertex( EP.tx, EP.ty, EP.tz );
    EraseEdgeKeys( EN ); AccRemoveVertex( EN.tx, EN.ty, EN.tz );

    // ei -> e' = (o -> r), dir u, tail o.
    edges_[ei].tx = ox; edges_[ei].ty = oy; edges_[ei].tz = oz; edges_[ei].dir = u;
    edges_[ei].prev = pp; edges_[ei].next = nn;
    edges_[pp].next = ei;
    edges_[nn].prev = ei;

    InsertEdgeKeys( edges_[ei] ); AccAddVertex( ox, oy, oz );

    // Delete the two staple slots (keys already handled; only pointer patching remains).
    const EIdx hi = (epS > enS) ? epS : enS;
    const EIdx lo = (epS > enS) ? enS : epS;
    DeleteSlot( hi );
    DeleteSlot( lo );
}

// FLIP (left type A + right type D): e=(p->q),e_next=(q->q'=r) become (p->p'),(p'->q').
// Vertex q leaves, p' joins. Reuse ei and enS; no allocation/deletion.
void ApplyFlipLeft( const EIdx ei, const std::uint8_t g )
{
    const Edge E   = edges_[ei];
    const std::uint8_t u = E.dir;
    const EIdx epS = E.prev;
    const EIdx enS = E.next;
    const Edge EN  = edges_[enS];
    const EIdx nn  = EN.next;

    const LCoord px = E.tx, py = E.ty, pz = E.tz;
    const LCoord ppx = px+dir_dx[g], ppy = py+dir_dy[g], ppz = pz+dir_dz[g];

    EraseEdgeKeys( E  ); AccRemoveVertex( E.tx,  E.ty,  E.tz  );
    EraseEdgeKeys( EN ); AccRemoveVertex( EN.tx, EN.ty, EN.tz );

    // ei -> c1 = (p -> p'), dir g (tail p unchanged).
    edges_[ei].dir = g;
    // enS -> mid = (p' -> q'), dir u, tail p'.
    edges_[enS].tx = ppx; edges_[enS].ty = ppy; edges_[enS].tz = ppz; edges_[enS].dir = u;

    edges_[ei ].prev = epS; edges_[ei ].next = enS;
    edges_[enS].prev = ei;  edges_[enS].next = nn;
    edges_[epS].next = ei;
    edges_[nn ].prev = enS;

    InsertEdgeKeys( edges_[ei ] ); AccAddVertex( px,  py,  pz  );
    InsertEdgeKeys( edges_[enS] ); AccAddVertex( ppx, ppy, ppz );
}

// FLIP (left type B + right type C): e_prev=(o=p'->p),e=(p->q) become (p'->q'),(q'->q).
// Vertex p leaves, q' joins. Reuse epS and ei; no allocation/deletion.
void ApplyFlipRight( const EIdx ei, const std::uint8_t g )
{
    const Edge E   = edges_[ei];
    const std::uint8_t u = E.dir;
    const EIdx epS = E.prev;
    const EIdx enS = E.next;
    const Edge EP  = edges_[epS];
    const EIdx pp  = EP.prev;

    const LCoord ox = EP.tx, oy = EP.ty, oz = EP.tz;                          // o = p'
    const LCoord qx = E.tx+dir_dx[u], qy = E.ty+dir_dy[u], qz = E.tz+dir_dz[u]; // q
    const LCoord qpx = qx+dir_dx[g], qpy = qy+dir_dy[g], qpz = qz+dir_dz[g];    // q'

    EraseEdgeKeys( EP ); AccRemoveVertex( EP.tx, EP.ty, EP.tz );
    EraseEdgeKeys( E  ); AccRemoveVertex( E.tx,  E.ty,  E.tz  );

    // epS -> mid = (o -> q'), dir u, tail o (unchanged).
    edges_[epS].dir = u;
    // ei -> c2 = (q' -> q), dir opp(g), tail q'.
    edges_[ei].tx = qpx; edges_[ei].ty = qpy; edges_[ei].tz = qpz; edges_[ei].dir = OppDir[g];

    edges_[epS].prev = pp;  edges_[epS].next = ei;
    edges_[ei ].prev = epS; edges_[ei ].next = enS;
    edges_[pp ].next = epS;
    edges_[enS].prev = ei;

    InsertEdgeKeys( edges_[epS] ); AccAddVertex( ox,  oy,  oz  );
    InsertEdgeKeys( edges_[ei ] ); AccAddVertex( qpx, qpy, qpz );
}

public:

// Full O(N) structural audit: cycle consistency, degree, unit edges, OccupiedSet exactly
// equal to the recomputed key set, and accumulator agreement. Debug safety net for moves.
bool SelfConsistentQ() const
{
    const EIdx m = static_cast<EIdx>(edges_.size());
    if( m == 0 ) { return true; }

    for( EIdx e = 0; e < m; ++e )
    {
        const Edge & E = edges_[e];
        if( E.dir > 5 ) { return false; }
        if( E.next < 0 || E.next >= m || E.prev < 0 || E.prev >= m ) { return false; }
        if( edges_[E.next].prev != e ) { return false; }
        if( edges_[E.prev].next != e ) { return false; }

        // head(E) must equal tail(next).
        const LCoord hx = E.tx+dir_dx[E.dir], hy = E.ty+dir_dy[E.dir], hz = E.tz+dir_dz[E.dir];
        const Edge & Nx = edges_[E.next];
        if( hx != Nx.tx || hy != Nx.ty || hz != Nx.tz ) { return false; }
    }

    // OccupiedSet must equal the union of every edge's {tail vertex, midpoint}.
    OccupiedSet_T rebuilt;
    rebuilt.reserve( occupied_.size() );
    for( EIdx e = 0; e < m; ++e )
    {
        if( !rebuilt.insert( VertexKey(edges_[e].tx, edges_[e].ty, edges_[e].tz) ).second )
        {
            return false;   // duplicate vertex => not self-avoiding
        }
        if( !rebuilt.insert( MidpointKey(edges_[e]) ).second )
        {
            return false;   // duplicate midpoint => not self-avoiding
        }
    }
    if( rebuilt.size() != occupied_.size() ) { return false; }
    for( const IVec3 & k : rebuilt )
    {
        if( !occupied_.contains(k) ) { return false; }
    }

    return AccumulatorsConsistentQ();
}
