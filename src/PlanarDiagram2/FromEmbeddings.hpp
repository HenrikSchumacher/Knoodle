/*!@brief Construction from `Knot_2D` object.
 *
 * Caution: This assumes that `Knot_2D::FindIntersections` has been called already!
 */

template<typename Real, typename BReal>
static PD_T FromKnotEmbedding(
    cref<Knot_2D<Real,Int,BReal>> K,
    const bool compressQ = false // TODO: Is this meaningful?
)
{
    static_assert(FloatQ<Real>,"");
    static_assert(FloatQ<BReal>,"");
    
    using Knot_T [[maybe_unused]] = Knot_2D<Real,Int,BReal>;
    
    TOOLS_PTIMER(timer,MethodName("FromKnotEmbedding")+"("+Knot_T::ClassName()+")");

    return PD_T::template FromLink<Real,BReal>(
        K.ComponentCount(),
        K.ComponentPointers().data(),
        K.EdgePointers().data(),
        K.EdgeIntersections().data(),
        K.EdgeOverQ().data(),
        K.Intersections(),
        compressQ
    ).first;
}

/*! @brief Construction from coordinates.
 */

template<typename Real, typename ExtInt>
static PD_T FromKnotEmbedding(
    cptr<Real> x,
    const ExtInt n,
    const bool compressQ = false // TODO: Is this meaningful?
)
{
    static_assert(FloatQ<Real>,"");
    static_assert(IntQ<ExtInt>,"");
    
    TOOLS_PTIMER(timer,MethodName("FromKnotEmbedding") + "("+TypeName<Real>+"*,"+TypeName<ExtInt>+")");

    Knot_2D<Real,Int,Real> L ( n );

    L.ReadVertexCoordinates(x);

    int err = L.template FindIntersections<true>();

    if( err != 0 )
    {
        eprint(MethodName("FromKnotEmbedding") + "("+TypeName<Real>+"*,"+TypeName<ExtInt>+"): FindIntersections reported error code " + ToString(err) + ". Returning invalid planar diagram.");
        return PD_T::InvalidDiagram();
    }

    // Deallocate tree-related data in L to make room for the PD_T.
    L.DeleteTree();

    // We delay the allocation until substantial parts of L have been deallocated.
    return PD_T::template FromLink<Real,Real>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections(),
        compressQ
    ).first;
}

/*!@brief Construction from `Link_2D` object. Returns a planar diagram and the number of unlinks found in the input.
 *
 * Caution: This assumes that `Link_2D::FindIntersections` has been called already!
 */

template<typename Real, typename BReal>
static std::pair<PD_T,Int> FromLinkEmbedding(
    cref<Link_2D<Real,Int,BReal>> L,
    const bool compressQ = false // TODO: Is this meaningful?
)
{
    static_assert(FloatQ<Real>,"");
    static_assert(FloatQ<BReal>,"");
    
    using Link_T [[maybe_unused]] = Link_2D<Real,Int,BReal>;
    
    TOOLS_PTIMER(timer,MethodName("FromLinkEmbedding")+"("+Link_T::ClassName()+")");

    return PD_T::template FromLink<Real,BReal>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections(),
        compressQ
    );
}


/*! @brief Construction from coordinates and edges. Returns a planar diagram and the number of unlinks found in the input.
 */

template<typename Real, typename ExtInt>
static std::pair<PD_T,Int> FromLinkEmbedding(
    cptr<Real> x,
    cptr<ExtInt> edges,
    const ExtInt n,
    const bool compressQ = false // TODO: Is this meaningful?
)
{
    static_assert(FloatQ<Real>,"");
    static_assert(IntQ<ExtInt>,"");
    
    TOOLS_PTIMER(timer,MethodName("FromLinkEmbedding") + "("+TypeName<Real>+"*,"+TypeName<ExtInt>+"*,"+TypeName<ExtInt>+")");

    using Link_T = Link_2D<Real,Int,Real>;

    Link_T L ( edges, n );

    L.ReadVertexCoordinates(x);

    int err = L.template FindIntersections<true>();

    if( err != 0 )
    {
        eprint(MethodName("FromLinkEmbedding") + "("+TypeName<Real>+"*,"+TypeName<ExtInt>+"*,"+TypeName<ExtInt>+") FindIntersections reported error code " + ToString(err) + ". Returning empty PlanarDiagram2.");
        return { PD_T::InvalidDiagram(), Int(0) };
    }

    // Deallocate tree-related data in L to make room for the PlanarDiagram2.
    L.DeleteTree();

    // We delay the allocation until substantial parts of L have been deallocated.
    return PD_T::template FromLink<Real,Real>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections(),
        compressQ
    );
}


