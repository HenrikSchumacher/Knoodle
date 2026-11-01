public:

Int TetCount() const { return tet_count; }

Int TetCapacity() const { return tet_capacity; }

cref<TetGluingContainer_T> TetGluings() const
{
    return tet_glue;
}

bool TetActiveQ( const Int t ) const { return tet_state[t]; }

/*!@brief Move the internal search pointer to the first next inactive tet and returns its index.*/
Int NextInactiveTet() const
{
    if( tet_count >= tet_capacity ) { return Uninitialized; }
    
    while( TetActiveQ(t_search_ptr) )
    {
        ++t_search_ptr;
        
        if( t_search_ptr >= tet_capacity ) { t_search_ptr = 0; }
    }
    
    return t_search_ptr;
}

Int CreateTet()
{
    if( LockedQ() ) { LockMessage("CreateTet"); return Uninitialized; }
    
    return CreateTet_Private();
}

//void CreateTets( const Int n )
//{
//    if( LockedQ() ) { LockMessage("CreateTets"); return Uninitialized; }
//    
//    CreateTets_Private(n);
//}



bool Glue( cref<Dface_T> df_0, cref<Dface_T> df_1, const Int rot )
{
    if( LockedQ() ) { LockMessage("Glue"); return Uninitialized; }
    
    return this->template Glue_Private<true>(df_0, df_1, rot);
}

//bool Glue( const Int t_0, const Int f_0, const Int t_1, const Int f_1, const Int rot )
//{
//    if( LockedQ() ) { LockMessage("Glue"); return Uninitialized; }
//
//    return this->template GlueTets_Private<true>(t_0, f_0, t_1, f_1, rot);
//}

//Int InsertTet( const TetGlue_T & tet )
//{
//    if( LockedQ() ) { LockMessage("InsertTet"); return Uninitialized; }
//    
//    return InsertTet_Private(tet);
//}
//
//Int InsertTet( const Int g_0, const Int g_1, const Int g_2, const Int g_3 )
//{
//    if( LockedQ() ) { LockMessage("InsertTet"); return Uninitialized; }
//    
//    return InsertTet_Private(g_0, g_1, g_2, g_3);
//}

private:

template<bool checksQ = true>
bool Glue_Private( cref<Dface_T> df_0, cref<Dface_T> df_1, const Int r )
{
    // TODO: Check / guarantee that df_0.f and df_1.f lie in {0,1,2,3}.
    // TODO: Check / guarantee that r lies in {0,1,2}.
    if constexpr ( checksQ )
    {
        if( !TetActiveQ(df_0.t) )
        {
            eprint(MethodName("Glue_Private"), ": first tet ", df_0.t, " is not active. Aborting.");
            return false;
        }
        
        if( !TetActiveQ(df_1.t) )
        {
            eprint(MethodName("Glue_Private"), ": second tet ", df_1.t, " is not active. Aborting.");
            return false;
        }
    }
    
    Gluing_T g_0 = Gluing(df_0);
    Gluing_T g_1 = Gluing(df_1);

    
    if constexpr ( checksQ )
    {
        if( g_0.ValidQ() )
        {
            eprint(MethodName("Glue_Private"), ": first tet's face ", df_0.f, " is already glued. Aborting.");
            return false;
        }
        
        if( g_1.ValidQ() )
        {
            eprint(MethodName("Glue_Private"), ": second tet's face ", df_1.f, " is already glued. Aborting.");
            return false;
        }
    }
    
    // Seems a bit strange, but the rotation needs to be the same on both sides.
    tet_glue(df_1.t,df_1.f) = Gluing_T(df_0, r).Compress();
    tet_glue(df_0.t,df_0.f) = Gluing_T(df_1, r).Compress();
    
    return true;
}

Int InsertTet_Private( const Int g_0, const Int g_1, const Int g_2, const Int g_3 )
{
    if( tet_count >= tet_capacity ) { Expand(); }
    
    const Int t = NextInactiveTet();
    tet_glue(t,Int{0}) = g_0;
    tet_glue(t,Int{1}) = g_1;
    tet_glue(t,Int{2}) = g_2;
    tet_glue(t,Int{3}) = g_3;
    tet_state[t] = true;
    ++tet_count;
    return t;
}

//Int InsertTet_Private( const TetGlue_T & tet )
//{
//    return InsertTet_Private( tet[0], tet[1], tet[2], tet[3] );
//}

Int CreateTet_Private()
{
    return InsertTet_Private( Uninitialized, Uninitialized, Uninitialized, Uninitialized );
}

//void CreateTets_Private( const Int n )
//{
//    RequireCapacity( tet_capacity + n );
//    for( Int i = 0; i < n; ++i )
//    {
//        (void)CreateTet_Private();
//    }
//}


// No checks, therefore it is private.
void DeactivateTet( const Int t )
{
    if( TetActiveQ(t) )
    {
        tet_state[t] = false;
        --tet_count;
    }
}
