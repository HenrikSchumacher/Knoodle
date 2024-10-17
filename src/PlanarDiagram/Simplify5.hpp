public:

// TODO: It lies in the nature of the way we detect connected summands that the local simplifications can only changes something around the seam. We should really do a local search that is sensitive to this.

/*! @brief This repeatedly applies `Simplify4` and attempts to split off connected summands with `SplitConnectedSummands`.
 *
 * @param PD_list A `std::vector` of instances of `PlanarDiagram` to push the newly created connected summands to.
 *
 * @param max_dist The maximum distance that you be used in Dijkstra's algorithm for rerouting strands. Smaller numbers make the breadth-first search stop earlier (and thus faster), but this will typically lead to results with more crossings because some potential optimizations are missed.
 *
 * @param compressQ Whether the `PlanarDiagram` shall be recompressed between various stages. Although this comes with an addition cost, this is typically easily ammortized in the strand optimization pass due to improved cache locality.
 *
 * @param simplify3_level Optimization level for `Simplify3`. Level `0` deactivated it entirely, levels greater or equal to `2` check only patterns that involve as many crossings as specified by this variable.
 *
 * @param simplify3_exhaustiveQ If `simplify3_level > 0`, then this changes its behavior. If set to `false`, only one optimization pass over the diagram is performed. If set to `true`, then optimization passes will applied
 */

bool Simplify5(
    std::vector<PlanarDiagram<Int>> & PD_list,
    const Int max_dist = std::numeric_limits<Int>::max(),
    const bool compressQ = true,
    const Size_T simplify3_level = 4,
    const bool simplify3_exhaustiveQ = true,
    bool strand_R_II_Q = true
)
{
    ptic(ClassName()+"::Simplify5");

    bool globally_changedQ = false;
    bool changedQ = false;
    
    do
    {
        changedQ = Simplify4(max_dist,compressQ,simplify3_level,simplify3_exhaustiveQ,strand_R_II_Q);
        
        if( CrossingCount() > 5 )
        {
            changedQ = changedQ || SplitConnectedSummands(PD_list,
                max_dist,compressQ,simplify3_level,simplify3_exhaustiveQ,strand_R_II_Q
            );
        }
        
        globally_changedQ = globally_changedQ || changedQ;
    }
    while( changedQ );
    
    if( compressQ && globally_changedQ )
    {
        *this = this->CreateCompressed();
    }
    
    if( globally_changedQ )
    {
        comp_initialized  = false;

        this->ClearCache();
    }
    
    ptoc(ClassName()+"::Simplify5");

    return globally_changedQ;
}
