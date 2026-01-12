cref<ColorPalette_T> ColorPalette() const
{
    return color_palette;
}

void ComputeArcColors()
{
    TOOLS_PTIMER(timer,MethodName("ComputeArcColors"));
    
    if( this->InCacheQ("ArcLinkComponents") )
    {
        ArcLinkComponents().Write(A_color.data());
        
        for( Int color = 0; color < LinkComponentCount(); ++color )
        {
            color_palette.insert(color);
        }
    }
    else
    {
        this->template Traverse<false,false>(
            [this]( const Int lc, const Int lc_begin )
            {
                (void)lc_begin;
                this->color_palette.insert(lc);
            },
            [this]
            ( const Int a, const Int a_pos, const Int lc )
            {
                (void)a_pos;
                this->A_color[a] = lc;
            },
            []( const Int lc, const Int lc_begin, const Int lc_end )
            {
                (void)lc;
                (void)lc_begin;
                (void)lc_end;
            }
        );
    }
}


bool CheckArcColors() const
{
    Size_T violation_count = 0;
    
    ColorPalette_T checking_palette;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            const Int a_next = NextArc(a,Head);
            
            checking_palette.insert(A_color[a]);
            
            if( A_color[a] != A_color[a_next] )
            {
                if( violation_count < Size_T(5) )
                {
                    eprint(MethodName("CheckArcColors") + "failed at arcs a = " + ArcString(a) + " and a_next = " + ArcString(a_next) + "." );
                }
                ++violation_count;
            }
        }
    }
    
    if( violation_count > Size_T(0) )
    {
        eprint(MethodName("CheckArcColors") + ": Found = " + ToString(violation_count) + " color mismatches." );
    }
    
    {
        ColorPalette_T missing_colors;
        
        for( auto & color : color_palette )
        {
            if( !checking_palette.contains(color) )
            {
                missing_colors.insert(color);
            }
        }
        
        if( missing_colors.size() > Size_T(0) )
        {
            eprint(MethodName("CheckArcColors") + ": Found " + ToString(missing_colors.size()) + " missing colors: = " + ToString(missing_colors) + ".");
            
            violation_count += missing_colors.size();
        }
    }
    
    {
        ColorPalette_T excess_colors;
        
        for( auto & color : checking_palette )
        {
            if( !color_palette.contains(color) )
            {
                excess_colors.insert(color);
            }
        }
        
        if( excess_colors.size() > Size_T(0) )
        {
            eprint(MethodName("CheckArcColors") + ": Found " + ToString(excess_colors.size()) + " excess colors: " + ToString(excess_colors) + ".");
            
            violation_count += excess_colors.size();
        }
    }
    
    return violation_count == Size_T(0);
}
