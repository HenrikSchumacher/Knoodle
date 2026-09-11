public:

/*!@brief Guarantee that the intersections are computed.
 *
 * @param force_recomputeQ If set to `true`, a recomputation of the intersections is enforced, even if the intersections are already computed. (Probably only useful for benchmarking and debugging.)
 *
 * @return `0` if the intersections have been computed successfully. Anything nonzero if either no vertex coordinates are loaded (check `VertexCoordinatesLoadedQ()`), if some self-intersection of the link in 3-space have been detected (check `IntersectionCount3D()`)), or if some other issue arose. Caution: The precise error flags might be subject to change.
 */
template<bool verboseQ = true> // whether to print errors and warnings
[[nodiscard]] int RequireIntersections( bool force_recomputeQ = false ) const
{
    if( !vertex_coords_loadedQ )
    {
        Msgr::wprint("RequireIntersections","Failed to compute intersections because no vertex coordinates have been loaded, yet.");
        return 1;
    }
    
    if( force_recomputeQ || !intersections_computedQ )
    {
        ComputeIntersections();
    }
    
    if( !intersections_computedQ )
    {
        Msgr::wprint("RequireIntersections","Failed to compute intersections for an unknown reason.");
        return 2;
    }
    
    if( intersection_count_3D > Size_T{0} )
    {
        Msgr::wprint("RequireIntersections","Detected at least ", intersection_count_3D, " self-intersections in 3-space after perturbation. Link is not an embedding.");
        return 3;
    }

    return 0;
}


/*!@brief Return flag that signals whether the intersections after projecting to the x-y-plane have been loaded already.*/
bool IntersectionsComputedQ() const
{
    return intersections_computedQ;
}

/*!@brief Return the number of intersections after (symbolically perturbed) projection to the x-y-plane.
 *
 * Calls `RequireIntersections()`.
 * */
Int IntersectionCount() const
{
    (void)RequireIntersections();
    return intersection_count;
}

/*!@brief Return the number of intersections in the 3-space.
 *
 * Calls `RequireIntersections()`.
 */
Int IntersectionCount3D() const
{
    (void)RequireIntersections();
    return intersection_count_3D;
}

/*!@brief Return a vector `p` of size `EdgeCount() + 1` so that `p[e+1] - p[e]` is the number of intersections on edge `e`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int,Int>> EdgePointers() const
{
    (void)RequireIntersections();
    return edge_ptr;
}

/*!@brief Return a vector `a` of information for edge-crossing pairs. The crossing information for edge `e` stored it `[ a[p[e]],...,a[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * The information or each pair of edge and crossing is stored in an opaque class `EdgeCrossing_T`. It contains an identifier that uniquely identifies the crossings (obtainable with `EdgeCrossing_T::Index()`), the handedness of the crossing (`EdgeCrossing_T::RightHandedQ()`), and whether the edge goes over or under (`EdgeCrossing_T::OverQ()`).
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<EdgeCrossing_T,Int>> EdgeCrossings() const
{
    (void)RequireIntersections();
    return edge_cross;
}

/*!@brief Return a vector `a` of intersection information. The labels of the intersections on edge `e` are `[ a[p[e]],...,a[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * Calls `RequireIntersections()`.
 */
//[[deprecated("Use EdgeCrossings instead.")]]
Tensor1<Int,Int> EdgeIntersections() const
{
    if( RequireIntersections() ) { return Tensor1<Int,Int>(); }
    
    Tensor1<Int,Int> edge_intersections (edge_cross.Size());
    
    for( Int idx = 0; idx < edge_cross.Size(); ++idx )
    {
        const EdgeCrossing_T & ec = edge_cross[idx];
        edge_intersections[idx] = ec.Index();
    }
    
    return edge_intersections;
}

/*!@brief Return a vector `s` with intersection state information. The states of intersection on edge `e` are `[ s[p[e]],...,s[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * The state is given by `(h << 1) | b`, where `h` is the handedness crossing and where `b` is `true` if this edge is the upper strand for the crossing `a[p[e]]` (and false otherwise), where `a = EdgeIntersections()`.
 *
 * Calls `RequireIntersections()`.
 */
//[[deprecated("Use EdgeCrossings instead.")]]
Tensor1<Int8,Int> EdgeStates() const
{
    if( RequireIntersections() ) { return Tensor1<Int8,Int>(); }
    
    Tensor1<Int8,Int> edge_state (edge_cross.Size());
    
    for( Int idx = 0; idx < edge_cross.Size(); ++idx )
    {
        const EdgeCrossing_T & ec = edge_cross[idx];
        edge_state[idx] = static_cast<Int8>(
            ((ec.RightHandedQ() ? Int8{1} : Int8{-1} ) << 1) | ec.OverQ()
        );
    }
    
    return edge_state;
}

/*!@brief Return a vector `t` with intersection time information. The times of intersection on edge `e` are `[ t[p[e]],...,t[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * Instead of giving the precise times (which are rational functions in the symbolic perturbation paramater `eps` with wide integer coefficient s), these are `double` approximations (for perturbation `eps = 0`).
 *
 * Calls `RequireIntersections()`.
 */
