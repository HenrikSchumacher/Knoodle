#pragma  once//

//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/boyer_myrvold_planar_test.hpp>

namespace Knoodle
{
    // TODO: Template this.
    // bool fancy_arc_stateQ -- whether the flags other than an active bit ought to be used at all.s
    
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
        
        static constexpr bool countQ = false;
        static constexpr bool debugQ = false;
        
        using Int                       = Int_;
        using UInt                      = ToUnsigned<Int>;
        
        using Base_T                    = CachedObject;
        using Class_T                   = PlanarDiagram2<Int>;
        using PD_T                      = PlanarDiagram2<Int>;
        
        using CrossingContainer_T       = Tiny::MatrixList_AoS<2,2,Int,Int>;
        using ArcContainer_T            = Tiny::VectorList_AoS<2,  Int,Int>;
        using ColorCounts_T             = AssociativeContainer<Int,Int>;
        
        using C_Arcs_T                  = Tiny::Matrix<2,2,Int,Int>;
        using A_Cross_T                 = Tiny::Vector<2,Int,Int>;
        
        using CrossingStateContainer_T  = Tensor1<CrossingState_T,Int>;
        using ArcStateContainer_T       = Tensor1<ArcState_T,Int>;
        using ArcColorContainer_T       = Tensor1<Int,Int>;
        
        using MultiGraph_T              = MultiGraph<Int,Int>;
        using ComponentMatrix_T         = MultiGraph_T::ComponentMatrix_T;
        
        friend class PlanarDiagramComplex<Int>;
        
        using PDC_T = PlanarDiagramComplex<Int>;
        
        friend class LoopRemover<Int>;
//        friend class ArcCrawler<Int>;
        friend class ArcSimplifier2<Int,0,true >;
        friend class ArcSimplifier2<Int,1,true >;
        friend class ArcSimplifier2<Int,2,true >;
        friend class ArcSimplifier2<Int,3,true >;
        friend class ArcSimplifier2<Int,4,true >;
//        friend class ArcSimplifier2<Int,0,false>;
//        friend class ArcSimplifier2<Int,1,false>;
//        friend class ArcSimplifier2<Int,2,false>;
//        friend class ArcSimplifier2<Int,3,false>;
//        friend class ArcSimplifier2<Int,4,false>;

        friend class StrandSimplifier2<Int,true >;
        friend class StrandSimplifier2<Int,false>;
            
        using HeadTail_T = bool;
        
        static constexpr HeadTail_T Tail  = 0;
        static constexpr HeadTail_T Head  = 1;
        
        static constexpr bool Left  = 0;
        static constexpr bool Right = 1;
        static constexpr bool Out   = 0;
        static constexpr bool In    = 1;
        
        static constexpr Int Uninitialized = SignedIntQ<Int> ? Int(-1): std::numeric_limits<Int>::max();
        
        static constexpr Int DoNotVisit = Uninitialized - Int(1);
        
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
        
        mutable Int last_color_deactivated = Uninitialized;
        bool proven_minimalQ = false;
        
        mutable Int c_search_ptr = 0;
        mutable Int a_search_ptr = 0;
        
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
        , C_state            { max_crossing_count, CrossingState_T::Inactive }
        , C_scratch          { max_crossing_count                              }
        , A_cross            { max_arc_count,      Uninitialized               }
        , A_state            { max_arc_count,      ArcState_T::Inactive        }
        , A_color            { max_arc_count,      Uninitialized               }
        , A_scratch          { max_arc_count                                   }
        {
            // needs to know all member variables
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
            // needs to know all member variables
            (void)dummy;
            static_assert(IntQ<ExtInt>,"");
        }

    public:
        
        /*!@brief Construct PlanarDiagram2 from internal data.
         */
        
