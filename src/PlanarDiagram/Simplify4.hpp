public:

/*!
 * @brief Attempts to simplify the planar diagram by applying the local moves implemented in the `ArcSimplifier` class as well as some global pass moves implemented in `StrandSimplifier`. Returns an estimate on the number of changes that have been made.
 *
 *  The pass moves attempt to simplify the diagram by detecting overstrands and understrands by rerouting them using shortest paths in the dual graph. Loop overstrands and loop understrands are also eliminated. 
 *
 */

Int Simplify4(
    const Int max_dist,
    const bool compressQ = true,
    const Size_T simplify3_level = 4,
    bool simplify3_exhaustiveQ = true,
    bool strand_R_II_Q = true
)
{
    ptic(ClassName()+"::Simplify4"
         + "(" + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_exhaustiveQ)
         + ")");
    
    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;
    
    // TODO: Toggle this Boolean for multi-component links.
    StrandSimplifier<Int,false> S(*this);
    
    // TODO: Maybe we should recompress only in the first pass of this?
    
//    const Int c_count = CrossingCount();
//
//    const Time start_time = Clock::now();
    do
    {
        ++iter;
        
//        logdump(iter);

        old_counter = counter;
        
//        logprint("Simplify3");
//        logdump(CrossingCount());
        
        if( simplify3_level > 0 )
        {
            // Since Simplify3 contains only inexpensive tests, we should call it first.
            const Int simpl3_changes = Simplify3(simplify3_level,simplify3_exhaustiveQ);
            
            counter += simpl3_changes;
            
            if( compressQ && (simpl3_changes > 0) )
            {
                (*this) = CreateCompressed();
            }
        }
        
//        logprint("overstrands");
//        logdump(CrossingCount());
        
        const Int o_changes = strand_R_II_Q
        ? S.template SimplifyStrands<true >(true,max_dist)
        : S.template SimplifyStrands<false>(true,max_dist);
        
        counter += o_changes;
        
        if( compressQ && (o_changes > 0) )
        {
            (*this) = CreateCompressed();
        }
        
        PD_ASSERT(CheckAll());
        
        // TODO: Should we do Simplify3 once more?
        // I tried it with exhaustive simplifcation and the timings became slightly worse.
        // Doing only one pass does not seem to harm, but it also does not help.
        
//        logprint("understrands");
//        logdump(CrossingCount());
        
        const Int u_changes = strand_R_II_Q
        ? S.template SimplifyStrands<true >(false,max_dist)
        : S.template SimplifyStrands<false>(false,max_dist);
        
        
        counter += u_changes;
        
        if( compressQ && (u_changes > 0) )
        {
            (*this) = CreateCompressed();
        }
        
        PD_ASSERT(CheckAll());
    }
    while( counter > old_counter );
    
//    dump(iter);
    
//    const Time stop_time = Clock::now();
//    
//    if( CrossingCount() < c_count)
//    {
//        logprint( ClassName()+"::Simplify4 needed " + ToString(iter) + " passes to reduce the number of crossings from " + ToString(c_count) + " to " + ToString(CrossingCount()) +".\nTime elapsed = " +ToString(Tools::Duration(start_time,stop_time)) + " s.");
//    }
//    else
//    {
//        logprint( ClassName()+"::Simplify4 did not find any improvements. Number of crossings = " + ToString(ToString(CrossingCount())) + ".\nTime elapsed = " +ToString(Tools::Duration(start_time,stop_time)) + " s.");
//    }
    

    if( counter > 0 )
    {
        comp_initialized  = false;

        this->ClearCache();
    }
    
    PD_ASSERT(CheckAll());

    ptoc(ClassName()+"::Simplify4"
         + "(" + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_exhaustiveQ)
         + ")");
    
    return counter;
}
