
struct GlueData_T
{
    Int col = Uninitialized; // colors to be joined
    Int i   = Uninitialized; // index of first diagram to be joined
    Int a   = Uninitialized; // arc of first  diagram to be joined
    Int j   = Uninitialized; // index of second diagram to be joined
    Int b   = Uninitialized; // arc of second diagram to be joined
};


void Connect()
{
    *this = this->CreateConnected();
}

PDC_T CreateConnected() const
{
    TOOLS_PTIMER(timer,"CreateConnected");
    
//    constexpr bool debugQ = true;
    
    using ColorMap_T = AssociativeContainer_T<Int,Int>;
    
    const Int pd_count = static_cast<Int>(pd_list.size());
    
    ColorMap_T col_reps;              // map from colors to representatives for each color
    ColorMap_T col_unlinks;           // map from colors to first unlink with that color
    
    std::vector<ColorMap_T> pd_col_A (pd_count); // for each pd one map from colors to arcs.

    if constexpr ( debugQ )
    {
        logprint("Step 1");
    }
    // Step 1: For each diagram and each color that it contains, find and record an arc of that color.
    // We also record all single-color diagrams and a "representative" for each color.
    for( Int i = 0; i < pd_count; ++i )
    {
        if constexpr ( debugQ )
        {
            TOOLS_LOGDUMP(i);
        }
        
        PD_T & pd = pd_list[static_cast<Size_T>(i)];
        
        if( !pd.ValidQ() ) { continue; }

        if( pd.ProvenUnknotQ() )
        {
            const Int col = pd.last_color_deactivated;
            
            if( !col_unlinks.contains(col) )
            {
                col_unlinks[col] = i;
            }
            continue;
        }
        
        ColorMap_T & col_A = pd_col_A[i];
        
        for( Int a = 0; a < pd.max_arc_count; ++a )
        {
            if( !pd.ArcActiveQ(a) ) { continue; }
            const Int col = pd.A_color[a];
            if( !col_A.contains(col) ) { col_A[col] = a; }
        }
        
        // col_reps shall contain only diagrams with at least one arc.
        
        // Make the first diagram with that color be the representative.
        for( auto [col,a] : col_A )
        {
            if( !col_reps.contains(col) ) { col_reps[col] = i; }
        }
    }
    
    if constexpr ( debugQ )
    {
        logvalprint("col_reps",ToString(col_reps));
        
        for( Int i = 0; i < pd_count; ++i )
        {
            logvalprint( std::string("pd_col_A[") + ToString(i) + "]", ToString(pd_col_A[static_cast<Size_T>(i)]) );
        }
        
        logprint("Step 2: Connect each color in each diagram with the corresponding representative.");
    }

    std::vector<std::array<Int,5>>  gluing_data;
    std::vector<std::pair<Int,Int>> gluing_arc_pairs;
    
    RaggedList<Int,Int> pd_arc_map = UnionArcMaps();
    
    for( Int i = 0; i < pd_count; ++ i )
    {
        for( auto [col,a] : pd_col_A[static_cast<Size_T>(i)] )
        {
            const Int j  = col_reps[col];
            
            if( i != j )
            {
                const Int b  = pd_col_A[j][col];
                gluing_data.push_back( {col, i, a, j, b } );
                gluing_arc_pairs.emplace_back( pd_arc_map[i][a], pd_arc_map[j][b] );
            }
        }
    }
    
    if constexpr ( debugQ )
    {
        logvalprint( "gluing_data", ArrayToString(&gluing_data[0][0], {gluing_data.size(),Size_T(5)}) );
        TOOLS_LOGDUMP(gluing_arc_pairs);
        
        logvalprint( "gluing_data", ArrayToString(&gluing_data[0][0], {gluing_data.size(),Size_T(5)}) );
        TOOLS_LOGDUMP(gluing_arc_pairs);
        
        logprint("Step 3: Unite all diagrams.");
    }
    
    PDC_T pdc = this->Union();
    
    
    if constexpr ( debugQ )
    {
        logprint("Step 3: Push the unlinks.");
    }
    pdc.pd_list.resize(1); // Delete the unlinks.
    for( auto [col,idx] : col_unlinks )
    {
        if( !col_reps.contains(col) )
        {
            pdc.pd_list.emplace_back( pd_list[idx] ); // We must make a copy here.
        }
    }
    
    if constexpr ( debugQ )
    {
        logprint("Step 4: Do the surgery.");
    }
    
    Int64 flag = 0;
    {
        PD_T & pd_0 = pdc.pd_list[0];
        
        for( auto [a,b] : gluing_arc_pairs )
        {
            // TODO: Is expensive as it recomputes ArcLinkComponents.
            if( a == b )
            {
                wprint(MethodName("CreateConnected")+": a == b.");
                continue;
            }
            
            flag += (!pd_0.Connect(a,b));
            
            // When things are debugged use this, as it is much faster than Connect(a,b).
//            pd_0.template ArcSwap_Private<Head>(a,b);
        }
    }
    
    pdc.SetCache("CreateConnectedFlag",flag);
    
    // Call Split() here?
    
    // We do not compress so that pd_arc_map stays valid, in principle.
    return pdc;
}

