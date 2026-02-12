struct SimplifyLocal_Args_T
{
    Size_T max_iter              = Scalar::Max<Size_T>;
    Int    compression_threshold = 0;
    bool   compressQ             = true;
    UInt8  opt_level             = 4;
};

friend std::string ToString( cref<SimplifyLocal_Args_T> args )
{
    return std::string("{ ")
            +   ".max_iter = " + ToString(args.max_iter)
            + ", .compression_threshold = " + ToString(args.compression_threshold)
            + ", .compressQ = " + ToString(args.compressQ)
            + ", .opt_level = " + ToString(args.opt_level)
    + " }";
}

Size_T SimplifyLocal( cref<SimplifyLocal_Args_T> args )
{
    if( DiagramCount() == Int(0) )
    {
        return 0;
    }
    
    const UInt8 level = Clamp(args.opt_level, UInt8(0), UInt8(4));
    
    switch ( level )
    {
        case UInt8(0):
        {
            return 0;
        }
        case UInt8(1):
        {
            return SimplifyLocal_impl<1,true>(args);
        }
        case UInt8(2):
        {
            return SimplifyLocal_impl<2,true>(args);
        }
        case UInt8(3):
        {
            return SimplifyLocal_impl<3,true>(args);
        }
        case UInt8(4):
        {
            return SimplifyLocal_impl<4,true>(args);
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

template<UInt8 opt_level, bool multi_compQ>
Size_T SimplifyLocal_impl( cref<SimplifyLocal_Args_T> args )
{
    TOOLS_PTIMER(timer,MethodName("SimplifyLocal_impl")+"<" + ToString(opt_level) + "," + ToString(multi_compQ) + ">");
    
    using ArcSimplifier_T = ArcSimplifier2<Int,opt_level,multi_compQ>;
    
    Size_T counter = 0;
        
    using std::swap;
    
    PD_ASSERT(pd_done.empty());
    PD_ASSERT(pd_todo.empty());
    
    // TODO: Suboptimal cycling through the list. We should use stack approach as in SimplifyGlobal.
    do
    {
        for( PD_T & pd : pd_list )
        {
            const Size_T changes = ArcSimplifier_T( *this, pd,
                {
                    .max_iter              = args.max_iter,
                    .compression_threshold = args.compression_threshold,
                    .compressQ             = args.compressQ
                }
            )();
            
            counter += changes;
            
            if( changes > Size_T(0) ) { pd.ClearCache(); }
            
            PushDiagramDone( std::move(pd) );
        }

        swap( pd_list, pd_todo );
        pd_todo = PD_List_T();
    }
    while( pd_list.size() != Size_T(0) );
    
    PD_ASSERT( pd_list.empty() );
    PD_ASSERT( pd_todo.empty() );

    swap( pd_list, pd_done );
    
    if( counter > Size_T(0) )
    {
        SortByCrossingCount();
        this->ClearCache();
    }
    
    return counter;
}
