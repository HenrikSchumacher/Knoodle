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
 *   @param max_iter The maximal number of times you want to cycle over the list of all arcs.
 *
 *   @param multi_compQ If set to `false`, then the algorithm may assume that the diagram belongs to a connected link and thus skip a couple of checks.
 */

Int Simplify3(
    const Int optimization_level,
    const Int max_iter = std::numeric_limits<Int>::max(),
    const bool multi_compQ = true
)
{
    if( provably_minimalQ )
    {
        return 0;
    }
    
    const Int level = Clip(optimization_level, Int(0), Int(4));
    
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
                return simplify3<1,true>(max_iter);
            }
            case 2:
            {
                return simplify3<2,true>(max_iter);
            }
            case 3:
            {
                return simplify3<3,true>(max_iter);
            }
            case 4:
            {
                return simplify3<4,true>(max_iter);
            }
            default:
            {
                eprint( ClassName()+"::Simplify3: Value " + ToString(level) + " is invalid" );
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
                return simplify3<1,false>(max_iter);
            }
            case 2:
            {
                return simplify3<2,false>(max_iter);
            }
            case 3:
            {
                return simplify3<3,false>(max_iter);
            }
            case 4:
            {
                return simplify3<4,false>(max_iter);
            }
            default:
            {
                eprint( ClassName()+"::Simplify3: Value " + ToString(level) + " is invalid" );
            }
        }
    }
    
    return 0;
}

private:

template<Int optimization_level, bool multi_compQ>
Int simplify3( Int max_iter )
{
    TOOLS_PTIC(ClassName()+"::Simplify3(" + ToString(optimization_level) + "," + ToString(max_iter) + "," + ToString(multi_compQ) + ")");
    
    Int old_counter = -1;
    Int counter = 0;
    Int iter = 0;
    
    ArcSimplifier<Int,optimization_level,multi_compQ> arc_simplifier (*this);

    while( (counter != old_counter) && (iter < max_iter) )
    {
        ++iter;
        
        old_counter = counter;
        
        for( Int a = 0; a < initial_arc_count; ++a )
        {
            counter += arc_simplifier(a);
        }
    }

    if( counter > 0 )
    {
        this->ClearCache();
    }
    
    TOOLS_PTOC(ClassName()+"::Simplify3(" + ToString(optimization_level) + "," + ToString(max_iter) + "," + ToString(multi_compQ) + ")");
    
    return counter;
}
