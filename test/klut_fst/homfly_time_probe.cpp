// How fast is libhomfly on KLUT-scale diagrams? Times the test suite's
// HOMFLY oracle path (PlanarDiagram -> Jenkins code -> homfly_str) over a
// random sample of reconstructed table keys, reporting the conversion and
// libhomfly costs separately with distribution statistics.
//
// Build (from test/klut_fst/, needs the vendored objects from `make -C ..
// libhomfly`):
//   clang++ -Wall -Wextra -std=c++20 -O3 -march=native -fenable-matrix \
//     -I../.. -I$(brew --prefix)/include -I../../submodules/Tensors \
//     -I../../submodules/Min-Cost-Flow-Class/OPTUtils \
//     -I../../submodules/Min-Cost-Flow-Class/MCFClass \
//     -I../../submodules/Min-Cost-Flow-Class/MCFSimplex \
//     homfly_time_probe.cpp ../build/libhomfly_*.o -o homfly_time_probe
//
// Usage: homfly_time_probe <pairs.bin> <key_len> <sample_size>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "Knoodle.hpp"

extern "C" {
#include "../vendor/libhomfly/homfly.h"
}
extern "C" void knoodle_gc_free_all(void);

namespace {

struct Stats
{
    double mean, median, p95, max;
};

Stats stats( std::vector<double>& v )
{
    std::sort( v.begin(), v.end() );
    double sum = 0;
    for( double x : v ) { sum += x; }
    return { sum / static_cast<double>(v.size()),
             v[v.size() / 2],
             v[static_cast<std::size_t>(0.95 * static_cast<double>(v.size() - 1))],
             v.back() };
}

} // namespace

int main( int argc, char** argv )
{
    using Int  = Knoodle::Int64;
    using PD_T = Knoodle::PlanarDiagram<Int>;

    if( argc != 4 )
    {
        std::fprintf(stderr, "usage: %s <pairs.bin> <key_len> <sample_size>\n", argv[0]);
        return 2;
    }
    const std::size_t n = static_cast<std::size_t>(std::atoi(argv[2]));
    const std::size_t sample = static_cast<std::size_t>(std::atoi(argv[3]));

    std::ifstream in( argv[1], std::ios::binary );
    std::vector<std::uint8_t> keys;
    while( true )
    {
        std::uint8_t rec[64 + 4];
        in.read( reinterpret_cast<char*>(rec), static_cast<std::streamsize>(n + 4) );
        if( !in ) { break; }
        keys.insert( keys.end(), rec, rec + n );
    }
    const std::size_t count = keys.size() / n;

    std::vector<std::size_t> pick( count );
    for( std::size_t i = 0; i < count; ++i ) { pick[i] = i; }
    std::mt19937_64 rng( 0x5eed );
    std::shuffle( pick.begin(), pick.end(), rng );
    pick.resize( std::min( sample, count ) );

    std::vector<double> conv_us, homfly_us;
    conv_us.reserve( pick.size() );
    homfly_us.reserve( pick.size() );

    for( std::size_t i : pick )
    {
        const auto t0 = std::chrono::steady_clock::now();
        PD_T pd = PD_T::FromMacLeodCode( keys.data() + i * n, Int(n), Int(0) );
        std::string jenkins ( pd.ToJenkinsCodeString() );
        const auto t1 = std::chrono::steady_clock::now();

        std::vector<char> buf( jenkins.begin(), jenkins.end() );
        buf.push_back('\0');
        char* out = homfly_str( buf.data() );
        const auto t2 = std::chrono::steady_clock::now();
        if( out == nullptr )
        {
            std::fprintf(stderr, "homfly_str returned null at record %zu\n", i);
            return 1;
        }
        knoodle_gc_free_all();

        conv_us.push_back( std::chrono::duration<double, std::micro>(t1 - t0).count() );
        homfly_us.push_back( std::chrono::duration<double, std::micro>(t2 - t1).count() );
    }

    const Stats c = stats( conv_us );
    const Stats h = stats( homfly_us );
    std::printf("sample %zu of %zu keys (key_len %zu)\n", pick.size(), count, n);
    std::printf("reconstruct+jenkins: mean %7.1f  median %7.1f  p95 %7.1f  max %9.1f  us\n",
                c.mean, c.median, c.p95, c.max);
    std::printf("libhomfly:           mean %7.1f  median %7.1f  p95 %7.1f  max %9.1f  us\n",
                h.mean, h.median, h.p95, h.max);
    return 0;
}
