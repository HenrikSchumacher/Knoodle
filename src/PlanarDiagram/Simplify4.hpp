public:

bool Simplify4()
{
    ptic(ClassName()+"::Simplify4");

    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;

    Int old_test_count = 0;
    
    // TODO: Toggle this for multi-component links.
    StrandSimplifier<Int,false> S(*this);

    while( counter != old_counter )
    {
//        dump(iter);
        ++iter;

        old_counter = counter;

        old_test_count = arc_simplifier.TestCount();
        
        // DEBUGGING
        
//        tic("Simplify4 loop");
        for( Int a = 0; a < initial_arc_count; ++a )
        {
            counter += arc_simplifier(a);
        }
//        toc("Simplify4 loop");
        
//        tic("RemoveStrandLoops");
        counter += S.SimplifyStrands(true);
        counter += S.SimplifyStrands(false);
//        toc("RemoveStrandLoops");

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
    
    PD_ASSERT(CheckAll());

    ptoc(ClassName()+"::Simplify4");
    
    return (counter > 0);
}
