template<typename ArcSelectorFun_T>
bool CheckArcSelector( ArcSelectorFun_T & select_arcQ ) const
{
    TOOLS_PTIMER(timer,MethodName("CheckArcSelector"));
    cptr<Int> A_next_A = ArcNextArc().data();
    
    for( Int a = 0; a < MaxArcCount(); ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        
        if( select_arcQ(a) != select_arcQ(A_next_A[a]) ) { return false; }
    }
    
    return true;
}


template<bool check_arc_selectorQ = true, typename ArcSelectorFun_T>
std::pair<PD_T,Tensor1<Int,Int>> Subdiagram( ArcSelectorFun_T && select_arcQ ) const
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Subdiagram"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( InvalidQ() ) { return {InvalidDiagram(),Tensor1<Int,Int>()}; }
    
    if( ProvenUnknotQ() )
    {
        return {InvalidDiagram(),Tensor1<Int,Int>(1,last_color_deactivated)};
    }
    
    if constexpr ( check_arc_selectorQ )
    {
        if( !CheckArcSelector(select_arcQ ) )
        {
            wprint(tag() + ": Selector function select_arcQ is not valid. Returning invalid diagram.");
            return {InvalidDiagram(),Tensor1<Int,Int>()};
        }
    }
    
    std::vector<std::array<Int,5>> pd_code;
    pd_code.reserve( crossing_count );
    
    std::vector<Int> arc_colors;
    std::vector<Int> unlink_colors;
    
    Tensor1<Int,Int> c_map( MaxCrossingCount(), Uninitialized );
    
    bool lc_contains_selected_arcsQ = false;
    Int  c_counter     = 0;
    Int  c_first_label = Uninitialized;
    Int  a_counter     = 0;
    Int  lc_color      = Uninitialized;
    Int  dangling_port = Uninitialized;
    
    this->template Traverse<true>(
        [&c_first_label,&lc_color,&lc_contains_selected_arcsQ,this](
            const Int a, const Int lc, const Int lc_begin
        )
        {
            (void)lc;
            (void)lc_begin;
            lc_color       = A_color[a];
            c_first_label  = Uninitialized;
            lc_contains_selected_arcsQ = false;
        },
        [&c_counter,&c_first_label,&c_map,&a_counter,&dangling_port,&lc_contains_selected_arcsQ,&select_arcQ,&pd_code,&arc_colors,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)a_pos;
            (void)lc;
            (void)c_0_pos;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            if( !select_arcQ(a) ) { return; }
            
            lc_contains_selected_arcsQ = true;
            
            const bool side = (C_arcs(c_0,Out,Right) == a);
            
            if( !select_arcQ(C_arcs(c_0,Out,!side)) ) { return; }
            
            Int c_label;
            
            if( !c_0_visitedQ )
            {
                c_label = c_counter++;
                c_map[c_0] = c_label;
                pd_code.push_back( { Uninitialized, Uninitialized, Uninitialized, Uninitialized, Uninitialized } );
            }
            else
            {
                c_label = c_map[c_0];
            }

            arc_colors.push_back(A_color[a]);
            
            const bool right_handedQ = CrossingRightHandedQ(c_0);
            const bool overQ = ArcOverQ(a,Tail);
            
            if( c_first_label == Uninitialized )
            {
                c_first_label = c_label;
                dangling_port = overQ ? (right_handedQ ? 3 : 1) : Int(0);
            }
            else
            {
                // We know the correct arc label of the incoming arc only of this is not the first arc in the current link component.
                
                if( overQ )
                {
                    // I hate PD codes.
                    pd_code[c_label][right_handedQ ? 3 : 1] = a_counter - Int(1);
                }
                else
                {
                    pd_code[c_label][0] = a_counter - Int(1);
                }
            }
            
            if( overQ )
            {
                // I hate PD codes.
                pd_code[c_label][right_handedQ ? 1 : 3] = a_counter++;
            }
            else
            {
                pd_code[c_label][2] = a_counter++;
            }
            
            pd_code[c_label][4] = right_handedQ;
        },
        [&a_counter,&c_first_label,&dangling_port,&lc_contains_selected_arcsQ,&pd_code,&unlink_colors,&lc_color](
            const Int lc, const Int lc_begin, const Int lc_end
        )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
            
            if( c_first_label == Uninitialized )
            {
                // No crossings has been collected in this link component.
                if( lc_contains_selected_arcsQ )
                {
                    // However, there _were_ some arcs we are interested in. It is just so that they do not form any interesting crossings.
                    unlink_colors.push_back(lc_color);
                }
            }
            else
            {
                // Fix the wrap-around.
                pd_code[c_first_label][dangling_port] = a_counter - Int(1);
            }
        }
    );
    
    PD_T pd;
    
    if( c_counter > Int(0) )
    {
        pd = PD_T::FromPDCode<true>(&pd_code[0][0],c_counter);
        pd.A_color.Read( &arc_colors[0] );
    }
    
    return { pd, Tensor1<Int,Int>( &unlink_colors[0], int_cast<Int>(unlink_colors.size()) ) };
}

template<bool check_arc_selectorQ = true>
std::pair<PD_T,Tensor1<Int,Int>> SubdiagramByColor( const Int color ) const
{
    TOOLS_PTIMER(timer,MethodName("SubdiagramByColor"));
    return this->template Subdiagram<check_arc_selectorQ>(
        [this, color]( const Int a ) -> bool { return A_color[a] == color; }
    );
}

template<bool check_arc_selectorQ = true, IntQ ExtInt, IntQ ExtInt2>
std::pair<PD_T,Tensor1<Int,Int>> SubdiagramByColors(
    cptr<ExtInt> colors, ExtInt2 color_count
) const
{
    TOOLS_PTIMER(timer,MethodName("SubdiagramByColors"));
    
    SetContainer<Int> color_set;
    for( Int i = 0; i < color_count; ++i )
    {
        color_set.insert(int_cast<Int>(colors[i]));
    }
    
    return this->template Subdiagram<check_arc_selectorQ>(
        [&color_set,this]( const Int a ) -> bool
        {
            return color_set.contains(this->ArcColors()[a]);
        }
    );
}
