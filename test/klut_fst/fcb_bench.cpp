// Front-coded-block experiment for KLUT compression (round 2 of the FST
// experiment; see REPORT.md). Builds a queryable in-RAM structure over the
// sorted (MacLeod key -> type ID) pairs:
//
//   - keys sorted, grouped into fixed-size blocks;
//   - per block: first key stored whole, later keys front-coded as
//     (LCP, suffix);
//   - sampled index: each block's first key + payload offset;
//   - query: binary search the index, decode the one block linearly.
//
// Variants:
//   fcb-byte      byte-aligned front coding (1 header byte + raw suffix)
//   fcb-6bit      4-bit LCP + 6-bit dense-remapped symbols, bit-packed
//   zstd-raw      per-block zstd (dictionary-trained) of the raw block keys
//   zstd-fc       per-block zstd (dictionary-trained) of the fcb-byte payload
//
// Type IDs are stored as a flat uint16 array in key order (2 B/key) for all
// variants; lookups return ids[global_rank]. Reported sizes split keys-side
// (payload + index + dict) from the ID array so both are visible.
//
// Build (from test/klut_fst/):
//   clang++ -Wall -Wextra -std=c++20 -O3 -march=native \
//     -I$(brew --prefix)/include -L$(brew --prefix)/lib -lzstd \
//     fcb_bench.cpp -o fcb_bench
//
// Usage: fcb_bench <pairs.bin> <key_len>

#include <zdict.h>
#include <zstd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kMiss = 0xFFFFFFFFu;
constexpr int kZstdLevel = 19;

std::size_t g_key_len = 0;

struct Pairs
{
    std::vector<std::uint8_t>  keys; // n bytes per record, sorted
    std::vector<std::uint16_t> ids;  // one per record
    std::size_t                count = 0;
};

// --- bit I/O (big-endian, order-preserving) -------------------------------

struct BitWriter
{
    std::vector<std::uint8_t> bytes;
    std::uint64_t acc = 0;
    unsigned nbits = 0;

    void put( std::uint32_t value, unsigned width )
    {
        acc = (acc << width) | value;
        nbits += width;
        while( nbits >= 8 )
        {
            bytes.push_back( static_cast<std::uint8_t>(acc >> (nbits - 8)) );
            nbits -= 8;
        }
    }
    void flush()
    {
        if( nbits > 0 )
        {
            bytes.push_back( static_cast<std::uint8_t>(acc << (8 - nbits)) );
            nbits = 0;
        }
        acc = 0;
    }
};

struct BitReader
{
    const std::uint8_t* p;
    std::uint64_t acc = 0;
    unsigned nbits = 0;

    explicit BitReader( const std::uint8_t* start ) : p(start) {}

    std::uint32_t get( unsigned width )
    {
        while( nbits < width )
        {
            acc = (acc << 8) | *p++;
            nbits += 8;
        }
        const std::uint32_t v =
            static_cast<std::uint32_t>((acc >> (nbits - width)) & ((1u << width) - 1u));
        nbits -= width;
        return v;
    }
};

// --- shared helpers -------------------------------------------------------

std::size_t lcp_len( const std::uint8_t* a, const std::uint8_t* b )
{
    std::size_t l = 0;
    while( l < g_key_len && a[l] == b[l] ) { ++l; }
    return l;
}

// Index over blocks: first key of each block (packed) + payload offsets.
struct BlockIndex
{
    std::vector<std::uint8_t>  first_keys; // key_len bytes per block
    std::vector<std::uint32_t> offsets;    // payload byte offset per block, +sentinel
    std::size_t                block_size = 0;

    std::size_t block_count() const { return offsets.size() - 1; }

    // Last block whose first key is <= q; nullopt if q precedes everything.
    std::optional<std::size_t> find_block( const std::uint8_t* q ) const
    {
        std::size_t lo = 0, hi = block_count(); // invariant: block[lo] <= q < block[hi]
        if( std::memcmp( first_keys.data(), q, g_key_len ) > 0 ) { return std::nullopt; }
        while( hi - lo > 1 )
        {
            const std::size_t mid = lo + (hi - lo) / 2;
            if( std::memcmp( first_keys.data() + mid * g_key_len, q, g_key_len ) <= 0 )
            {
                lo = mid;
            }
            else
            {
                hi = mid;
            }
        }
        return lo;
    }

    std::size_t bytes() const
    {
        return first_keys.size() + offsets.size() * sizeof(std::uint32_t);
    }
};

// --- variant: byte-aligned front coding -----------------------------------

struct FcbByte
{
    BlockIndex idx;
    std::vector<std::uint8_t> payload;

