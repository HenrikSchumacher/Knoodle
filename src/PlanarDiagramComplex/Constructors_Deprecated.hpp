public:


/*!@brief Initialize from a `LinkEmbedding_Float`, taking ownership.*/
template<typename Real, typename BReal>
explicit PlanarDiagramComplex( LinkEmbedding_Float<Real,Int,BReal> && L )
:   PlanarDiagramComplex( PD_T::FromLinkEmbedding(L) )
{}

/*!@brief Initialize from a `LinkEmbedding_Float`.*/
template<typename Real, typename BReal>
explicit PlanarDiagramComplex( LinkEmbedding_Float<Real,Int,BReal> & L )
:   PlanarDiagramComplex( PD_T::FromLinkEmbedding(L) )
{}

/*!@brief Initialize from a `KnotEmbedding_Float`, taking ownership.*/
template<typename Real, typename BReal>
explicit PlanarDiagramComplex( KnotEmbedding_Float<Real,Int,BReal> && K  )
:   PlanarDiagramComplex( PD_T::FromKnotEmbedding(K) )
{}

/*!@brief Initialize from a `KnotEmbedding_Float`.*/
template<typename Real, typename BReal>
explicit PlanarDiagramComplex( KnotEmbedding_Float<Real,Int,BReal> & K )
:   PlanarDiagramComplex( PD_T::FromKnotEmbedding(K) )
{}
