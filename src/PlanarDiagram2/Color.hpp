//        Int ColorCount() const
//        {
////            return int_cast<Int>( color_palette.size() );
//            return int_cast<Int>( color_arc_counts.size() );
//        }
//
//        Int ActiveColorCount() const
//        {
//            Int color_count = 0;
//            for( auto & x : color_arc_counts )
//            {
//                if( x.second > Int(0) )
//                {
//                    ++color_count;
//                }
//                else
//                {
//#ifdef PD_DEBUG
//                    if ( x.second < Int(0) )
//                    {
//                        eprint(MethodName("ActiveColorCount")+": Found a color with a negative color count.");
//                    }
//#endif // PD_DEBUG
//                }
//            }
//
//            PD_ASSERT(color_count <= arc_count);
//
//            return color_count;
//        }

cref<ColorCounts_T> ColorArcCounts() const
{
    if( !this->InCacheQ("ColorArcCounts") )
    {
        ColorCounts_T color_arc_counts;
        
        if( this->InCacheQ("LinkComponentArcs") )
        {
            const auto & lc_arcs = LinkComponentArcs();
            
            for( Int lc = 0; lc < lc_arcs.SublistCount(); ++lc )
            {
                color_arc_counts[lc] = lc_arcs.SublistSize(lc);
            }
        }
        else
        {
            for( Int a = 0; a < max_arc_count; ++a )
            {
                if( !ArcActiveQ(a) )
                {
                    continue;
                }
                
                const Int a_color = A_color[a];
                
                if( color_arc_counts.contains(a_color) )
                {
                    color_arc_counts[a_color] += Int(1);
                }
                else
                {
                    color_arc_counts[a_color]  = Int(1);
                }
            }
        }
        
        this->SetCache("ColorArcCounts",std::move(color_arc_counts));
    }
   
    return this->template GetCache<ColorCounts_T>("ColorArcCounts");
}


void ComputeArcColors()
{
    TOOLS_PTIMER(timer,MethodName("ColorArcCounts"));
    
    ColorCounts_T color_arc_counts;
    
    if( this->InCacheQ("ArcLinkComponents") && this->InCacheQ("LinkComponentArcs") )
    {
        ArcLinkComponents().Write(A_color.data());
        
        const auto & lc_arcs = LinkComponentArcs();
        
        for( Int lc = 0; lc < lc_arcs.SublistCount(); ++lc )
        {
            color_arc_counts[lc] = lc_arcs.SublistSize(lc);
        }
    }
    else
    {
        this->template Traverse<false,false>(
            []( const Int lc, const Int lc_begin )
            {
                (void)lc;
                (void)lc_begin;
            },
            [this]
            ( const Int a, const Int a_pos, const Int lc )
            {
                (void)a_pos;
                this->A_color[a] = lc;
            },
            [&color_arc_counts]( const Int lc, const Int lc_begin, const Int lc_end )
            {
                (void)lc;
                (void)lc_begin;
                (void)lc_end;
                
                color_arc_counts[lc] = lc_end - lc_begin;
            }
        );
    }
    
    this->SetCache("ColorArcCounts", std::move(color_arc_counts));
}

Int ColorCount()  const
{
    return int_cast<Int>(ColorArcCounts().size());
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

            const Int a_next  = NextArc(a,Head);
            
            if( a_color != A_color[a_next] )
            {
                if( violation_count < Size_T(5) )
                {
                    eprint(MethodName("CheckArcColors") + " failed at arcs a = " + ArcString(a) + " and a_next = " + ArcString(a_next) + "." );
                }
                ++violation_count;
            }
        }
    }
    
    if( violation_count > Size_T(0) )
    {
        eprint(MethodName("CheckArcColors") + ": Found = " + ToString(violation_count) + " color mismatches." );
    }
    
    return (violation_count == Size_T(0));
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
    
//    const Int a_color = A_color[a];
//    --color_arc_counts[a_color];
    A_color[a] = new_color;
//    ++color_arc_counts[new_color];
}