    static std::vector<std::uint8_t> encode_block(
        const Pairs& P, std::size_t begin, std::size_t end )
    {
        std::vector<std::uint8_t> out;
        const std::uint8_t* prev = nullptr;
        for( std::size_t i = begin; i < end; ++i )
        {
            const std::uint8_t* k = P.keys.data() + i * g_key_len;
            if( prev == nullptr )
            {
                out.insert( out.end(), k, k + g_key_len );
            }
            else
            {
                const std::size_t l = lcp_len( prev, k );
                out.push_back( static_cast<std::uint8_t>(l) );
                out.insert( out.end(), k + l, k + g_key_len );
            }
            prev = k;
        }
        return out;
    }

    void build( const Pairs& P, std::size_t B )
    {
        idx.block_size = B;
        for( std::size_t b = 0; b * B < P.count; ++b )
        {
            const std::size_t begin = b * B;
            const std::size_t end   = std::min( begin + B, P.count );
            idx.first_keys.insert( idx.first_keys.end(),
                P.keys.data() + begin * g_key_len,
                P.keys.data() + begin * g_key_len + g_key_len );
            idx.offsets.push_back( static_cast<std::uint32_t>(payload.size()) );
            auto blk = encode_block( P, begin, end );
            payload.insert( payload.end(), blk.begin(), blk.end() );
        }
        idx.offsets.push_back( static_cast<std::uint32_t>(payload.size()) );
    }

    // Scan a decoded-on-the-fly block; kept static so zstd-fc can reuse it.
    static std::uint32_t scan_block(
        const std::uint8_t* p, const std::uint8_t* p_end,
        std::size_t base_rank, const std::uint8_t* q, const Pairs& P )
    {
        std::uint8_t cur[64];
        std::memcpy( cur, p, g_key_len );
        p += g_key_len;
        std::size_t rank = base_rank;
        while( true )
        {
            const int cmp = std::memcmp( cur, q, g_key_len );
            if( cmp == 0 ) { return P.ids[rank]; }
            if( cmp > 0 || p >= p_end ) { return kMiss; }
            const std::size_t l = *p++;
            std::memcpy( cur + l, p, g_key_len - l );
            p += g_key_len - l;
            ++rank;
        }
    }

    std::uint32_t query( const std::uint8_t* q, const Pairs& P ) const
    {
        const auto b = idx.find_block( q );
        if( !b ) { return kMiss; }
        return scan_block( payload.data() + idx.offsets[*b],
                           payload.data() + idx.offsets[*b + 1],
                           *b * idx.block_size, q, P );
    }

    std::size_t key_side_bytes() const { return payload.size() + idx.bytes(); }
};

// --- variant: 6-bit packed front coding -----------------------------------

struct Fcb6Bit
{
    BlockIndex idx;
    std::vector<std::uint8_t> payload;
    std::array<std::uint8_t, 256> to_sym{};
    std::array<std::uint8_t, 64>  to_byte{};
    unsigned sym_bits = 6, lcp_bits = 4;

    void build( const Pairs& P, std::size_t B )
    {
        // dense order-preserving remap of the alphabet
        std::vector<bool> used( 256, false );
        for( std::uint8_t byte : P.keys ) { used[byte] = true; }
        unsigned n_sym = 0;
        for( unsigned v = 0; v < 256; ++v )
        {
            if( used[v] )
            {
                to_sym[v] = static_cast<std::uint8_t>(n_sym);
                to_byte[n_sym] = static_cast<std::uint8_t>(v);
                ++n_sym;
            }
        }
        assert( n_sym <= 64 );

        idx.block_size = B;
        for( std::size_t b = 0; b * B < P.count; ++b )
        {
            const std::size_t begin = b * B;
            const std::size_t end   = std::min( begin + B, P.count );
            idx.first_keys.insert( idx.first_keys.end(),
                P.keys.data() + begin * g_key_len,
                P.keys.data() + begin * g_key_len + g_key_len );
            idx.offsets.push_back( static_cast<std::uint32_t>(payload.size()) );

            BitWriter w;
            const std::uint8_t* prev = nullptr;
            for( std::size_t i = begin; i < end; ++i )
            {
                const std::uint8_t* k = P.keys.data() + i * g_key_len;
                const std::size_t l = prev ? lcp_len( prev, k ) : 0;
                if( prev ) { w.put( static_cast<std::uint32_t>(l), lcp_bits ); }
                for( std::size_t j = l; j < g_key_len; ++j )
                {
                    w.put( to_sym[k[j]], sym_bits );
                }
                prev = k;
            }
            w.flush();
            payload.insert( payload.end(), w.bytes.begin(), w.bytes.end() );
        }
        idx.offsets.push_back( static_cast<std::uint32_t>(payload.size()) );
    }

