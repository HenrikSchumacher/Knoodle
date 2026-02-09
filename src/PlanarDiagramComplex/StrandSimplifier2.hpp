#pragma once

// TODO: Make RemoveLoop work also for Big Figure-8 Unlink and Big Hopf Link.
// TODO: Use `ArcSide`.
// TODO: `WalkBackToStrandStart` -> Cut out over/under loops already here? ...
//          --> less headache later.
//          --> more efficient.

namespace Knoodle
{
    enum class DijkstraStrategy_T : Int8
    {
        Unidirectional = 0,
        Alternating    = 1,
        Bidirectional  = 2,
        Legacy         = 3
    };
    
    std::string ToString( const DijkstraStrategy_T strategy )
    {
        switch( strategy )
        {
            case DijkstraStrategy_T::Unidirectional : return "Unidirectional";
            case DijkstraStrategy_T::Alternating    : return "Alternating";
            case DijkstraStrategy_T::Bidirectional  : return "Bidirectional";
            case DijkstraStrategy_T::Legacy         : return "Legacy";
        }
    }
    
    template<typename Int_, bool mult_compQ_>
    class alignas( ObjectAlignment ) StrandSimplifier2 final
    {
    public:
        
//        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
        
        using PDC_T      = PlanarDiagramComplex<Int>;
        using PD_T       = PDC_T::PD_T;
        
//        // We need a signed integer type for Mark_T because we use "negative" values to indicate directions in the dual graph.
//        using Mark_T     = ToSigned<Int>;
        
        using CrossingContainer_T        = typename PD_T::CrossingContainer_T;
        using ArcContainer_T             = typename PD_T::ArcContainer_T;
        using C_Arcs_T                   = typename PD_T::C_Arcs_T;
        using A_Cross_T                  = typename PD_T::A_Cross_T;
        using CrossingStateContainer_T   = typename PD_T::CrossingStateContainer_T;
        using ArcColorContainer_T        = typename PD_T::ArcColorContainer_T;
        using ArcStateContainer_T        = typename PD_T::ArcStateContainer_T;
        using Stack_T                    = Stack<Int,Int>;
        using Dijkstra_T                 = DijkstraStrategy_T;
        
        static constexpr bool debugQ     = false;
        static constexpr bool mult_compQ = mult_compQ_;
        
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Out   = PD_T::Out;
        
        static constexpr bool Uninitialized = PD_T::Uninitialized;
        
        
        struct DualArc_T
        {
            Int from;
            bool left_to_rightQ;
            bool forwardQ;
        };
        
        friend std::string ToString( cref<DualArc_T> a )
        {
            return std::string("{") + ToString(a.from) + "," + ToString(a.left_to_rightQ) + "," + ToString(a.forwardQ) + "}";
        }
        
        using ArcData_Vector2List_T = Tiny::VectorList_AoS<2,Int,Int>;
        using ArcData_Vector4List_T = Tiny::VectorList_AoS<4,Int,Int>;
        using ArcData_HasMap2_T      = AssociativeContainer<Int,std::array<Int,2>>;
        using ArcData_HasMap1_T      = AssociativeContainer<Int,Int>;
        using ArcData_HasMapStruct_T = AssociativeContainer<Int,DualArc_T>;
        
//        using ArcData_HasMap2_T      = std::unordered_map<Int,std::array<Int,2>>;
//        using ArcData_HasMap1_T      = std::unordered_map<Int,Int>;
//        using ArcData_HasMapStruct_T = std::unordered_map<Int,DualArc_T>;
        
        using ArcDataContainer_T    = ArcData_Vector2List_T;
//        using ArcDataContainer_T    = ArcData_Vector4List_T;
//        using ArcDataContainer_T    = ArcData_HasMap2_T;
//        using ArcDataContainer_T    = ArcData_HasMap1_T;
//        using ArcDataContainer_T    = ArcData_HasMapStruct_T;

