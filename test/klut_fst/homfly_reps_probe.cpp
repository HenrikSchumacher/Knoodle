// Compute the HOMFLY polynomial of one representative key per knot type in
// a KLUT pairs file (HOMFLY is an invariant, so any key of the type serves).
// Prints "type_id<TAB>polynomial" lines; shard/mod split the type-ID space
// so several processes can run in parallel (libhomfly has global state, so
// in-process threading is off the table).
//
//   homfly_reps_probe <pairs.bin> <key_len> <shard> <mod>
//
// Build: same line as homfly_time_probe.cpp (see its header comment).

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Knoodle.hpp"

extern "C" {
#include "../vendor/libhomfly/homfly.h"
}
extern "C" void knoodle_gc_free_all(void);

int main( int argc, char** argv )
{
    using Int  = Knoodle::Int64;
    using PD_T = Knoodle::PlanarDiagram<Int>;

    if( argc != 5 )
    {
        std::fprintf(stderr, "usage: %s <pairs.bin> <key_len> <shard> <mod>\n", argv[0]);
        return 2;
    }
    const std::size_t n     = static_cast<std::size_t>(std::atoi(argv[2]));
    const std::uint32_t shard = static_cast<std::uint32_t>(std::atoi(argv[3]));
    const std::uint32_t mod   = static_cast<std::uint32_t>(std::atoi(argv[4]));

    // first key seen per type id (pairs are key-sorted; any key will do)
    std::unordered_map<std::uint32_t, std::vector<std::uint8_t>> rep;
    std::ifstream in( argv[1], std::ios::binary );
    while( true )
    {
        std::uint8_t rec[64 + 4];
        in.read( reinterpret_cast<char*>(rec), static_cast<std::streamsize>(n + 4) );
        if( !in ) { break; }
        std::uint32_t id;
        std::memcpy( &id, rec + n, 4 );
        if( id % mod == shard && !rep.contains(id) )
        {
            rep.emplace( id, std::vector<std::uint8_t>( rec, rec + n ) );
        }
    }

    for( const auto& [id, key] : rep )
    {
        PD_T pd = PD_T::FromMacLeodCode( key.data(), Int(n), Int(0) );
        std::string jenkins ( pd.ToJenkinsCodeString() );
        std::vector<char> buf( jenkins.begin(), jenkins.end() );
        buf.push_back('\0');
        char* out = homfly_str( buf.data() );
        if( out == nullptr )
        {
            std::fprintf(stderr, "homfly_str returned null for type %u\n", id);
            return 1;
        }
        std::printf("%u\t%s\n", id, out);
        knoodle_gc_free_all();
    }
    return 0;
}
