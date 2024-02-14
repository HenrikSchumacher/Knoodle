Int Reidemeister_II()
{
    Int prev_count = 1;
    Int count = 0;
    
    while( count != prev_count )
    {
        prev_count = count;
        
        for( Int c = 0; c < C_arcs.Size(); ++c )
        {
            count += Reidemeister_II(c);
        }
    }
    
    return count;
}


#include "Reidemeister_II_Horizontal.hpp"
#include "Reidemeister_II_Vertical.hpp"
#include "HandleTangle.hpp"

bool Reidemeister_II( const Int c_0 )
{
    PD_print("\nReidemeister_II( c = "+CrossingString(c_0)+" )");
        
    if( !CrossingActiveQ(c_0) )
    {
        PD_print("Crossing "+ToString(c_0)+" is not active. Skipping");
        return false;
    }

    bool is_switch_candidate = false;
    
    const Int C [2][2] = {
        { NextCrossing(c_0,0,0), NextCrossing(c_0,0,1) },
        { NextCrossing(c_0,1,0), NextCrossing(c_0,1,1) }
    };

    PD_print( "\n\tC = {  {"  + ToString(C[0][0]) + ", " + ToString(C[0][1]) + " }, { " + ToString(C[1][0]) + ", " + ToString(C[1][1]) + " } }\n" );
    
    // We better test whether we can perform a Reidermeister_I move (and do it). That rules out a couple of nasty cases in the remainder.
    
    const bool R_I = Reidemeister_I(c_0);
    if( R_I )
    {
        PD_wprint("Called Reidemeister_II, but Reidemeister_I was performed on crossing "+ToString(c_0)+".");
        return true;
    }
    
    PD_print("\tReidemeister_I did not apply.");
    
    
    // Test vertical cases
    
    for( bool io : { In, Out} )
    {
        const Int c_1 = C[io][Left ];
        const Int c_2 = C[io][Right];
        
        if( c_1 == c_2 )
        {
            if( OppositeCrossingSigns(c_0,c_1) )
            {
                PD_assert( CrossingActiveQ(c_1) );
                
                if( io == Out )
                {
                    Reidemeister_II_Vertical(c_0, c_1);
                }
                else
                {
                    Reidemeister_II_Vertical(c_1, c_0);
                }
                
                return true;
            }
            else
            {
                PD_print("\tCrossing "+ToString(c_1)+" does not have opposite sing.");
                PD_valprint("\tC_state["+ToString(c_0)+"]", to_underlying(C_state[c_0]) );
                PD_valprint("\tC_state["+ToString(c_1)+"]", to_underlying(C_state[c_1]) );
                
                is_switch_candidate = true;
            }
        }
    }
    
    
//    Test horizontal cases
    
    for( bool side : {Left,Right} )
    {
        const Int c_1 = C[Out][side];
        const Int c_2 = C[In ][side];
        
        if( c_1 == c_2 )
        {
            // For the horizontal cases, it is better to make sure that no Reidermeister_I move can be applied to c_1.
            const bool flipped = Reidemeister_I(c_1);
            
            if( flipped )
            {
                PD_wprint("Called Reidemeister_II, but Reidemeister_I was performed on crossing "+ToString(c_1)+".");
                return true;
            }
            
            if( C_arcs(c_0,Out,side) == C_arcs(c_1,In ,side) )
            {
// This horizontal alignment in the case of side==Right.
//
//                   C_arcs(c_0,Out,side) = b = C_arcs(c_1,In ,side)
//
//                   O----<----O       O---->----O       O----<----O
//                       e_3    ^     ^     b     \     /    e_2
//                               \   /             \   /
//                                \ /               \ /
//                             c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                       e_0    /     \     a     v     v    e_1
//                   O---->----O       O----<----O       O---->----O
//
//                   C_arcs(c_0,In ,side) = a = C_arcs(c_1,Out,side)
//
//               Beware: The cases e_0 == e_3 and e_1 == e_2 might still be possible.
//               Well, actually, the preceeding test for Reidemeister_I should rule this out.
                
                if( OppositeCrossingSigns(c_0,c_1) )
                {
                    Reidemeister_II_Horizontal(c_0,c_1,side);
                    return true;
                }
                else
                {
                    // At least we have found a potential candidate for a later break/switch.
                    is_switch_candidate = true;
                
                }
            }
            else
            {
                // Very peculiar and seldom situation here!
                // Call yourself lucky when you encouter it!
                HandleTangle(c_0,c_1,side);
                return true;
            }
        }
    }
    
    // If we arrive here then no simplifications happend. But maybe we found that c_0 is a good candidate for a later break/switch.
    
    if( is_switch_candidate )
    {
        switch_candidates.push_back(c_0);
    }

    return false;
}
