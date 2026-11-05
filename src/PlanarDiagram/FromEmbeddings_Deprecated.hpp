/*!@brief Construction from `KnotEmbedding` object.
 *
 * @param K The input diagram.
 */

template<FloatQ Real, FloatQ BReal>
static std::pair<PD_T,Tensor1<Int,Int>> FromKnotEmbedding( mref<KnotEmbedding_Float<Real,Int,BReal>> K )
{
    using Knot_T [[maybe_unused]] = KnotEmbedding_Float<Real,Int,BReal>;
    
    constexpr auto tag = MethodName("FromKnotEmbedding") + "(" + Knot_T::ClassName() + ")";
    
    TOOLS_PTIMER(timer,tag);

    Tensor1<Int,Int> comp_color(Int{1},Int{0});
    
    const int err = K.RequireIntersections();
    
    if( err != 0 )
    {
        eprint(tag, ": RequireIntersections reported error code ", err, ". Returning invalid diagram.");
        return { PD_T::InvalidDiagram(), Tensor1<Int,Int>() };
    }
    
    return FromLinkEmbedding_Raw(
        K.ComponentCount(),
        K.ComponentPointers().data(),
        comp_color.data(),
        K.IntersectionCount(),
        K.EdgePointers().data(),
        K.EdgeCrossings().data()
    );
}

/*!@brief Construction from `LinkEmbedding_Float` object. Returns a planar diagram and the number of unlinks found in the input.
 */

template<FloatQ Real, FloatQ BReal>
static std::pair<PD_T,Tensor1<Int,Int>> FromLinkEmbedding( mref<LinkEmbedding_Float<Real,Int,BReal>> L )
{
    using Link_T [[maybe_unused]] = LinkEmbedding_Float<Real,Int,BReal>;
 
    constexpr auto tag = MethodName("FromLinkEmbedding(") + Link_T::ClassName() + ")";
    
    TOOLS_PTIMER(timer,tag);

    const int err = L.template RequireIntersections<true>();

    if( err != 0 )
    {
        eprint(tag, ": RequireIntersections reported error code ", err, ". Returning invalid diagram.");
        return { PD_T::InvalidDiagram(), Tensor1<Int,Int>() };
    }
    
    return FromLinkEmbedding_Raw(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.ComponentColors().data(),
        L.IntersectionCount(),
        L.EdgePointers().data(),
        L.EdgeCrossings().data()
    );
}
