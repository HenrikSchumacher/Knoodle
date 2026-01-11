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

    if( intersections.size() <= Size_T(0) )
    {
        unlink_count = component_count;
        proven_minimalQ = true;
        return;
    }
    
    unlink_count = 0;
    crossing_count = int_cast<Int>(intersections.size());
    arc_count = Int(2) * crossing_count;
    
    C_scratch.Fill(Uninitialized);
    
    mptr<Int> C_label = C_scratch.data();
    Int C_counter = 0;
        
    auto process = [&,C_label,edge_intersections,edge_overQ,this]( const Int a, const Int b
    )
    {
        const Int c_pos = edge_intersections[b];
        
        if( !ValidIndexQ(C_label[c_pos]) )
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
        
        C_state[c] = righthandedQ ? CrossingState_T::RightHanded : CrossingState_T::LeftHanded;
        
        A_state[a] = ArcState_T::Active;
        
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
        
        for( Int b = b_begin, a = b_end-Int(1); b < b_end; a = (b++) )
        {
            process(a,b);
        }
    }
    
    // TODO: Check whether this is really neccessary.
    
    if constexpr ( always_compressQ )
    {
        Compress();
    }
}
