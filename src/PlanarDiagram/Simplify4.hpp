public:

/*!
 * @brief Attempts to simplify the planar diagram by applying the local moves implemented in the `ArcSimplifier` class as well as some global pass moves implemented in `StrandSimplifier`. Returns an estimate on the number of changes that have been made.
 *
 *  The pass moves attempt to simplify the diagram by detecting overstrands and understrands by rerouting them using shortest paths in the dual graph. Loop overstrands and loop understrands are also eliminated. 
 *
 */

Int Simplify4(
    const Int max_dist_checked,
    const bool compressQ = true,
    bool simplify3Q = true,
    bool simplify3_exhaustiveQ = true
)
{
    ptic(ClassName()+"::Simplify4"
         + "(" + ToString(max_dist_checked)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_exhaustiveQ)
         + ")");
    
    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;
    
    // TODO: Toggle this for multi-component links.
    StrandSimplifier<Int,false> S(*this);
    
    // TODO: Maybe we should recompress only in the first pass of this.
    
//    const Int c_count = CrossingCount();
//
//    const Time start_time = Clock::now();
    do
    {
        ++iter;

        old_counter = counter;
        
        if( simplify3Q )
        {
            // Since Simplify3 contains only inexpensive tests, we should call it first.
            const Int simpl3_changes = Simplify3(simplify3_exhaustiveQ);
            
            counter += simpl3_changes;
            
            if( compressQ && (simpl3_changes > 0) )
            {
                (*this) = CreateCompressed();
            }
        }
        
        const Int o_changes = S.SimplifyStrands(true,max_dist_checked);
        
        counter += o_changes;
        
        if( compressQ && (o_changes > 0) )
        {
            (*this) = CreateCompressed();
        }
        
        const Int u_changes = S.SimplifyStrands(false,max_dist_checked);
        
        counter += u_changes;
        
        if( compressQ && (u_changes > 0) )
        {
            (*this) = CreateCompressed();
        }
    }
    while( counter > old_counter );
    
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
        faces_initialized = false;
        comp_initialized  = false;

        this->ClearCache();
    }
    
    PD_ASSERT(CheckAll());

    ptoc(ClassName()+"::Simplify4"
         + "(" + ToString(max_dist_checked)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_exhaustiveQ)
         + ")");
    
    return counter;
}
