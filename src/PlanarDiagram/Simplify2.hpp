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

void Simplify2()
{
    ptic(ClassName()+"::Simplify2");

    Int counter = 0;
    
    Int old_counter = -1;
    Int iter = 0;
    
    CrossingSimplifier<Int,true> S(*this);
    
    while( counter != old_counter )
    {
        ++iter;
        
        old_counter = counter;

        for( Int c = 0; c < initial_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                AssertCrossing(c);
                
                bool changedQ = S.Reidemeister_I(c);
                
                counter += changedQ;
                
                // If Reidemeister_I was successful, then c is inactive now.
                if( !changedQ )
                {
                    bool changedQ = S.template Reidemeister_II<true>(c);
                    
                    // If Reidemeister_II was successful, then c is _probably_ inactive now.
                    if( !changedQ )
                    {
                        changedQ = S.Reidemeister_IIa_Vertical(c);
                        
                        // If Reidemeister_IIa_Vertical was successful, then c is _probably_ inactive now.
                        if( !changedQ )
                        {
                            changedQ = S.Reidemeister_IIa_Horizontal(c);
                        }
                    }
                    
                    counter += changedQ;
                }
            }
        }
    }
    
    if( counter > 0 )
    {
        this->ClearCache();
    }
    
    ptoc(ClassName()+"::Simplify2");
}
