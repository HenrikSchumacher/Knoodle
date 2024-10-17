public:

/*!
 * @brief Attempts to simplify the planar diagram by applying some local moves implemented by the `ArcSimplifier` class. Returns an estimate on the number of changes made.
 *
 *  Reidemeister I and Reidemeister II moves are supported as
 *  well as some other local moves, which we call "Reidemeister Ia", "Reidemeister IIa", and "twist move". Each of these moves changes only up to 4 crossings in close vicinity and their incident arcs.
 *  See the ASCII-art in the implementation of the class `ArcSimplifier` for more details.
 *
 *  This routine is somewhat optimized for cache locality: It operates on up to 4 crossings that are likely localized in only two consecutive memory locations. And it tries to check as many patterns as possible once two to four crossings are loaded.
 *
 *  @param optimization_level Specifies what kind of patterns are analyzed. More precisely, only patterns involving `<= optimization_level` crossings are considered for the simplifications. `optimization_level <= 0` deactivates this entirely. The maximum level is `4`.
 *
 *   @param exhaustiveQ If set to `true`, then multiple passes of this will be run over the diagram until no further patterns are found.
 *
 *   @param multi_compQ If set to `false`, then the algorithm may assume that the diagram belongs to a connected link and thus skip a couple of checks.
 */

Int Simplify3(
    const Int optimization_level,
    const bool exhaustiveQ = true,
    const bool multi_compQ = true
)
{
    const Int level = Clamp(optimization_level, Int(0), Int(4));
    
    if( multi_compQ )
    {
        switch ( level )
        {
            case 0:
            {
                return 0;
            }
            case 1:
            {
                return simplify3<1,true>(exhaustiveQ);
            }
            case 2:
            {
                return simplify3<2,true>(exhaustiveQ);
            }
            case 3:
            {
                return simplify3<3,true>(exhaustiveQ);
            }
            case 4:
            {
                return simplify3<4,true>(exhaustiveQ);
            }
        }
    }
    else
    {
        switch ( level )
        {
            case 0:
            {
                return 0;
            }
            case 1:
            {
                return simplify3<1,false>(exhaustiveQ);
            }
            case 2:
            {
                return simplify3<2,false>(exhaustiveQ);
            }
            case 3:
            {
                return simplify3<3,false>(exhaustiveQ);
            }
            case 4:
            {
                return simplify3<4,false>(exhaustiveQ);
            }
        }
    }
    
    return 0;
}

private:

template<Int optimization_level, bool multi_compQ>
Int simplify3( bool exhaustiveQ = true )
{
    ptic(ClassName()+"::Simplify3(" + ToString(optimization_level) + "," + ToString(exhaustiveQ) + "," + ToString(multi_compQ) + ")");
    
    Int old_counter = -1;
    Int counter = 0;
    
    ArcSimplifier<Int,optimization_level,false,multi_compQ> arc_simplifier (*this);

    if ( exhaustiveQ )
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
        comp_initialized  = false;
        
        this->ClearCache();
    }
    
    ptoc(ClassName()+"::Simplify3(" + ToString(optimization_level) + "," + ToString(exhaustiveQ) + "," + ToString(multi_compQ) + ")");
    
    return counter;
}
