public:

bool TetGluingsOkayQ() const
{
    for( Int t = 0; t < tet_capacity; ++ t )
    {
        if( !TetActiveQ(t) ) { continue; }

        if( !this->template TetGluingOkayQ<false>(t) ) { return false; }
    }
    
    return true;
}

template<bool bound_checkQ = true>
bool TetGluingOkayQ( const Int t ) const
{
    if constexpr ( bound_checkQ )
    {
        if( (t < Int{0}) || (t >= tet_capacity) )
        {
            eprint(MethodName("TetGluingsOkayQ"),": tet index t = ", t, " is out of bounds.");
            return false;
        }
    }
    
    if( !TetActiveQ(t) ) { return true; }
    
    for( Int f = 0; f < Int{4}; ++f )
    {
        if( !OkayQ_impl(Dface_T{t,f}) ) { return false; }
    }
    
    return true;
}
    
bool OkayQ( cref<Dface_T> df ) const
{
    if( (df.t < Int{0}) || (df.t >= tet_capacity) )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df), "): tet index is out of bounds.");
        return false;
    }
    
    if( (df.f < Int{0}) || (df.f >= Int{4}) )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df), "): face index is out out bounds");
        return false;
    }

    if( !TetActiveQ(df.t) ) { return true; }
    
    return OkayQ_impl(df);
}

private:

bool OkayQ_impl( cref<Dface_T> df_0 ) const
{
    // From this point on we assume that f_0 is in-bounds and t_0 is valid, in-bounds, and active.
    
    const Gluing_T g_0 = Gluing(df_0);
    
    if( !g_0.ValidQ() ) { return true; }
    
    Dface_T df_1 = g_0.df;
    
    if( (df_1.t < Int{0}) || (df_1.t >= tet_capacity) )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): glued tet index ", df_1.t, "is out of bounds.");
        return false;
    }
    
    if( !TetActiveQ(df_1.t) )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): glued tet ", df_1.t, " is not active.");
        return false;
    }
    
    if( (df_1.f < Int{0}) || (df_1.f >= Int{4}) )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): glued face index ", df_1.f, " is out of bounds.");
        return false;
    }
    
    const Gluing_T g_1 = Gluing(df_1);
    
    if( !g_1.ValidQ() )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): glued tet ", df_1.t, " is not glued from the other side.");
        return false;
    }
    
    const Dface_T df_2 = g_1.df;
    
    if( df_0.t != df_2.t )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): tet indices do not match.");
        return false;
    };
    
    if( df_0.f != df_2.f )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): face indices do not match.");
        return false;
    };
    
    if( g_0.r != g_1.r )
    {
        eprint(MethodName("OkayQ"),"(", ToString(df_0), "): rotations do not match.");
        return false;
    };
    
    return true;
}
