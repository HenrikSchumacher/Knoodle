public:

template<typename T, IntQ ExtInt, IntQ ExtInt2>
static PDC_T FromMacLeodCode(
    cptr<T>       s_mac_leod,
    const ExtInt  crossing_count_,
    const ExtInt2 color,
    const bool    proven_minimalQ_ = false
)
{
    return PDC_T(
        PD_T::FromMacLeodCode(s_mac_leod,crossing_count_,color,proven_minimalQ_)
    );
}

template<typename T, IntQ ExtInt, IntQ ExtInt2>
static PDC_T FromExtendedGaussCode(
    cptr<T>       gauss_code,
    const ExtInt  arc_count_,
    const ExtInt2 color,
    const bool    proven_minimalQ_ = false
)
{
    return PDC_T(
        PD_T::FromExtendedGaussCode(gauss_code,arc_count_,color,proven_minimalQ_)
    );
}

template<typename Real, IntQ ExtInt>
static PDC_T FromKnotEmbedding( cptr<Real> x, const ExtInt n )
{
    return PDC_T( PD_T::FromKnotEmbedding(x,n) );
}

template<typename Real, IntQ ExtInt>
static PDC_T FromLinkEmbedding( cptr<Real> x, cptr<ExtInt> edges, const ExtInt n )
{
    return PDC_T( PD_T::FromLinkEmbedding(x,edges,n) );
}
