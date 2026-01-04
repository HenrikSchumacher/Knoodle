#pragma  once

//#include <unordered_set>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>


namespace Knoodle
{

    
//    template<typename Int_, bool mult_compQ_> class StrandSimplifier;
//    
//    template<typename Int_, Size_T optimization_level, bool mult_compQ_>
//    class ArcSimplifier;
    
    // TODO: Checking routine for arc states.
    // TODO: Test ArcOverQ and ArcUnderQ.
    
    // TODO: Test NextArc, NextLeftArc, and NextRightArc.
    // TODO: Make Arrow_T obsolete. It is used in NextLeftArc, NextRightArc, etc.
    // TODO: ---> change NextLeftArc, NextRightArc to NextLeftDarc, NextRightDarc.
    
    // TODO: Port the methods in Unported.hpp
    // TODO: Port methods in Counters.hpp?
    
    // TODO: Enable unsigned integers because they should be about 15% faster in task heavy bit manipulations.

    
    template<typename Int_>
    class alignas( ObjectAlignment ) PlanarDiagram2 final : public CachedObject
    {
        static_assert(IntQ<Int_>,"");

    public:
        
        static constexpr bool always_compressQ = true;
        static constexpr bool countQ           = false;
        static constexpr bool debugQ           = false;
            
        using Int                       = Int_;
        using UInt                      = ToUnsigned<Int>;

        using Base_T                    = CachedObject;
        using Class_T                   = PlanarDiagram2<Int>;
        using PlanarDiagram_T           = PlanarDiagram2<Int>;

        using CrossingContainer_T       = Tiny::MatrixList_AoS<2,2,Int,Int>;
        using ArcContainer_T            = Tiny::VectorList_AoS<2,  Int,Int>;

#include "PlanarDiagram2/CrossingState.hpp"
#include "PlanarDiagram2/ArcState.hpp"
                
    public:
        
        using CrossingStateContainer_T  = Tensor1<CrossingState_T,Int>;
        using ArcStateContainer_T       = Tensor1<ArcState_T,Int>;
        
        using ArcColorContainer_T       = Tensor1<Int,Int>;

        using MultiGraph_T              = MultiGraph<Int,Int>;
        using ComponentMatrix_T         = MultiGraph_T::ComponentMatrix_T;

//        using PD_List_T                 = std::vector<PlanarDiagram>;

//        template<typename I, Size_T lvl, bool mult_compQ_>
//        friend class ArcSimplifier;
//
//        template<typename I, bool mult_compQ_>
//        friend class StrandSimplifier;
            
        using HeadTail_T = bool;
        using Arrow_T = std::pair<Int,HeadTail_T>;
        
        static constexpr HeadTail_T Tail  = 0;
        static constexpr HeadTail_T Head  = 1;
        
        static constexpr bool Left  = 0;
        static constexpr bool Right = 1;
        static constexpr bool Out   = 0;
        static constexpr bool In    = 1;
        
        static constexpr Int Uninitialized = SignedIntQ<Int> ? Int(-1): std::numeric_limits<Int>::max();
        
        // For the faces I need at least one invalid value that is different from `Uninitialized`. So we consider (Uninitialized - 1)` as another invalid index. If `Int` is unsigned, some special precaution has to be taken.
        
        static constexpr Int MaxValidIndex = SignedIntQ<Int> ? std::numeric_limits<Int>::max() : std::numeric_limits<Int>::max() - Int(2);
        
        static constexpr bool ValidIndexQ( const Int i )
        {
//            logprint("ValidIndexQ(" + ToString(i) + ")");
            if constexpr ( SignedIntQ<Int> )
            {
                return (i >= Int(0));
            }
            else
            {
                return (i <= MaxValidIndex);
            }
        }
        
        static constexpr Int UninitializedIndex()
        {
            return Uninitialized;
        }
        
    protected:
        
        // Class data members
        
        Int crossing_count      = 0;
        Int arc_count           = 0;
        Int max_crossing_count  = 0;
        Int max_arc_count       = 0;
        
        // This flag is needed to store unknots, as every link component in a LinkComplex needs a color. Colors are usually stored in arcs, but an unknot has no arcs.
        // An unknot is represented by a PlanarDiagram with max_crossing_count == 0 and with color_flag being set so that ValidIndexQ(color_flag) evaluates to true.
        Int color_flag          = Uninitialized;
        
