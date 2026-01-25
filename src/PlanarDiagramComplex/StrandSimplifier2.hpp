#pragma once

// TODO: Make RemoveLoop work also for Big Figure-8 Unlink and Big Hopf Link.
// TODO: Implement FindShortestPath_impl with two-sided Dijkstra! This should converge faster, but might require more reloads because the stacks might become cold. Do it in ABBAABBABBA-fashion to remedy this a little.
// TODO: Use `ArcSide`.
// TODO: `WalkBackToStrandStart` -> Cut out over/under loops already here? ...
//          --> less headache later.
//          --> more efficient.

namespace Knoodle
{
    
    enum class SearchStrategy_T : Int8
    {
        Dijkstra    = 0,
        TwoSided    = 1,
        DijkstraLegacy = 2
    };
    
    std::string ToString( const SearchStrategy_T strategy )
    {
        switch( strategy )
        {
            case SearchStrategy_T::Dijkstra       : return "Dijkstra";
            case SearchStrategy_T::TwoSided       : return "TwoSided";
            case SearchStrategy_T::DijkstraLegacy : return "DijkstraLegacy";
        }
    }
    
    template<typename Int_, bool R_II_Q_, bool mult_compQ_, SearchStrategy_T strategy_>
    class alignas( ObjectAlignment ) StrandSimplifier2 final
    {
    public:
        
//        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
        
        using PDC_T      = PlanarDiagramComplex<Int>;
        using PD_T       = PDC_T::PD_T;
        
//        // We need a signed integer type for Mark_T because we use "negative" values to indicate directions in the dual graph.
//        using Mark_T     = ToSigned<Int>;
        
        using CrossingContainer_T       = typename PD_T::CrossingContainer_T;
        using ArcContainer_T            = typename PD_T::ArcContainer_T;
        using C_Arcs_T                  = typename PD_T::C_Arcs_T;
        using A_Cross_T                 = typename PD_T::A_Cross_T;
        using CrossingStateContainer_T  = typename PD_T::CrossingStateContainer_T;
        using ArcColorContainer_T       = typename PD_T::ArcColorContainer_T;
        using ArcStateContainer_T       = typename PD_T::ArcStateContainer_T;

        static constexpr SearchStrategy_T strategy = strategy_;
        
        static constexpr bool R_II_Q     = R_II_Q_;
        static constexpr bool mult_compQ = mult_compQ_;
        
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Out   = PD_T::Out;
        
        static constexpr bool Uninitialized = PD_T::Uninitialized;
        
    private:
        
        PDC_T & restrict pdc;
        
        PD_T  & restrict pd;
        
        CrossingContainer_T      & restrict C_arcs;
        CrossingStateContainer_T & restrict C_state;
        
        ArcContainer_T           & restrict A_cross;
        ArcColorContainer_T      & restrict A_color;
        ArcStateContainer_T      & restrict A_state;
        
        Tensor1<Int,Int>         & restrict D_mark2;
        
        
        // Marks for the crossings and arcs to mark the current strand.
        // We use this to detect loop strands.
        Tensor1<Int,Int> C_mark;
        Tensor1<Int,Int> A_mark;
        
        ArcContainer_T   D_data;  // Two Int per (dual) arc: The first one stores the current marker and some bits for left/right direction and one for traversal backward/forward direction.
        
        Int * restrict dA_left;
        
        Int current_mark = 0;
        Int a_ptr = 0;

        Int    strand_length  = 0;
        Size_T change_counter = 0;
        
        bool overQ;
        bool strand_completeQ;
        
        Stack<Int,Int> X_next;
        Stack<Int,Int> X_prev;
        
        Stack<Int,Int> Y_next;
        Stack<Int,Int> Y_prev;
        
        Tensor1<Int,Int> path;
        Int path_length = 0;
        
        std::vector<Int> touched;
        
        double Time_RemoveLoop            = 0;
        double Time_FindShortestPath      = 0;
        double Time_RerouteToPath         = 0;
        double Time_SimplifyStrands       = 0;
        double Time_CollapseArcRange      = 0;
        
    public:
        
