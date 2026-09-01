public:

/*!@brief Guarantee that the intersections are computed.
 *
 * @param force_recomputeQ If set to `true`, a recomputation of the intersections is enforced, even if the intersections are already computed. (Probably only useful for benchmarking and debugging.)
 *
 * @return `0` if the intersections have been computed successfully. Anything nonzero if either no vertex coordinates are loaded (check `VertexCoordinatesLoadedQ()`), if some self-intersection of the link in 3-space have been detected (check `IntersectionCount3D()`)), or if some other issue arose. Caution: The precise error flags might be subject to change.
 */
template<bool verboseQ = true> // whether to print errors and warnings
[[nodiscard]] int RequireIntersections( bool force_recomputeQ = false )
{
   [[maybe_unused]] auto tag = [](){ return MethodName("RequireIntersections"); };
       
    if( !vertex_coords_loadedQ )
    {
        wprint(tag() + ": Failed to compute intersections because no vertex coordinates have been loaded, yet." );
        return 1;
    }
    
    if( force_recomputeQ || !intersections_computedQ )
    {
        ComputeIntersections();
    }
    
    if( !intersections_computedQ )
    {
        wprint(tag() + ": Failed to compute intersections for an unknown reason." );
        return 2;
    }
    
    if( intersection_count_3D > Size_T{0} )
    {
        wprint(tag() + ": Detected at least "  + ToString(intersection_count_3D)+ " self-intersections in 3-space after perturbation. Link is not an embedding." );
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
Int IntersectionCount()
{
    (void)RequireIntersections();
    return intersection_count;
}

/*!@brief Return the number of intersections in the 3-space.
 *
 * Calls `RequireIntersections()`.
 */
Int IntersectionCount3D()
{
    (void)RequireIntersections();
    return intersection_count_3D;
}

/*!@brief Return a vector `p` of size `EdgeCount() + 1` so that `p[e+1] - p[e]` is the number of intersections on edge `e`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int,Int>> EdgePointers()
{
    (void)RequireIntersections();
    return edge_ptr;
}

/*!@brief Return a vector `a` intersection information. The labels of the intersections on edge `e` are `[ a[p[e]],...,a[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int,Int>> EdgeIntersections()
{
    (void)RequireIntersections();
    return edge_intersections;
}

/*!@brief Return a vector `t` with intersection time information. The times of intersection on edge `e` are `[ t[p[e]],...,t[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * Instead of giving the precise times (which are rational functions in the symbolic perturbation paramater `eps` with wide integer coefficient s), these are `double` approximations (for perturbation `eps = 0`).
 *
 * Calls `RequireIntersections()`.
 */
Tensor1<double,Int> EdgeIntersectionTimesAsDouble()
{
    using Tools::ToDouble;
    
    (void)RequireIntersections();
    
    Tensor1<double,Int> result ( edge_times.Size() );
    
    for( Int i = 0; i < edge_times.Size(); ++i )
    {
        result[i] = ToDouble(edge_times[i]);
    }
        
    return result;
}

/*!@brief Return a vector `s` with intersection state information. The states of intersection on edge `e` are `[ s[p[e]],...,s[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * The state is given by `(h << 1) | b`, where `h` is the handedness crossing and where `b` is `true` if this edge is the upper strand for the crossing `a[p[e]]` (and false otherwise), where `a = EdgeIntersections()`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int8,Int>> EdgeStates()
{
    (void)RequireIntersections();
    return edge_state;
}

private:

/*!@brief (Re)compute the intersections.*/
template<bool verboseQ = true> // whether to print errors and warnings
void ComputeIntersections()
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ComputeIntersections"); };
    
    TOOLS_PTIMER(timer,tag());
    
    intersections_computedQ  = false;
    intersections.Clear();
    intersection_count       = 0;
    intersection_count_3D    = 0;
    
    edge_ptr.Fill(0);
    
    intersections.RequireCapacity( Int{2} * EdgeCount() );
    
    // Potentially to be replaced by `FindIntersectingEdges_SweepLine()`.
    {
        RequireBoundingBoxes();
        
        if( !bounding_boxes_computedQ )
        {
            wprint(tag() + ": Boundung boxes not computed, yet. Aborting.");
        }
        
        FindIntersectingEdges_DFS();
    }
    
    if( !std::in_range<Int>( Size_T{8} * ToSize_T(intersections.Size())) )
    {
        eprint(tag() + ": More intersections found (intersections.size() = " + ToString(intersections.Size()) + ") than can be handled by integer type Int = " + TypeName<Int> + " = " + std::string(PrettyTypeName<Int>()) + ". Please try again with a wider integer type." );
        
        intersections_computedQ = true;
        return;
    }
    
    // Earlier versions of this function checked `intersection_count_3D > 0` and stopped here or threw warnings or errors. We just let it flow. Computing the other planar intersections might still be worthwhile. Moreover, `RequireIntersections` will finally report on this issue. Makes the control flow simpler and better maintainable.
        
    edge_ptr.Accumulate();
    
    intersection_count = intersections.Size();

    {
        TOOLS_PTIMER(sort_timer, tag() + ": coarse sorting.");
        
        Tensor1<Int,Int> edge_ctr = edge_ptr;
        
        if( edge_intersections.Size() != edge_ptr.Last() )
        {
            edge_intersections = Tensor1<Int   ,Int>( edge_ptr.Last() );
            edge_times         = Tensor1<Time_T,Int>( edge_ptr.Last() );
            edge_state         = Tensor1<Int8  ,Int>( edge_ptr.Last() );
        }

        for( Int k = intersection_count; k --> Int{0};  )
        {
            const Intersection_T & isec = intersections[k];
            
            // We have to write BEFORE the positions specified by edge_ctr (and decrease it for the next write;
            
            const Int pos_0 = --edge_ctr[isec.edges[0]+Int{1}];
            const Int pos_1 = --edge_ctr[isec.edges[1]+Int{1}];
            
            edge_intersections[pos_0] = k;
            edge_times        [pos_0] = isec.times[0];
            edge_state        [pos_0] = static_cast<Int8>(isec.handedness << 1) | 1;
            
            edge_intersections[pos_1] = k;
            edge_times        [pos_1] = isec.times[1];
            edge_state        [pos_1] = static_cast<Int8>(isec.handedness << 1) | 0;
        }
        
        // We don't need this anymore.
        intersections = Aggregator<Intersection_T,Int>();
    }

    {
        TOOLS_PTIMER(sort_timer, tag() + ": fine sorting.");

        // Sort intersections edgewise w.r.t. edge_times.
        ThreeArraySort<Time_T,Int,Int8,Int> sort ( intersection_count );
        
        for( Int i = 0; i < edge_count; ++i )
        {
            // This is the range of data in edge_intersections/edge_times that belongs to edge i.
            const Int k_begin = edge_ptr[i  ];
            const Int k_end   = edge_ptr[i+1];

            // We need to sort only if there are at least two intersections on that edge.
            if( k_begin + Int{1} < k_end )
            {
                sort(
                    &edge_times[k_begin],
                    &edge_intersections[k_begin],
                    &edge_state[k_begin],
                    k_end - k_begin
                );
            }
        }
    }
    
    // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
    intersections_computedQ = true;
}
