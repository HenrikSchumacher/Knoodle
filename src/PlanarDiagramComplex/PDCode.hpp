public:


template<IntQ T = Int>
Tensor2<T,Int> PDCode()
{
    TOOLS_PTIMER(timer,ClassName("PDCode") + "<" + TypeName<T> + ">");
    
    Size_T c_count = 0;
    
    for( PD_T & pd : pd_list )
    {
        if( pd.InvalidQ() ) { continue; }
        
        if( pd.AnelloQ() )
        {
            ++c_count;
        }
        else
        {
            c_count += ToSize_T(pd.CrossingCount());
        }
    }
    

    Tensor2<T,Int> pd_code ( c_count, Size_T(7) );
    
    WritePDCode( pd_code.data() );
    
    return pd_code;
}

template<IntQ T>
void WritePDCode( mptr<T> pd_code ) const
{
    TOOLS_PTIMER(timer,MethodName("WritePDCode") + "<" + TypeName<T> + ">");
    
    constexpr Size_T step = 7;
    
    Size_T pos = 0;
    T offset   = 0;
    
    for( PD_T & pd : pd_list )
    {
        if( pd.InvalidQ() ) { continue; }
        
        pd.template WritePDCode<T,true,true,false,true>( &pd_code[pos], offset );
        
        if( pd.AnelloQ() )
        {
            pos    += step;
            offset += T(2);
        }
        else
        {
            offset += int_cast<T>(pd.ArcCount());
            pos    += step * ToSize_T(pd.CrossingCount());
        }
    }
}