Int64 CreateConnectedFlag() const
{
    if( this->InCacheQ("CreateConnectedFlag") )
    {
        return this->template GetCache<Int64>("CreateConnectedFlag");
    }
    else
    {
        return Int64(-1);
    }
}


// Step 1: For each diagram and each color that it contains, find and record an arc of that color.
//  ---> Check also that no diagram contains two link component of the same color! Potentially call Split and check again.

// Step 2: Connect each single-color diagram with any other diagram that has the same color. This will reduce the number of edges in the next step.
//  ---> Step 1a: For each color pick one representative.
//  ---> Step 1b: For each other single-color diagram determine a pair of gluing edges {a,b} to glue that diagram to the representative of that color.

// Step 3: Search all the color representatives for pairs that share exactly one color. Make them an edge in a graph.
//  ---> Check that the one-color diagrams among the representative should be isolated vertices in that graph.
//  ---> Check that the connected components in that graph are trees.

// Step 4: For each edge in that tree determine pairs {a,b} of gluing edges in the respective color representatives.

// Step 5: For all the gluing pairs {a,b} determine to which pair {a',b'} they are mapped by Unite.
//  ---> Can be done beforehand

// Step 6: Call Unite to create a new diagram.

// Step 7: Call pdc_new.Connect(0,a',b') on all pairs {a',b'} of gluing edges in the new diagram;

// Step 8: (optionally) Compress.





