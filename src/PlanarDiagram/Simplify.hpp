public:

/*!
 * @brief Attempts to simplify the planar diagram by applying some standard moves.
 *
 *  So far, Reidemeister I and Reidemeister II moves are support as
 *  well as a move we call "twist move". See the ASCII-art in
 *  TwistMove.hpp for more details.
 *
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
    
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
    
#ifdef PD_USE_TOUCHING
    
    while( !touched_crossings.empty() )
    {
        const Int c = touched_crossings.back();
        touched_crossings.pop_back();
        
        if( CrossingActiveQ(c) )
        {
            ++test_counter;
            
            AssertCrossing(c);
            
            const bool R_I = Reidemeister_I(c);
            
            counter += R_I;
            
            if( !R_I )
            {
                counter += Reidemeister_II<allow_R_IaQ>(c);
            }
        }
    }
    
#else
    
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
                
                bool changed = Reidemeister_I(c);
                
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
        
//        toc("Simplify loop");
//        
//        const Int changed = counter - old_counter;
//        
//        dump(changed);
    }
    
#endif
    
#pragma clang diagnostic pop
    
//    dump(test_counter);
    
    if( counter > 0 )
    {
        faces_initialized = false;
        comp_initialized  = false;
        
        this->ClearCache();
    }
    
//            dump(R_Ia_horizontal_counter);
//            dump(R_Ia_vertical_counter);
//            dump(R_II_horizontal_counter);
//            dump(R_II_vertical_counter);
//            dump(R_IIa_counter);
    
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
