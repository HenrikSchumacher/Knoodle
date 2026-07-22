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
static PDC_T FromCoordinates( cptr<Real> x, const ExtInt n )
{
    return PDC_T( PD_T::FromCoordinates(x,n) );
}

template<typename Real, IntQ ExtInt>
static PDC_T FromCoordinatesAndEdges( cptr<Real> x, cptr<ExtInt> edges, const ExtInt n )
{
    return PDC_T( PD_T::FromCoordinatesAndEdges(x,edges,n) );
}

template<IntQ ExtInt, IntQ ExtInt2>
static PDC_T FromLinkEmbedding_Raw(
    const ExtInt  component_count_,
    cptr<ExtInt>  component_ptr,
    cptr<ExtInt>  component_color,
    const ExtInt  crossing_count_,
    cptr<ExtInt>  edge_ptr,
    cptr<ExtInt>  edge_intersections,
    cptr<ExtInt2> edge_state
)
{
    return PDC_T( PD_T::FromLinkEmbedding_Raw(
        component_count_,
        component_ptr,
        component_color,
        crossing_count_,
        edge_ptr,
        edge_intersections,
        edge_state
    ) );
}
