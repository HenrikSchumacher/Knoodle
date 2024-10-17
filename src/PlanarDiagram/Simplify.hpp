public:

/*!
 * @brief Attempts to simplify the planar diagram by applying some standard moves.
 *
 *  So far, Reidemeister I and Reidemeister II moves are support as
 *  well as a move we call "twist move". See the ASCII-art in
 *  TwistMove.hpp for more details.
 *
 *  This is a very dated simplification routine and only persists for benchmarking reasons. Better use `Simplify3` or `Simplify4` instead.
 */

template<bool allow_R_IaQ = true, bool allow_R_IIaQ = true>
void Simplify()
{
    ptic(ClassName()+"::Simplify"
        + "<" + Tools::ToString(allow_R_IaQ)
        + "," + Tools::ToString(allow_R_IIaQ)
        + ">");
    
    Int test_counter = 0;
    Int counter = 0;
    
    Int old_counter = -1;
    Int iter = 0;
    
    while( counter != old_counter )
    {
        ++iter;
//        dump(iter);
        
        old_counter = counter;
        
//        tic("Simplify loop");
        for( Int c = 0; c < initial_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                ++test_counter;
             
                AssertCrossing(c);
                
                bool changed = Reidemeister_I_at_Crossing(c);
                
                counter += changed;
                
                // If Reidemeister_I was successful, then c is inactive now.
                if( !changed )
                {
                    bool changed = Reidemeister_II<allow_R_IaQ>(c);
                    
                    if constexpr( allow_R_IIaQ )
                    {
                        // If Reidemeister_II was successful, then c is _probably_ inactive now.
                        if( !changed )
                        {
                            changed = Reidemeister_IIa_Vertical(c);
                            
                            // If Reidemeister_IIa_Vertical was successful, then c is _probably_ inactive now.
                            if( !changed )
                            {
                                changed = Reidemeister_IIa_Horizontal(c);
                            }
                        }
                    }
                    
                    counter += changed;
                }
            }
        }
    }
    
    if( counter > 0 )
    {
        comp_initialized  = false;
        
        this->ClearCache();
    }
    
    ptoc(ClassName()+"::Simplify"
        + "<" + Tools::ToString(allow_R_IaQ)
        + "," + Tools::ToString(allow_R_IIaQ)
        + ">");
}


void SanitizeArcStates()
{
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            A_state[a] = ArcState::Active;
        }
    }
}
