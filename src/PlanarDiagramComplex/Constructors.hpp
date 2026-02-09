public:

template<bool PDsignedQ, bool checksQ = true, typename ExtInt, typename ExtInt2>
static PDC_T FromPDCode(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const bool proven_minimalQ_ = false,
    const bool compressQ = false
)
{
    return PDC_T( PD_T::template FromPDCode<PDsignedQ,checksQ>(
        pd_codes_,crossing_count_,proven_minimalQ_,compressQ
    ));
}

template<typename T, typename ExtInt>
static PDC_T FromMacLeodCode(
    cptr<T>       s_mac_leod,
    const ExtInt  crossing_count_,
    const bool    proven_minimalQ_ = false,
    const bool    compressQ = false
)
{
    return PDC_T(
        PD_T::FromMacLeodCode(s_mac_leod,crossing_count_,proven_minimalQ_,compressQ)
    );
}

template<typename T, typename ExtInt>
static PDC_T FromExtendedGaussCode(
    cptr<T>      gauss_code,
    const ExtInt arc_count_,
    const bool   proven_minimalQ_ = false,
    const bool   compressQ = false
)
{
    return PDC_T(
        PD_T::FromExtendedGaussCode(gauss_code,arc_count_,proven_minimalQ_,compressQ)
    );
}

template<typename Real, typename BReal>
static PDC_T FromKnotEmbedding(
    cref<Knot_2D<Real,Int,BReal>> K, const bool compressQ = false
)
{
    return PDC_T( PD_T::FromKnotEmbedding(K,compressQ) );
}

template<typename Real, typename ExtInt>
static PDC_T FromKnotEmbedding(
    cptr<Real> x, const ExtInt n, const bool compressQ = false
)
{
    return PDC_T( PD_T::FromKnotEmbedding(x,n,compressQ) );
}

template<typename Real, typename BReal>
static PDC_T FromLinkEmbedding(
    cref<LinkEmbedding<Real,Int,BReal>> L, const bool compressQ = false
)
{
    return PDC_T( PD_T::FromLinkEmbedding(L,compressQ) );
}

template<typename Real, typename ExtInt>
static PDC_T FromLinkEmbedding(
    cptr<Real> x, cptr<ExtInt> edges, const ExtInt n, const bool compressQ = false
)
{
    return PDC_T( PD_T::FromLinkEmbedding(x,edges,n,compressQ) );
}
