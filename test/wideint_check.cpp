/**
 * @file wideint_check.cpp
 * @brief WideInt's bitwise operations, negation and long multiplication, checked
 *        against Boost's cpp_int.
 *
 * The bug this pins (v1.1.0, found right after release): bitwise.hpp computed
 * `~`, `==` and `ZeroQ` by reinterpret_cast'ing the limb array to a std::bitset.
 * That is a strict-aliasing violation. gcc -O3 assumes a std::bitset load cannot
 * see a preceding WideInt store, so `Negate()` (= `~` then `++`) could work on
 * stale limbs. Every 3D input goes through Prosector_Int, which multiplies an
 * Int64 coordinate by an Int128 determinant via long_mul(WideInt<1>,
 * WideInt<2>) -> long_fma; negating a negative operand there produced a value
 * that was still negative, and `assert(!NegativeQ(b))` in long_fma_unsigned
 * aborted. Without assertions it would have been a wrong determinant sign.
 *
 * clang did not miscompile it, and neither did gcc at -O0/-O1/-O2 or with
 * -fno-strict-aliasing, so on macOS this test passes either way. It earns its
 * keep on the gcc legs of CI, and in the heavy tier's gcc_build_check.
 *
 * The reference arithmetic is boost::multiprecision::cpp_int, which shares no
 * code with WideInt. Operands come from a fixed-seed generator with random bit
 * lengths, so small, large and mixed-sign values all occur, plus edge values.
 *
 * Build: see test/Makefile (target: wideint_check).
 */

#include "../Knoodle.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using namespace Knoodle;
using BigInt = boost::multiprecision::cpp_int;

using Int64  = std::int64_t;
using UInt64 = std::uint64_t;
using Int128 = __int128;

static int checks = 0;
static int fails  = 0;

static void check( bool ok, const std::string & what )
{
    ++checks;
    if( !ok )
    {
        ++fails;
        if( fails <= 20 )
        {
            std::printf("FAIL: %s\n", what.c_str());
        }
    }
}

static void section( const char * name )
{
    std::printf("=== %s ===\n", name);
}

// ---------------------------------------------------------------------------
// Conversions to the reference type.
// ---------------------------------------------------------------------------

template<int n, typename L, typename C, bool s>
static BigInt ToBig( const WideInt<n,L,C,s> & w )
{
    BigInt r = 0;
    for( int i = n - 1; i >= 0; --i )
    {
        r <<= 64;
        r += BigInt( static_cast<UInt64>(w[i]) );
    }
    if( s && ( (static_cast<UInt64>(w[n-1]) >> 63) != 0 ) )
    {
        r -= ( BigInt(1) << (64 * n) );
    }
    return r;
}

static BigInt ToBig( Int128 x )
{
    const bool neg = x < 0;
    // Two's complement magnitude without overflow: work in unsigned.
    unsigned __int128 u = neg ? (~static_cast<unsigned __int128>(x) + 1)
                              :   static_cast<unsigned __int128>(x);
    BigInt r = BigInt( static_cast<UInt64>(u >> 64) );
    r <<= 64;
    r += BigInt( static_cast<UInt64>(u) );
    return neg ? BigInt(-r) : r;
}

static std::string Str( const BigInt & x ) { return x.str(); }

template<int n, typename L, typename C, bool s>
static std::string LimbStr( const WideInt<n,L,C,s> & w )
{
    std::string r = "{";
    char buf[32];
    for( int i = 0; i < n; ++i )
    {
        std::snprintf(buf, sizeof buf, "%s%016llx", i ? "," : "",
                      static_cast<unsigned long long>(w[i]));
        r += buf;
    }
    return r + "}";
}

// ---------------------------------------------------------------------------
// Random operands with random bit lengths, so every magnitude occurs.
// ---------------------------------------------------------------------------

static std::mt19937_64 rng (20261106);

static Int64 RandomInt64()
{
    const int bits = static_cast<int>(rng() % 64);           // 0..63 magnitude bits
    UInt64 m = (bits == 0) ? 0 : (rng() >> (64 - bits));
    const bool neg = (rng() & 1) != 0;
    return neg ? -static_cast<Int64>(m) : static_cast<Int64>(m);
}

static Int128 RandomInt128()
{
    const int bits = static_cast<int>(rng() % 127);          // 0..126 magnitude bits
    unsigned __int128 m = (static_cast<unsigned __int128>(rng()) << 64) | rng();
    m = (bits == 0) ? 0 : (m >> (128 - bits));
    const bool neg = (rng() & 1) != 0;
    return neg ? -static_cast<Int128>(m) : static_cast<Int128>(m);
}

template<int n>
using W = WideInt<n,UInt64,unsigned __int128,true>;

template<int n>
static W<n> RandomW()
{
    std::array<UInt64,n> limbs;
    const int top = static_cast<int>(rng() % (64 * n));     // random bit length
    for( int i = 0; i < n; ++i )
    {
        limbs[i] = rng();
        const int lo = 64 * i;
        if( top <= lo )             { limbs[i] = 0; }
        else if( top < lo + 64 )    { limbs[i] &= ( (UInt64(1) << (top - lo)) - 1 ); }
    }
    W<n> w ( &limbs[0] );
    if( rng() & 1 ) { w = -w; }
    return w;
}

template<int n>
static std::vector<W<n>> EdgeValues()
{
    std::vector<W<n>> v;
    std::array<UInt64,n> a {};
    v.push_back( W<n>(&a[0]) );                                 // 0
    a[0] = 1;              v.push_back( W<n>(&a[0]) );          // 1
    for( auto & x : a ) { x = ~UInt64(0); }
    v.push_back( W<n>(&a[0]) );                                 // -1
    a[n-1] = ~UInt64(0) >> 1; v.push_back( W<n>(&a[0]) );       // max
    for( int i = 0; i < n; ++i )                                // each limb boundary
    {
        std::array<UInt64,n> b {};
        b[i] = UInt64(1) << 63;                                 // i == n-1: the minimum
        v.push_back( W<n>(&b[0]) );
        b[i] = ~UInt64(0);
        v.push_back( W<n>(&b[0]) );
    }
    return v;
}

