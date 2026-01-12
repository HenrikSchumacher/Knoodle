public:

template<bool PDsignedQ, bool checksQ = true, typename ExtInt, typename ExtInt2>
static PDC_T FromPDCode(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const bool compressQ,
    const bool proven_minimalQ_
)
{
    return PDC_T( PD_T::template FromPDCode<PDsignedQ,checksQ>(
        pd_codes_,crossing_count_,compressQ,proven_minimalQ_
    ));
}

template<typename T, typename ExtInt>
static PDC_T FromMacLeodCode(
    cptr<T>       s_mac_leod,
    const ExtInt  crossing_count_,
    const bool    compressQ = false,
    const bool    proven_minimalQ_ = false
)
{
    return PDC_T( PD_T::FromMacLeodCode(s_mac_leod,crossing_count_,compressQ,proven_minimalQ_) );
}

template<typename T, typename ExtInt>
static PDC_T FromExtendedGaussCode(
    cptr<T>      gauss_code,
    const ExtInt arc_count_,
    const bool   compressQ = true,
    const bool   proven_minimalQ_ = false
)
{
    return PDC_T( PD_T::FromExtendedGaussCode(gauss_code,arc_count_,compressQ,proven_minimalQ_) );
}

template<typename Real, typename BReal>
static PDC_T FromKnotEmbedding( cref<Knot_2D<Real,Int,BReal>> K )
{
    return PDC_T( PD_T::FromKnotEmbedding(K) );
}

template<typename Real, typename ExtInt>
static PDC_T FromKnotEmbedding( cptr<Real> x, const ExtInt n )
{
    return PDC_T( PD_T::FromKnotEmbedding(x,n) );
}

template<typename Real, typename BReal>
static PDC_T FromLinkEmbedding( cref<Link_2D<Real,Int,BReal>> L )
{
    return PDC_T( PD_T::FromLinkEmbedding(L) );
}

template<typename Real, typename ExtInt>
static PDC_T FromLinkEmbedding( cptr<Real> x, cptr<ExtInt> edges, const ExtInt n )
{
    return PDC_T( PD_T::FromLinkEmbedding(x,edges,n) );
}
