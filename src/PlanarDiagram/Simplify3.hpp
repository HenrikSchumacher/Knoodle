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

        this->ClearCache();
    }

    ptoc(ClassName()+"::Simplify3");
}




bool SimplifyArc3( const Int a )
{
    if( arc_simplifier(a) )
    {
        faces_initialized = false;
        
        this->ClearCache();
        
        return true;
    }
    else
    {
        return false;
    }
    
    return false;
}



void Simplify4()
{
    ptic(ClassName()+"::Simplify4");

    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;

    Int old_test_count = 0;

    while( counter != old_counter )
    {
        ++iter;

        old_counter = counter;

        old_test_count = arc_simplifier4.TestCount();
        // DEBUGGING
//        dump(iter);
//        tic("Simplify3 loop");
        for( Int a = 0; a < initial_arc_count; ++a )
        {
            counter += arc_simplifier4(a);
        }
//        toc("Simplify3 loop");
//
//        const Int changes = counter-old_counter;
//        dump(changes);
//        const Int test_count = arc_simplifier4.TestCount()-old_test_count;
//        dump(test_count);
    }
    
//    dump(arc_simplifier4.TestCount());

    if( counter > 0 )
    {
        faces_initialized = false;

        this->ClearCache();
    }

    ptoc(ClassName()+"::Simplify4");
}

bool SimplifyArc4( const Int a )
{
    if( arc_simplifier4(a) )
    {
        faces_initialized = false;
        
        this->ClearCache();
        
        return true;
    }
    else
    {
        return false;
    }
    
    return false;
}


//void Simplify3_Stack()
//{
//    ptic(ClassName()+"::Simplify3_Stack");
//
//    stack.reserve(initial_arc_count);
//    
//    dump(stack.capacity());
//    
//    stack.clear();
//    
//    for( Int a = initial_arc_count; a--> 0; )
//    {
////        if( ArcChangedQ(a) )
////        {
////            stack.push_back(a);
////        }
//        
//        if( ArcActiveQ(a) )
//        {
//            stack.push_back(a);
//        }
//    }
//    
//    Int counter = 0;
//    
//    tic("Simplify3 stack");
//    while( !stack.empty() )
//    {
////        Size_T size = stack.size();
////        Size_T s    = std::min( Size_T(16), size );
////     
////        logvalprint(
////            "stack",
////            ArrayToString( &stack[size-s], {s} )
////        );
//        
//        const Int a = stack.back();
//        
//        stack.pop_back();
//        
//        counter += arc_simplifier(a);
//    }
//    toc("Simplify3 stack");
//    
//    dump(arc_simplifier.TestCount());
//    dump(stack.capacity());
//    dump(initial_arc_count);
//
//    if( counter > 0 )
//    {
//        faces_initialized = false;
//        
//        this->ClearCache();
//    }
//    
//    ptoc(ClassName()+"::Simplify3_Stack");
//}
//
//
//
//
//