// ---------------------------------------------------------------------------
// Bitwise NOT, negation, equality, ZeroQ -- one limb count.
// ---------------------------------------------------------------------------

template<int n>
static void CheckBitwiseAndNegate( int reps )
{
    const BigInt mod = BigInt(1) << (64 * n);
    const BigInt min = -(BigInt(1) << (64 * n - 1));

    auto one = [&]( const W<n> & x, const char * tag )
    {
        const BigInt X = ToBig(x);

        // ~x: every limb complemented, i.e. -x - 1.
        const W<n> nx = ~x;
        bool limbs_ok = true;
        for( int i = 0; i < n; ++i ) { limbs_ok &= ( nx[i] == ~x[i] ); }
        check( limbs_ok, std::string(tag) + " ~x limbs, x=" + LimbStr(x) + " got " + LimbStr(nx) );
        check( ToBig(nx) == -X - 1, std::string(tag) + " ~x == -x-1, x=" + Str(X) );

        // -x and in-place Negate, modulo 2^(64n) (the minimum is its own negative).
        BigInt expect = -X;
        if( expect >= -min ) { expect -= mod; }
        const W<n> mx = -x;
        check( ToBig(mx) == expect, std::string(tag) + " -x, x=" + Str(X) + " got " + Str(ToBig(mx)) );
        W<n> y = x;
        y.Negate();
        check( ToBig(y) == expect, std::string(tag) + " Negate(), x=" + Str(X) + " got " + Str(ToBig(y)) );
        if( X != 0 && X != min )
        {
            check( NegativeQ(mx) == !NegativeQ(x),
                   std::string(tag) + " sign flips under -, x=" + Str(X) );
        }
        check( -(-x) == x, std::string(tag) + " -(-x) == x, x=" + Str(X) );

        // == and ZeroQ.
        check( x == W<n>(x), std::string(tag) + " x == copy, x=" + Str(X) );
        check( ZeroQ(x) == (X == 0), std::string(tag) + " ZeroQ, x=" + Str(X) );
        for( int i = 0; i < n; ++i )
        {
            W<n> z = x;
            z[i] ^= (UInt64(1) << (i * 7 % 64));
            check( !(x == z), std::string(tag) + " x != x^bit, limb " + std::to_string(i) );
            check( ZeroQ(z) == (ToBig(z) == 0), std::string(tag) + " ZeroQ(x^bit), limb " + std::to_string(i) );
        }
    };

    for( const auto & x : EdgeValues<n>() ) { one(x, "edge"); }
    for( int r = 0; r < reps; ++r )         { one(RandomW<n>(), "rand"); }
}

// ---------------------------------------------------------------------------
// The arithmetic Prosector_Int actually does on 3D input.
// ---------------------------------------------------------------------------

static void CheckLongArithmetic( int reps )
{
    for( int r = 0; r < reps; ++r )
    {
        const Int64  a = RandomInt64();
        const Int64  b = RandomInt64();
        const Int64  c = RandomInt64();
        const Int128 B = RandomInt128();
        const Int128 D = RandomInt128();

        // Int64 x Int64 -> native Int128 (LInt in Prosector_Int).
        const auto ab = long_mul(a, b);
        check( ToBig(ab) == BigInt(a) * BigInt(b),
               "long_mul(Int64,Int64), a=" + std::to_string(a) + " b=" + std::to_string(b) );
        const auto abc = long_fma(a, b, c);
        check( ToBig(abc) == BigInt(a) * BigInt(b) + BigInt(c),
               "long_fma(Int64,Int64,Int64)" );

        // Int64 x Int128 -> WideInt<3>: long_mul(WideInt<1>, WideInt<2>) -> long_fma.
        // This is the call that aborted (Prosector_Int's det_3).
        const auto aB = long_mul(a, B);
        check( ToBig(aB) == BigInt(a) * ToBig(B),
               "long_mul(Int64,Int128), a=" + std::to_string(a) + " B=" + Str(ToBig(B))
               + " got " + Str(ToBig(aB)) );

        // det_3-shaped sum of three such products.
        const auto det = long_mul(a, B) + long_mul(b, D) + long_mul(c, B);
        check( ToBig(det) == BigInt(a) * ToBig(B) + BigInt(b) * ToBig(D) + BigInt(c) * ToBig(B),
               "det_3-shaped sum" );

        // Int128 x Int128 -> WideInt<4> (LLInt).
        const auto BD = long_mul(B, D);
        check( ToBig(BD) == ToBig(B) * ToBig(D),
               "long_mul(Int128,Int128), B=" + Str(ToBig(B)) + " D=" + Str(ToBig(D)) );
    }
}

int main()
{
    section("bitwise + negate, 1 limb");  CheckBitwiseAndNegate<1>(2000);
    section("bitwise + negate, 2 limbs"); CheckBitwiseAndNegate<2>(2000);
    section("bitwise + negate, 3 limbs"); CheckBitwiseAndNegate<3>(2000);
    section("bitwise + negate, 4 limbs"); CheckBitwiseAndNegate<4>(2000);
    section("long_mul / long_fma");       CheckLongArithmetic(20000);

    std::printf("\n%s (%d checks, %d failed)\n",
                fails == 0 ? "WIDEINT CHECK OK" : "WIDEINT CHECK FAILED",
                checks, fails);
    return fails == 0 ? 0 : 1;
}
