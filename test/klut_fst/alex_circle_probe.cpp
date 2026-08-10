// Unit-circle Alexander fingerprint per KLUT knot type: the ceiling
// measurement for Alexander-based cascade stages. Bucketing by |Delta| at
// many |t|=1 points is bucketing by the full Alexander polynomial (up to
// float resolution), so the residual bits computed from this output bound
// what ANY set of unit-circle evaluations can achieve.
//
// Prints "type_id<TAB>log10|det| x5" per type (first key = representative;
// the unit t^k has modulus 1 on the circle, so any key of the type serves).
// Also spot-checks constancy on a second key of every 50th type, and
// reports the mean per-diagram evaluation cost (5-point UMFPACK batch).
//
// Build (from test/klut_fst/):
//   clang++ -Wall -Wextra -std=c++20 -O3 -march=native -fenable-matrix \
//     -DKNOODLE_USE_UMFPACK -I../.. -I$(brew --prefix)/include \
//     -I$(brew --prefix)/include/suitesparse -I../../submodules/Tensors \
//     -I../../submodules/Min-Cost-Flow-Class/OPTUtils \
//     -I../../submodules/Min-Cost-Flow-Class/MCFClass \
//     -I../../submodules/Min-Cost-Flow-Class/MCFSimplex \
//     alex_circle_probe.cpp -L$(brew --prefix)/lib -lumfpack \
//     -framework Accelerate -o alex_circle_probe
//
// Usage: alex_circle_probe <pairs.bin> <key_len>

#include <chrono>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

#include "Knoodle.hpp"
#include "../link_alexander.hpp"

int main( int argc, char** argv )
{
    using Int      = std::int64_t;
    using Cplx     = std::complex<double>;
    using PD_T     = Knoodle::PlanarDiagram<Int>;
    using LinkAlex = knoodle_test::LinkAlexander<Cplx, Int>;

    if( argc != 3 )
    {
        std::fprintf(stderr, "usage: %s <pairs.bin> <key_len>\n", argv[0]);
        return 2;
    }
    const std::size_t n = static_cast<std::size_t>(std::atoi(argv[2]));

    // reps[id] = first key; spare[id] = a later key of every 50th type
    std::map<std::uint32_t, std::vector<std::uint8_t>> reps, spare;
    std::ifstream in( argv[1], std::ios::binary );
    while( true )
    {
        std::uint8_t rec[64 + 4];
        in.read( reinterpret_cast<char*>(rec), static_cast<std::streamsize>(n + 4) );
        if( !in ) { break; }
        std::uint32_t id;
        std::memcpy( &id, rec + n, 4 );
        if( !reps.count(id) )
        {
            reps.emplace( id, std::vector<std::uint8_t>( rec, rec + n ) );
        }
        else if( id % 50 == 0 && !spare.count(id) )
        {
            spare.emplace( id, std::vector<std::uint8_t>( rec, rec + n ) );
        }
    }

    LinkAlex alex;
    auto fingerprint = [&]( const std::vector<std::uint8_t>& key )
    {
        PD_T pd = PD_T::FromMacLeodCode( key.data(), Int(n), Int(0) );
        return alex( pd );
    };

    double secs = 0;
    std::size_t evals = 0, bad = 0, checked = 0, mismatched = 0;
    for( const auto& [id, key] : reps )
    {
        const auto t0 = std::chrono::steady_clock::now();
        const auto v = fingerprint( key );
        secs += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        ++evals;
        if( !v.ok ) { ++bad; continue; }

        std::printf("%u", id);
        for( double x : v.logdet ) { std::printf("\t%.10f", x); }
        std::printf("\n");

        const auto it = spare.find(id);
        if( it != spare.end() )
        {
            ++checked;
            if( !LinkAlex::Equal( v, fingerprint(it->second), 1e-6 ) ) { ++mismatched; }
        }
    }
    std::fprintf(stderr,
        "types %zu, failed %zu; constancy spot-check %zu types, %zu mismatches\n"
        "eval cost: %.1f us/diagram (5-point unit-circle batch, UMFPACK)\n",
        evals, bad, checked, mismatched, secs / static_cast<double>(evals) * 1e6);
    return 0;
}