        template<typename ExtInt, typename ExtInt2, typename ExtInt3, typename ExtInt4, typename ExtInt5>
        PlanarDiagram2(
            const ExtInt  crossing_count_,
            cptr<ExtInt>  crossings,
            cptr<ExtInt2> crossing_states,
            cptr<ExtInt>  arcs,
            cptr<ExtInt3> arc_states,
            cptr<ExtInt4> arc_colors,
            const ExtInt5 last_color_deactivated_,
            const bool proven_minimalQ_ = false,
            const bool compressQ = false
        )
        :   PlanarDiagram2( crossing_count_, true ) // Allocate, but do not fill.
        {
            // needs to know all member variables
            
            static_assert(IntQ<ExtInt>,"");
            static_assert(IntQ<ExtInt2>||SameQ<ExtInt2,CrossingState_T>,"");
            static_assert(IntQ<ExtInt3>||SameQ<ExtInt3,ArcState_T>,"");
            static_assert(IntQ<ExtInt4>,"");
            static_assert(IntQ<ExtInt5>,"");
            
            last_color_deactivated = int_cast<Int>(last_color_deactivated_);
            proven_minimalQ = proven_minimalQ_;
            
            if( max_crossing_count == Int(0) )
            {
                if( proven_minimalQ && (last_color_deactivated != Uninitialized) )
                {
                    *this = Unknot(last_color_deactivated);
                    return;
                }
                else
                {
                    *this = InvalidDiagram();
                    return;
                }
            }
            
            C_arcs .Read(crossings);
            C_state.Read(crossing_states);
            A_cross.Read(arcs);
            A_state.Read(arc_states);
            A_color.Read(arc_colors);
            
            crossing_count       = CountActiveCrossings();
            arc_count            = CountActiveArcs();
            

            if( (crossing_count == 0)
                && proven_minimalQ
                && (last_color_deactivated != Uninitialized)
            )
            {
                *this = Unknot(last_color_deactivated);
                return;
            }
            
            if( compressQ )
            {
                this->template Compress<true>();
            }
        }
        
        /*!@brief Construct PlanarDiagram2 from internal data withput colors.
         */
        
        template<typename ExtInt, typename ExtInt2, typename ExtInt3>
        PlanarDiagram2(
            const ExtInt  crossing_count_,
            cptr<ExtInt>  crossings,
            cptr<ExtInt2> crossing_states,
            cptr<ExtInt>  arcs,
            cptr<ExtInt3> arc_states,
            const bool proven_minimalQ_ = false,
            const bool compressQ = false
        )
        :   PlanarDiagram2( crossing_count_, true ) // Allocate, but do not fill.
        {
            // needs to know all member variables
            
            static_assert(IntQ<ExtInt>,"");
            static_assert(IntQ<ExtInt2>||SameQ<ExtInt2,CrossingState_T>,"");
            static_assert(IntQ<ExtInt3>||SameQ<ExtInt3,ArcState_T>,"");
            
            proven_minimalQ = proven_minimalQ_;
            
            if( max_crossing_count == Int(0) )
            {
                if( proven_minimalQ )
                {
                    eprint(ClassName()+"(): input identifies this as an unknot, but the color information is missing. Use a different constructor.");
                    *this = InvalidDiagram();
                    return;
                }
                else
                {
                    *this = InvalidDiagram();
                    return;
                }
            }
            
            C_arcs .Read(crossings);
            C_state.Read(crossing_states);
            A_cross.Read(arcs);
            A_state.Read(arc_states);

            crossing_count       = CountActiveCrossings();
            arc_count            = CountActiveArcs();
            
            TOOLS_DUMP(crossing_count);
            TOOLS_DUMP(arc_count);
            
            if( (crossing_count == 0) && proven_minimalQ )
            {
                eprint(ClassName()+"(): input identifies this as an unknot, but the color information is missing. Use a different constructor.");
                *this = InvalidDiagram();
                return;
            }
            
            if( compressQ )
            {
                this->template Compress<true>();
            }
            else
            {
                ComputeArcColors();
            }
        }
        
        /*! @brief Create a new planar diagram with the copied of this one, but with the internal buffers large enough for at least `new_max_crossing_count` crossings. Caches will be cleared.
         */
        