        // Exposed to user via Crossings().
        CrossingContainer_T      C_arcs;
        // Exposed to user via CrossingStates().
        CrossingStateContainer_T C_state;
        // Some multi-purpose scratch buffers.
        mutable Tensor1<Int,Int> C_scratch;
        
        // Exposed to user via Arcs().
        ArcContainer_T           A_cross;
        // Exposed to user via ArcStates().
        ArcStateContainer_T      A_state;
        // Exposed to user via ArcColors().
        ArcColorContainer_T      A_color;
        // Some multi-purpose scratch buffers.
        mutable Tensor1<Int,Int> A_scratch;
        
        bool proven_minimalQ = false;
        
    public:
  
        // Default constructor
        PlanarDiagram2() = default;
        // Destructor (virtual because of inheritance)
        virtual ~PlanarDiagram2() override = default;
        // Copy constructor
        PlanarDiagram2( const PlanarDiagram2 & other ) = default;
        // Copy assignment operator
        PlanarDiagram2 & operator=( const PlanarDiagram2 & other ) = default;
        // Move constructor
        PlanarDiagram2( PlanarDiagram2 && other ) = default;
        // Move assignment operator
        PlanarDiagram2 & operator=( PlanarDiagram2 && other ) = default;
 
    private:
        
        /*! @brief This constructor is supposed to only allocate all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
        template<typename ExtInt>
        PlanarDiagram2( const ExtInt max_crossing_count_ )
        : crossing_count     { Int(0)                                          }
        , arc_count          { Int(0)                                          }
        , max_crossing_count { int_cast<Int>(max_crossing_count_)              }
        , max_arc_count      { Int(Int(2) * max_crossing_count)                }
        , C_arcs             { max_crossing_count, Uninitialized               }
        , C_state            { max_crossing_count, CrossingState_T::Inactive() }
        , C_scratch          { max_crossing_count                              }
        , A_cross            { max_arc_count,      Uninitialized               }
        , A_state            { max_arc_count,      ArcState_T::Inactive()      }
        , A_color            { max_arc_count,      Int(0)                      }
        , A_scratch          { max_arc_count                                   }
        {
            static_assert(IntQ<ExtInt>,"");
        }

    public:
        
        template<typename ExtInt, typename ExtInt2, typename ExtInt3, typename ExtInt4>
        PlanarDiagram2(
            cptr<ExtInt>  crossings,
            cptr<ExtInt2> crossing_states,
            cptr<ExtInt>  arcs,
            cptr<ExtInt3> arc_states,
            cptr<ExtInt4> arc_colors,
            const ExtInt crossing_count_,
            const bool proven_minimalQ_ = false
        )
        :   PlanarDiagram2( crossing_count_ )
        {
            static_assert(IntQ<ExtInt>,"");
            static_assert(IntQ<ExtInt2>||SameQ<ExtInt2,CrossingState_T>,"");
            static_assert(IntQ<ExtInt3>||SameQ<ExtInt3,ArcState_T>,"");
            
            C_arcs.Read(crossings);
            C_state.Read(crossing_states);
            A_cross.Read(arcs);
            A_state.Read(arc_states);
            A_color.Read(arc_colors);
            proven_minimalQ = proven_minimalQ_;
            
            crossing_count = CountActiveCrossings();
            arc_count      = CountActiveArcs();
        }
        
    public:
        
#include "PlanarDiagram2/Crossings.hpp"
#include "PlanarDiagram2/Arcs.hpp"
#include "PlanarDiagram2/Darcs.hpp"
#include "PlanarDiagram2/Checks.hpp"
        
#include "PlanarDiagram2/Traverse.hpp"
#include "PlanarDiagram2/LinkComponents.hpp"
        
#include "PlanarDiagram2/CreateCompressed.hpp"
#include "PlanarDiagram2/Reconnect.hpp"
        
#include "PlanarDiagram2/PDCode.hpp"
        
//#include "PlanarDiagram2/ReadFromLink.hpp"


//#include "PlanarDiagram2/R_I.hpp"


//#include "PlanarDiagram2/Faces.hpp"

//#include "PlanarDiagram2/DiagramComponents.hpp"
//#include "PlanarDiagram2/Strands.hpp"
//#include "PlanarDiagram2/DisconnectSummands.hpp"
        
//#include "PlanarDiagram2/Simplify1.hpp"
//#include "PlanarDiagram2/Simplify2.hpp"
//#include "PlanarDiagram2/Simplify3.hpp"
//#include "PlanarDiagram2/Simplify4.hpp"
//#include "PlanarDiagram2/Simplify5.hpp"
//#include "PlanarDiagram2/Simplify6.hpp"
        

//#include "PlanarDiagram2/GaussCode.hpp"
#include "PlanarDiagram2/LongMacLeodCode.hpp"
#include "PlanarDiagram2/MacLeodCode.hpp"
        
//#include "PlanarDiagram2/ResolveCrossing.hpp"
//#include "PlanarDiagram2/SwitchCrossing.hpp"
        
//#include "PlanarDiagram2/VerticalSummandQ.hpp"
        
#include "PlanarDiagram2/DepthFirstSearch.hpp"
//#include "PlanarDiagram2/SpanningForest.hpp"
        
#include "PlanarDiagram2/Permute.hpp"
#include "PlanarDiagram2/Planarity.hpp"
        
    public:
        
        bool ProvenMinimalQ() const
        {
            return proven_minimalQ;
        }
        
        bool InvalidQ() const
        {
            return (max_crossing_count == Int(0)) && ValidIndexQ(color_flag);
        }
        
        bool ValidQ() const
        {
            return !InvalidQ();
        }
        
        bool ProvenTrefoilQ() const
        {
            return proven_minimalQ && (CrossingCount() == Int(3)) && (LinkComponentCount() == Int(1));
        }
        
        bool ProvenFigureEightQ() const
        {
            return proven_minimalQ && (CrossingCount() == Int(4)) && (LinkComponentCount() == Int(1));
        }
        
        
    public:
        
        void PrintInfo()
        {
            logprint(ClassName()+"::PrintInfo");
            
            TOOLS_LOGDUMP( C_arcs );
            TOOLS_LOGDUMP( C_state );
            TOOLS_LOGDUMP( A_cross );
            TOOLS_LOGDUMP( A_state );
            TOOLS_LOGDUMP( A_color );
//            TOOLS_LOGDUMP( unlink_count );
        }

/*!@brief A coarse estimator of heap-allocated memory in use for this class instance. Does not account for quantities stored in the class' cache.
*/
        Size_T AllocatedByteCount()
        {
            Size_T byte_count =
                  C_arcs.AllocatedByteCount()
                + C_state.AllocatedByteCount()
                + C_scratch.AllocatedByteCount()
                + A_cross.AllocatedByteCount()
                + A_state.AllocatedByteCount()
                + A_color.AllocatedByteCount()
                + A_scratch.AllocatedByteCount();
            
            // TODO: This does not account for Cache and PersistentCache.
            
            return byte_count;
        }
        
        std::string AllocatedByteCountString() const
        {
            return
                ClassName() + " allocations \n"
                + "\t" + TOOLS_MEM_DUMP_STRING(C_arcs)
                + "\t" + TOOLS_MEM_DUMP_STRING(C_state)
                + "\t" + TOOLS_MEM_DUMP_STRING(C_scratch)
                + "\t" + TOOLS_MEM_DUMP_STRING(A_cross)
                + "\t" + TOOLS_MEM_DUMP_STRING(A_state)
                + "\t" + TOOLS_MEM_DUMP_STRING(A_color)
                + "\t" + TOOLS_MEM_DUMP_STRING(A_scratch);
        }
        
/*!@brief A coarse estimator of memory in use for this class instance. Does not account for quantities stored in the class' cache.
*/
        Size_T ByteCount()
        {
            return sizeof(PlanarDiagram2) + AllocatedByteCount();
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
/*!@brief Returns a string that identifies this class with type information. Mostly used for logging and in error messages.
 */
        
        static std::string ClassName()
        {
            return ct_string("PlanarDiagram2")
                + "<" + TypeName<Int>
                + ">";
        }
    };

} // namespace Knoodle



