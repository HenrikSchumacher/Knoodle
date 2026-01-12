Size_T SimplifyLocal(
    const Int  opt_level = 4,
    const bool compressQ   = true
)
{
    if( DiagramCount() == Int(0) )
    {
        return 0;
    }
    
    const Int level    = Clamp(opt_level, Int(0), Int(4));
    const Int max_iter = std::numeric_limits<Int>::max();
    
    switch ( level )
    {
        case 0:
        {
            return 0;
        }
        case 1:
        {
            return SimplifyLocal_impl<1,true>(max_iter,compressQ);;
        }
        case 2:
        {
            return SimplifyLocal_impl<2,true>(max_iter,compressQ);;
        }
        case 3:
        {
            return SimplifyLocal_impl<3,true>(max_iter,compressQ);;
        }
        case 4:
        {
            return SimplifyLocal_impl<4,true>(max_iter,compressQ);;
        }
        default:
        {
            eprint( MethodName("SimplifyLocal")+": Value " + ToString(level) + " is invalid" );
            return 0;
        }
    }
    
    return 0;
}

private:

template<Int opt_level, bool multi_compQ>
Size_T SimplifyLocal_impl( const Size_T max_iter, const bool compressQ )
{
    TOOLS_PTIMER(timer,MethodName("SimplifyLocal_impl")+"<" + ToString(opt_level) + "," + ToString(multi_compQ) + ">");
    
    using ArcSimplifier_T = ArcSimplifier2<Int,opt_level,multi_compQ>;
    
    Size_T counter     = 0;
        
    // This list is used for the diagrams that cannot processed any further.
    PDList_T pd_list_done;
    
    using std::swap;
    
    PD_ASSERT( pd_list_new.size() == Size_T(0) );
    
    do
    {
        for( PD_T & pd : pd_list )
        {
            const Size_T changes = ArcSimplifier_T( *this, pd, max_iter, compressQ )();
            
            counter += changes;
            
            if( changes > Size_T(0) )
            {
                pd.ClearCache();
            }
            
            if( pd.ValidQ() )
            {
                pd_list_done.push_back( std::move(pd) );
            }
            else
            {
                wprint(MethodName("SimplifyLocal_impl")+": A diagram that came out of " + pd.ClassName() + " was identified as invalid and discarded. This is per se not a bad thing to happen." );
            }
        }

        swap( pd_list, pd_list_new );
        pd_list_new = PDList_T();
    }
    while( pd_list.size() != Size_T(0) );
    
    PD_ASSERT( pd_list_new.size() == Size_T(0) );
    PD_ASSERT( pd_list.size()     == Size_T(0) );
    
    swap( pd_list, pd_list_done );
    
    return counter;
}
