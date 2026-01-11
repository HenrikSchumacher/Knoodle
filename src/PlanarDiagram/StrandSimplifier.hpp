#pragma once


// DONE: Performing possible Reidemeister II moves an a strand might unlock further moves. More importantly, it will reduce the length of the current strand and thus make Dijkstra faster!

// DONE: Speed this up by reducing the redirecting in NextLeftArc by using and maintaining ArcLeftArc.

// TODO: Can this be sped up by using and meintaining ArcNextArc?

namespace Knoodle
{
    
    template<typename Int_, bool mult_compQ_>
    class alignas( ObjectAlignment ) StrandSimplifier final
    {
    public:
        
//        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
        
        using PD_T = PlanarDiagram<Int>;
        
        // We need a signed integer type for Mark_T because we use negative marks to indicate directions in the dual graph.
        using Mark_T = ToSigned<Int>;
        
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
        
        static constexpr bool Uninitialized = PD_T::Uninitialized;
        
    private:
        
        PD_T & restrict pd;
        
        CrossingContainer_T      & restrict C_arcs;
        CrossingStateContainer_T & restrict C_state;
        
        ArcContainer_T           & restrict A_cross;
        ArcStateContainer_T      & restrict A_state;

        // TODO: I think we do not need it at all.
        Tensor1<Int,Int>         & restrict A_source;
        
    private:
        
        // Colors for the crossings and arcs to mark the current strand.
        // We use this to detect loop strands.
        Tensor1<Mark_T,Int> C_mark;
        Tensor1<Mark_T,Int> A_mark;
        
        Tensor1<Mark_T,Int> D_mark;  // colors for the dual arcs
        // D_from[a] is the dual arc from which we visited dual arc a in the pass with mark D_mark[a].
        Tensor1<Int,Int> D_from;
        
        Int * restrict dA_left;
        
        Mark_T current_mark = 1; // We start with 1 so that we can store things in the sign bit of current_mark.
        Int a_ptr = 0;

        Int strand_length  = 0;
        Int change_counter = 0;
        
        bool overQ;
        bool strand_completeQ;
                
        Stack<Int,Int> next_front;
        Stack<Int,Int> prev_front;
        
        Tensor1<Int,Int> path;
        Int path_length = 0;
        
//        std::vector<Int> touched;
        std::vector<Int> duds;
        
        double Time_RemoveLoop            = 0;
        double Time_FindShortestPath      = 0;
        double Time_RerouteToShortestPath = 0;
//        double Time_RepairArcLeftArcs     = 0;
        double Time_SimplifyStrands       = 0;
        double Time_CollapseArcRange      = 0;
        
//        //DEBUGGING COUNTERS
//        Size_T reconnect_counter          = 0;
//        Size_T touched_counter            = 0;
//        Size_T repair_counter_0           = 0;
//        Size_T needless_repair_counter_0  = 0;
//        Size_T repair_counter_1           = 0;
//        Size_T needless_repair_counter_1  = 0;
        
    public:
        
        StrandSimplifier( PD_T & pd_ )
        :   pd         { pd_                             }
        ,   C_arcs     { pd.C_arcs                       }  
        ,   C_state    { pd.C_state                      }
        ,   A_cross    { pd.A_cross                      }
        ,   A_state    { pd.A_state                      }
        ,   A_source   { pd.A_scratch                    }
        // We initialize by 0 because actual colors will have to be positive to use sign bit.
        ,   C_mark     { C_arcs.Dim(0),  Mark_T(0)       }
        ,   A_mark     { A_cross.Dim(0), Mark_T(0)       }
        ,   D_mark     { A_cross.Dim(0), Mark_T(0)       }
        ,   D_from     { A_cross.Dim(0), Uninitialized   }
        ,   next_front { A_cross.Dim(0)                  }
        ,   prev_front { A_cross.Dim(0)                  }
        ,   path       { A_cross.Dim(0), Uninitialized   }
        {}
        
        // No default constructor
        StrandSimplifier() = delete;
        
        // Destructor
        ~StrandSimplifier()
        {
#ifdef PD_TIMINGQ
            logprint("");
            logprint(ClassName() + " absolute timings:");
            logvalprint("SimplifyStrands", Time_SimplifyStrands);
            logvalprint("RemoveLoop", Time_RemoveLoop);
            logvalprint("FindShortestPath", Time_FindShortestPath);
            logvalprint("RerouteToShortestPath", Time_RerouteToShortestPath);
            logvalprint("CollapseArcRange", Time_CollapseArcRange);
//            logvalprint("RepairArcLeftArcs", Time_RepairArcLeftArcs);
            logprint("");
            logprint(ClassName() + " relative timings:");
            logvalprint("SimplifyStrands", Time_SimplifyStrands/Time_SimplifyStrands);
            logvalprint("RemoveLoop", Time_RemoveLoop/Time_SimplifyStrands);
            logvalprint("FindShortestPath", Time_FindShortestPath/Time_SimplifyStrands);
            logvalprint("RerouteToShortestPath", Time_RerouteToShortestPath/Time_SimplifyStrands);
            logvalprint("CollapseArcRange", Time_CollapseArcRange/Time_SimplifyStrands);
//            logvalprint("RepairArcLeftArcs", Time_RepairArcLeftArcs/Time_SimplifyStrands);
            logprint("");
#endif
        }
        