private:
    
template<typename Real, typename BReal>
static std::pair<PD_T,Int> FromLink(
    const Int  component_count,
    cptr<Int>  component_ptr,
    cptr<Int>  edge_ptr,
    cptr<Int>  edge_intersections,
    cptr<bool> edge_overQ,
    cref<std::vector<typename Link_2D<Real,Int,BReal>::Intersection_T>> intersections,
    const bool compressQ = false // TODO: Is this meaningful?
)
{
    // needs to know all member variables
    
    static_assert(FloatQ<Real>,"");
    static_assert(FloatQ<BReal>,"");
    
    using Intersection_T = typename Link_2D<Real,Int,BReal>::Intersection_T;
    using Sign_T         = typename Intersection_T::Sign_T;
    
    const Int crossing_count = int_cast<Int>(intersections.size());
    
    if( component_count <= Int(0) )
    {
        return { InvalidDiagram(), Int(0) };
    }
    
    if( (crossing_count <= Int(0)) && (component_count >= Int(1)) )
    {
        return { Unknot(0), component_count - Int(1) };
    }
    
    PD_T pd ( crossing_count, true );
    
    pd.crossing_count = crossing_count;
    pd.arc_count      = Int(2) * crossing_count;
    
    pd.C_scratch.Fill(Uninitialized);
    mptr<Int> C_label = pd.C_scratch.data();
    
    Int unlink_count = 0;
    Int C_counter = 0;
    
    // Now we go through all components
    //      then through all edges of the component
    //      then through all intersections of the edge
    // and generate new vertices, edges, crossings, and arcs in one go.
    
    Int color = 0;
    ColorCounts_T color_arc_counts;
    
    // We put the unlinks at the back so that it is easier to communicate with PlanarDiagramComplex.
    
    for( Int comp = 0; comp < component_count; ++comp )
    {
        // The range of arcs belonging to this component.
        const Int b_begin = edge_ptr[component_ptr[comp  ]];
        const Int b_end   = edge_ptr[component_ptr[comp+1]];
        
        if( b_begin == b_end )
        {
            // Component is an unlink. Just count it.
            ++unlink_count;
            continue;
        }

        // If we arrive here, then there is definitely a crossing in the first edge.
        for( Int b = b_begin, a = b_end-Int(1); b < b_end; a = (b++) )
        {
            const Int c_pos = edge_intersections[b];
            
            if( !ValidIndexQ(C_label[c_pos]) )
            {
                C_label[c_pos] = C_counter++;
            }
            
            const Int c = C_label[c_pos];
            
            const bool overQ = edge_overQ[b];
            
            cref<Intersection_T> inter = intersections[static_cast<Size_T>(c_pos)];
            
            pd.A_cross(a,Head) = c; // c is head of a
            pd.A_cross(b,Tail) = c; // c is tail of b
            
            PD_ASSERT( (inter.handedness > Sign_T(0)) || (inter.handedness < Sign_T(0)) );
            
            bool righthandedQ = inter.handedness > Sign_T(0);
            
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
        ++color;
    }
    
    // TODO: Extract LinkComponentArcs, ArcLinkComponents from here (only if needed)?
    // Not so easy to do as we have to ignore the unlinks.
    
    pd.template SetCache<false>("LinkComponentCount",component_count - unlink_count);
    
    pd.SetCache("ColorArcCoumts",std::move(color_arc_counts));
    
    // TODO: Check whether this is really neccessary.
    
    if( compressQ )
    {
        return { pd.template CreateCompressed<false>(), unlink_count };
    }
    else
    {
        return { pd, unlink_count };
    }
}