        static constexpr bool vector2_listQ    = std::is_same_v<ArcDataContainer_T,ArcData_Vector2List_T>;
        static constexpr bool vector4_listQ    = std::is_same_v<ArcDataContainer_T,ArcData_Vector4List_T>;
        static constexpr bool vector_listQ     = vector2_listQ || vector4_listQ;
        static constexpr bool hash_map2Q       = std::is_same_v<ArcDataContainer_T,ArcData_HasMap2_T>;
        static constexpr bool hash_map1Q       = std::is_same_v<ArcDataContainer_T,ArcData_HasMap1_T>;
        static constexpr bool hash_map_structQ = std::is_same_v<ArcDataContainer_T,ArcData_HasMapStruct_T>;
        static constexpr bool hash_mapQ        = hash_map2Q || hash_map1Q || hash_map_structQ;
        
    private:

        PDC_T & restrict pdc;

        PD_T  * restrict pd = nullptr;

        Int    max_crossing_count = 0;
        Int    max_arc_count      = 0;
        Int    current_mark       = 0;
        Int    initial_mark       = 0;
        Int    s_begin            = 0; // First arc in strand.
        Int    strand_arc_count   = 0;
        Int    path_length        = 0;
        Size_T change_counter     = 0;
        
        // Marks for the crossings and arcs to mark the current strand.
        // We use this to detect loop strands.
        // We need quite close control on the values in these containers; this is why we cannot use pd->C_scratch or pd->A_scratch here.
        Tensor1<Int,Int>   C_mark;
        Tensor1<Int,Int>   A_mark;
        
        ArcDataContainer_T D_data;  // Two Int per (dual) arc: The first one stores the current marker and some bits for left/right direction and one for traversal backward/forward direction.
        
        Int * restrict dA_left;
        
        Tensor1<Int,Int> path;
        Stack_T X_front;
        Stack_T Y_front;
        Stack_T prev_front;
        
        DijkstraStrategy_T strategy = DijkstraStrategy_T::Bidirectional;
        bool overQ;
        bool strand_completeQ;
        
    public:
        
        StrandSimplifier2( PDC_T & pdc_, DijkstraStrategy_T strategy_ )
        :   pdc                { pdc_                                       }
        ,   strategy           { strategy_                                  }
        {
            Allocate(pdc.MaxMaxCrossingCount());
        }
        
        // No default constructor
        StrandSimplifier2() = delete;
        
        // Destructor
        ~StrandSimplifier2() = default;
        
        // Copy constructor
        StrandSimplifier2( const StrandSimplifier2 & other ) = default;
        // Copy assignment operator
        StrandSimplifier2 & operator=( const StrandSimplifier2 & other ) = default;
        // Move constructor
        StrandSimplifier2( StrandSimplifier2 && other ) = default;
        // Move assignment operator
        StrandSimplifier2 & operator=( StrandSimplifier2 && other ) = default;

    private:
        
        void Allocate( const Int max_crossing_count_ )
        {
            TOOLS_PTIMER(timer,MethodName("Allocate")+"(" + ToString(max_crossing_count_) + ")");
            
            max_crossing_count = max_crossing_count_;
            max_arc_count      = int_cast<Int>(Int(2) * max_crossing_count); // int_cast for e.g., Int16
            C_mark             = Tensor1<Int,Int>( max_crossing_count, Int(0) );
            A_mark             = Tensor1<Int,Int>( max_arc_count     , Int(0) );
            path               = Tensor1<Int,Int>( max_arc_count     , Uninitialized );
            X_front            = Stack_T( max_arc_count );
            Y_front            = Stack_T( max_arc_count );
            prev_front         = Stack_T( max_arc_count );
            
            if constexpr ( hash_mapQ )
            {
                D_data.reserve(max_arc_count);
            }
            
            if constexpr ( vector_listQ )
            {
                D_data = ArcDataContainer_T(max_arc_count,Int(0));
            }
            
            initial_mark = 1;
            current_mark = 1;
            strand_arc_count  = 0;
        }
        
    public:
        