    std::uint32_t query( const std::uint8_t* q, const Pairs& P ) const
    {
        const auto b = idx.find_block( q );
        if( !b ) { return kMiss; }
        const std::size_t begin = *b * idx.block_size;
        const std::size_t end   = std::min( begin + idx.block_size, P.count );
        BitReader r( payload.data() + idx.offsets[*b] );
        std::uint8_t cur[64];
        std::size_t rank = begin;
        for( std::size_t j = 0; j < g_key_len; ++j ) { cur[j] = to_byte[r.get(sym_bits)]; }
        while( true )
        {
            const int cmp = std::memcmp( cur, q, g_key_len );
            if( cmp == 0 ) { return P.ids[rank]; }
            if( cmp > 0 || rank + 1 >= end ) { return kMiss; }
            const std::size_t l = r.get( lcp_bits );
            for( std::size_t j = l; j < g_key_len; ++j ) { cur[j] = to_byte[r.get(sym_bits)]; }
            ++rank;
        }
    }

    std::size_t key_side_bytes() const { return payload.size() + idx.bytes(); }
};

// --- variants: per-block zstd with trained dictionary ---------------------

struct FcbZstd
{
    BlockIndex idx;
    std::vector<std::uint8_t> payload; // concatenated compressed blocks
    std::vector<std::uint8_t> dict;
    ZSTD_DDict* ddict = nullptr;
    bool front_codedQ = false; // false: raw block keys; true: fcb-byte payload

    void build( const Pairs& P, std::size_t B, bool front_coded )
    {
        front_codedQ = front_coded;
        idx.block_size = B;

        std::vector<std::vector<std::uint8_t>> blocks;
        for( std::size_t b = 0; b * B < P.count; ++b )
        {
            const std::size_t begin = b * B;
            const std::size_t end   = std::min( begin + B, P.count );
            if( front_coded )
            {
                blocks.push_back( FcbByte::encode_block( P, begin, end ) );
            }
            else
            {
                blocks.emplace_back(
                    P.keys.data() + begin * g_key_len,
                    P.keys.data() + end * g_key_len );
            }
        }

        // train a dictionary on the blocks themselves
        std::vector<std::uint8_t> samples;
        std::vector<std::size_t>  sizes;
        for( const auto& blk : blocks )
        {
            samples.insert( samples.end(), blk.begin(), blk.end() );
            sizes.push_back( blk.size() );
        }
        dict.resize( 110 * 1024 );
        const std::size_t d = ZDICT_trainFromBuffer(
            dict.data(), dict.size(), samples.data(), sizes.data(),
            static_cast<unsigned>(sizes.size()) );
        if( ZDICT_isError(d) )
        {
            std::fprintf(stderr, "dict training failed (%s), using no dict\n",
                         ZDICT_getErrorName(d));
            dict.clear();
        }
        else
        {
            dict.resize( d );
        }

        ZSTD_CDict* cdict = ZSTD_createCDict( dict.data(), dict.size(), kZstdLevel );
        ddict = ZSTD_createDDict( dict.data(), dict.size() );
        ZSTD_CCtx* cctx = ZSTD_createCCtx();

        std::vector<std::uint8_t> buf;
        for( std::size_t b = 0; b < blocks.size(); ++b )
        {
            const auto& blk = blocks[b];
            idx.first_keys.insert( idx.first_keys.end(),
                P.keys.data() + b * B * g_key_len,
                P.keys.data() + b * B * g_key_len + g_key_len );
            idx.offsets.push_back( static_cast<std::uint32_t>(payload.size()) );
            buf.resize( ZSTD_compressBound( blk.size() ) );
            const std::size_t c = ZSTD_compress_usingCDict(
                cctx, buf.data(), buf.size(), blk.data(), blk.size(), cdict );
            assert( !ZSTD_isError(c) );
            payload.insert( payload.end(), buf.data(), buf.data() + c );
        }
        idx.offsets.push_back( static_cast<std::uint32_t>(payload.size()) );
        ZSTD_freeCCtx( cctx );
        ZSTD_freeCDict( cdict );
    }

    std::uint32_t query( const std::uint8_t* q, const Pairs& P,
                         ZSTD_DCtx* dctx, std::uint8_t* scratch ) const
    {
        const auto b = idx.find_block( q );
        if( !b ) { return kMiss; }
        const std::size_t begin = *b * idx.block_size;
        const std::size_t end   = std::min( begin + idx.block_size, P.count );
        const std::size_t raw_cap = idx.block_size * g_key_len;
        const std::size_t got = ZSTD_decompress_usingDDict(
            dctx, scratch, raw_cap,
            payload.data() + idx.offsets[*b],
            idx.offsets[*b + 1] - idx.offsets[*b], ddict );
        assert( !ZSTD_isError(got) );

        if( front_codedQ )
        {
            return FcbByte::scan_block( scratch, scratch + got, begin, q, P );
        }
        // raw: fixed-stride records
        for( std::size_t i = begin; i < end; ++i )
        {
            const int cmp = std::memcmp(
                scratch + (i - begin) * g_key_len, q, g_key_len );
            if( cmp == 0 ) { return P.ids[i]; }
            if( cmp > 0 ) { return kMiss; }
        }
        return kMiss;
    }

