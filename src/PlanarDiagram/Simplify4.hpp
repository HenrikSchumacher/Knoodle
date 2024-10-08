public:

/*!
 * @brief Attempts to simplify the planar diagram by applying the local moves implemented in the `ArcSimplifier` class as well as some global pass moves implemented in `StrandSimplifier`. Returns an estimate on the number of changes that have been made.
 *
 *  The pass moves attempt to simplify the diagram by detecting overstrands and understrands by rerouting them using shortest paths in the dual graph. Loop overstrands and loop understrands are also eliminated. 
 *
 */

template<bool compressQ = true, bool simplify3_exhaustiveQ = true>
Int Simplify4()
{
    ptic(ClassName()+"::Simplify4");
    
    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;
    
    // TODO: Toggle this for multi-component links.
    StrandSimplifier<Int,false> S(*this);

    do
    {
        ++iter;

        old_counter = counter;
        
        // Since Simplify3 contains very inexpensive tests, we should call it first.
        counter += Simplify3<simplify3_exhaustiveQ>();

        if constexpr ( compressQ )
        {
            if( counter > old_counter)
            {
                (*this) = CreateCompressed(); // TODO: This looks like asking for trouble.
            }
        }
        
        counter += S.SimplifyStrands(true);
        
        counter += S.SimplifyStrands(false);
    }
    while( counter != old_counter );

    if( counter > 0 )
    {
        faces_initialized = false;
        comp_initialized  = false;

        this->ClearCache();
    }
    
    PD_ASSERT(CheckAll());

    ptoc(ClassName()+"::Simplify4");
    
    return counter;
}
