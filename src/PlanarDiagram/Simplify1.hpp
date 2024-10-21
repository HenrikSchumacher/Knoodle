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

void Simplify1()
{
    ptic(ClassName()+"::Simplify1");
    
    Int test_counter = 0;
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
                ++test_counter;
             
                AssertCrossing(c);
                
                bool changedQ = S.Reidemeister_I(c);
                
                counter += changedQ;
                
                // If Reidemeister_I was successful, then c is inactive now.
                if( !changedQ )
                {
                    bool changedQ = S.template Reidemeister_II<false>(c);
                    
                    counter += changedQ;
                }
            }
        }
    }
    
    if( counter > 0 )
    {
        comp_initialized  = false;
        
        this->ClearCache();
    }
    
    ptoc(ClassName()+"::Simplify1");
}
