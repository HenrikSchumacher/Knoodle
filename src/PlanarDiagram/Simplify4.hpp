public:

/*!
 * @brief Attempts to simplify the planar diagram by applying the local moves implemented in the `ArcSimplifier` class as well as some global pass moves implemented in `StrandSimplifier`. Returns an estimate on the number of changes that have been made.
 *
 *  The pass moves attempt to simplify the diagram by detecting overstrands and understrands by rerouting them using shortest paths in the dual graph. Loop overstrands and loop understrands are also eliminated.
 */

Int Simplify4(
    const Int  max_dist           = std::numeric_limits<Int>::max(),
    const bool compressQ          = true,
    const Int  simplify3_level    = 4,
    const Int  simplify3_max_iter = std::numeric_limits<Int>::max(),
    const bool strand_R_II_Q      = true
)
{
    if( provably_minimalQ )
    {
        return 0;
    }
    
    TOOLS_PTIC(ClassName()+"::Simplify4"
         + "(" + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_max_iter)
         + "," + ToString(strand_R_II_Q)
         + ")");
    
    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;
    
    // TODO: Toggle this Boolean for multi-component links.
    StrandSimplifier<Int,true> S(*this);
    
    // TODO: Maybe we should recompress only in the first pass of this?
    
//    const Int c_count = CrossingCount();
//
//    const Time start_time = Clock::now();
    
    do
    {
        ++iter;

        old_counter = counter;
        
        if( simplify3_level > 0 )
        {
            // Since Simplify3 contains only inexpensive tests, we should call it first.
            const Int simpl3_changes = Simplify3(
                simplify3_level,simplify3_max_iter,true
            );
            
            counter += simpl3_changes;
            
            if( compressQ && (simpl3_changes > 0) )
            {
                (*this) = CreateCompressed();
            }
        }
        
        const Int o_changes = strand_R_II_Q
            ? S.template SimplifyStrands<true >(true,max_dist)
            : S.template SimplifyStrands<false>(true,max_dist);
        
        counter += o_changes;
        
        if( compressQ && (o_changes > 0) )
        {
            (*this) = CreateCompressed();
        }
        
        PD_ASSERT(CheckAll());
        
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
    
    if( counter > 0 )
    {
        this->ClearCache();
    }
    
    PD_ASSERT(CheckAll());

    TOOLS_PTOC(ClassName()+"::Simplify4"
         + "(" + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_max_iter)
         + "," + ToString(strand_R_II_Q)
         + ")");
    
    return counter;
}