Tensor1<double,Int> EdgeIntersectionTimesAsDouble() const
{
    using Tools::ToDouble;
    
    if( RequireIntersections() ) { return Tensor1<double,Int>(); }
    
    Tensor1<double,Int> result ( edge_cross.Size() );
    
    for( Int i = 0; i < edge_count; ++i )
    {
        // This is the range of data in edge_cross that belongs to edge i.
        const Int begin = edge_ptr[i  ];
        const Int end   = edge_ptr[i+1];
        const Int n     = end - begin;
        
        if( n <= Int{0} ) { continue; }
        
        S.LoadFirstLineSegment(i, EdgeData(i,0), EdgeData(i,1));
        
        for( Int a = begin; a < end; ++a )
        {
            const EdgeCrossing_T & ec   = edge_cross[a];
            const Intersection_T & isec = intersections[ec.Index()];
            const Int over_edge = isec.OverEdge();
            const Int j = (i == over_edge) ? isec.UnderEdge() : over_edge;

            result[a] = ToDouble(
                S.ComputeIntersectionTime(j, EdgeData(j,0), EdgeData(j,1))
            );
        }
    }
        
    return result;
}

private:

/*!@brief (Re)compute the intersections.*/
template<bool verboseQ = true> // whether to print errors and warnings
void ComputeIntersections() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeIntersections"));
    
    intersections_computedQ  = false;
    intersections.Clear();
    intersection_count       = 0;
    intersection_count_3D    = 0;
    
    edge_ptr.SetZero();
    
    intersections.RequireCapacity( Int{2} * EdgeCount() );
    
    // Potentially to be replaced by `FindIntersectingEdges_SweepLine()`.
    {
        RequireBoundingBoxes();
        
        if( !bounding_boxes_computedQ )
        {
            Msgr::wprint("ComputeIntersections","Boundung boxes not computed, yet. Aborting.");
        }
        
        FindIntersectingEdges_DFS();
    }
    
    // TODO: To reduce peak memory, we could free the bounding boxes here.
    
    if( !std::in_range<Int>( Size_T{8} * ToSize_T(intersections.Size())) )
    {
        Msgr::eprint("ComputeIntersections","More intersections found (intersections.size() = ", intersections.Size(), ") than can be handled by integer type Int = ", TypeName<Int>, " = ", PrettyTypeName<Int>(), ". Please try again with a wider integer type." );
        
        intersections_computedQ = true;
        return;
    }
    
    // Earlier versions of this function checked `intersection_count_3D > 0` and stopped here or threw warnings or errors. We just let it flow. Computing the other planar intersections might still be worthwhile. Moreover, `RequireIntersections` will finally report on this issue. Makes the control flow simpler and better maintainable.
        
    Int max_per_edge = 0;
    for( Int i = 0; i < edge_count; ++i )
    {
        const Int per_edge = edge_ptr[i+Int{1}];
        max_per_edge = Max(max_per_edge,per_edge);
        edge_ptr[i+Int{1}] = edge_ptr[i] + per_edge;
    }
    
    intersection_count = intersections.Size();
    
    {
        TOOLS_PTIMER(sort_timer, MethodName("ComputeIntersections") + ": coarse sorting.");
        
        // We do a counting sort here.
        
        Tensor1<Int,Int> edge_ctr = edge_ptr;

        edge_cross.template Resize<false>(edge_ptr.Last());

        for( Int idx = intersection_count; idx --> Int{0};  )
        {
            const Intersection_T & isec = intersections[idx];
            
            // We have to write BEFORE the positions specified by edge_ctr (and decrease it for the next write);
            
            const Int pos_0 = --edge_ctr[isec.OverEdge()  + Int{1}];
            const Int pos_1 = --edge_ctr[isec.UnderEdge() + Int{1}];

            const bool right_handedQ = isec.RightHandedQ();
            
            edge_cross[pos_0] = EdgeCrossing_T(idx,right_handedQ,true );
            edge_cross[pos_1] = EdgeCrossing_T(idx,right_handedQ,false);
        }
    }

    {
        TOOLS_PTIMER(sort_timer, MethodName("ComputeIntersections") + ": fine sorting.");
        
        Tensor1<EdgeCrossing_T,Int> buffer( max_per_edge );
        Tensor1<Int,Int>            perm  ( max_per_edge );
        Tensor1<Time_T,Int>         times ( max_per_edge );

        for( Int i = 0; i < edge_count; ++i )
        {
            // This is the range of data in edge_cross that belongs to edge i.
            const Int begin = edge_ptr[i       ];
            const Int end   = edge_ptr[i+Int{1}];
            const Int n     = end - begin;
            
            // We need to sort only if there are at least two intersections on that edge.
            if( n <= Int{1} ) { continue; }
            
            S.LoadFirstLineSegment(i, EdgeData(i,0), EdgeData(i,1));
            
            for( Int a = 0; a < n; ++a )
            {
                const EdgeCrossing_T & ec   = edge_cross[begin + a];
                const Intersection_T & isec = intersections[ec.Index()];
                const Int over_edge = isec.OverEdge();
                const Int j = (i == over_edge) ? isec.UnderEdge() : over_edge;
                buffer[a] = ec;
                perm  [a] = a;
                times [a] = S.ComputeIntersectionTime(j, EdgeData(j,0), EdgeData(j,1));
            }
            
            // Potentially very expensive because `<` requires very long integer multiplication.
            Sort( &perm[0], &perm[n],
                [&times]( Int a, Int b ) { return (times[a] < times[b]); }
            );
            
            for( Int a = 0; a < n; ++a )
            {
                edge_cross[begin + a] = buffer[perm[a]];
            }
        }
    }
    
    // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
    intersections_computedQ = true;
}
