#pragma  once

//#include <unordered_set>

//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/boyer_myrvold_planar_test.hpp>

namespace Knoodle
{
    // TODO: Template this.
    // bool fancy_arc_stateQ -- whether the flags other than an active bit ought to be used at all.

    
    template<typename Int> class PlanarDiagramComplex;
    
//    template<typename Int_, bool mult_compQ_> class StrandSimplifier;
//    
//    template<typename Int_, Size_T optimization_level, bool mult_compQ_>
//    class ArcSimplifier;
    
    // TODO: SwitchCrossing should also correctly set the arc states.
    
    // TODO: Port the methods in Unported.hpp
    // TODO: Port methods in Counters.hpp?

    template<typename Int_>
    class alignas( ObjectAlignment ) PlanarDiagram2 final : public CachedObject
    {
        static_assert(IntQ<Int_>,"");
        
    public:
        
        static constexpr bool always_compressQ = true;
        static constexpr bool countQ           = false;
        static constexpr bool debugQ           = false;
        
        using Int                   = Int_;
        using UInt                  = ToUnsigned<Int>;
        
        using Base_T                = CachedObject;
        using Class_T               = PlanarDiagram2<Int>;
        using PD_T                  = PlanarDiagram2<Int>;
        
        using CrossingContainer_T   = Tiny::MatrixList_AoS<2,2,Int,Int>;
        using ArcContainer_T        = Tiny::VectorList_AoS<2,  Int,Int>;
        using ColorList_T           = std::unordered_set<Int>;
        
#include "PlanarDiagram2/CrossingState.hpp"
#include "PlanarDiagram2/ArcState.hpp"
        
    public:
        
        using CrossingStateContainer_T  = Tensor1<CrossingState_T,Int>;
        using ArcStateContainer_T       = Tensor1<ArcState_T,Int>;
        using ArcColorContainer_T       = Tensor1<Int,Int>;
        
        using MultiGraph_T              = MultiGraph<Int,Int>;
        using ComponentMatrix_T         = MultiGraph_T::ComponentMatrix_T;
        
        friend class PlanarDiagramComplex<Int>;
        
        template<typename Int, Size_T, bool>
        friend class ArcSimplifier2;

        template<typename I, bool R_II_Q, bool mult_compQ_>
        friend class StrandSimplifier2;
            
        using HeadTail_T = bool;
        
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
        
        
        static constexpr Int InvalidColor = Uninitialized;
        
    protected:
        
        // Class data members
        
        Int crossing_count      = 0;
        Int arc_count           = 0;
        Int max_crossing_count  = 0;
        Int max_arc_count       = 0;
        
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
            
        // This color_list is needed, among other things, to store unknots, as every link component in a LinkComplex needs a color. Colors are usually stored in arcs, but an unknot has no arcs.
        // Moreover, color_list will make some search queries terminate early.
        // ComputeArcColors initializes color_list.
        // Cut and past operations have to maintain color_list.
        // An unknot is represented by a planar diagram with CrossingCount() == 0 and with color_list containing a single value != InvalidColor.

        ColorList_T color_list;
        
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
        
        /*! @brief This constructor is supposed to only allocate and initialize all relevant buffers.
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
        , A_color            { max_arc_count,      Uninitialized               }
        , A_scratch          { max_arc_count                                   }
        {
            static_assert(IntQ<ExtInt>,"");
        }
        
        
        /*! @brief This constructor is supposed to only allocate all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
        template<typename ExtInt>
        PlanarDiagram2( const ExtInt max_crossing_count_, bool dummy )
        : crossing_count     { Int(0)                                          }
        , arc_count          { Int(0)                                          }
        , max_crossing_count { int_cast<Int>(max_crossing_count_)              }
        , max_arc_count      { Int(Int(2) * max_crossing_count)                }
        , C_arcs             { max_crossing_count                              }
        , C_state            { max_crossing_count                              }
        , C_scratch          { max_crossing_count                              }
        , A_cross            { max_arc_count                                   }
        , A_state            { max_arc_count                                   }
        , A_color            { max_arc_count                                   }
        , A_scratch          { max_arc_count                                   }
        {
            (void)dummy;
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
        :   PlanarDiagram2( crossing_count_, true ) // Allocate, but do not fill.
        {
            static_assert(IntQ<ExtInt>,"");
            static_assert(IntQ<ExtInt2>||SameQ<ExtInt2,CrossingState_T>,"");
            static_assert(IntQ<ExtInt3>||SameQ<ExtInt3,ArcState_T>,"");
            
            C_arcs .Read(crossings);
            C_state.Read(crossing_states);
            A_cross.Read(arcs);
            A_state.Read(arc_states);
            A_color.Read(arc_colors);
            proven_minimalQ = proven_minimalQ_;
            
            crossing_count = CountActiveCrossings();
            arc_count      = CountActiveArcs();
        }
        
        /*! @brief Make a copy without copying cache and persistent cache.
         */
        PD_T CachelessCopy() const
        {
            PD_T pd ( max_crossing_count, true ); // Allocate, but do not fill.
            pd.crossing_count  = crossing_count;
            pd.arc_count       = arc_count;
            pd.max_arc_count   = max_arc_count;
            
            pd.C_arcs .Read(C_arcs .data());
            pd.C_state.Read(C_state.data());
            
            pd.A_cross.Read(A_cross.data());
            pd.A_state.Read(A_state.data());
            pd.A_color.Read(A_color.data());
            
            pd.proven_minimalQ = proven_minimalQ;
            pd.color_list      = color_list;
            
            return pd;
        }
        
