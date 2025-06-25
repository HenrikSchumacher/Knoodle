public:

// TODO: A connected summand split off by `Simplify5` should be irreducible if `AlternatingQ` evaluates to `true`. We should set a flag to shield such a summand from further simplification attempts.

// TODO: It lies in the nature of the way we detect connected summands that the local simplifications can only changes something around the seam. We should really do a local search that is sensitive to this.

/*! @brief This repeatedly applies `Simplify4` and attempts to split off connected summands with `DisconnectSummands`.
 *
 * @param pd_list A `std::vector` of instances of `PlanarDiagram` to push the newly created connected summands to.
 *
 * @param max_dist The maximum distance that you be used in Dijkstra's algorithm for rerouting strands. Smaller numbers make the breadth-first search stop earlier (and thus faster), but this will typically lead to results with more crossings because some potential optimizations are missed.
 *
 * @param compressQ Whether the `PlanarDiagram` shall be recompressed between various stages. Although this comes with an addition cost, this is typically easily ammortized in the strand optimization pass due to improved cache locality.
 *
 * @param simplify3_level Optimization level for `Simplify3`. Level `0` deactivates it entirely, levels greater or equal to `2` check only patterns that involve as many crossings as specified by this variable.
 *
 * @param simplify3_max_iter If `simplify3_level > 0`, then this set the maximal number of times to cycle through the list of all arcs to look for local patterns.
 */

bool Simplify5(
    PD_List_T & pd_list,
    const Int  max_dist = std::numeric_limits<Int>::max(),
    const bool compressQ = true,
    const Int  simplify3_level = 4,
    const Int  simplify3_max_iter = std::numeric_limits<Int>::max(),
    const bool strand_R_II_Q = true
)
{
    if( proven_minimalQ || InvalidQ() )
    {
        return false;
    }
    
    TOOLS_PTIC(ClassName()+"::Simplify5"
         + "(" + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_max_iter)
         + "," + ToString(strand_R_II_Q)
         + ")");

    bool globally_changedQ = false;
    bool changedQ = false;
    
    do
    {
        changedQ = Simplify4(
            max_dist, compressQ,
            simplify3_level, simplify3_max_iter, strand_R_II_Q
        );
        
        if( CrossingCount() >= 6 )
        {
            changedQ = changedQ
            ||
            DisconnectSummands(
                pd_list, max_dist, compressQ,
                simplify3_level, simplify3_max_iter, strand_R_II_Q
            );
        }
        
        globally_changedQ = globally_changedQ || changedQ;
    }
    while( changedQ );

    
    if( globally_changedQ )
    {
        if( compressQ )
        {
            Compress();   // This also erases the Cache.
        }
        else
        {
            this->ClearCache();
        }
    }

    if( AlternatingQ() && (LinkComponentCount() <= 1) )
    {
        proven_minimalQ = true;
    }
    
    if( ValidQ() && (CrossingCount() == Int(0)) )
    {
        proven_minimalQ = true;
    }
    
    TOOLS_PTOC(ClassName()+"::Simplify5"
         + "(" + ToString(max_dist)
         + "," + ToString(compressQ)
         + "," + ToString(simplify3_level)
         + "," + ToString(simplify3_max_iter)
         + "," + ToString(strand_R_II_Q)
         + ")");

    return globally_changedQ;
}