        PD_T CreateEnlarged( const Int new_max_crossing_count ) const
        {
            // needs to know all member variables
            
            PD_T pd ( new_max_crossing_count, true );  // Allocate, but do not fill.
            
            pd.crossing_count = crossing_count;
            pd.arc_count      = arc_count;
            
            const Int n       = max_crossing_count;
            const Int n_new   = Max(n,pd.max_crossing_count);
            
            const Int m       = max_arc_count;
            const Int m_new   = Max(m,pd.max_arc_count);
            
            C_arcs.Write( pd.C_arcs.data()   );
            fill_buffer( pd.C_arcs.data(n) , Uninitialized            , Int(4) * (n_new - n) );
            
            C_state.Write( pd.C_state.data() );
            fill_buffer( pd.C_state.data(n), CrossingState_T::Inactive, Int(1) * (n_new - n) );
            
            A_cross.Write( pd.A_cross.data() );
            fill_buffer( pd.A_cross.data(m), Uninitialized            , Int(2) * (m_new - m) );
            
            A_state.Write( pd.A_state.data() );
            fill_buffer( pd.A_state.data(m), ArcState_T::Inactive     , Int(1) * (m_new - m) );
            
            A_color.Write( pd.A_color.data() );
            fill_buffer( pd.A_color.data(m), Uninitialized            , Int(1) * (m_new - m) );
            
            pd.last_color_deactivated = last_color_deactivated;
            pd.proven_minimalQ = proven_minimalQ;
            
            // This guarantees that we will find free space very quickly.
            pd.c_search_ptr = max_crossing_count;
            pd.a_search_ptr = max_arc_count;
            
            return pd;
        }
        
        /*! @brief Resize internal buffers to make room for at least `new_max_crossing_count` crossings. Caches will be cleared.
         */
        void RequireCrossingCount( const Int new_max_crossing_count, bool doubleQ = true )
        {
            if( max_crossing_count < new_max_crossing_count )
            {
                (*this) = CreateEnlarged( (doubleQ ? Int(2) : Int(1) ) * new_max_crossing_count );
            }
        }
        
        
        /*! @brief Make a copy without copying cache and persistent cache.
         */
        PD_T CachelessCopy() const
        {
            return CreateEnlarged( max_crossing_count );
        }
        
    public:
        
#include "PlanarDiagram2/FromEmbeddings.hpp"
        
#include "PlanarDiagram2/Crossings.hpp"
#include "PlanarDiagram2/Arcs.hpp"
#include "PlanarDiagram2/Darcs.hpp"
#include "PlanarDiagram2/Checks.hpp"
        
#include "PlanarDiagram2/Traverse.hpp"
#include "PlanarDiagram2/LinkComponents.hpp"
#include "PlanarDiagram2/Color.hpp"
#include "PlanarDiagram2/DiagramComponents.hpp"
#include "PlanarDiagram2/StandardDiagrams.hpp"

#include "PlanarDiagram2/Compress.hpp"
#include "PlanarDiagram2/Reconnect.hpp"
#include "PlanarDiagram2/SwitchCrossing.hpp"
#include "PlanarDiagram2/Modify.hpp"

#include "PlanarDiagram2/PDCode.hpp"
#include "PlanarDiagram2/GaussCode.hpp"
#include "PlanarDiagram2/LongMacLeodCode.hpp"
#include "PlanarDiagram2/MacLeodCode.hpp"
        
//#include "PlanarDiagram2/R_I.hpp"

#include "PlanarDiagram2/Faces.hpp"
#include "PlanarDiagram2/Certificates.hpp"
        
//#include "PlanarDiagram2/DiagramComponents.hpp"
#include "PlanarDiagram2/Strands.hpp"
//#include "PlanarDiagram2/DisconnectSummands.hpp"
        
//#include "PlanarDiagram2/Simplify1.hpp"
//#include "PlanarDiagram2/Simplify2.hpp"
//#include "PlanarDiagram2/Simplify3.hpp"
//#include "PlanarDiagram2/Simplify4.hpp"
//#include "PlanarDiagram2/Simplify5.hpp"
//#include "PlanarDiagram2/Simplify6.hpp"


//#include "PlanarDiagram2/ResolveCrossing.hpp"
//#include "PlanarDiagram2/VerticalSummandQ.hpp"
        
#include "PlanarDiagram2/DepthFirstSearch.hpp"
//#include "PlanarDiagram2/SpanningForest.hpp"
        

#include "PlanarDiagram2/Permute.hpp"
//#include "PlanarDiagram2/Planarity.hpp"
        
