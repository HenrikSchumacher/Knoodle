public:

/*!@brief Long multiply routine that computes `r = a * d - b * c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs.*/
template<int other_limb_count>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+other_limb_count,Limb_T,Comp_T,signQ>
long_det(
    cref<WideInt> a,
    cref<WideInt> b,
    cref<WideInt<other_limb_count,Limb_T,Comp_T,signQ>> c,
    cref<WideInt<other_limb_count,Limb_T,Comp_T,signQ>> d
)
{
    return long_mul(a,d) - long_mul(b,c);
}
