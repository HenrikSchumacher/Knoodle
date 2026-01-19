void Unite()
{
    Tensor1<Int,Size_T> crossing_ptr ( DiagramCount() + Int(1) );
    
    const Size_T diagram_count = Size_T(DiagramCount());
    
    Int crossing_count = 0;
    Int arc_count = 0;
    
    PDList_T unlinks;
    
    for( Size_T idx = 0; idx < diagram_count; ++idx )
    {
        mref<PD_T> pd = pd_list[idx];
        
        if( pd.InvalidQ() ) { continue; }
        
        if( pd.ProvenUnknotQ() )
        {
            unlinks.push_back( std::move(pd) );
            
            pd_list[idx] = PD_T::InvalidDiagram();
            
            crossing_ptr[idx+Int(1)] = crossing_ptr[idx];
            
            wprint(MethodName("Unite") +": Diagram no. " + ToString(idx)+ " is an unlink. This method might not work correctly for this case, yet.");
            continue;
        }
        else
        {
            crossing_count += pd.crossing_count;
            arc_count      += pd.arc_count;
            crossing_ptr[idx+1] = crossing_ptr[idx] + pd.max_crossing_count;
        }
    }
    
    PD_T pd_union ( crossing_ptr.Last(), true );
    
    pd_union.crossing_count = crossing_count;
    pd_union.arc_count      = arc_count;

    for( Size_T idx = 0; idx < diagram_count; ++idx )
    {
        mref<PD_T> pd = pd_list[idx];
        
        if( pd.InvalidQ() ) { continue; }

        const Int C_pos = crossing_ptr[idx];
        const Int A_pos = Int(2) * C_pos;

        for( Int c = 0; c < pd.max_crossing_count; ++c )
        {
            const Int c_pos = C_pos + c;
            C_Arc_T C = pd.CopyCrossing(c);
            C += A_pos;
            C.Write(pd_union.C_arcs.data(c_pos));
        }
        pd.C_state.Write( pd_union.C_state.data(C_pos) );
        
        for( Int a = 0; a < pd.max_arc_count; ++a )
        {
            const Int a_pos = A_pos + a;
            A_Cross_T A = pd.CopyArc(a);
            A += C_pos;
            A.Write(pd_union.A_cross.data(a_pos));
        }
        pd.A_state.Write( pd_union.A_state.data(A_pos) );
        pd.A_color.Write( pd_union.A_color.data(A_pos) );
    }
    
    pd_list.clear();
    pd_list.push_back( std::move(pd_union) );
    
    for( PD_T & pd : unlinks )
    {
        pd_list.push_back( std::move(pd) );
    }
    
    ClearCache();
}
