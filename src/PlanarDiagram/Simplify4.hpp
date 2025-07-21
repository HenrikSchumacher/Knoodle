public:

/*!
 * @brief Attempts to simplify the planar diagram by applying the local moves implemented in the `ArcSimplifier` class as well as some global pass moves implemented in `StrandSimplifier`. Returns an estimate on the number of changes that have been made.
 *
 *  The pass moves attempt to simplify the diagram by detecting overstrands and understrands by rerouting them using shortest paths in the dual graph. Loop overstrands and loop understrands are also eliminated.
 */


Int Simplify4(
    const Int  min_dist           = 6,
    const Int  max_dist           = std::numeric_limits<Int>::max(),
    const bool compressQ          = true,
    const Int  simplify3_level    = 4,
    const Int  simplify3_max_iter = std::numeric_limits<Int>::max(),
    const bool strand_R_II_Q      = true
)
{
    if( proven_minimalQ || InvalidQ() )
    {
        return 0;
    }
    
    TOOLS_PTIMER(timer,ClassName()+"::Simplify4"
         + "(" + ToString(min_dist)
         + "," + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_max_iter)
         + "," + ToString(strand_R_II_Q)
         + ")");
    
//    static_assert(SignedIntQ<Int>,"");
    Int old_counter = 0;
    Int counter = 0;
    
    // TODO: Toggle this Boolean for multi/single-component links.
    StrandSimplifier<Int,true> S(*this);
    
//    const Int c_count = CrossingCount();
//
//    const Time start_time = Clock::now();
    
    Int dist = min_dist;
    bool do_simplify3Q = true;
    bool do_simplify4Q = max_dist > Int(0);
    
    do
    {
        old_counter = counter;
        
        dist = Min(dist,max_dist);

        Int simplify3_changes = 0;
        
        if( do_simplify3Q && (simplify3_level > Int(0)) )
        {
            // Since Simplify3 contains only inexpensive tests, we should call it first.
            simplify3_changes = Simplify3(
                simplify3_level,simplify3_max_iter,true
            );
            
            counter += simplify3_changes;
            
            if( compressQ && (simplify3_changes > Int(0)) ) { Compress(); }
        }
        
        
        if( !do_simplify4Q ) { break; }
        
        const Int o_changes = strand_R_II_Q
                            ? S.template SimplifyStrands<true >(true,dist)
                            : S.template SimplifyStrands<false>(true,dist);
        
        counter += o_changes;
        
        if( compressQ && (o_changes > Int(0)) ) { Compress(); }
        
        PD_ASSERT(CheckAll());
        
        const Int u_changes = strand_R_II_Q
                            ? S.template SimplifyStrands<true >(false,dist)
                            : S.template SimplifyStrands<false>(false,dist);
        
        counter += u_changes;
        
        if( compressQ && (u_changes > Int(0)) ) { Compress(); }

        if( dist <= Scalar::Max<Int> / Int(2) )
        {
            dist *= Int(2);
        }
        else
        {
            dist = Scalar::Max<Int>;
        }
        do_simplify3Q = (o_changes + u_changes > Int(0));
        
        PD_ASSERT(CheckAll());

    }
    while(
          (counter > old_counter)
          ||
          ( (dist <= max_dist) && (dist < arc_count) )
    );
    
    if( counter > Int(0) )
    {
        this->ClearCache();
    }
    
    if( ValidQ() && (CrossingCount() == Int(0)) )
    {
        proven_minimalQ = true;
    }
    
    PD_ASSERT(CheckAll());
    
    return counter;
}
