public:

/*!@brief Test for equality.*/
TOOLS_FORCE_INLINE constexpr friend bool operator==( const This_T & a, const This_T & b )
{
    return As_BitSet(a) == As_BitSet(b);
    
//            if( &a == &b ) { return true; }
//
//            // For the equality operator it may be better to start at the bottom as leading bits are often 0.
//            for( Idx i = 0; i < limb_count; ++i )
//            {
//                if( a[i] != b[i] ) { return false; }
//            }
//
//            return true;
}

/*!@brief Check whether this wide integer is zero.*/
TOOLS_FORCE_INLINE constexpr bool ZeroQ() const
{
//    for( Idx k = 0; k < limb_count; ++k )
//    {
//        if( limbs[k] != zero_limb ) { return false; }
//    }
    constexpr BitSet_T zero;   // Fills with zeroes; constexpr.
    return As_BitSet(*this) == zero;
}

/*!@brief Bitwise NOT-operation; in-place.*/
TOOLS_FORCE_INLINE constexpr This_T operator~()
{
    BitSet_T b = ~As_BitSet(*this);
    return This_T(b);
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
