#include "R_II_Horizontal.hpp"
#include "R_II_Vertical.hpp"
#include "TwistMove.hpp"

#include "R_Ia_Vertical.hpp"
#include "R_Ia_Horizontal.hpp"

public:

/*! @brief Checks whether a Reidemeister II move can be made at crossing `c`,
 *  then applies it (if possible), and returns a Boolean that indicates whether
 *  any change has been made or not.
 *  Occasionally, it may perform a Reidemeister I move or a twist move,
 *  instead. Then it also returns `true`.
 */

template<bool allow_R_IaQ = true>
bool Reidemeister_II( const Int c_0 )
{
    PD_PRINT("\nReidemeister_II( c = "+CrossingString(c_0)+" )");
    
    // TODO: Test might be redundant.
    if( !CrossingActiveQ(c_0) )
    {
        PD_PRINT("Crossing "+ToString(c_0)+" is not active. Skipping");
        return false;
    }
    
    //    bool is_switch_candidate = false;
    
    const Int C [2][2] = {
        { NextCrossing(c_0,Out,Left), NextCrossing(c_0,Out,Right) },
        { NextCrossing(c_0,In ,Left), NextCrossing(c_0,In ,Right) }
    };
    
    PD_PRINT( "\n\tC = {  {"  + ToString(C[0][0]) + ", " + ToString(C[0][1]) + " }, { " + ToString(C[1][0]) + ", " + ToString(C[1][1]) + " } }\n" );
    
    // We better test whether we can perform a Reidemeister_I move (and do it). That rules out a couple of nasty cases in the remainder.
    // TODO: Test might be redundant if called when Reidemeister_I failed.
    {
        const bool R_I = Reidemeister_I_at_Crossing(c_0);
        if( R_I )
        {
            PD_WPRINT("Called Reidemeister_II on crossing "+ToString(c_0)+", but Reidemeister_I was performed instead.");
            return true;
        }
    }
    PD_PRINT("\tReidemeister_I did not apply.");
    
    
    // Test vertical cases
    // TODO: Do we really have to check both the In and Out port? Typically, we check all crossings anyways (unless we use touching).
    
    for( bool io : { In, Out} )
    {
#ifdef PD_COUNTERS
        ++R_II_check_counter;
#endif
        
        const Int c_1 = C[io][Left ];
        const Int c_2 = C[io][Right];
        
        if( c_1 == c_2 )
        {
            if( OppositeHandednessQ(c_0,c_1) )
            {
                // Find out which one is above the other.
                if( io == Out )
                {
                    Reidemeister_II_Vertical(c_0,c_1);
                }
                else
                {
                    Reidemeister_II_Vertical(c_1,c_0);
                }
                return true;
            }
            else
            {
                if constexpr ( allow_R_IaQ )
                {
                    // Find out which one is above the other.
                    if( io == Out )
                    {
                        const bool R_Ia = Reidemeister_Ia_Vertical(c_0,c_1);
                        
                        if( R_Ia )
                        {
                            return true;
                        }
                    }
                    else
                    {
                        const bool R_Ia = Reidemeister_Ia_Vertical(c_1,c_0);
                        
                        if( R_Ia )
                        {
                            return true;
                        }
                    }
                }
            }
        }
        else // ( c_1 != c_2 )
        {
            // TODO: We could hook Reidemeister_IIa here.
        }
    }
    
    
//    Test horizontal cases
    
    for( bool side : {Left,Right} )
    {
#ifdef PD_COUNTERS
        ++R_II_check_counter;
#endif
        
        const Int c_1 = C[Out][side];
        const Int c_2 = C[In ][side];
        
        if( c_1 == c_2 )
        {
            PD_ASSERT( CrossingActiveQ(c_1) );
            
            // For the horizontal cases, it is better to make sure that no Reidemeister_I move can be applied to c_1.
            const bool R_I = Reidemeister_I_at_Crossing(c_1);
            
            if( R_I )
            {
//                PD_WPRINT("Called Reidemeister_II, but Reidemeister_I was performed on crossing "+ToString(c_1)+".");
                return true;
            }
            else
            {
                PD_PRINT("\tReidemeister_I did not apply.");
            }
            
            if( C_arcs(c_0,Out,side) == C_arcs(c_1,In ,side) )
            {
                PD_ASSERT(C_arcs(c_0,Out,side) == C_arcs(c_1,In ,side));
                PD_ASSERT(C_arcs(c_0,In ,side) == C_arcs(c_1,Out,side));
                
//              This horizontal alignment in the case of side==Right.
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
//               Because of the previous Reidemeister I tests we now know
//               e_0 != e_3 and e_1 != e_2.
                
                if( OppositeHandednessQ(c_0,c_1) )
                {
                    Reidemeister_II_Horizontal(c_0,c_1,side);
                    return true;
                }
                else
                {
                    if constexpr ( allow_R_IaQ )
                    {
                        const bool R_Ia = Reidemeister_Ia_Horizontal(c_0,c_1,side);
                        
                        if( R_Ia )
                        {
                            return true;
                        }
                    }
                }
            }
            else
            {
                // Very peculiar and seldom situation here!
                // Call yourself lucky when you encounter it!
                TwistMove(c_0,c_1,side);
                return true;
            }
        }
    }
    
    return false;
}
