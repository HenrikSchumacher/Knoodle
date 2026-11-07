public:

// CAUTION: Do not read `limbs` through `As_BitSet` (a `reinterpret_cast` to `std::bitset`).
// That is a strict-aliasing violation: gcc -O3 assumes a `std::bitset` load cannot see a
// preceding `WideInt` store and reorders them. `Negate()` (= `~` then `++`) then worked on
// stale limbs, and every 3D input tripped `assert(!NegativeQ(b))` in `long_fma_unsigned`.
// The loops over limbs below compile to the same few instructions.

// It is tempting to use std::bitset here to vectorize bitwise operations, but compilers are clever enough to see that these loop operations are meant to perform bitwise operations and to optimize accordingly.

/*!@brief Test for equality.*/
TOOLS_FORCE_INLINE constexpr friend bool operator==( const WideInt & a, const WideInt & b )
{
    // We start comparison at the least significant limb `a[0]` because leading limbs may often be 0.
    // Alas, the compiler typically transforms this into byte-wise memory comparison without early exit.
    for( Idx i = 0; i < limb_count; ++i )
    {
        if( a[i] != b[i] ) { return false; }
    }

    return true;
}

/*!@brief Check whether this wide integer is zero.*/
TOOLS_FORCE_INLINE constexpr friend
bool ZeroQ( const WideInt & a )
{
    for( Idx i = 0; i < limb_count; ++i )
    {
        if( a[i] != zero_limb ) { return false; }
    }
    return true;
}

/*!@brief Bitwise NOT-operation.*/
TOOLS_FORCE_INLINE constexpr friend WideInt operator~( const WideInt & a )
{
    WideInt b;
    for( Idx i = 0; i < limb_count; ++i )
    {
        b[i] = static_cast<Limb_T>(~a[i]);
    }
    return b;
}

/*!@brief Bitwise AND-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr WideInt & operator&=( const WideInt & a )
{
    for( Idx i = 0; i < limb_count; ++i )
    {
        limbs[i] &= a[i];
    }
    return *this;
}

/*!@brief Bitwise OR-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr WideInt & operator|=( const WideInt & a )
{
    for( Idx i = 0; i < limb_count; ++i )
    {
        limbs[i] |= a[i];
    }
    return *this;
}

/*!@brief Bitwise XOR-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr WideInt operator^=( const WideInt & a )
{
    for( Idx i = 0; i < limb_count; ++i )
    {
        limbs[i] ^= a[i];
    }
    return *this;
}

/*!@brief Bitwise AND-operation.*/
TOOLS_FORCE_INLINE constexpr friend WideInt operator&( const WideInt & a, const WideInt & b )
{
    WideInt c;
    for( Idx i = 0; i < limb_count; ++i )
    {
        c[i] = a[i] & b[i];
    }
    return c;
}

/*!@brief Bitwise OR-operation.*/
TOOLS_FORCE_INLINE constexpr friend WideInt operator|( const WideInt & a, const WideInt & b )
{
    WideInt c;
    for( Idx i = 0; i < limb_count; ++i )
    {
        c[i] = a[i] | b[i];
    }
    return c;
}

/*!@brief Bitwise XOR-operation.*/
TOOLS_FORCE_INLINE constexpr friend WideInt operator^( const WideInt & a, const WideInt & b )
{
    WideInt c;
    for( Idx i = 0; i < limb_count; ++i )
    {
        c[i] = a[i] ^ b[i];
    }
    return c;
}