        // Copy constructor
        StrandSimplifier( const StrandSimplifier & other ) = default;
        // Copy assignment operator
        StrandSimplifier & operator=( const StrandSimplifier & other ) = default;
        // Move constructor
        StrandSimplifier( StrandSimplifier && other ) = default;
        // Move assignment operator
        StrandSimplifier & operator=( StrandSimplifier && other ) = default;
        

#include "StrandSimplifier/Helpers.hpp"
#include "StrandSimplifier/Reconnect.hpp"
#include "StrandSimplifier/Strings.hpp"
#include "StrandSimplifier/Checks.hpp"
#include "StrandSimplifier/RepairArcs.hpp"

#include "StrandSimplifier/Reidemeister.hpp"
#include "StrandSimplifier/RemoveLoop.hpp"
#include "StrandSimplifier/FindShortestPath.hpp"
#include "StrandSimplifier/ReroutePath.hpp"
#include "StrandSimplifier/CollapseArcRange.hpp"
#include "StrandSimplifier/SimplifyStrands.hpp"

    private:

        void MarkCrossing( const Int c )
        {
            C_mark(c) = current_mark;
        }
        
    public:
        
        void SetStrandMode( const bool overQ_ )
        {
            overQ = overQ_;
        }
        
    public:
        
        template<typename Int_0, typename Int_1>
        Int MarkArcs(
            const Int_0 a_first, const Int_0 a_last, const Int_1 mark
        )
        {
            static_assert( IntQ<Int_0>, "" );
            static_assert( IntQ<Int_1>, "" );
            
            const Mark_T c    = int_cast<Mark_T>(mark );
            const Int a_begin = int_cast<Int>(a_first);
            const Int a_end   = int_cast<Int>(pd.NextArc(a_last,Head));
            Int a = a_begin;
            
            Int counter = 0;
            
            do
            {
                ++counter;
                A_mark(a) = c;
                a = pd.NextArc(a,Head);
            }
            while( (a != a_end) && (a != a_begin) );
            
            return counter;
        }
        
        template<typename Int_0, typename Int_1, typename Int_2>
        void MarkArcs( cptr<Int_0> arcs, const Int_1 strand_length_, const Int_2 mark )
        {
            static_assert( IntQ<Int_0>, "" );
            static_assert( IntQ<Int_1>, "" );
            static_assert( IntQ<Int_2>, "" );
            
            const Int n    = int_cast<Int>(strand_length_);
            const Mark_T m = int_cast<Mark_T>(mark);
            
            if( m <= Mark_T(0) )
            {
                pd_eprint("Argument mark is <= 0. This is an illegal value.");
                
                return;
            }
            
            for( Int i = 0; i < n; ++i )
            {
                A_mark(arcs[i]) = m;
            }
        }

        
    private:
        
        Int WalkBackToStrandStart( const Int a_0 ) const
        {
            Int a = a_0;

            AssertArc<1>(a);

            // TODO: We could maybe save some lookups here if we inline ArcUnderQ and NextArc. However, I tried it and it did not improve anything. Probably because this loop is typically not very long or because the compiler is good at optimizing this on its own. So I decided to keep this for better readibility.
            
            // Take a first step in any case.
            if( pd.ArcUnderQ(a,Tail) != overQ  )
            {
                a = pd.NextArc(a,Tail);
                AssertArc<1>(a);
            }
            
            // If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well. So we need a guard against cycling around this unlink forever! ----------------+
            //                                                      |
            //                                                      V
            while( (pd.ArcUnderQ(a,Tail) != overQ) && (a != a_0) )
            {
                a = pd.NextArc(a,Tail);
                AssertArc<1>(a);
            }

            // We could catch the unlink already here, but that would need some change of communication here. Instead, we dealy this to the do-loop in StrandSimplifier. This will be double work, but only in very rare cases.
            
            return a;
        }

        template<bool headtail>
        void MoveWhileBranching( mref<Int> a, mref<Int> p ) const
        {
            // `a` is an arc.
            // `p` points to a position in `path`.
            
            Int c = A_cross(a,headtail);
            
            Int side = (C_arcs(c,headtail,Right) == a);
            
            Int b = path[p];
            
            while(
                (C_arcs(c,headtail,!side) == b)
                ||
                (C_arcs(c,!headtail,side) == b)
            )
            {
                a = C_arcs(c,!headtail,!side);
                
                c = A_cross(a,headtail);

                side = (C_arcs(c,headtail,Right) == a);
                
                p = headtail ? p + Int(1) : p - Int(1);
                
                b = path[p];
            }
        }
    
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("StrandSimplifier")
                + "<" + TypeName<Int>
                + "," + ToString(mult_compQ)
                + ">";
        }

    }; // class StrandSimplifier
    
} // namespace Knoodle
