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
