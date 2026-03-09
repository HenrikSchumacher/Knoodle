public:

template<bool check_arc_selectorQ = true, typename PDArcSelectorFun_T>
PDC_T Subcomplex( PDArcSelectorFun_T && pd_arc_selectQ ) const
{
    TOOLS_PTIMER(timer,MethodName("Subcomplex"));
    PDC_T pdc;
    
    for( PD_T & pd : pd_list )
    {
        auto arc_selectQ = [&pd,&pd_arc_selectQ]( const Int a )
        {
            return pd_arc_selectQ(pd,a);
        };
        
        auto [pd_new,unlink_colors] = pd.template Subdiagram<check_arc_selectorQ>(arc_selectQ);

        pdc.Push(std::move(pd_new));
        
        for( Int color : unlink_colors )
        {
            pdc.Push(PD_T::Unlink(color));
        }
    }
    
    pdc.ClearCache();
    
    return pdc;
}

template<bool check_arc_selectorQ = true>
PDC_T SubcomplexByColor( const Int color ) const
{
    TOOLS_PTIMER(timer,MethodName("SubcomplexByColor"));
    return this->template Subcomplex<check_arc_selectorQ>(
        [color]( cref<PD_T> pd, const Int a ) -> bool
        {
            return (pd.ArcColors()[a] == color);
        }
    );
}

template<bool check_arc_selectorQ = true, IntQ ExtInt, IntQ ExtInt2>
PDC_T SubcomplexByColors(
    cptr<ExtInt> colors, ExtInt2 color_count
) const
{
    TOOLS_PTIMER(timer,MethodName("SubcomplexByColors"));

    SetContainer<Int> color_set;
    for( Int i = 0; i < color_count; ++i )
    {
        color_set.insert(int_cast<Int>(colors[i]));
    }
    
    return this->template Subcomplex<check_arc_selectorQ>(
        [&color_set]( cref<PD_T> pd, const Int a ) -> bool
        {
            (void)pd;
            return color_set.contains(pd.ArcColors()[a]);
        }
    );
}