    std::size_t key_side_bytes() const
    {
        return payload.size() + idx.bytes() + dict.size();
    }
};

// --- driver ---------------------------------------------------------------

void shuffle_order( std::vector<std::size_t>& order )
{
    std::uint64_t seed = 0x5eed;
    auto next = [&]() {
        seed += 0x9e3779b97f4a7c15ULL;
        std::uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    };
    for( std::size_t i = order.size() - 1; i > 0; --i )
    {
        std::swap( order[i], order[next() % (i + 1)] );
    }
}

template<typename QueryFn>
void report( const char* name, std::size_t block_size, const Pairs& P,
             std::size_t key_side, QueryFn&& query )
{
    // verify all keys, then 10k perturbed misses
    for( std::size_t i = 0; i < P.count; ++i )
    {
        const std::uint32_t got = query( P.keys.data() + i * g_key_len );
        if( got != P.ids[i] )
        {
            std::fprintf(stderr, "%s B=%zu: WRONG id at rank %zu (%u vs %u)\n",
                         name, block_size, i, got, P.ids[i]);
            std::exit(1);
        }
    }
    std::size_t misses = 0;
    std::uint8_t bad[64];
    for( std::size_t i = 0; i < 10'000; ++i )
    {
        std::memcpy( bad, P.keys.data() + i * g_key_len, g_key_len );
        bad[g_key_len - 1] ^= 0xFF;
        if( query( bad ) == kMiss ) { ++misses; }
    }

    std::vector<std::size_t> order( P.count );
    for( std::size_t i = 0; i < P.count; ++i ) { order[i] = i; }
    shuffle_order( order );

    std::uint64_t sink = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for( std::size_t i : order ) { sink ^= query( P.keys.data() + i * g_key_len ); }
    const auto t1 = std::chrono::steady_clock::now();
    const double q_ns = std::chrono::duration<double, std::nano>(t1 - t0).count()
                      / static_cast<double>(P.count);
    if( sink == 0xDEADBEEF ) { std::puts(""); } // keep sink alive

    const double n = static_cast<double>(P.count);
    const std::size_t id_bytes = P.count * sizeof(std::uint16_t);
    std::printf(
        "%-9s B=%4zu  keys %8.0f B (%5.2f B/key)  +ids %.2f  total %5.2f B/key"
        "  query %5.0f ns  miss %zu/10000\n",
        name, block_size, static_cast<double>(key_side), key_side / n,
        id_bytes / n, (key_side + id_bytes) / n, q_ns, misses );
}

} // namespace

int main( int argc, char** argv )
{
    if( argc != 3 )
    {
        std::fprintf(stderr, "usage: %s <pairs.bin> <key_len>\n", argv[0]);
        return 2;
    }
    g_key_len = static_cast<std::size_t>(std::atoi(argv[2]));

    std::ifstream in( argv[1], std::ios::binary );
    Pairs P;
    while( true )
    {
        std::uint8_t rec[64 + 4];
        in.read( reinterpret_cast<char*>(rec),
                 static_cast<std::streamsize>(g_key_len + 4) );
        if( !in ) { break; }
        P.keys.insert( P.keys.end(), rec, rec + g_key_len );
        std::uint32_t id;
        std::memcpy( &id, rec + g_key_len, 4 );
        assert( id < 0x10000 );
        P.ids.push_back( static_cast<std::uint16_t>(id) );
        ++P.count;
    }
    std::printf("keys %zu, key_len %zu, ids as uint16\n", P.count, g_key_len);

    for( std::size_t B : {16, 64, 256} )
    {
        FcbByte v;
        v.build( P, B );
        report( "fcb-byte", B, P, v.key_side_bytes(),
                [&]( const std::uint8_t* q ) { return v.query( q, P ); } );
    }
    for( std::size_t B : {16, 64, 256} )
    {
        Fcb6Bit v;
        v.build( P, B );
        report( "fcb-6bit", B, P, v.key_side_bytes(),
                [&]( const std::uint8_t* q ) { return v.query( q, P ); } );
    }
    for( bool fc : {false, true} )
    {
        for( std::size_t B : {64, 256, 1024} )
        {
            FcbZstd v;
            v.build( P, B, fc );
            ZSTD_DCtx* dctx = ZSTD_createDCtx();
            std::vector<std::uint8_t> scratch( B * g_key_len + 64 );
            report( fc ? "zstd-fc" : "zstd-raw", B, P, v.key_side_bytes(),
                    [&]( const std::uint8_t* q )
                    { return v.query( q, P, dctx, scratch.data() ); } );
            ZSTD_freeDCtx( dctx );
        }
    }
    return 0;
}
