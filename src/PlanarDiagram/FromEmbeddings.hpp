
/*!@brief Construction from `KnotEmbedding` object.
 *
 * @param K The input diagram.
 */

template<FloatQ Real, FloatQ BReal>
static std::pair<PD_T,Tensor1<Int,Int>> FromKnotEmbedding( mref<KnotEmbedding<Real,Int,BReal>> K )
{
    using Knot_T [[maybe_unused]] = KnotEmbedding<Real,Int,BReal>;
    
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

/*!@brief Construction from the coordinates of a polygonal curve in 3-space.
 *
 * @param x An array of size `n * 3` storing the coordinates of the curve. The curve is implicitly assumed to be closed, i.e., an edge from the last point to the first one is automatically added.
 *
 * @param n The number of vertices.
 */

template<FloatQ Real, IntQ ExtInt, FloatQ BReal = float>
static std::pair<PD_T,Tensor1<Int,Int>> FromCoordinates( cptr<Real> x, const ExtInt n )
{
    constexpr auto tag = MethodName("FromCoordinates") + "<" + TypeName<Real> + "," + TypeName<ExtInt> + ">";
    
    TOOLS_PTIMER(timer,tag);

    KnotEmbedding<Real,Int,BReal> L ( n );

    L.ReadVertexCoordinates(x);

    const int err = L.template FindIntersections<true>();

    if( err != 0 )
    {
        eprint(tag, ": FindIntersections reported error code ", err, ". Returning invalid diagram.");
        return { PD_T::InvalidDiagram(), Tensor1<Int,Int>() };
    }

    // Deallocate tree-related data in L to make room for the PD_T.
    L.DeleteTree();
    
    Tensor1<Int,Int> comp_color(Int{1},Int{0});

    // We delay the allocation until substantial parts of L have been deallocated.
    return FromLinkEmbedding_Raw(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        comp_color.data(),
        L.IntersectionCount(),
        L.EdgePointers().data(),
        L.EdgeCrossings().data()
    );
}

/*!@brief Construction from `FromLinkEmbedding` object. Returns a planar diagram and the number of unlinks found in the input.
 */

template<FloatQ Real, FloatQ BReal>
static std::pair<PD_T,Tensor1<Int,Int>> FromLinkEmbedding( mref<LinkEmbedding<Real,Int,BReal>> L )
{
    using Link_T [[maybe_unused]] = LinkEmbedding<Real,Int,BReal>;
 
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

/*!@brief Construction from `FromLinkEmbedding_Int` object. Returns a planar diagram and the number of unlinks found in the input.
 */
template<typename Real, typename Prosector_T, bool mortonQ>
static std::pair<PD_T,Tensor1<Int,Int>> FromLinkEmbedding(
    mref<LinkEmbedding_Int<Real,Prosector_T,mortonQ>> L
)
{
    using Link_T [[maybe_unused]] = LinkEmbedding_Int<Real,Prosector_T,mortonQ>;
    
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



/*!@brief Construction from the coordinates of a polygonal curve in 3-space.
 *
 * @param x An array of size `n * 3` storing the coordinates of the curve.
 *
 * @param edges An array of size `n * 2` storing the indices of the edges in interleaved form. It is the user's responsibility to guarantee that these edges describe a one-dimensional simplicial complex in which each vertes has in-degree 1 and out-degree 1.
 *
 * @param n The number of vertices = the number of edges.
 */

/*!@brief Construction from coordinates and edges. Returns a planar diagram and the number of unlinks found in the input.
 */

template<FloatQ Real, IntQ ExtInt>
static std::pair<PD_T,Tensor1<Int,Int>> FromCoordinatesAndEdges(
    cptr<Real> x,
    cptr<ExtInt> edges,
    const ExtInt n
)
{
    constexpr auto tag = MethodName("FromCoordinatesAndEdges") + "<" + TypeName<Real> + "," + TypeName<ExtInt> + ">";
    
    TOOLS_PTIMER(timer,tag);

    using LinkEmbedding_T = LinkEmbedding<Real,Int,Real>;
    
    ExtInt * edge_colors = nullptr;

    LinkEmbedding_T L ( edges, edge_colors, n );

    L.ReadVertexCoordinates(x);

    const int err = L.template FindIntersections<true>();

    if( err != 0 )
    {
        eprint(tag, ": FindIntersections reported error code ", err, ". Returning invalid diagram.");
        return { PD_T::InvalidDiagram(), Tensor1<Int,Int>() };
    }

    // Deallocate tree-related data in L to make room for the planar diagram.
    L.DeleteTree();

    // We delay the allocation until substantial parts of L have been deallocated.
    return FromLinkEmbedding_Raw(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.ComponentColors().data(),
        L.IntersectionCount(),
        L.EdgePointers().data(),
        L.EdgeCrossings().data()
    );
}

template<FloatQ Real, IntQ ExtInt>
[[deprecated("This is a misnomer; renamed to `FromCoordinatesAndEdges`.")]]
static std::pair<PD_T,Tensor1<Int,Int>> FromLinkEmbedding(
    cptr<Real> x,
    cptr<ExtInt> edges,
    const ExtInt n
)
{
    return FromCoordinatesAndEdges(x, edges, n);
}


public:
    
/*!@brief For internal use only. Users should not call this. Testing makes it necessary to make this public.
 */
template<IntQ ExtInt>
static std::pair<PD_T,Tensor1<Int,Int>> FromLinkEmbedding_Raw(
    const ExtInt component_count_,
    cptr<ExtInt> component_ptr,
    cptr<ExtInt> component_color,
    const ExtInt crossing_count_,
    cptr<ExtInt> edge_ptr,
    cptr<EdgeCrossing<ExtInt>> edge_cross
)
{
    TOOLS_PTIMER(timer,MethodName("FromLinkEmbedding_Raw"));
    // needs to know all member variables
    
    using EdgeCrossing_T = EdgeCrossing<ExtInt>;
    
    if( component_count_ <= ExtInt{0} )
    {
        return { InvalidDiagram(), Tensor1<Int,Int>() };
    }
    
    if( (crossing_count_ <= ExtInt{0}) && (component_count_ >= ExtInt{1}) )
    {
        return { InvalidDiagram(), Tensor1<Int,Int>(component_color,component_count_) };
    }
    
    const Int component_count = int_cast<Int>( component_count_ );
    const Int crossing_count  = int_cast<Int>( crossing_count_  );
    
    PD_T pd ( crossing_count, true );
    
    pd.crossing_count = crossing_count;
    pd.arc_count      = Int{2} * pd.crossing_count;
    
#ifdef PD_ALLOCATE_SCRATCH
    pd.C_scratch.Fill(Uninitialized);
    mptr<Int> C_label = pd.C_scratch.data();
#else
    Tensor1<Int,Int> C_label_buffer (crossing_count,Uninitialized);
    mptr<Int> C_label = C_label_buffer.data();
#endif
    
    Aggregator<Int,Int> anello_colors;
    Int C_counter = 0;
    
    // Now we go through all components
    //      then through all edges of the component
    //      then through all intersections of the edge
    // and generate new vertices, edges, crossings, and arcs in one go.
    
    ColorCounts_T color_arc_counts;
    
    // We put the anellos at the back so that it is easier to communicate with PlanarDiagramComplex.
    
    for( Int comp = 0; comp < component_count; ++comp )
    {
        // The range of arcs belonging to this component.
        const Int b_begin = edge_ptr[component_ptr[comp  ]];
        const Int b_end   = edge_ptr[component_ptr[comp+1]];
        
        const Int color = (component_color == nullptr) ? comp : static_cast<Int>(component_color[comp]);
        
        if( b_begin == b_end )
        {
            // Component is an unlink. Just count it.
            anello_colors.Push(color);
            continue;
        }

        // If we arrive here, then there is definitely a crossing in the first edge.
        for( Int b = b_begin, a = b_end-Int{1}; b < b_end; a = (b++) )
        {
            EdgeCrossing_T ec = edge_cross[b];
            
            const Int c_pos = static_cast<Int>(ec.Index());
            
            if( !ValidIndexQ(C_label[c_pos]) )
            {
                C_label[c_pos] = C_counter++;
            }
            
            const Int  c            = C_label[c_pos];
            const bool overQ        = ec.OverQ();
            const bool righthandedQ = ec.RightHandedQ();
            
            pd.A_cross(a,Head) = c; // c is head of a
            pd.A_cross(b,Tail) = c; // c is tail of b

            /*
             *
             *    negative         positive
             *    right-handed     left-handed
             *    .       .        .       .
             *    .       .        .       .
             *    O       O        O       O
             *     ^     ^          ^     ^
             *      \   /            \   /
             *       \ /              \ /
             *        /                \
             *       / \              / \
             *      /   \            /   \
             *     /     \          /     \
             *    O       O        O       O
             *    .       .        .       .
             *    .       .        .       .
             *
             */
            
            const CrossingState_T c_state = BooleanToCrossingState(righthandedQ);
            pd.C_state[c] = c_state;
            
            /*
            * righthandedQ == true and overQ == true:
            *
            *         C_arcs(c,Out,Left) |       | C_arcs(c,Out,Right) = b
            *                            |       |
            *                            O       O
            *                             ^     ^
            *                              \   /
            *                               \ /
            *                                /
            *                               / \
            *                              /   \
            *                             /     \
            *                            O       O
            *                            |       |
            *      a = C_arcs(c,In,Left) |       | C_arcs(c,In,Right)
            */
            
            const bool a_side = (righthandedQ == overQ) ? Left : Right ;
            
            pd.C_arcs(c,In , a_side) = a;
            pd.C_arcs(c,Out,!a_side) = b;
            
            pd.A_state[b] = ArcState_T::Active;
            pd.A_color[b] = color;
        }
        
        color_arc_counts[color] = b_end - b_begin;
    }
    
    pd.template SetCache<false>("LinkComponentCount",component_count - anello_colors.Size());
    
    pd.template SetCache<false>("ColorArcCounts",std::move(color_arc_counts));
    
    return { pd, anello_colors.Disband() };
}

template<IntQ ExtInt>
static std::pair<PD_T,Tensor1<Int,Int>> FromLinkEmbedding_Raw(
    const ExtInt component_count_,
    cptr<ExtInt> component_ptr,
    cptr<ExtInt> component_color,
    const ExtInt crossing_count_,
    cptr<ExtInt> edge_ptr,
    cptr<ExtInt> edge_cross
)
{
    return FromLinkEmbedding_Raw(
        component_count_,
        component_ptr,
        component_color,
        crossing_count_,
        edge_ptr,
        reinterpret_cast<const EdgeCrossing<ExtInt> *>(edge_cross)
    );
}
