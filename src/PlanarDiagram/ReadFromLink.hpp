private:
    
template<typename Real, typename BReal>
void ReadFromLink(
    const Int  component_count,
    cptr<Int>  component_ptr,
    cptr<Int>  edge_ptr,
    cptr<Int>  edge_intersections,
    cptr<bool> edge_overQ,
    cref<std::vector<typename Link_2D<Real,Int,BReal>::Intersection_T>> intersections
)
{
    static_assert(FloatQ<Real>,"");
    static_assert(FloatQ<BReal>,"");
    
    using Intersection_T = typename Link_2D<Real,Int,BReal>::Intersection_T;
    using Sign_T         = typename Intersection_T::Sign_T;
    // TODO: Handle over/under in ArcState.
//    using F_T            = Underlying_T<ArcState>;
    
    
    if( intersections.size() <= Size_T(0) )
    {
        unlink_count = component_count;
        proven_minimalQ = true;
        return;
    }
    
    unlink_count = 0;
    
    C_scratch.Fill(Int(-1));
    
    mptr<Int> C_label = C_scratch.data();
    Int C_counter = 0;
    
    // TODO: If we want to canonicalize here, then we also need arc labels!
    
    auto process = [&,C_label,edge_intersections,edge_overQ,this]( const Int a, const Int b
    )
    {
        const Int c_pos = edge_intersections[b];
        
        if( C_label[c_pos] < 0 )
        {
            C_label[c_pos] = C_counter++;
        }
        
        const Int c = C_label[c_pos];
        
        const bool overQ = edge_overQ[b];
        
        cref<Intersection_T> inter = intersections[static_cast<Size_T>(c_pos)];
        
        A_cross(a,Head) = c; // c is head of a
        A_cross(b,Tail) = c; // c is tail of b
        
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
        
        C_state[c] = righthandedQ ? CrossingState::RightHanded : CrossingState::LeftHanded;
        
        // TODO: Handle over/under in ArcState.
        A_state[a] = ArcState::Active;
//                A_state[a] = ToUnderlying(A_state[a]) | F_T(1) | (F_T(overQ) << 2);
//                A_state[b] = ToUnderlying(A_state[b]) | F_T(1) | (F_T(overQ) << 1);
        
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
        
        const bool over_in_side = (righthandedQ == overQ) ? Left : Right ;
        
        C_arcs(c,In , over_in_side) = a;
        C_arcs(c,Out,!over_in_side) = b;
    };
    
    
    // TODO: Extract LinkComponentArcPointers, LinkComponentIndices, ArcLinkComponents from here (only if needed)?
    
    // Now we go through all components
    //      then through all edges of the component
    //      then through all intersections of the edge
    // and generate new vertices, edges, crossings, and arcs in one go.
    
    
    for( Int comp = 0; comp < component_count; ++comp )
    {
        // The range of arcs belonging to this component.
        const Int b_begin = edge_ptr[component_ptr[comp  ]];
        const Int b_end   = edge_ptr[component_ptr[comp+1]];

        if( b_begin == b_end )
        {
            // Component is an unlink. Just skip it.
            ++unlink_count;
            continue;
        }
        
        // If we arrive here, then there is definitely a crossing in the first edge.
        
        
        // TODO: Do the traversal so that the resulting `PlanarDiagram` is in canonical ordering in the sense of `CanonicallyOrderedQ`. For this, we have to go forward along the component until reach the first edge whose tail goes under. If if there is no such edge, we start at `b_begin`.
        
        for( Int b = b_begin, a = b_end-Int(1); b < b_end; a = (b++) )
        {
            process(a,b);
        }
    }
    
    // TODO: For some reason, the resulting `PlanarDiagram` is not canonically ordered in the sense of `CanonicallyOrderedQ`. This may be a big issue when using `PlanarDiagram` in conjunction of the external visualization tools as `PlanarDiagram` returns PDCode always in canonically ordered form. So we do here a maybe unnecessary recanonicalization here. Maybe we can create the diagram in canonical form already?
    
    if constexpr ( always_canonicalizeQ )
    {
        CanonicalizeInPlace();
    }
}
