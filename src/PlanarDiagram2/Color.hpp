//cref<ColorPalette_T> ColorPalette() const
//{
//    return color_palette;
//}

cref<ColorCounts_T> ColorArcCounts() const
{
    return color_arc_counts;
}


void CountArcColor( const Int color )
{
    if( color_arc_counts.contains(color) )
    {
        color_arc_counts[color] += Int(1);
    }
    else
    {
        color_arc_counts[color]  = Int(1);
    }
}

void ComputeArcColors()
{
    TOOLS_PTIMER(timer,MethodName("ComputeArcColors"));
    
    if( this->InCacheQ("ArcLinkComponents") && this->InCacheQ("LinkComponentArcs") )
    {

        ArcLinkComponents().Write(A_color.data());
        
        color_arc_counts = ColorCounts_T();
        
        const auto & lc_arcs = LinkComponentArcs();
        
        for( Int lc = 0; lc < lc_arcs.SublistCount(); ++lc )
        {
            color_arc_counts[lc] = lc_arcs.SublistSize(lc);
        }
        
//        for( Int color = 0; color < LinkComponentCount(); ++color )
//        {
//            color_palette.insert(color);
//        }
    }
    else
    {
//        color_palette = ColorPalette_T();
        color_arc_counts = ColorCounts_T();
        
        this->template Traverse<false,false>(
            [this]( const Int lc, const Int lc_begin )
            {
                (void)lc_begin;
//                this->color_palette.insert(lc);
                (void)this;
                (void)lc;
            },
            [this]
            ( const Int a, const Int a_pos, const Int lc )
            {
                (void)a_pos;
                this->A_color[a] = lc;
            },
            [this]( const Int lc, const Int lc_begin, const Int lc_end )
            {
                (void)lc;
                (void)lc_begin;
                (void)lc_end;
                
                this->color_arc_counts[lc] = lc_end - lc_begin;
            }
        );
    }
}


bool CheckArcColors() const
{
    if( InvalidQ() )
    {
        return false;
    }
    
    if( ProvenUnknotQ() )
    {
        return true;
    }
    
    Size_T violation_count = 0;
   
    ColorCounts_T check_counts;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            const Int a_color = A_color[a];
            
            if( check_counts.contains(a_color) )
            {
                check_counts[a_color] += Int(1);
            }
            else
            {
                check_counts [a_color] = Int(1);
            }
            
            const Int a_next  = NextArc(a,Head);
            
            if( a_color != A_color[a_next] )
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
        ColorCounts_T missing_counts;

        for( auto & x: color_arc_counts )
        {
            if( !check_counts.contains(x.first) )
            {
                missing_counts[x.first] = x.second;
            }
        }

        if( missing_counts.size() > Size_T(0) )
        {
            eprint(MethodName("CheckArcColors") + ": Found " + ToString(missing_counts.size()) + " missing colors: = " + ToString(missing_counts) + ".");

            violation_count += missing_counts.size();
        }
    }
    
    {
        ColorCounts_T excess_counts;
        ColorCounts_T wrong_counts;
        ColorCounts_T correct_counts;

        for( auto & x: check_counts )
        {
            if( color_arc_counts.contains(x.first) )
            {
                if ( x.second != color_arc_counts.at(x.first) )
                {
                    wrong_counts  [x.first] = color_arc_counts.at(x.first);
                    correct_counts[x.first] = x.second;
                }
            }
            else
            {
                excess_counts[x.first] = x.second;
            }
        }

        if( excess_counts.size() > Size_T(0) )
        {
            eprint(MethodName("CheckArcColors") + ": Found " + ToString(excess_counts.size()) + " excess colors: = " + ToString(excess_counts) + ".");

            violation_count += excess_counts.size();
        }
        
        if( wrong_counts.size() > Size_T(0) )
        {
            eprint(MethodName("CheckArcColors") + ": Found " + ToString(wrong_counts.size()) + " wrong color counts: " + ToString(wrong_counts) + "; correct color counts would be : " + ToString(correct_counts) + ".");

            violation_count += wrong_counts.size();
        }
    }
    
    return violation_count == Size_T(0);
}

static Tensor2<Int,Int> ColorCountsToTensor2( cref<ColorCounts_T> counts )
{
    Tensor2<Int,Int> a( counts.size(), 2 );
    
    Int i = 0;
    for( auto & x : counts )
    {
        a(i,0) = x.first;
        a(i,1) = x.second;
        ++i;
    }
    
    return a;
}

template<typename ExtInt,typename ExtInt2>
static ColorCounts_T ArrayToColorCounts( cptr<ExtInt> a, ExtInt2 color_count )
{
    static_assert(IntQ<ExtInt>, "");
    static_assert(IntQ<ExtInt2>, "");
    
    ColorCounts_T counts;
    
    for( ExtInt2 i = 0; i < color_count; ++i )
    {
        counts[ a[ ExtInt2(2) * i] ] = a[ ExtInt2(2) * i + ExtInt2(1)];
    }
    
    return counts;
}

private:

void ChangeArcColor( const Int a, const Int new_color )
{
    PD_ASSERT( ArcActiveQ(a) );
    
    const Int a_color = A_color[a];
    
    PD_ASSERT( color_arc_counts.contains(a_color) && color_arc_counts.at(a_color) > Int(0) );
    PD_ASSERT( color_arc_counts.contains(new_color) );
              
    --color_arc_counts[a_color];
    A_color[a] = new_color;
    ++color_arc_counts[new_color];
}
