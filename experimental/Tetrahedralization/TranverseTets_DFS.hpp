// Not so important.

template<
    typename ConnCompPre_T, typename TetFun_T, typename ConnCompPost_T
>
void TraverseTets_DFS(
    ConnCompPre_T && cc_pre, TetFun_T && tet_fun, ConnCompPost_T && cc_post
)  const
{
    Tensor1<bool,Int> tet_visitedQ ( tet_capacity, false );
    
    Tensor1<Int,Int> stack ( Int{4} * tet_capacity + Int{1}, Uninitialized );
    Int stack_ptr = 0;
    
    cptr<Int> T_G = tet_glue.data();
    
    auto conditional_push = [&stack,stack_ptr,T_G,this]( const Int df )
    {
        if( !visitedQ(t) && (T_G[df] != Uninitialized) )
        {
            stack[++stack_ptr] = ToDFace(t,f);
        }
    };
    

    Int cc_counter = 0;
    
    for( Int t_ptr = 0; t_ptr < tet_capacity; ++t_ptr )
    {
        if( !TetActiveQ(t_ptr) || tet_visitedQ[t_ptr] ) { continue; };
        
        cc_pre(cc_counter);
        
        tet_fun(t_ptr);
        visitedQ[t_ptr] = true;
        
        stack_ptr = 0;
        for( Int i = 0; i < 4; ++i ) { conditional_push( ToDface(t_ptr,i) ); }
             
        while( stack_ptr >= Int{0} )
        {
            auto df = stack[stack_ptr--]; // pop;
            
            // Tet t must be active, because we start with active tets and those are allowed t be glued only to active tets.
            
            if( tet_visitedQ[t] ) { continue; }

            const Int g = tet_glue(t,f);
            
            // g is guaranteed to be != Uninitialized.
            
            auto [t_,f_,r_] = FromGlue(g);
            
            // We cycle over the face of t_ not equal to f_.
            // Those local indices are the same as the vertices of f_!
            for( Int v : face_vertex_lut[f_] )
            {
                conditional_push(t_,v);
            }
        }
        
        cc_post(cc_counter);
        ++cc_counter;
        t_ptr = next_tet(t_ptr);
    }
    
    
    Int t_ptr = next_tet(Int{0});
    

    while( t_ptr < tet_capacity )
    {
        cc_pre(cc_counter);
        
    }
    
}
