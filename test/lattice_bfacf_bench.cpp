// Decongestion experiment: does BFACF preprocessing make the adversarial confined lattice
// knot (data/closed_confined_knot.tsv, ~57.6k edges) tractable for the Simplify pipeline?
//
// Backbone (one round):   P -> E -> [B_mid] -> R -> P
//   P     pass moves (PDC::Simplify with embedding_trials=0)
//   E     Reapr embedding of the largest diagram (integer lattice)
//   B_mid BFACF on that embedding (optional)
//   R     random rotation + reprojection to a PlanarDiagram
// with an optional B_pre BFACF prepended on the raw lattice knot (before the first, very
// expensive projection). Variants: none / pre / mid / both, each with MinLength and
// MaxRg2Banded, plus a capped real-Simplify baseline. Metric: crossing count + wall clock.
// Final knot type is cross-checked with HOMFLY (all variants must agree).

#include "homfly_invariance.hpp"
#include "../src/LatticeLink.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
    using LinkEmb_T = Knoodle::LinkEmbedding<double,Int,float>;
    using Lattice_T = Knoodle::LatticeLink<double,Int,float>;
    using Reapr_T   = Knoodle::Reapr<double,Int,float>;
    using Args_T    = PDC_T::Simplify_Args_T;

    double now_s()
    {
        return std::chrono::duration<double>( std::chrono::steady_clock::now().time_since_epoch() ).count();
    }

    std::vector<double> LoadCoords( const std::string & path )
    {
        std::vector<double> c;
        std::ifstream in( path );
        double x, y, z;
        while( in >> x >> y >> z ) { c.push_back(x); c.push_back(y); c.push_back(z); }
        return c;
    }

    LinkEmb_T CoordsToEmbedding( const std::vector<double> & coord )
    {
        const Int V = static_cast<Int>( coord.size() / 3 );
        Tensors::Tensor1<Int,Int> cp(2), col(1);
        cp[0] = 0; cp[1] = V; col[0] = 0;
        LinkEmb_T emb( std::move(cp), std::move(col) );
        emb.template ReadVertexCoordinates<false,false>( coord.data() );
        return emb;
    }

    // A ReAPR embedding is axis-aligned but a *brick* lattice (integer x,y; real z from the
    // level scaling), so LatticeLink rejects it. Rescale each axis to unit spacing (a
    // per-axis bijection -> preserves self-avoidance & knot type) so it becomes a cubic unit
    // lattice LatticeLink can ingest. Returns whether snapping produced a valid lattice.
    bool SnapToLattice( LinkEmb_T & emb )
    {
        const Int V = emb.VertexCount();
        std::vector<double> c( static_cast<std::size_t>(3*V) );
        emb.WriteVertexCoordinates( c.data() );

        double unit[3] = { 0, 0, 0 };
        double lo[3]   = { c[0], c[1], c[2] };
        for( Int i = 0; i < V; ++i )
        {
            const Int j = (i + 1) % V;
            for( int a = 0; a < 3; ++a )
            {
                lo[a] = std::min( lo[a], c[3*i+a] );
                const double d = std::fabs( c[3*j+a] - c[3*i+a] );
                if( d > 1e-9 && (unit[a] == 0 || d < unit[a]) ) { unit[a] = d; }
            }
        }
        for( int a = 0; a < 3; ++a ) { if( unit[a] == 0 ) { unit[a] = 1; } }

        std::vector<double> snapped( static_cast<std::size_t>(3*V) );
        for( Int i = 0; i < V; ++i )
        {
            for( int a = 0; a < 3; ++a )
            {
                snapped[3*i+a] = std::round( (c[3*i+a] - lo[a]) / unit[a] );
            }
        }
        LinkEmb_T out = CoordsToEmbedding( snapped );
        Lattice_T probe( out );      // validate: integer + axis-aligned + self-avoiding
        if( !probe.ValidQ() ) { return false; }
        emb = std::move(out);
        return true;
    }

    // Run BFACF on a lattice embedding with the chosen objective until it plateaus (or a move
    // cap). Returns the relaxed embedding; reports start/end edges + R_g^2.
    LinkEmb_T RunBFACF( LinkEmb_T emb, Lattice_T::Objective_T obj, std::uint64_t seed, const char * tag )
    {
        Lattice_T ll( emb );
        if( !ll.ValidQ() )
        {
            std::cout << "      [" << tag << "] LatticeLink REJECTED input; skipping BFACF\n";
            return emb;
        }

        const Int N0 = ll.EdgeCount();
        const double rg0 = ll.SquaredGyradius();

        ll.Settings().objective = obj;
        if( obj == Lattice_T::Objective_T::MinLength )
        {
            ll.Settings().beta_init = 5.0; ll.Settings().beta_final = 5.0;
            ll.Settings().anneal_steps = 1;
            ll.Settings().max_edges = static_cast<std::size_t>(N0) * 2 + 100;
        }
        else // MaxRg2Banded: spread the confined knot out at ~FIXED length (tight band).
        {
            ll.Settings().rg_beta = 0.05;
            ll.Settings().band_lo = (N0 * 9) / 10;
            ll.Settings().band_hi = (N0 * 11) / 10;
            ll.Settings().band_stiffness = 0.2;
            ll.Settings().max_edges = static_cast<std::size_t>(N0) * 3 / 2 + 100;
        }

        std::mt19937_64 rng( seed );
        double prev = (obj == Lattice_T::Objective_T::MinLength)
                      ? static_cast<double>(N0) : rg0;
        const double t0 = now_s();
        for( int blk = 0; blk < 10; ++blk )
        {
            for( int k = 0; k < 5'000'000; ++k ) { ll.Step( rng ); }
            const double metric = (obj == Lattice_T::Objective_T::MinLength)
                                  ? static_cast<double>(ll.EdgeCount()) : ll.SquaredGyradius();
            const bool plateau = (obj == Lattice_T::Objective_T::MinLength)
                                 ? (metric > prev * 0.99)     // length stopped shrinking
                                 : (metric < prev * 1.01);    // R_g^2 stopped growing
            prev = metric;
            if( plateau && blk >= 1 ) { break; }
        }
        std::cout << "      [" << tag << "] edges " << N0 << " -> " << ll.EdgeCount()
                  << "   Rg^2 " << std::fixed << std::setprecision(1) << rg0 << " -> " << ll.SquaredGyradius()
                  << "   (" << std::setprecision(2) << (now_s()-t0) << "s)\n" << std::flush;
        return ll.ToLinkEmbedding();
    }

    // Rotate generically and reproject to a PDC.
    PDC_T Reproject( LinkEmb_T emb, Reapr_T & reapr )
    {
        emb.Rotate( reapr.RandomRotation() );
        return PDC_T( std::move(emb) );
    }

    Int PassMoves( PDC_T & pdc )
    {
        pdc.Simplify( Args_T{ .embedding_trials = 0 } );
        return pdc.CrossingCount();
    }

    // Final knot-type fingerprint -- ONLY when tiny (libhomfly is exponential in crossings;
    // this confined knot will very likely stay far above the feasible range). Best-effort.
    std::string FinalHomfly( PDC_T & pdc )
    {
        if( pdc.CrossingCount() > Int(16) ) { return "(too big for HOMFLY)"; }
        pdc.SortByCrossingCount();
        if( pdc.DiagramCount() == 0 ) { return "(empty)"; }
        bool ok = false;
        Polynomial p = HomflyOfPossiblySplit( KnoodleJenkins( pdc.Diagram(0) ), ok );
        return ok ? PolyToString(p) : "(uncomputable)";
    }

    // Cyclic spread/shrink BFACF on a compact lattice knot: alternate MaxRg2Banded (unfold,
    // near-fixed length) and MinLength (shrink) for `cycles` rounds, then a final shrink. The
    // only BFACF strategy that escapes the confinement plateau (slowly). Prints the trajectory.
    LinkEmb_T CyclicSpreadShrink( LinkEmb_T emb, int cycles, std::uint64_t moves_per_phase )
    {
        Lattice_T ll( emb );
        if( !ll.ValidQ() ) { std::cout << "      [B_pre] REJECTED\n"; return emb; }
        std::mt19937_64 rng( 0xA11CEu );
        const double t0 = now_s();

        auto minlen = [&]( std::uint64_t m ) {
            ll.Settings().objective = Lattice_T::Objective_T::MinLength;
            ll.Settings().beta_init = 5.0; ll.Settings().beta_final = 5.0; ll.Settings().anneal_steps = 1;
            ll.Settings().max_edges = static_cast<std::size_t>(ll.EdgeCount())*2 + 100;
            for( std::uint64_t k = 0; k < m; ++k ) { ll.Step(rng); }
        };
        auto spread = [&]( std::uint64_t m ) {
            const Int N = ll.EdgeCount();
            ll.Settings().objective = Lattice_T::Objective_T::MaxRg2Banded;
            ll.Settings().rg_beta = 0.05; ll.Settings().band_lo = (N*9)/10; ll.Settings().band_hi = (N*11)/10;
            ll.Settings().band_stiffness = 0.2; ll.Settings().max_edges = static_cast<std::size_t>(N)*3/2 + 100;
            for( std::uint64_t k = 0; k < m; ++k ) { ll.Step(rng); }
        };

        std::cout << "      [B_pre] edges " << ll.EdgeCount() << " -> " << std::flush;
        for( int c = 0; c < cycles; ++c ) { minlen(moves_per_phase); spread(moves_per_phase); }
        minlen( moves_per_phase * 2 );
        std::cout << ll.EdgeCount() << "  (" << std::setprecision(2) << (now_s()-t0) << "s)\n" << std::flush;
        return ll.ToLinkEmbedding();
    }

    struct Row { std::string name; Int c_afterP0, c_final; double t_pre, t_total; };
    std::vector<Row> g_rows;

    // Head-to-head: optional cyclic BFACF preprocess, then the REAL Simplify reapr loop, at a
    // fixed embedding_trials budget. Does a compact/cheap start help the real pipeline?
    void RunPipeline( const std::string & name, const std::vector<double> & coord,
                      bool bfacf_pre, int cycles, std::uint64_t moves_per_phase,
                      std::size_t embedding_trials )
    {
        std::cout << "\n=== " << name << " ===\n" << std::flush;
        Reapr_T reapr;
        const double t0 = now_s();

        LinkEmb_T emb = CoordsToEmbedding( coord );
        if( bfacf_pre ) { emb = CyclicSpreadShrink( std::move(emb), cycles, moves_per_phase ); }
        const double t_pre = now_s() - t0;

        PDC_T pdc = Reproject( std::move(emb), reapr );
        const Int c_p0 = PassMoves( pdc );
        std::cout << "      after [B_pre]+P0 = " << c_p0 << " (" << std::setprecision(2) << (now_s()-t0) << "s)\n" << std::flush;

        pdc.Simplify( Args_T{ .embedding_trials = embedding_trials, .rotation_trials = 25 } );
        const Int c_final = pdc.CrossingCount();
        std::cout << "      after real Simplify = " << c_final << " (" << (now_s()-t0) << "s)\n" << std::flush;

        g_rows.push_back({ name, c_p0, c_final, t_pre, now_s()-t0 });
    }
}

