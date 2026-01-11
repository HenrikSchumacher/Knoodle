void ComputeArcColors()
{
    TOOLS_PTIMER(timer,MethodName("ComputeArcColors"));
    
    if( this->InCacheQ("ArcLinkComponents") )
    {
        ArcLinkComponents().Write(A_color.data());
        
        for( Int color = 0; color < LinkComponentCount(); ++color )
        {
            color_list.insert(color);
        }
    }
    else
    {
        this->template Traverse<false,false>(
            [this]( const Int lc, const Int lc_begin )
            {
                (void)lc_begin;
                this->color_list.insert(lc);
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
