#pragma once

// TODO: Use the state flag of arcs to mark changedQ/unchanged arcs.
// TODO: In particular after applying this to a connected summand that was recently split-off from another diagram, often only very few positions are to be checked on the first (and often last) run.

namespace KnotTools
{
    template<typename Int_,
        Size_T optimization_level_ = 4,
        bool mult_compQ_ = true
    >
    class alignas( ObjectAlignment ) ArcSimplifier
    {
    public:
        
        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
        
        using PD_T = PlanarDiagram<Int>;
        
        using CrossingContainer_T       = typename PD_T::CrossingContainer_T;
        using ArcContainer_T            = typename PD_T::ArcContainer_T;
        using CrossingStateContainer_T  = typename PD_T::CrossingStateContainer_T;
        using ArcStateContainer_T       = typename PD_T::ArcStateContainer_T;
        
        static constexpr bool mult_compQ = mult_compQ_;
        
        static constexpr Int optimization_level = optimization_level_;
        
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Out   = PD_T::Out;
        
    private:
        
        PD_T & restrict pd;
        
        CrossingContainer_T      & restrict C_arcs;
        CrossingStateContainer_T & restrict C_state;
        
        ArcContainer_T           & restrict A_cross;
        ArcStateContainer_T      & restrict A_state;
        
    private:
        
        Int a;
        Int c_0;
        Int c_1;
        Int c_2;
        Int c_3;
        
        // For crossing c_0
        Int n_0;
        //        Int e_0; // always = a.
        Int s_0;
        Int w_0;
        
        // For crossing c_1
        Int n_1;
        Int e_1;
        Int s_1;
        //        Int w_1; // always = a.
        
        // For crossing c_2
        //        Int n_2; // always = s_0.
        Int e_2;
        Int s_2;
        Int w_2;
        
        // For crossing c_3
        Int n_3;
        Int e_3;
        //        Int s_3; // always = n_0.
        Int w_3;
        
        Int test_count = 0;
        
        // Whether the vertical strand at corresponding crossing goes over.
        bool o_0;
        bool o_1;
        bool o_2;
        bool o_3;
        
        // Whether the vertical strand at corresponding crossing goes up.
        bool u_0;
        bool u_1;
        
    public:
        
        ArcSimplifier() = delete;
        
        ArcSimplifier( PD_T & pd_ )
        :   pd     { pd_        }
        ,   C_arcs { pd.C_arcs  }
        ,   C_state{ pd.C_state }
        ,   A_cross{ pd.A_cross }
        ,   A_state{ pd.A_state }
        {}
        
        ~ArcSimplifier() = default;
        
        ArcSimplifier( const ArcSimplifier & other ) = delete;
        
        ArcSimplifier( ArcSimplifier && other ) = delete;

    private:
        
        bool ArcActiveQ( const Int a_ ) const
        {
            return pd.ArcActiveQ(a_);
        }
        
        void DeactivateArc( const Int a_ ) const
        {
            return pd.DeactivateArc(a_);
        }
        
        template<bool must_be_activeQ>
        void AssertArc( const Int a_ )
        {
#ifdef DEBUG
            if constexpr( must_be_activeQ )
            {
                if( !ArcActiveQ(a_) )
                {
                    pd_eprint("AssertArc<1>: " + ArcString(a_) + " is not active.");
                }
                PD_ASSERT(pd.CheckArc(a_));
            }
            else
            {
                if( ArcActiveQ(a_) )
                {
                    pd_eprint("AssertArc<0>: " + ArcString(a_) + " is not inactive.");
                }
            }
#else
            (void)a_;
#endif
        }
        
        std::string ArcString( const Int a_ ) const
        {
            return pd.ArcString(a_);
        }
        
        bool CrossingActiveQ( const Int c_ ) const
        {
            return pd.CrossingActiveQ(c_);
        }
        
        void DeactivateCrossing( const Int c_ )
        {
            pd.DeactivateCrossing(c_);
        }
        