    public:
        
        Int LastColorDeactivated() const
        {
            return last_color_deactivated;
        }
        
        bool ProvenMinimalQ() const
        {
            return proven_minimalQ;
        }

        
        static PD_T InvalidDiagram()
        {
            return PD_T();
        }
        
        
        
        bool ProvenUnknotQ() const
        {
            return proven_minimalQ && (crossing_count == Int(0)) && ValidIndexQ(last_color_deactivated);
        }

        bool ProvenHopfLinkQ() const
        {
            return proven_minimalQ && (crossing_count == Int(2)) && (LinkComponentCount() == Int(2));
        }
        
        bool ProvenTrefoilQ() const
        {
            return proven_minimalQ && (crossing_count == Int(3)) && (LinkComponentCount() == Int(1));
        }
        
        bool ProvenFigureEightQ() const
        {
            return proven_minimalQ && (crossing_count == Int(4)) && (LinkComponentCount() == Int(1));
        }

        bool InvalidQ() const
        {
            return (crossing_count == Int(0)) && !ValidIndexQ(last_color_deactivated);
        }
        
        bool ValidQ() const
        {
            return !InvalidQ();
        }
        

        Int NextInactiveCrossing() const
        {
            if( crossing_count >= max_crossing_count )
            {
                return Uninitialized;
            }
            
            while( CrossingActiveQ(c_search_ptr) )
            {
                ++c_search_ptr;
                
                if( c_search_ptr >= max_crossing_count ) { c_search_ptr = 0; }
            }
            
            return c_search_ptr;
        }
        
        Int NextInactiveArc() const
        {
            if( arc_count >= max_arc_count )
            {
                return Uninitialized;
            }
            
            while( ArcActiveQ(a_search_ptr) )
            {
                ++a_search_ptr;
                
                if( a_search_ptr >= max_arc_count ) { a_search_ptr = 0; }
            }
            
            return a_search_ptr;
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

//        template<bool verboseQ = true>
//        bool EulerCharacteristicValidQ() const
//        {
//            TOOLS_PTIMER(timer,MethodName("EulerCharacteristicValidQ"));
//            const Int euler_char  = EulerCharacteristic();
//            const Int euler_char0 = Int(2) * DiagramComponentCount();
//
//            const bool validQ = (euler_char == euler_char0);
//
//            if constexpr ( verboseQ )
//            {
//                if( !validQ )
//                {
//                    wprint(ClassName()+"::EulerCharacteristicValidQ: Computed Euler characteristic is " + ToString(euler_char) + " != 2 * DiagramComponentCount() = " + ToString(euler_char0) + ". The processed diagram cannot be planar.");
//                }
//            }
//
//            return validQ;
//        }
        

        
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
            
            for( Int c = 0; c < max_crossing_count; ++c )
            {
                const C_Arcs_T C = CopyCrossing(c);
                C_arcs(c,0,0) = C[i0][j0];
                C_arcs(c,0,1) = C[i0][j1];
                C_arcs(c,1,0) = C[i1][j0];
                C_arcs(c,1,1) = C[i1][j1];
            }
            
            if( mirrorQ )
            {
                for( Int c = 0; c < max_crossing_count; ++c )
                {
                    C_state(c) = Switch(C_state(c));
                }
            }
            
            if( reverseQ )
            {
                using std::swap;
                
                for( Int a = 0; a < max_arc_count; ++a )
                {
                    swap(A_cross(a,Tail),A_cross(a,Head));
                }
            }
        }
        
    public:
        
        void PrintInfo() const
        {
            logprint(MethodName("PrintInfo") + " -- begin");
            
            TOOLS_LOGDUMP( C_arcs );
            TOOLS_LOGDUMP( C_state );
            TOOLS_LOGDUMP( A_cross );
            TOOLS_LOGDUMP( A_state );
            TOOLS_LOGDUMP( A_color );
            
            TOOLS_LOGDUMP( last_color_deactivated );
            TOOLS_LOGDUMP( proven_minimalQ );
            
            TOOLS_LOGDUMP( this->CacheKeys() );
            
            logprint(MethodName("PrintInfo") + " -- end");
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



