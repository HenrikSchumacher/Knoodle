void Canonicalize()
{
    using MacLeod_T = Tensor1<UInt,Int>;
    
    ColorCounts_T pdc_color_arc_counts = ColorArcCounts();
    
    for( PD_T & pd : pd_list )
    {
        if( pd.InvalidQ() ) { continue; };
        
        if( pd.ProvenUnknotQ() )
        {
            // We replace unknots that can be connected to arcs of the same color by invalid knots.
            // They will be filtered out in the end.
            
            const Int color = pd.LastColorDeactivated();
            
            if( pdc_color_arc_counts.contains(color) && (pdc_color_arc_counts[color] > Int(0)) )
            {
                pd = PD_T::InvalidDiagram();
            }
            continue;
        };
        
        if( pd.LinkComponentCount() != Int(1) ) { continue; }
        if( pd.ColorCount() != Int(1) ) { continue; }

        // Thing is MacLeod code is not good at normalizing figure-eight knot.
        if( pd.ProvenFigureEightQ() )
        {
            auto color_arc_counts = pd.ColorArcCounts(); // Make copy.
            const Int color = color_arc_counts.begin()->first;
            pd = PD_T::FigureEightKnot(color);
            pd.SetCache("BufferedMacLeodCode",pd.MacLeodCode());
            pd.SetCache("ColorArcCounts",std::move(color_arc_counts));
            continue;
        }
        
        // Maybe do this only for knots with less than 100 crossings?
        
        auto color_arc_counts = pd.ColorArcCounts(); // Make copy.
        const Int color = color_arc_counts.begin()->first;
//        TOOLS_LOGDUMP(color);
        MacLeod_T macleod = pd.MacLeodCode();
        pd = PD_T::FromMacLeodCode(macleod,color,pd.ProvenMinimalQ());
        pd.SetCache("BufferedMacLeodCode",std::move(macleod));
        pd.SetCache("ColorArcCounts",std::move(color_arc_counts));
        
//        TOOLS_LOGDUMP(pd.A_color);
//        logvalprint("pd.ColorArcCounts()",ToString(pd.ColorArcCounts()));
    }
    
    std::sort(
        pd_list.begin(),
        pd_list.end(),
        []( cref<PD_T> pd_0, cref<PD_T> pd_1 )
        {
            // TODO: Not sure whether this is my favorite ordering.
            
            // Make sure that the invalid diagrams are sorted to the very back.
            const bool I_0 = pd_0.InvalidQ();
            const bool I_1 = pd_1.InvalidQ();
            
            if( I_0 < I_1 ) { return true;  }
            if( I_0 > I_1 ) { return false;  }
            
            if( I_0 ) { return false;}
            
            const Int C_0 = pd_0.CrossingCount();
            const Int C_1 = pd_1.CrossingCount();
            
            if( C_0 > C_1 ) { return true;  }
            if( C_0 < C_1 ) { return false; }
            
            const Int L_0 = pd_0.LinkComponentCount();
            const Int L_1 = pd_1.LinkComponentCount();
            
            if( L_0 > L_1 ) { return true;  }
            if( L_0 < L_1 ) { return false; }
            
            const Int color_count_0 = pd_0.ColorCount();
            const Int color_count_1 = pd_1.ColorCount();
            
            if( color_count_0 > color_count_1 ) { return true;  }
            if( color_count_0 < color_count_1 ) { return false; }
            
            if( color_count_0 != Int(1) ) { return false; }
            
            const Int color_0 = pd_0.ColorArcCounts().begin()->first;
            const Int color_1 = pd_1.ColorArcCounts().begin()->first;
            
            if( color_0 > color_1 ) { return true;  }
            if( color_0 < color_1 ) { return false; }
            
            if( L_0 != Int(1)) { return false; }
            
            auto & M_0 = pd_0.template GetCache<MacLeod_T>("BufferedMacLeodCode");
            auto & M_1 = pd_1.template GetCache<MacLeod_T>("BufferedMacLeodCode");
            
            for( Int i = 0; i < C_0; ++i )
            {
                const Int m_0 = M_0[i];
                const Int m_1 = M_1[i];
                if( m_0 > m_1 ) { return true;  }
                if( m_0 < m_1 ) { return false; }
            }
            
            return false;
        }
    );
    
    // Drop the invalid diagrams at the back.
    while( !pd_list.empty() )
    {
        if( pd_list.back().InvalidQ() )
        {
            pd_list.pop_back();
        }
        else
        {
            break;
        }
    }
          
    for( PD_T & pd : pd_list )
    {
        pd.ClearCache("BufferedMacLeodCode");
    }
    
    ClearCache();
}
