public:

void Simplify3()
{
    ptic(ClassName()+"::Simplify3");

    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;

    Int old_test_count = 0;

    while( counter != old_counter )
    {
        ++iter;
//        dump(iter);
        old_counter = counter;

        old_test_count = arc_simplifier.TestCount();
        
        // DEBUGGING
        
//        tic("Simplify3 loop");
        for( Int a = 0; a < initial_arc_count; ++a )
        {
            counter += arc_simplifier(a);
        }
//        toc("Simplify3 loop");
//
//        const Int changes = counter-old_counter;
//        dump(changes);
//        const Int test_count = arc_simplifier.TestCount()-old_test_count;
//        dump(test_count);
    }
    
//    dump(arc_simplifier.TestCount());

    if( counter > 0 )
    {
        faces_initialized = false;
        comp_initialized  = false;
        
        this->ClearCache();
    }

    ptoc(ClassName()+"::Simplify3");
}




bool SimplifyArc3( const Int a )
{
    if( arc_simplifier(a) )
    {
        faces_initialized = false;
        comp_initialized  = false;
        
        this->ClearCache();
        
        return true;
    }
    else
    {
        return false;
    }
    
    return false;
}
