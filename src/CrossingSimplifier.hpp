#pragma once

// TODO: Use the state flag of arcs to mark changedQ/unchanged arcs.
// TODO: In particular after applying this to a connected summand that was recently split-off from another diagram, often only very few positions are to be checked on the first (and often last) run.

namespace Knoodle
{
    template<typename Int_, bool mult_compQ_ = true>
    class alignas( ObjectAlignment ) CrossingSimplifier
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
        
    public:
        
        CrossingSimplifier( PD_T & pd_ )
        :   pd     { pd_        }
        ,   C_arcs { pd.C_arcs  }
        ,   C_state{ pd.C_state }
        ,   A_cross{ pd.A_cross }
        ,   A_state{ pd.A_state }
        {}
        
        // Default constructor
        CrossingSimplifier() = delete;
        // Destructor
        ~CrossingSimplifier() = default;
        // Copy constructor
        CrossingSimplifier( const CrossingSimplifier & other ) = default;
        // Copy assignment operator
        CrossingSimplifier & operator=( const CrossingSimplifier & other ) = default;
        // Move constructor
        CrossingSimplifier( CrossingSimplifier && other ) = default;
        // Move assignment operator
        CrossingSimplifier & operator=( CrossingSimplifier && other ) = default;

    public:
        
#include "CrossingSimplifier/CrossingView.hpp"
#include "CrossingSimplifier/ArcView.hpp"
        
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
#ifdef PD_DEBUG
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

        void Reconnect( const Int a, const bool headtail, const Int b )
        {
            pd.Reconnect(a,headtail,b);
        }
        
        template<bool headtail>
        void Reconnect( const Int a, const Int b )
        {
            pd.template Reconnect<headtail>(a,b);
        }
        
//        void Reconnect( ArcView & A, const bool headtail, ArcView & B )
//        {
//            pd.Reconnect(A,headtail,B);
//        }
        
//        template<bool headtail>
//        void Reconnect( ArcView & A, ArcView & B )
//        {
//            pd.template Reconnect<headtail>(A,B);
//        }
        
//        CrossingView GetCrossing(const Int c )
//        {
//            return pd.GetCrossing(c);
//        }
//        
//        ArcView GetArc(const Int a )
//        {
//            return pd.GetArc(a);
//        }
        
    public:
        
#include "CrossingSimplifier/R_I.hpp"
#include "CrossingSimplifier/R_II.hpp"
#include "CrossingSimplifier/R_II_Horizontal.hpp"
#include "CrossingSimplifier/R_II_Vertical.hpp"
#include "CrossingSimplifier/R_Ia_Horizontal.hpp"
#include "CrossingSimplifier/R_Ia_Vertical.hpp"
#include "CrossingSimplifier/R_IIa_Horizontal.hpp"
#include "CrossingSimplifier/R_IIa_Vertical.hpp"
#include "CrossingSimplifier/TwistMove.hpp"

        
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("CrossingSimplifier")
                + "<" + TypeName<Int>
                + "," + ToString(mult_compQ) + ">";
        }

    }; // class CrossingSimplifier
            
} // namespace Knoodle