//// TODO: Remove argument fullQ.
//PDC_T CreateConnected( bool fullQ ) const
//{
//    TOOLS_PTIMER(timer,"CreateConnected");
//    
////    constexpr bool debugQ = true;
//    
//    using ColorMap_T = AssociativeContainer_T<Int,Int>;
//    
//    const Size_T pd_count = pd_list.size();
//    
//    ColorMap_T col_reps;              // map from colors to representatives for each color
//    ColorMap_T col_unlinks;           // map from colors to first unlink with that color
////    ColorMap_T col_counts;            // map from colors to number of diagrams that contain that color.
//    std::vector<Int> single_col_pds;  // list of diagrams with a single color
//    std::vector<Int> essential_pds; // list of diagrams that have more than one color or are a color representative
//    
//    single_col_pds.reserve(pd_count);
//    essential_pds.reserve(pd_count);
//    
//    std::vector<ColorMap_T> pd_col_A (pd_count); // for each pd one map from colors to arcs.
//
//    if constexpr ( debugQ )
//    {
//        logprint("Step 1");
//    }
//    // Step 1: For each diagram and each color that it contains, find and record an arc of that color.
//    // We also record all single-color diagrams and a "representative" for each color.
//    for( Size_T idx = 0; idx < pd_count; ++idx )
//    {
//        if constexpr ( debugQ )
//        {
//            TOOLS_LOGDUMP(idx);
//        }
//        
//        PD_T & pd = pd_list[idx];
//        
//        if( !pd.ValidQ() ) { continue; }
//
//        if( pd.ProvenUnknotQ() )
//        {
//            const Int col = pd.last_color_deactivated;
//            
//            if( !col_unlinks.contains(col) )
//            {
//                col_unlinks[col] = idx;
//            }
//            continue;
//        }
//        
//        ColorMap_T & col_A = pd_col_A[idx];
//        
//        for( Int a = 0; a < pd.max_arc_count; ++a )
//        {
//            if( !pd.ArcActiveQ(a) ) { continue; }
//            const Int col = pd.A_color[a];
//            if( !col_A.contains(col) ) { col_A[col] = a; }
//        }
//        
//        bool repQ = false;
//        
//        // col_reps shall contain only diagrams with at least one arc.
//        for( auto [col,a] : col_A )
//        {
//            if( !col_reps.contains(col) )
//            {
//                col_reps[col] = idx;
//                repQ = true;
//            }
//        }
//        
//        bool multQ = (col_A.size() > 1);
//        
//        // single_col_pds and essential_pds shall contain only diagrams with at least one arc.
//        if( !multQ ) { single_col_pds.push_back(idx); }
//        if( multQ || repQ ) { essential_pds.push_back(idx); }
//    }
//    
//    if constexpr ( debugQ )
//    {
//        logvalprint("col_reps",ToString(col_reps));
//        logvalprint("single_col_pds",ToString(single_col_pds));
//        logvalprint("essential_pds",ToString(essential_pds));
//        
//        for( Size_T i = 0; i < pd_count; ++i )
//        {
//            logvalprint( std::string("pd_col_A[") + ToString(i) + "]", ToString(pd_col_A[i]) );
//        }
//        
//        logprint("Step 2: Connect each single-color diagram with any other diagram that has the same color.");
//    }
//
//    std::vector<std::array<Int,5>>  gluing_data;
//    std::vector<std::pair<Int,Int>> gluing_arc_pairs;
//    
//    RaggedList<Int,Int> pd_arc_map = UnionArcMaps();
//    
//    for( Int i : single_col_pds )
//    {
//        auto [col,a] = *pd_col_A[i].begin();
//        const Int j  = col_reps[col];
//        
//        if( i != j )
//        {
//            const Int b = pd_col_A[j][col];
//            gluing_data.push_back( {col, i, a, j, b } );
//            gluing_arc_pairs.emplace_back( pd_arc_map[i][a], pd_arc_map[j][b] );
//        }
//    }
//    
//    if constexpr ( debugQ )
//    {
//        logvalprint( "gluing_data", ArrayToString(&gluing_data[0][0], {gluing_data.size(),Size_T(5)}) );
//        TOOLS_LOGDUMP(gluing_arc_pairs);
//        
//        logprint("Step 3");
//    }
//    if( fullQ )
//    {
//        // This is probably not optimal.
//        for( Int i : essential_pds)
//        {
//            auto & i_col_A = pd_col_A[i];
//            
//            for( Int j : essential_pds )
//            {
//                if( i >= j ) { continue; }
//                
//                auto & j_col_A = pd_col_A[j];
//                
//                Size_T common_count = 0;
//                Int    common_color = Uninitialized;
//                Int    a            = Uninitialized;
//                Int    b            = Uninitialized;
//                
//                for( auto [col,arc] : i_col_A )
//                {
//                    if( j_col_A.contains(col) )
//                    {
//                        ++common_count;
//                        common_color = col;
//                        a = arc;
//                        b = j_col_A[col];
//                    }
//                }
//            
//                if( common_count == Size_T(1) )
//                {
//                    gluing_data.push_back( { common_color, i, a, j, b} );
//                    gluing_arc_pairs.emplace_back( pd_arc_map[i][a], pd_arc_map[j][b] );
//                }
//                else if( common_count > Size_T(1) )
//                {
//                    eprint(MethodName("ConnectedSummationData") + ": More than one common colors between two diagram detected. Something must be corrupted. Try to split the planar diagram complex first.");
//                    
//                    return PDC_T();
//                }
//            }
//        }
//    }
//
//    if constexpr ( debugQ )
//    {
//        logvalprint( "gluing_data", ArrayToString(&gluing_data[0][0], {gluing_data.size(),Size_T(5)}) );
//        TOOLS_LOGDUMP(gluing_arc_pairs);
//        
//        logprint("Step 4: Push the unlinks.");
//    }
//    
//    if constexpr ( debugQ )
//    {
//        logprint("Step 5: Unite all diagrams.");
//    }
//    
//    PDC_T pdc = this->Union();
//    pdc.pd_list.resize(1); // Delete the unlinks.
//    for( auto [col,idx] : col_unlinks )
//    {
//        if( !col_reps.contains(col) )
//        {
//            pdc.pd_list.emplace_back( pd_list[idx] ); // We must make a copy here.
//        }
//    }
//    
//    if constexpr ( debugQ )
//    {
//        logprint("Step 6: Do the surgery.");
//    }
//    
//    Int64 flag = 0;
//    
//    {
//        PD_T & pd_0 = pdc.pd_list[0];
//        
//        for( auto [a,b] : gluing_arc_pairs )
//        {
//            // TODO: Is expensive as it recomputes ArcLinkComponents.
//            // TODO: Manage this manually here.
//            if( a == b )
//            {
//                wprint(MethodName("CreateConnected")+": a == b.");
//                continue;
//            }
//            
//            flag += (!pd_0.Connect(a,b));
//            
//            // When things are debugged use this, as it is much faster than Connect(a,b).
////            pd_0.template ArcSwap_Private<Head>(a,b);
//        }
//    }
//    
//    pdc.SetCache("CreateConnectedFlag",flag);
//    
//    // We do not compress so that pd_arc_map stays valid, in principle.
//    return pdc;
//}
