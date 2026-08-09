// Baseline probe for the KLUT FST experiment (handoff/klut-fst-compression).
//
// Loads one Klut subtable exactly as knoodleidentify ships it (boost
// unordered_flat_map keyed on 16-byte packed MacLeod codes), reports the
// resident-memory cost of the loaded table, and times FindID over every
// key in random order for comparison with the FST query benchmark.
//
// Build (from test/klut_fst/):
//   clang++ -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
//     -fenable-matrix -DKNOODLE_USE_BOOST_UNORDERED \
//     -I../.. -I$(brew --prefix)/include \
//     -I../../submodules/Tensors klut_hash_probe.cpp -o klut_hash_probe
//
// Usage: klut_hash_probe <data_dir> <crossings> <pairs.bin>

#include <mach/mach.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <random>
#include <vector>

#include "Knoodle.hpp"

static std::size_t ResidentBytes()
{
    mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if( task_info( mach_task_self(), MACH_TASK_BASIC_INFO,
                   reinterpret_cast<task_info_t>(&info), &count ) != KERN_SUCCESS )
    {
        return 0;
    }
    return info.resident_size;
}

int main( int argc, char** argv )
{
    using Klut_T = Knoodle::Klut;
    using Key_T  = Klut_T::Key_T;

    if( argc != 4 )
    {
        std::fprintf(stderr, "usage: %s <data_dir> <crossings> <pairs.bin>\n", argv[0]);
        return 2;
    }
    const std::size_t n = static_cast<std::size_t>(std::atoi(argv[2]));

    // Load the (key,id) pairs exported for the FST run: [n bytes][4 bytes LE id].
    std::ifstream pairs_stream( argv[3], std::ios::binary );
    std::vector<Key_T>        keys;
    std::vector<std::uint32_t> ids;
    while( true )
    {
        unsigned char rec[16 + 4] = {};
        pairs_stream.read( reinterpret_cast<char*>(rec), static_cast<std::streamsize>(n + 4) );
        if( !pairs_stream ) { break; }
        Key_T key = {};
        std::memcpy( &key[0], rec, n );
        keys.push_back(key);
        std::uint32_t id;
        std::memcpy( &id, rec + n, 4 );
        ids.push_back(id);
    }
    std::printf("keys read: %zu\n", keys.size());

    const std::size_t rss_before = ResidentBytes();

    Klut_T klut { std::filesystem::path(argv[1]) };
    // First lookup forces the subtable load.
    volatile auto warm = klut.FindID<int>( keys[0] );
    (void)warm;

    const std::size_t rss_after = ResidentBytes();
    const double table_bytes = static_cast<double>(rss_after - rss_before);
    std::printf("RSS delta on load: %.0f B  (%.2f B/key)\n",
                table_bytes, table_bytes / static_cast<double>(keys.size()));

    // Query benchmark: every key once, deterministically shuffled.
    std::vector<std::size_t> order( keys.size() );
    for( std::size_t i = 0; i < order.size(); ++i ) { order[i] = i; }
    std::mt19937_64 rng( 0x5eed );
    std::shuffle( order.begin(), order.end(), rng );

    std::uint64_t sink = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for( std::size_t i : order )
    {
        sink ^= klut.FindID<int>( keys[i] ).second;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ns = std::chrono::duration<double,std::nano>(t1 - t0).count()
                    / static_cast<double>(keys.size());
    std::printf("FindID: %.0f ns/key  (sink %llu)\n", ns,
                static_cast<unsigned long long>(sink));

    // Spot-check correctness against the exported ids.
    std::size_t mismatches = 0;
    for( std::size_t i = 0; i < keys.size(); i += 997 )
    {
        if( klut.FindID<int>( keys[i] ).second != ids[i] ) { ++mismatches; }
    }
    std::printf("id spot-check mismatches: %zu\n", mismatches);

    return 0;
}