        void LoadDiagram( PD_T & pd_input )
        {
            TOOLS_PTIMER(timer,MethodName("LoadDiagram"));

            pd = &pd_input;

            if( (pd->max_crossing_count > Int(0)) && (max_crossing_count < pd->max_crossing_count) )
            {
                Allocate(pd->max_crossing_count);
            }

            if( current_mark >= max_mark/Int(2) )
            {
                C_mark.Fill(0);
                A_mark.Fill(0);
                if constexpr ( vector_listQ )
                {
                    D_data.Fill(0);
                }
                if constexpr ( hash_mapQ )
                {
                    D_data.clear();
                }
                initial_mark = 1;
                current_mark = 1;
                strand_arc_count  = 0;
                
                // TODO: We might have to reset the counters, too.
            }
            else
            {
                [[maybe_unused]] bool resetQ = NewStrand();
                PD_ASSERT(!resetQ);
                initial_mark = current_mark;
            }
            dA_left = pd->ArcLeftDarcs().data();
            PD_ASSERT(CheckDarcLeftDarc());
        }
        
        
        Int DarcLeftDarc( const Int da )
        {
            return dA_left[da];
        }

        void SetDarcLeftDarc( const Int da, const Int db )
        {
            dA_left[da] = db;
        }
        
        void Cleanup()
        {
            PD_TIMER(timer,MethodName("Cleanup"));
            
            PD_ASSERT(pd->CheckAll());
            PD_ASSERT(CheckDarcLeftDarc());
            
            dA_left = nullptr;
            pd      = nullptr;
        }
        
        DijkstraStrategy_T DijkstraStrategy() const
        {
            return strategy;
        }
        
        mref<StrandSimplifier2> SetDijkstraStrategy( DijkstraStrategy_T strategy_ )
        {
            strategy = strategy_;
            
            return *this;
        }

#include "StrandSimplifier/DualArcs.hpp"
#include "StrandSimplifier/Marks.hpp"
#include "StrandSimplifier/Checks.hpp"
#include "StrandSimplifier/Helpers.hpp"
#include "StrandSimplifier/RepairArcs.hpp"
#include "StrandSimplifier/Reconnect.hpp"
#include "StrandSimplifier/CollapseArcRange.hpp"
#include "StrandSimplifier/RemoveLoopPath.hpp"
#include "StrandSimplifier/FindShortestPath.hpp"
#include "StrandSimplifier/FindShortestPath_Legacy.hpp"
#include "StrandSimplifier/RerouteToPath.hpp"
#include "StrandSimplifier/Reidemeister.hpp"
#include "StrandSimplifier/SimplifyStrands.hpp"
#include "StrandSimplifier/Strings.hpp"
#include "StrandSimplifier/Counters.hpp"
        
//#include "StrandSimplifier/SimplifyLocal.hpp" // Only meant for debugging.
        
    private:
        
        void CountReidemeister_I() const
        {}
        
        void CountReidemeister_II() const
        {}
        
    private:
        
        
        bool StrandMode() const
        {
            return overQ;
        }
        
        void SetStrandMode( const bool overQ_ )
        {
            overQ = overQ_;
        }
        
        
        // Returns false if reset is equired.
        bool NewStrand()
        {
            // Safeguard against integer overflow.
            if( current_mark >= max_mark ) [[unlikely]]
            {
                wprint("NewStrand returns true.");
                // Return a positive number to encourage that this function is called again upstream.
                change_counter++;
                return true;
            }
            
            PD_PRINT(std::string("NewStrand created new ") + (overQ ? "over" : "under") + "strand with current_mark = " + ToString(current_mark) + "." );
            
            if constexpr( hash_map1Q || hash_map_structQ )
            {
                D_data.clear();
            }
            
            ++current_mark;
            strand_arc_count = 0;
            return false;
        }

        
        Int WalkBackToStrandStart( const Int a_0 ) const
        {
            Int a = a_0;
            AssertArc<1>(a);

            // Take a first step in any case.
            if( ArcOverQ(a,Tail) == overQ  )
            {
                a = NextArc(a,Tail);
                AssertArc<1>(a);
            }
            
            // If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well. So we need a guard against cycling around this unlink forever!
            //                                        |
            //                                        V
            while( (ArcOverQ(a,Tail) == overQ) && (a != a_0) )
            {
                a = NextArc(a,Tail);
                AssertArc<1>(a);
            }
            
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
                + "," + ToString(mult_compQ)
                + ">";
        }

    }; // class StrandSimplifier2
    
} // namespace Knoodle