        template<bool must_be_activeQ>
        void AssertCrossing( const Int c_ ) const
        {
#ifdef DEBUG
            if constexpr( must_be_activeQ )
            {
                if( !CrossingActiveQ(c_) )
                {
                    pd_eprint("AssertCrossing<1>: " + CrossingString(c_) + " is not active.");
                }
                PD_ASSERT(pd.CheckCrossing(c_));
            }
            else
            {
                if( CrossingActiveQ(c_) )
                {
                    pd_eprint("AssertCrossing<0>: " + CrossingString(c_) + " is not inactive.");
                }
            }
#else
            (void)c_;
#endif
        }
        
        std::string CrossingString( const Int c_ ) const
        {
            return pd.CrossingString(c_);
        }

        void Reconnect( const Int a_, const bool headtail, const Int b_ )
        {
            pd.Reconnect(a_,headtail,b_);
        }
        
        template<bool headtail>
        void Reconnect( const Int a_, const Int b_ )
        {
            pd.template Reconnect<headtail>(a_,b_);
        }
        
    public:
        
        Int TestCount() const
        {
            return test_count;
        }
        
        
        bool operator()( const Int a_ )
        {
            if( !ArcActiveQ(a_) ) return false;
            
            ++test_count;
            a = a_;
            
            PD_PRINT( "===================================================" );
            PD_PRINT( "Simplify a = " + ArcString(a) );
            
            AssertArc<1>(a);

            c_0 = A_cross(a,Tail);
            AssertCrossing<1>(c_0);
            
            c_1 = A_cross(a,Head);
            AssertCrossing<1>(c_1);
            
            load_c_0();
            
            /*              n_0
             *               O
             *               |
             *       w_0 O---X-->O a
             *               |c_0
             *               O
             *              s_0
             */
            
            // Check for Reidemeister I move
            if( R_I_center() )
            {
                PD_VALPRINT( "a  ", ArcString(n_0) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                return true;
            }
             
            if constexpr ( optimization_level < 2 )
            {
                return false;
            }
            
            load_c_1();
            
            /*              n_0           n_1
             *               O             O
             *               |      a      |
             *       w_0 O---X---O---->O---X-->O e_1
             *               |c_0          |c_1
             *               O             O
             *              s_0           s_1
             */
            
            // We want to check for twist move _before_ Reidemeister I because the twist move can remove both crossings.
            
            if( twist_at_a() )
            {
                PD_VALPRINT( "a  ", ArcString(n_0) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                PD_VALPRINT( "c_1", CrossingString(c_1) );
                PD_VALPRINT( "n_1", ArcString(n_1) );
                PD_VALPRINT( "e_1", ArcString(e_1) );
                PD_VALPRINT( "s_1", ArcString(s_1) );
                
                return true;
            }

            // Next we check for Reidemeister_I at crossings c_0 and c_1.
            // This will also remove some unpleasant cases for the Reidemeister II and Ia moves.
            
            if( R_I_left() )
            {
                PD_VALPRINT( "a  ", ArcString(n_0) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                PD_VALPRINT( "c_1", CrossingString(c_1) );
                PD_VALPRINT( "n_1", ArcString(n_1) );
                PD_VALPRINT( "e_1", ArcString(e_1) );
                PD_VALPRINT( "s_1", ArcString(s_1) );
                
                return true;
            }
            
            if( R_I_right() )
            {
                PD_VALPRINT( "a  ", ArcString(n_0) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                PD_VALPRINT( "c_1", CrossingString(c_1) );
                PD_VALPRINT( "n_1", ArcString(n_1) );
                PD_VALPRINT( "e_1", ArcString(e_1) );
                PD_VALPRINT( "s_1", ArcString(s_1) );
                
                return true;
            }
            
            // Neglecting asserts, this is the only time we access C_state[c_0].
            // Find out whether the vertical strand at c_0 goes over.
            o_0 = ( u_0 == LeftHandedQ(C_state[c_0]));
            PD_VALPRINT("o_0", o_0);
            
            // Neglecting asserts, this is the only time we access C_state[c_1].
            // Find out whether the vertical strand at c_1 goes over.
            o_1 = ( u_1 == LeftHandedQ(C_state[c_1]));
            PD_VALPRINT("o_1", o_1);

            // Deal with the case that a is part of a loop of length 2.
            // This can only occur if  the diagram has a more than one component.
            if constexpr( mult_compQ )
            {
                // This requires o_0 and o_1 to be defined already.
                if( a_is_2loop() )
                {
                    PD_VALPRINT( "a  ", ArcString(n_0) );
                    
                    PD_VALPRINT( "c_0", CrossingString(c_0) );
                    PD_VALPRINT( "n_0", ArcString(n_0) );
                    PD_VALPRINT( "s_0", ArcString(s_0) );
                    PD_VALPRINT( "w_0", ArcString(w_0) );
                    
                    PD_VALPRINT( "c_1", CrossingString(c_1) );
                    PD_VALPRINT( "n_1", ArcString(n_1) );
                    PD_VALPRINT( "e_1", ArcString(e_1) );
                    PD_VALPRINT( "s_1", ArcString(s_1) );
                    
                    return true;
                }
            }
            
            if(o_0 == o_1)
            {
                /*       |     a     |             |     a     |
                 *    -->|---------->|-->   or  -->----------->--->
                 *       |c_0        |c_1          |c_0        |c_1
                 */
                
                // Attempt the moves that rely on the vertical strands being both on top or both below the horizontal strand.
                if( strands_same_o() )
                {
                    PD_VALPRINT( "a  ", ArcString(a) );
                    
                    PD_VALPRINT( "c_0", CrossingString(c_0) );
                    PD_VALPRINT( "n_0", ArcString(n_0) );
                    PD_VALPRINT( "s_0", ArcString(s_0) );
                    PD_VALPRINT( "w_0", ArcString(w_0) );
                    
                    PD_VALPRINT( "c_1", CrossingString(c_1) );
                    PD_VALPRINT( "n_1", ArcString(n_1) );
                    PD_VALPRINT( "e_1", ArcString(e_1) );
                    PD_VALPRINT( "s_1", ArcString(s_1) );
                    
                    return true;
                }
            }
            else
            {
                /*       |     a     |             |     a     |
                 *    -->|---------->--->   or  -->----------->|-->
                 *       |c_0        |c_1          |c_0        |c_1
                 */
        
                // Attempt the moves that rely on the vertical strands being separated by the horizontal strand.
                if( strands_diff_o() )
                {
                    PD_VALPRINT( "a  ", ArcString(a) );
        
                    PD_VALPRINT( "c_0", CrossingString(c_0) );
                    PD_VALPRINT( "n_0", ArcString(n_0) );
                    PD_VALPRINT( "s_0", ArcString(s_0) );
                    PD_VALPRINT( "w_0", ArcString(w_0) );
        
                    PD_VALPRINT( "c_1", CrossingString(c_1) );
                    PD_VALPRINT( "n_1", ArcString(n_1) );
                    PD_VALPRINT( "e_1", ArcString(e_1) );
                    PD_VALPRINT( "s_1", ArcString(s_1) );
        
                    return true;
                }
            }
            
            AssertArc<1>(a);
            
            return false;
        }
        
    private:
        
#include "ArcSimplifier/load.hpp"
#include "ArcSimplifier/a_is_2loop.hpp"
#include "ArcSimplifier/twist_at_a.hpp"
#include "ArcSimplifier/strands_same_o.hpp"
#include "ArcSimplifier/strands_diff_o.hpp"
#include "ArcSimplifier/R_I_center.hpp"
#include "ArcSimplifier/R_I_left.hpp"
#include "ArcSimplifier/R_I_right.hpp"
#include "ArcSimplifier/R_Ia_above.hpp"
#include "ArcSimplifier/R_Ia_below.hpp"
#include "ArcSimplifier/R_II_above.hpp"
#include "ArcSimplifier/R_II_below.hpp"
#include "ArcSimplifier/R_IIa_same_o_same_u.hpp"
#include "ArcSimplifier/R_IIa_same_o_diff_u.hpp"
#include "ArcSimplifier/R_IIa_diff_o_same_u.hpp"
#include "ArcSimplifier/R_IIa_diff_o_diff_u.hpp"
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("ArcSimplifier")
                + "<" + TypeName<Int>
                + "," + ToString(mult_compQ) + ">";
        }

    }; // class ArcSimplifier
            
} // namespace KnotTools
