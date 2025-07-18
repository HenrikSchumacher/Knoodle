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

Int Simplify1()
{
    if( proven_minimalQ || InvalidQ() ) { return 0; }
    
    TOOLS_PTIMER(timer,ClassName()+"::Simplify1");
    
    Int test_counter = 0;
    Int counter = 0;
    
    Int old_counter = 0;
    Int iter = 0;
    
    CrossingSimplifier<Int,true> S(*this);
    
    do
    {
        ++iter;
        
        old_counter = counter;

        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                ++test_counter;
             
                AssertCrossing(c);
                
                bool changed_I_Q = S.Reidemeister_I(c);
                
                counter += changed_I_Q;
                
                // If Reidemeister_I was successful, then c is inactive now.
                if( !changed_I_Q )
                {
                    bool changed_II_Q = S.template Reidemeister_II<false>(c);
                    
                    counter += changed_II_Q;
                }
            }
        }
    }
    while( counter != old_counter );
    
    if( counter > Int(0) )
    {
        this->ClearCache();
    }
    
    if( ValidQ() && (CrossingCount() == Int(0)) )
    {
        proven_minimalQ = true;
    }
    
    return counter;
}