        StrandSimplifier2( PDC_T & pdc_, PD_T & pd_ )
        :   pdc     { pdc_                          }
        ,   pd      { pd_                           }
        ,   C_arcs  { pd.C_arcs                     }
        ,   C_state { pd.C_state                    }
        ,   A_cross { pd.A_cross                    }
        ,   A_color { pd.A_color                    }
        ,   A_state { pd.A_state                    }
        ,   D_mark2 { pd.A_scratch                  }
        // We initialize by 0, indicating invalid/uninitialized.
        ,   C_mark  { C_arcs .Dim(0), Uninitialized }
        ,   A_mark  { A_cross.Dim(0), Uninitialized }
//        ,   D_mark  { A_cross.Dim(0), Uninitialized }
//        ,   D_from  { A_cross.Dim(0), Uninitialized }
        ,   D_data  { A_cross.Dim(0), Uninitialized }
        ,   X_next  { A_cross.Dim(0)                }
        ,   X_prev  { A_cross.Dim(0)                }
        ,   Y_next  { A_cross.Dim(0)                } // TODO: Only needed for two-sided Dijkstra.
        ,   Y_prev  { A_cross.Dim(0)                } // TODO: Only needed for two-sided Dijkstra.
        ,   path    { A_cross.Dim(0), Uninitialized }
        {}
        
        // No default constructor
        StrandSimplifier2() = delete;
        
        // Destructor
        ~StrandSimplifier2()
        {
#ifdef PD_TIMINGQ
            logprint("");
            logprint(ClassName() + " absolute timings:");
            logvalprint("SimplifyStrands", Time_SimplifyStrands);
            logvalprint("RemoveLoop", Time_RemoveLoop);
            logvalprint("FindShortestPath", Time_FindShortestPath);
            logvalprint("RerouteToPath", Time_RerouteToPath);
            logvalprint("CollapseArcRange", Time_CollapseArcRange);
            logprint("");
            logprint(ClassName() + " relative timings:");
            logvalprint("SimplifyStrands", Time_SimplifyStrands/Time_SimplifyStrands);
            logvalprint("RemoveLoop", Time_RemoveLoop/Time_SimplifyStrands);
            logvalprint("FindShortestPath", Time_FindShortestPath/Time_SimplifyStrands);
            logvalprint("Time_RerouteToPath", Time_RerouteToPath/Time_SimplifyStrands);
            logvalprint("CollapseArcRange", Time_CollapseArcRange/Time_SimplifyStrands);
            logprint("");
#endif
        }
        
        // Copy constructor
        StrandSimplifier2( const StrandSimplifier2 & other ) = default;
        // Copy assignment operator
        StrandSimplifier2 & operator=( const StrandSimplifier2 & other ) = default;
        // Move constructor
        StrandSimplifier2( StrandSimplifier2 && other ) = default;
        // Move assignment operator
        StrandSimplifier2 & operator=( StrandSimplifier2 && other ) = default;


#include "StrandSimplifier/Marks.hpp"
#include "StrandSimplifier/Checks.hpp"
#include "StrandSimplifier/Helpers.hpp"
#include "StrandSimplifier/RepairArcs.hpp"
#include "StrandSimplifier/Reconnect.hpp"
#include "StrandSimplifier/CollapseArcRange.hpp"
#include "StrandSimplifier/RemoveLoop.hpp"
#include "StrandSimplifier/FindShortestPath.hpp"
#include "StrandSimplifier/FindShortestPath2.hpp"
#include "StrandSimplifier/RerouteToPath.hpp"
#include "StrandSimplifier/Reidemeister.hpp"
#include "StrandSimplifier/SimplifyStrands.hpp"
#include "StrandSimplifier/Strings.hpp"
        
    private:
        
        void CreateUnlinkFromArc( const Int a_ )
        {
            pdc.CreateUnlinkFromArc(pd,a_);
        }
        
        void CreateHopfLinkFromArcs( const Int a_0, const Int a_1 )
        {
            pdc.CreateHopfLinkFromArcs(pd,a_0,a_1);
        }
        
        void CountReidemeister_I() const
        {}
        
        void CountReidemeister_II() const
        {}
        
    private:
        
        Int WalkBackToStrandStart( const Int a_0 ) const
        {
            Int a = a_0;

            AssertArc<1>(a);

            // Take a first step in any case.
            if( ArcUnderQ(a,Tail) != overQ  )
            {
                a = NextArc(a,Tail);
                AssertArc<1>(a);
            }
            
            // If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well. So we need a guard against cycling around this unlink forever! ----------------------+
            //                                        |
            //                                        V
            while( (ArcUnderQ(a,Tail) != overQ) && (a != a_0) )
            {
                a = NextArc(a,Tail);
                AssertArc<1>(a);
            }

            // TODO: We could catch the unlink already here, but that would need some change of communication here. Instead, we delay this to the do-loop in StrandSimplifier2. This will be double work, but only in very rare cases.
            
            return a;
        }
    
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("StrandSimplifier2")
                + "<" + TypeName<Int>
                + "," + ToString(R_II_Q)
                + "," + ToString(mult_compQ)
//                + "," + ToString(strategy)
                + ">";
        }

    }; // class StrandSimplifier2
    
} // namespace Knoodle
