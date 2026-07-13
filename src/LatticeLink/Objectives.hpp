// Included inside class LatticeLink. Objective-driven acceptance for BFACF moves.
//
// Step() proposes a move (Moves.hpp), then accepts/rejects it with a Metropolis rule chosen
// by settings_.objective -- the enum + switch is the swappable seam (the Reapr::Energy_T
// idiom). All admissibility / self-avoidance lives in Propose(), so every objective, no
// matter how it scores, preserves the knot/link type. Objectives read only the Proposal's
// {dN, dRg2, N, proposal_ratio} plus the attempt counter; they never touch the lattice.

private:

// Annealed inverse temperature for the MinLength schedule (linear ramp over anneal_steps).
Real CurrentBeta() const
{
    if( settings_.anneal_steps == 0 ) { return settings_.beta_final; }
    long double t = static_cast<long double>(attempt_ctr_)
                  / static_cast<long double>(settings_.anneal_steps);
    if( t > 1.0L ) { t = 1.0L; }
    return static_cast<Real>( settings_.beta_init
        + (settings_.beta_final - settings_.beta_init) * static_cast<Real>(t) );
}

// Soft-wall length penalty V(N): zero inside [band_lo, band_hi], quadratic outside.
static Real BandPenalty( const Settings_T & s, const Int N )
{
    if( s.band_lo == Int(0) && s.band_hi == Int(0) ) { return Real(0); }
    Real d = Real(0);
    if     ( N < s.band_lo ) { d = static_cast<Real>(s.band_lo - N); }
    else if ( N > s.band_hi ) { d = static_cast<Real>(N - s.band_hi); }
    return Real(0.5) * s.band_stiffness * d * d;
}

// Metropolis acceptance ratio for the active objective. Values >= 1 always accept.
Real AcceptanceRatio( const Proposal & pr ) const
{
    switch( settings_.objective )
    {
        case Objective_T::MinLength:
        {
            // Energy = length. exp(-beta * dN) biases toward deaths as beta grows; flips
            // (dN=0) are neutral. Includes the BFACF proposal ratio for detailed balance.
            const Real beta = CurrentBeta();
            return std::exp( -beta * static_cast<Real>(pr.dN) ) * pr.proposal_ratio;
        }
        case Objective_T::MaxRg2Banded:
        {
            // Climb R_g^2 at inverse temperature rg_beta, with a soft length band. All three
            // move types stay active (flips-only is non-ergodic for unfolding).
            const Int  N1 = pr.N + pr.dN;
            const Real dV = BandPenalty(settings_, N1) - BandPenalty(settings_, pr.N);
            return std::exp( settings_.rg_beta * pr.dRg2 - dV ) * pr.proposal_ratio;
        }
        default:
            return Real(1);
    }
}

template<class RNG>
bool AcceptByObjective( const Proposal & pr, RNG & rng )
{
    const Real ratio = AcceptanceRatio( pr );
    if( ratio >= Real(1) ) { return true; }
    const Real u = std::generate_canonical<Real, std::numeric_limits<Real>::digits>( rng );
    return u < ratio;
}

public:

// One objective-driven BFACF step: propose, score via the active objective, apply if
// accepted. Increments the attempt counter (which drives the annealing schedule).
template<class RNG>
MoveType Step( RNG & rng )
{
    const Proposal pr = Propose( rng, settings_.max_edges );
    ++attempt_ctr_;
    if( !pr.admissibleQ ) { return MoveType::Rejected; }
    if( !AcceptByObjective( pr, rng ) ) { return MoveType::Rejected; }
    ApplyProposal( pr );
    return PublicType( pr.kind );
}

// Convenience: run n objective-driven steps.
template<class RNG>
void Run( RNG & rng, const std::uint64_t n )
{
    for( std::uint64_t i = 0; i < n; ++i ) { Step( rng ); }
}
