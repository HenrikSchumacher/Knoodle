
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
    *this = this->ConnectedSum();
}

PDC_T ConnectedSum() const
{
    TOOLS_PTIMER(timer,"ConnectedSum");
    
//    constexpr bool debugQ = true;
    
    using ColorMap_T = AssociativeContainer<Int,Int>;
    
    const Int pd_count = static_cast<Int>(pd_list.size());
    
    ColorMap_T col_reps;             // map from colors to representatives for each color
    ColorMap_T col_anelli;           // map from colors to first anello with that color
    
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

        if( pd.AnelloQ() )
        {
            const Int col = pd.last_color_deactivated;
            
            if( !col_anelli.contains(col) )
            {
                col_anelli[col] = i;
            }
            continue;
        }
        
        ColorMap_T & col_A = pd_col_A[i];
        
        for( Int a = 0; a < pd.MaxArcCount(); ++a )
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
        logvalprint( "gluing_data", OutString::FromMatrix( &gluing_data[0][0], gluing_data.size(), 5 ) );
        TOOLS_LOGDUMP(gluing_arc_pairs);
        
        logvalprint( "gluing_data", OutString::FromMatrix( &gluing_data[0][0], gluing_data.size(), 5 ) );
        TOOLS_LOGDUMP(gluing_arc_pairs);
        
        logprint("Step 3: Unite all diagrams.");
    }
    
    PDC_T pdc = this->Union();
    
    if constexpr ( debugQ )
    {
        logprint("Step 3: Push the unlinks.");
    }
    
    // Delete the anelli
    // All diagrams except the first one are guaranteed to be anelli (otherwise Union would have merged them with the first diagram.)
    // The first diagram might be an anello, too. We have to check for that.
    if( pdc.pd_list.front().AnelloQ() )
    {
        pdc.pd_list.resize(0);
    }
    else
    {
        pdc.pd_list.resize(1);
    }
    
    for( auto [col,idx] : col_anelli )
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
                wprint(MethodName("ConnectedSum")+": a == b.");
                continue;
            }
            
            flag += (!pd_0.Connect(a,b));
            
            // When things are debugged use this, as it is much faster than Connect(a,b).
//            pd_0.template ArcSwap_Private<Head>(a,b);
        }
    }
    
    pdc.SetCache("ConnectedSumFlag",flag);
    
    // Call Split() here?
    
    // We do not compress so that pd_arc_map stays valid, in principle.
    return pdc;
}

Int64 ConnectedSumFlag() const
{
    if( this->InCacheQ("ConnectedSumFlag") )
    {
        return this->template GetCache<Int64>("ConnectedSumFlag");
    }
    else
    {
        return Int64(-1);
    }
}
