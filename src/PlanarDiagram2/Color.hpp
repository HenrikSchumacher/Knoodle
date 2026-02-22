public:

cref<ColorCounts_T> ColorArcCounts() const
{
    if( !this->InCacheQ("ColorArcCounts") )
    {
        ColorCounts_T color_arc_counts;
        for( Int a = 0; a < max_arc_count; ++a )
        {
            if( ArcActiveQ(a) ) { Increment(color_arc_counts,A_color[a]); }
        }
        this->SetCache("ColorArcCounts",std::move(color_arc_counts));
    }
    return this->template GetCache<ColorCounts_T>("ColorArcCounts");
}

Int MaxColor() const
{
    if( InvalidQ() ) { return 0; }
    
    Int max_color = 0;
    
    if( (crossing_count <= Int(0)) )
    {
        if( last_color_deactivated != Uninitialized )
        {
            return last_color_deactivated;
        }
    }
    
    auto & lut = ColorArcCounts();
    
    for( auto & x : lut )
    {
        max_color = Max(max_color,x.first);
    }
    
    return max_color;
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


bool ContainsColor( Int color ) const
{
    auto & color_counts = ColorArcCounts();
    
    return color_counts.contains(color) && (color_counts[color] > Int(0));
}

// TODO: There must be a more efficient way to do this!
Int FindEdgeWithColor( Int color ) const
{
    if( crossing_count <= Int(0) ) { return Uninitialized; }
   
    if( !ContainsColor(color) ) { return Uninitialized; }
    
    Int a = 0;
    while( a < max_arc_count )
    {
        if( ActiveQ(a) && (A_color[a] == color) )
        {
            return a;
        }
    }
    
    eprint(MethodName("FindEdgeWithColor") + ": ContainsColor(color) reported that diagram contains an arc with color " + ToString(color) + ", but no such arc was found. Try to ClearCache() and do the query again.");
    
    return Uninitialized;
}

private:

// Dangerous function. At the moment, only for internal use.
void ChangeArcColor_Private( const Int a, const Int new_color )
{
    PD_ASSERT( ArcActiveQ(a) );

    A_color[a] = new_color;
}



//PD_T ExtractByColor( const Int color )
//{
//    if( InvalidQ() ) { return InvalidDiagram(); }
//    
//    if( ProvenUnknotQ() ) { return Unknot( last_color_deactivated ); }
//    
//    
//    Tensor1<Int,Int> c_map ( max_crossing_count );
//    Tensor1<Int,Int> a_map ( max_arc_count );
//    
//    bool color_existQ = false;
//    Int  c_counter    = 0;
//    Int  a_counter    = 0;
//    Int  c_dangling   = Uninitialized;  // If ValidIndexQ(c_dangling), then this is the head of the previous are collected
//    Int  c_first_label = Uninitialized;
//    
//    this->template Traverse<true>(
//        [&c_dangling,&c_first_label]( const Int lc, const Int lc_begin )
//        {
//            (void)lc;
//            (void)lc_begin;
//            c_dangling    = Uninitialized;
//            c_first_label = Uninitialized;
//        },
//        [&color_existQ,&c_counter,&c_dangling,&c_first_label,&a_counter,&c_map,&a_map,color,this](
//            const Int a,   const Int a_pos,   const Int  lc,
//            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
//            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
//        )
//        {
//            (void)a_pos;
//            (void)lc;
//            (void)c_0_pos;
//            (void)c_0_visitedQ;
//            (void)c_1;
//            (void)c_1_pos;
//            (void)c_1_visitedQ;
//            
//            if( A_color[a] == color )
//            {
//                color_existQ = true;
//                
//                C_Arcs_T C = CopyCrossing(c_0);
//                
//                const bool side = (C[Out][Right] == a);
//                
//                PD_ASSERT(A_color[C[In][!side]] == color);
//                
//                if( (A_color[C[Out][!side]] == color) && (A_color[C[In][side]] == color) )
//                {
//                    const Int c_label = c_0_visitedQ ? c_map[c_0] : c_counter++;
//                    
//                    // If ValidIndexQ(c_dangling), then this is the head of the previous are collected.
//                    // If ValidIndexQ(c_first_label), then this is the label of the first collected crossing in this c_first_label.
//                    
//                    if( ValidIndexQ(c_dangling) )
//                    {
//                        c_map[c_dangling] = c_label;
//                    }
//                    else
//                    {
//                        c_first_label = c_label;
//                    }
//                    c_map[c_0] = c_label;
//                    c_dangling = c_1;
//                    a_map[a]   = a_counter++;
//                }
//                else
//                {
//                    c_map[c_0] = UninitializedIndex();
//                    a_map[a  ] = UninitializedIndex();
//                }
//            }
//            
//        },
//        [&c_dangling,&c_first_label,&c_map]( const Int lc, const Int lc_begin, const Int lc_end )
//        {
//            (void)lc;
//            (void)lc_begin;
//            (void)lc_end;
//            c_map[c_dangling] = c_first_label;
//        }
//    );
//    
//    TOOLS_DUMP(c_map);
//    TOOLS_DUMP(a_map);
//    
//    if( !color_existQ ) { return InvalidDiagram(); }
//    
//    if( c_counter == Int(0) ) { return InvalidDiagram(); }
//    
//    return CreateRelabelled( c_map, c_counter, a_map, a_counter, true /*surjectiveQ*/ );
//}
