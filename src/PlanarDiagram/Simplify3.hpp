public:

/*!
 * @brief Attempts to simplify the planar diagram by applying some local moves implemented by the `ArcSimplifier` class. Returns an estimate on the number of changes made.
 *
 *  Reidemeister I and Reidemeister II moves are supported as
 *  well as some other local moves, which we call "Reidemeister Ia", "Reidemeister IIa", and "twist move". Each of these moves changes only up to 4 crossings in close vicinity and their incident arcs.
 *  See the ASCII-art in the implementation of the class `ArcSimplifier` for more details.
 *
 *  This routine is somewhat optimized for cache locality: It operates on up to 4 crossings that are likely to localized in only two consecutive memory locations. And it tries to check as many moves as possible once two to four crossings are loaded.
 *
 */

template<bool exhaustiveQ = true>
Int Simplify3()
{
    ptic(ClassName()+"::Simplify3");
    
    Int old_counter = -1;
    Int counter = 0;
    
    ArcSimplifier_T arc_simplifier (*this);

    if constexpr ( exhaustiveQ )
    {
        do
        {
            old_counter = counter;
            
            for( Int a = 0; a < initial_arc_count; ++a )
            {
                counter += arc_simplifier(a);
            }
        }
        while( counter != old_counter );
    }
    else
    {
        for( Int a = 0; a < initial_arc_count; ++a )
        {
            counter += arc_simplifier(a);
        }
    }

    if( counter > 0 )
    {
        faces_initialized = false;
        comp_initialized  = false;
        
        this->ClearCache();
    }
    
    ptoc(ClassName()+"::Simplify3");
    
    return counter;
}