    public:
        
#include "PlanarDiagram2/Constructors.hpp"
#include "PlanarDiagram2/Crossings.hpp"
#include "PlanarDiagram2/Arcs.hpp"
#include "PlanarDiagram2/Darcs.hpp"
#include "PlanarDiagram2/Checks.hpp"
        
#include "PlanarDiagram2/Traverse.hpp"
#include "PlanarDiagram2/LinkComponents.hpp"
#include "PlanarDiagram2/Color.hpp"
        
#include "PlanarDiagram2/CreateCompressed.hpp"
#include "PlanarDiagram2/Reconnect.hpp"
#include "PlanarDiagram2/SwitchCrossing.hpp"
        
#include "PlanarDiagram2/PDCode.hpp"
        
//#include "PlanarDiagram2/R_I.hpp"

#include "PlanarDiagram2/Faces.hpp"

//#include "PlanarDiagram2/DiagramComponents.hpp"
#include "PlanarDiagram2/Strands.hpp"
//#include "PlanarDiagram2/DisconnectSummands.hpp"
        
//#include "PlanarDiagram2/Simplify1.hpp"
//#include "PlanarDiagram2/Simplify2.hpp"
//#include "PlanarDiagram2/Simplify3.hpp"
//#include "PlanarDiagram2/Simplify4.hpp"
//#include "PlanarDiagram2/Simplify5.hpp"
//#include "PlanarDiagram2/Simplify6.hpp"
        

#include "PlanarDiagram2/GaussCode.hpp"
#include "PlanarDiagram2/LongMacLeodCode.hpp"
#include "PlanarDiagram2/MacLeodCode.hpp"
        
//#include "PlanarDiagram2/ResolveCrossing.hpp"

        
//#include "PlanarDiagram2/VerticalSummandQ.hpp"
        
#include "PlanarDiagram2/DepthFirstSearch.hpp"
//#include "PlanarDiagram2/SpanningForest.hpp"
        
#include "PlanarDiagram2/Permute.hpp"
//#include "PlanarDiagram2/Planarity.hpp"
        
    public:
        
        Int ColorCount() const
        {
            return int_cast<Int>( color_list.size() );
        }
        
        bool ProvenMinimalQ() const
        {
            return proven_minimalQ;
        }
        
        static PD_T InvalidDiagram()
        {
            return PD_T();
        }
        
        static PD_T Unknot( const Int color )
        {
            PD_T pd ( Int(0) );
            pd.proven_minimalQ = true;
            pd.color_list      = {color};
            return pd;
        }
        
        bool ProvenUnknotQ() const
        {
            return proven_minimalQ && (crossing_count == Int(0)) && (ColorCount() == Size_T(1));
        }

        bool ProvenTrefoilQ() const
        {
            return proven_minimalQ && (crossing_count == Int(3)) && (ColorCount() == Size_T(1));
        }
        
        bool ProvenFigureEightQ() const
        {
            return proven_minimalQ && (crossing_count == Int(4)) && (ColorCount() == Size_T(1));
        }

        bool InvalidQ() const
        {
            return (max_crossing_count == Int(0)) && (ColorCount() != Size_T(1));
        }
        
        bool ValidQ() const
        {
            return !InvalidQ();
        }
        
    public:
        
        /*!@brief Sets all entries of all deactivated crossings and arcs to `Uninitialized`.
         */

