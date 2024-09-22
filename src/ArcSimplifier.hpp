#pragma once

// TODO: Use the state flag of arcs to mark unchanged arcs.
namespace KnotTools
{
    template<typename Int_, bool mult_compQ_ = true >
    class alignas( ObjectAlignment ) ArcSimplifier
    {
    public:
        
        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
//        using Sint = int;
        
        using PD_T = PlanarDiagram<Int>;
        
        using CrossingContainer_T       = PD_T::CrossingContainer_T;
        using ArcContainer_T            = PD_T::ArcContainer_T;
        using CrossingStateContainer_T  = PD_T::CrossingStateContainer_T;
        using ArcStateContainer_T       = PD_T::ArcStateContainer_T;
        
        static constexpr bool mult_compQ = mult_compQ_;
        
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Out   = PD_T::Out;
        
    private:
        
        PD_T & pd;
        
        CrossingContainer_T      & C_arcs;
        CrossingStateContainer_T & C_state;
        
        ArcContainer_T           & A_cross;
        ArcStateContainer_T      & A_state;
        
    private:
        
        bool ArcActiveQ( const Int a_ ) const
        {
            return pd.ArcActiveQ(a_);
        }
        
        void DeactivateArc( const Int a_ ) const
        {
            return pd.DeactivateArc(a_);
        }
        
        
        void AssertArc( const Int a_ )
        {
            pd.AssertArc(a_);
        }
        
        std::string ArcString( const Int a_ )
        {
            return pd.ArcString(a_);
        }
        
        
        
        bool CrossingActiveQ( const Int c_ ) const
        {
            return pd.CrossingActiveQ(c_);
        }
        
        void DeactivateCrossing( const Int c_ ) const
        {
            return pd.DeactivateCrossing(c_);
        }
        
        void AssertCrossing( const Int c_ )
        {
            pd.AssertCrossing(c_);
        }
        
        std::string CrossingString( const Int c_ )
        {
            return pd.CrossingString(c_);
        }
        
        
        
        void Reconnect( const Int a_, const bool headtail, const Int b_ )
        {
            pd.Reconnect(a_,headtail,b_);
        }
        
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
        
//        // Copy constructor
//        ArcSimplifier( const ArcSimplifier & other ) = default;
//        
//        friend void swap(ArcSimplifier & A, ArcSimplifier & B ) noexcept
//        {
//            using std::swap;
//    
//            swap( A.pd,      B.pd      );
//            swap( A.C_arcs,  B.C_arcs  );
//            swap( A.C_state, B.C_state );
//            swap( A.A_cross, B.A_cross );
//            swap( A.A_state, B.A_state );
//            
//            swap( A.a,   B.a   );
//            
//            swap( A.c_0, B.c_0 );
//            swap( A.c_1, B.c_1 );
//            swap( A.c_2, B.c_2 );
//            swap( A.c_3, B.c_3 );
//            
//            swap( A.n_0, B.n_0 );
//            swap( A.s_0, B.s_0 );
//            swap( A.w_0, B.w_0 );
//            
//            swap( A.n_1, B.n_1 );
//            swap( A.e_1, B.e_1 );
//            swap( A.s_1, B.s_1 );
//
//            swap( A.e_2, B.e_2 );
//            swap( A.s_2, B.s_2 );
//            swap( A.w_2, B.w_2 );
//            
//            swap( A.n_3, B.n_3 );
//            swap( A.e_3, B.e_3 );
//            swap( A.w_3, B.w_3 );
//            
//            swap( A.o_0, B.o_0 );
//            swap( A.o_1, B.o_1 );
//            swap( A.o_2, B.o_2 );
//            swap( A.o_3, B.o_3 );
//            
//            swap( A.u_0, B.u_0 );
//            swap( A.o_1, B.u_1 );
//        }
//        
//        // Move constructor
//        ArcSimplifier( ArcSimplifier && other ) noexcept
//        :   ArcSimplifier()
//        {
//            swap(*this, other);
//        }
//
//        /* Copy assignment operator */
//        ArcSimplifier & operator=( ArcSimplifier other ) noexcept
//        {   //                                     ^
//            //                                     |
//            // Use the copy constructor     -------+
//            swap( *this, other );
//            return *this;
//        }
        
        
        
        bool operator()( const Int a_ )
        {
            if( !pd.ArcActiveQ(a_) ) return false;

            a = a_;
            
            PD_DPRINT( "===================================================" );
            PD_DPRINT( "Simplify a = " + ArcString(a) );
            
            AssertArc(a);

            c_0 = A_cross(a,Tail);
            AssertCrossing(c_0);

            c_1 = A_cross(a,Head);
            AssertCrossing(c_1);
            
            load_c_0();

            if( R_I_center() )
            {
                PD_VALPRINT( "a  ", ArcString(n_0) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                return true;
            }
            
            /*              n_0           n_1
             *               O             O
             *               |      a      |
             *       w_0 O---X-->O-----O-->X-->O e_1
             *               |c_0          |c_1
             *               O             O
             *              s_0           s_1
             */
             
            load_c_1();
            
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
            // Whether the vertical strand at c_0 goes over.
            o_0 = ( u_0 == ( C_state[c_0] == CrossingState::LeftHanded ) );
            PD_VALPRINT( "o_0", o_0 );
            
            // Neglecting asserts, this is the only time we access C_state[c_1].
            // Whether the vertical strand at c_1 goes over.
            o_1 = ( u_1 == ( C_state[c_1] == CrossingState::LeftHanded ) );
            PD_VALPRINT( "o_1", o_1 );

            // Deal with the case that a is part of a loop of length 2.
            // This can only occur if  the diagram has a more than one component.
            if constexpr( mult_compQ )
            {
                // Caution: This requires o_0 and o_1 to be defined already.
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
            
            if( o_0 == o_1 )
            {
                /*       |     a     |             |     a     |
                 *    -->|---------->|-->   or  -->----------->--->
                 *       |c_0        |c_1          |c_0        |c_1
                 */
                
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
                    
                    PD_VALPRINT( "c_2", CrossingString(c_2) );
                    PD_VALPRINT( "e_2", ArcString(e_2) );
                    PD_VALPRINT( "s_2", ArcString(s_2) );
                    PD_VALPRINT( "w_2", ArcString(w_2) );
                    
                    PD_VALPRINT( "c_3", CrossingString(c_3) );
                    PD_VALPRINT( "n_3", ArcString(n_3) );
                    PD_VALPRINT( "e_3", ArcString(e_3) );
                    PD_VALPRINT( "w_3", ArcString(w_3) );
                    
                    return true;
                }
            }
            else
            {
                /*       |     a     |             |     a     |
                 *    -->|---------->--->   or  -->----------->|-->
                 *       |c_0        |c_1          |c_0        |c_1
                 */
        
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
        
                    PD_VALPRINT( "c_2", CrossingString(c_2) );
                    PD_VALPRINT( "e_2", ArcString(e_2) );
                    PD_VALPRINT( "s_2", ArcString(s_2) );
                    PD_VALPRINT( "w_2", ArcString(w_2) );
        
                    PD_VALPRINT( "c_3", CrossingString(c_3) );
                    PD_VALPRINT( "n_3", ArcString(n_3) );
                    PD_VALPRINT( "e_3", ArcString(e_3) );
                    PD_VALPRINT( "w_3", ArcString(w_3) );
        
                    return true;
                }
            }
                   
            A_state[a] = ArcState::ActiveUnchanged;
            
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
            return std::string("ArcSimplifier") + "<" + TypeName<Int> + ">";
        }

    }; // class ArcSimplifier
            
} // namespace KnotTools