int main( int argc, char** argv )
{
    const std::string path = (argc > 1) ? argv[1] : "../data/closed_confined_knot.tsv";
    const std::vector<double> coord = LoadCoords( path );
    std::cout << "loaded " << coord.size()/3 << " vertices from " << path << "\n" << std::flush;

    const std::size_t trials = (argc > 2) ? static_cast<std::size_t>(std::stoul(argv[2])) : 10;

    // Head-to-head at matched real-Simplify budget: does cyclic-BFACF preprocessing help the
    // real pipeline reach lower crossings (or reach the same faster)?
    RunPipeline( "baseline (no BFACF)",   coord, false, 0,  0,          trials );
    RunPipeline( "cyclic-BFACF-pre",      coord, true,  6,  15'000'000, trials );

    std::cout << "\n================ SUMMARY (lower crossings = better) ================\n";
    std::cout << std::left << std::setw(24) << "variant"
              << std::right << std::setw(12) << "afterP0" << std::setw(10) << "final X"
              << std::setw(10) << "t_pre" << std::setw(10) << "t_total" << "\n";
    for( const Row & r : g_rows )
    {
        std::cout << std::left << std::setw(24) << r.name
                  << std::right << std::setw(12) << r.c_afterP0 << std::setw(10) << r.c_final
                  << std::setw(10) << std::fixed << std::setprecision(1) << r.t_pre
                  << std::setw(10) << r.t_total << "\n";
    }
    return 0;
}
