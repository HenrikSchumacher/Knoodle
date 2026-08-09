// Emit every key of a KLUT pairs file (see export_pairs.py) as a
// knoodlesimplify-style knot block (k / s / 5-column signed PD rows), in
// pairs-file order, on stdout. Feed the result to an invariant filter, e.g.
//
//   ./keys_to_pd pairs_13.bin 13 | ../../..../knoodleinvariants/tools/v2 -q
//
// yields one v2 value per key, line i matching record i of the pairs file.
//
// Build: same line as klut_hash_probe.cpp (see its header comment).

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "Knoodle.hpp"

int main( int argc, char** argv )
{
    using Int  = Knoodle::Int64;
    using PD_T = Knoodle::PlanarDiagram<Int>;

    if( argc != 3 )
    {
        std::fprintf(stderr, "usage: %s <pairs.bin> <key_len>\n", argv[0]);
        return 2;
    }
    const std::size_t n = static_cast<std::size_t>(std::atoi(argv[2]));

    std::ifstream pairs_stream( argv[1], std::ios::binary );
    std::ios::sync_with_stdio(false);

    std::size_t count = 0;
    while( true )
    {
        unsigned char rec[16 + 4] = {};
        pairs_stream.read( reinterpret_cast<char*>(rec), static_cast<std::streamsize>(n + 4) );
        if( !pairs_stream ) { break; }

        PD_T pd = PD_T::FromMacLeodCode( rec, Int(n), Int(0) );
        if( !pd.ValidQ() )
        {
            std::fprintf(stderr, "record %zu: invalid reconstruction\n", count);
            return 1;
        }

        std::cout << "k\ns\n";
        auto pd_code = pd.template PDCode<Int, {.signQ = true, .colorQ = false}>();
        const Int c = pd.CrossingCount();
        for( Int x = 0; x < c; ++x )
        {
            std::cout << pd_code(x,0);
            for( Int col = 1; col < 5; ++col ) { std::cout << '\t' << pd_code(x,col); }
            std::cout << '\n';
        }
        ++count;
    }
    std::fprintf(stderr, "emitted %zu diagrams\n", count);
    return 0;
}
