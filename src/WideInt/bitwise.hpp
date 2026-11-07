public:

// CAUTION: Do not read `limbs` through `As_BitSet` (a `reinterpret_cast` to `std::bitset`).
// That is a strict-aliasing violation: gcc -O3 assumes a `std::bitset` load cannot see a
// preceding `WideInt` store and reorders them. `Negate()` (= `~` then `++`) then worked on
// stale limbs, and every 3D input tripped `assert(!NegativeQ(b))` in `long_fma_unsigned`.
// The loops over limbs below compile to the same few instructions.

/*!@brief Test for equality.*/
TOOLS_FORCE_INLINE constexpr friend bool operator==( const This_T & a, const This_T & b )
{
    // For the equality operator it may be better to start at the bottom as leading bits are often 0.
    for( Idx i = 0; i < limb_count; ++i )
    {
        if( a[i] != b[i] ) { return false; }
    }

    return true;
}

///*!@brief Check whether this wide integer is zero.*/
//TOOLS_FORCE_INLINE constexpr bool ZeroQ() const
//{
//    constexpr BitSet_T zero;   // Fills with zeroes; constexpr.
//    return As_BitSet(*this) == zero;
//}

/*!@brief Check whether this wide integer is zero.*/
TOOLS_FORCE_INLINE constexpr friend
bool ZeroQ( cref<WideInt> a )
{
    for( Idx i = 0; i < limb_count; ++i )
    {
        if( a[i] != zero_limb ) { return false; }
    }

    return true;
}

/*!@brief Bitwise NOT-operation.*/
TOOLS_FORCE_INLINE constexpr friend This_T operator~( cref<This_T> a )
{
    This_T b;

    for( Idx i = 0; i < limb_count; ++i )
    {
        b[i] = static_cast<Limb_T>(~a[i]);
    }

    return b;
}

/*!@brief Bitwise AND-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr This_T & operator&=( cref<This_T> a )
{
    As_BitSet(*this) &= As_BitSet(a);
    return *this;
}

/*!@brief Bitwise OR-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr This_T & operator|=( cref<This_T> a )
{
    As_BitSet(*this) |= As_BitSet(a);
    return *this;
}

/*!@brief Bitwise XOR-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr This_T operator^=( cref<This_T> a )
{
    As_BitSet(*this) ^= As_BitSet(a);
    return *this;
}

/*!@brief Bitwise AND-operation.*/
TOOLS_FORCE_INLINE constexpr friend This_T operator&( cref<This_T> a, cref<This_T> b )
{
    return This_T(As_BitSet_T(a) & As_BitSet(b));
}

/*!@brief Bitwise OR-operation.*/
TOOLS_FORCE_INLINE constexpr friend This_T operator|( cref<This_T> a, cref<This_T> b )
{
    return This_T(As_BitSet_T(a) | As_BitSet(b));
}

/*!@brief Bitwise XOR-operation.*/
TOOLS_FORCE_INLINE constexpr friend This_T operator^( cref<This_T> a, cref<This_T> b )
{
    return This_T(As_BitSet_T(a) ^ As_BitSet(b));
}