        void CleanseDeactivated()
        {
            for( Int c = 0; c < max_crossing_count; ++c )
            {
                if( !CrossingActiveQ(c) )
                {
                    fill_buffer<4>(C_arcs.data(c),Uninitialized);
                }
            }
            
            for( Int a = 0; a < max_arc_count; ++a )
            {
                if( !ArcActiveQ(a) )
                {
                    fill_buffer<2>(A_cross.data(a),Uninitialized);
                }
            }
        }
        
    /*!
     * @brief Computes the writhe = number of right-handed crossings - number of left-handed crossings.
     */

    Int Writhe() const
    {
        Int writhe = 0;
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingRightHandedQ(c) )
            {
                ++writhe;
            }
            else if ( CrossingLeftHandedQ(c) )
            {
                --writhe;
            }
        }
        
        return writhe;
    }

    Int EulerCharacteristic() const
    {
        TOOLS_PTIMER(timer,MethodName("EulerCharacteristic"));
        return CrossingCount() - ArcCount() + FaceCount();
    }

//    template<bool verboseQ = true>
//    bool EulerCharacteristicValidQ() const
//    {
//        TOOLS_PTIMER(timer,MethodName("EulerCharacteristicValidQ"));
//        const Int euler_char  = EulerCharacteristic();
//        const Int euler_char0 = Int(2) * DiagramComponentCount();
//        
//        const bool validQ = (euler_char == euler_char0);
//        
//        if constexpr ( verboseQ )
//        {
//            if( !validQ )
//            {
//                wprint(ClassName()+"::EulerCharacteristicValidQ: Computed Euler characteristic is " + ToString(euler_char) + " != 2 * DiagramComponentCount() = " + ToString(euler_char0) + ". The processed diagram cannot be planar.");
//            }
//        }
//        
//        return validQ;
//    }
        
        
    // Applies the transformation in-place.
    void ChiralityTransform( const bool mirrorQ, const bool reverseQ )
    {
        if( !mirrorQ && !reverseQ )
        {
            return;
        }
        
        ClearCache();
        
        const bool i0 = reverseQ;
        const bool i1 = !reverseQ;
        
        const bool j0 = (mirrorQ != reverseQ);
        const bool j1 = (mirrorQ == reverseQ);
        
        Int C [2][2] = {};
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            copy_buffer<4>( C_arcs.data(c), &C[0][0] );
            
            C_arcs(c,0,0) = C[i0][j0];
            C_arcs(c,0,1) = C[i0][j1];
            C_arcs(c,1,0) = C[i1][j0];
            C_arcs(c,1,1) = C[i1][j1];
        }
        
        if( mirrorQ )
        {
            mptr<CrossingState_T> C_state_ptr = C_state.data();
            
            for( Int c = 0; c < max_crossing_count; ++c )
            {
                C_state_ptr[c] = C_state_ptr[c].Reflect();
            }
        }
        
        mptr<ArcState_T> A_state_ptr = A_state.data();
        
        if( mirrorQ )
        {
            if( reverseQ )
            {
                using std::swap;
                
                for( Int a = 0; a < max_arc_count; ++a )
                {
                    swap(A_cross(a,Tail),A_cross(a,Head));
                    A_state_ptr[a] = A_state_ptr[a].ReflectReverse();
                }
            }
            else
            {
                for( Int a = 0; a < max_arc_count; ++a )
                {
                    A_state_ptr[a] = A_state_ptr[a].Reflect();
                }
            }
        }
        else
        {
            if( reverseQ )
            {
                using std::swap;
                
                for( Int a = 0; a < max_arc_count; ++a )
                {
                    swap(A_cross(a,Tail),A_cross(a,Head));
                    A_state_ptr[a] = A_state_ptr[a].Reverse();
                }
            }
            else
            {
                // Do nothing;
            }
        }
        
        // A_color remains as it was.
    }

        
        
    public:
        
        void PrintInfo() const
        {
            logprint(MethodName("PrintInfo"));
            
            TOOLS_LOGDUMP( C_arcs );
            TOOLS_LOGDUMP( C_state );
            TOOLS_LOGDUMP( A_cross );
            TOOLS_LOGDUMP( A_state );
            TOOLS_LOGDUMP( A_color );
        }

/*!@brief A coarse estimator of heap-allocated memory in use for this class instance. Does not account for quantities stored in the class' cache.
*/
        Size_T AllocatedByteCount() const
        {
            Size_T byte_count = C_arcs.AllocatedByteCount()
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
        Size_T ByteCount() const
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



