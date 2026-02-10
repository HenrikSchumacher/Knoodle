#pragma  once

//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/boyer_myrvold_planar_test.hpp>

// TODO: Darcs.hpp

// TODO: Remove Arrow_T
// Arcs.hpp
// Color.hpp
// Constructors.hpp
// CreateCompressed.hpp
// Permute.hpp
// CreateUnlink.hpp
// PlanarDiagram2.hpp

// TODO: De-templating?

namespace Knoodle
{
    template<typename Int_, Size_T optimization_level, bool mult_compQ_>
    class ArcSimplifier;
    
    template<typename Int_, bool mult_compQ_> class CrossingSimplifier;
    
    template<typename Int_, bool mult_compQ_> class StrandSimplifier;

    
    // TODO: Enable unsigned integers because they should be about 15% faster in task heavy bit manipulations.
    template<typename Int_ = Int64>
    class PlanarDiagram final : public CachedObject<1,0,0,0>
    {
//        static_assert(SignedIntQ<Int_>,"");
        static_assert(IntQ<Int_>,"");

    public:
        
        static constexpr bool debugQ = false;
        
        using Int     = Int_;
        using UInt    = ToUnsigned<Int>;

        using Base_T  = CachedObject<1,0,0,0>;
        using Class_T = PlanarDiagram<Int>;
        using PD_T    = PlanarDiagram<Int>;
        
        using CrossingContainer_T       = Tiny::MatrixList_AoS<2,2,Int,Int>;
        using CrossingStateContainer_T  = Tensor1<CrossingState_T,Int>;
        
        using C_Arcs_T                  = Tiny::Matrix<2,2,Int,Int>;
        using A_Cross_T                 = Tiny::Vector<2,Int,Int>;
        
        using ArcContainer_T            = Tiny::VectorList_AoS<2,  Int,Int>;
        using ArcStateContainer_T       = Tensor1<ArcState_T,Int>;
        using ArcColorContainer_T       = Tensor1<Int,Int>;
        
        using MultiGraph_T              = MultiGraph<Int,Int>;
        using ComponentMatrix_T         = MultiGraph_T::ComponentMatrix_T;

        using PD_List_T                 = std::vector<PlanarDiagram>;

        template<typename I, Size_T lvl, bool mult_compQ_>
        friend class ArcSimplifier;

        template<typename I, bool mult_compQ_>
        friend class CrossingSimplifier;

        template<typename I, bool mult_compQ_>
        friend class StrandSimplifier;
            
        using HeadTail_T = bool;
        using Arrow_T = std::pair<Int,HeadTail_T>;
        
    public:
        
        
        static constexpr HeadTail_T Tail  = 0;
        static constexpr HeadTail_T Head  = 1;
        
        static constexpr bool Left  = 0;
        static constexpr bool Right = 1;
        static constexpr bool Out   = 0;
        static constexpr bool In    = 1;

        static constexpr bool always_compressQ = true;
        
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
        Int unlink_count        = 0;
        Int max_crossing_count  = 0;
        Int max_arc_count       = 0;
        
        // Exposed to user via Crossings().
        CrossingContainer_T      C_arcs;
        // Exposed to user via CrossingStates().
        CrossingStateContainer_T C_state;
        // Some multi-purpose scratch buffers. Mostly for Traversal.
        mutable Tensor1<Int,Int> C_scratch;
        
        // Exposed to user via Arcs().
        ArcContainer_T           A_cross;
        // Exposed to user via ArcStates().
        ArcStateContainer_T      A_state;
        // Some multi-purpose scratch buffers. Mostly for Traversal.
        mutable Tensor1<Int,Int> A_scratch;
        
        // Counters for Reidemeister moves.
        Int R_I_counter   = 0;
        Int R_Ia_counter  = 0;
        Int R_II_counter  = 0;
        Int R_IIa_counter = 0;
        Int twist_counter = 0;
        Int four_counter  = 0;
        
        bool proven_minimalQ = false;
        
    public:
  
        // Default constructor
        PlanarDiagram() = default;
        // Destructor (virtual because of inheritance)
        virtual ~PlanarDiagram() override = default;
        // Copy constructor
        PlanarDiagram( const PlanarDiagram & other ) = default;
        // Copy assignment operator
        PlanarDiagram & operator=( const PlanarDiagram & other ) = default;
        // Move constructor
        PlanarDiagram( PlanarDiagram && other ) = default;
        // Move assignment operator
        PlanarDiagram & operator=( PlanarDiagram && other ) = default;
 
    private:
        
        /*! @brief This constructor is supposed to only allocate and initialize all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
        template<typename ExtInt>
        PlanarDiagram( const ExtInt max_crossing_count_, const ExtInt unlink_count_ )
//        : crossing_count     { int_cast<Int>(max_crossing_count_)          }
//        , arc_count          { Int(2) * int_cast<Int>(max_crossing_count_) }
        : crossing_count     { Int(0)                                        }
        , arc_count          { Int(0)                                        }
        , unlink_count       { int_cast<Int>(unlink_count_)                  }
        , max_crossing_count { int_cast<Int>(max_crossing_count_)            }
        , max_arc_count      { Int(Int(2) * max_crossing_count)              }
        , C_arcs             { max_crossing_count, Uninitialized             }
        , C_state            { max_crossing_count, CrossingState_T::Inactive }
        , C_scratch          { max_crossing_count                            }
        , A_cross            { max_arc_count,      Uninitialized             }
        , A_state            { max_arc_count,      ArcState_T::Inactive      }
        , A_scratch          { max_arc_count                                 }
        {
            static_assert(IntQ<ExtInt>,"");
        }
        
        
        /*! @brief This constructor is supposed to only allocate all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
        template<typename ExtInt>
        PlanarDiagram( const ExtInt max_crossing_count_, const ExtInt unlink_count_, bool dummy )
        : crossing_count     { Int(0)                                       }
        , arc_count          { Int(0)                                       }
        , unlink_count       { int_cast<Int>(unlink_count_)                 }
        , max_crossing_count { int_cast<Int>(max_crossing_count_)           }
        , max_arc_count      { Int(Int(2) * max_crossing_count)             }
        , C_arcs             { max_crossing_count                           }
        , C_state            { max_crossing_count                           }
        , C_scratch          { max_crossing_count                           }
        , A_cross            { max_arc_count                                }
        , A_state            { max_arc_count                                }
        , A_scratch          { max_arc_count                                }
        {
            (void)dummy;
            static_assert(IntQ<ExtInt>,"");
        }

    public:
        
        template<typename ExtInt, typename ExtInt2, typename ExtInt3>
        PlanarDiagram(
            cptr<ExtInt>  crossings,
            cptr<ExtInt2> crossing_states,
            cptr<ExtInt>  arcs,
            cptr<ExtInt3> arc_states,
            const ExtInt  crossing_count_,
            const ExtInt  unlink_count_,
            const bool    proven_minimalQ_ = false
        )
        :   PlanarDiagram( crossing_count_, unlink_count_, true )
        {
            static_assert(IntQ<ExtInt>,"");
            static_assert(IntQ<ExtInt2>||SameQ<ExtInt2,CrossingState_T>,"");
            static_assert(IntQ<ExtInt3>||SameQ<ExtInt3,ArcState_T>,"");
            
            C_arcs.Read(crossings);
            C_state.Read(crossing_states);
            A_cross.Read(arcs);
            A_state.Read(arc_states);
            proven_minimalQ = proven_minimalQ_;
            
            crossing_count = CountActiveCrossings();
            arc_count      = CountActiveArcs();
        }
        
        /*! @brief Make a copy without copying cache and persistent cache.
         */
        PD_T CachelessCopy() const
        {
            PD_T pd ( max_crossing_count, unlink_count, true ); // Allocate, but do not fill.
            pd.crossing_count  = crossing_count;
            pd.arc_count       = arc_count;
            pd.max_arc_count   = max_arc_count;
            pd.proven_minimalQ = proven_minimalQ;

            pd.C_arcs .Read(C_arcs .data());
            pd.C_state.Read(C_state.data());
            
            pd.A_cross.Read(A_cross.data());
            pd.A_state.Read(A_state.data());
            
            return pd;
        }
        
        /*!@brief Construction from `Knot_2D` object.
         *
         * Caution: This assumes that `Knot_2D::FindIntersections` has been called already!
         */
        
        template<typename Real, typename BReal>
        explicit PlanarDiagram( cref<Knot_2D<Real,Int,BReal>> L )
        :   PlanarDiagram( L.CrossingCount(), Int(0) )
        {
            static_assert(FloatQ<Real>,"");
            static_assert(FloatQ<BReal>,"");
            
            ReadFromLink<Real,BReal>(
                L.ComponentCount(),
                L.ComponentPointers().data(),
                L.EdgePointers().data(),
                L.EdgeIntersections().data(),
                L.EdgeOverQ().data(),
                L.Intersections()
            );
        }
        
        /*!@brief Construction from `Link_2D` object.
         *
         * Caution: This assumes that `Link_2D::FindIntersections` has been called already!
         */
        
        template<typename Real, typename BReal>
        explicit PlanarDiagram( cref<Link_2D<Real,Int,BReal>> L )
        :   PlanarDiagram( L.CrossingCount(), Int(0) )
        {
            static_assert(FloatQ<Real>,"");
            static_assert(FloatQ<BReal>,"");
            
            ReadFromLink<Real,BReal>(
                L.ComponentCount(),
                L.ComponentPointers().data(),
                L.EdgePointers().data(),
                L.EdgeIntersections().data(),
                L.EdgeOverQ().data(),
                L.Intersections()
            );
        }
        
        /*! @brief Construction from coordinates.
         */
        
        template<typename Real, typename ExtInt>
        PlanarDiagram( cptr<Real> x, const ExtInt n )
        {
            static_assert(FloatQ<Real>,"");
            static_assert(IntQ<ExtInt>,"");
            
            Knot_2D<Real,Int,Real> L ( n );

            L.ReadVertexCoordinates ( x );
            
            int err = L.template FindIntersections<true>();
            
            if( err != 0 )
            {
                eprint(ClassName() + "(): FindIntersections reported error code " + ToString(err) + ". Returning empty PlanarDiagram.");
                return;
            }
            
            // Deallocate tree-related data in L to make room for the PlanarDiagram.
            L.DeleteTree();
            
            // We delay the allocation until substantial parts of L have been deallocated.
            *this = PlanarDiagram( L.CrossingCount(), Int(0) );
            
            ReadFromLink<Real,Real>(
                L.ComponentCount(),
                L.ComponentPointers().data(),
                L.EdgePointers().data(),
                L.EdgeIntersections().data(),
                L.EdgeOverQ().data(),
                L.Intersections()
            );
        }
        
        template<typename Real, typename ExtInt>
        PlanarDiagram( cptr<Real> x, cptr<ExtInt> edges, const ExtInt n )
        {
            static_assert(FloatQ<Real>,"");
            static_assert(IntQ<ExtInt>,"");
            
            using Link_T = Link_2D<Real,Int,Real>;
            
            Link_T L ( edges, n );

            L.ReadVertexCoordinates ( x );
            
            int err = L.template FindIntersections<true>();
            
            if( err != 0 )
            {
                eprint(ClassName() + "(): FindIntersections reported error code " + ToString(err) + ". Returning empty PlanarDiagram.");
                return;
            }
            
            // Deallocate tree-related data in L to make room for the PlanarDiagram.
            L.DeleteTree();
            
            // We delay the allocation until substantial parts of L have been deallocated.
            *this = PlanarDiagram( L.CrossingCount(), Int(0) );
            
            ReadFromLink<Real,Real>(
                L.ComponentCount(),
                L.ComponentPointers().data(),
                L.EdgePointers().data(),
                L.EdgeIntersections().data(),
                L.EdgeOverQ().data(),
                L.Intersections()
            );
        }
        
    public:

        /*!
         * @brief Returns the number of trivial unlinks in the diagram, i.e., unknots that do not share any crossings with other link components.
         */
        
        Int UnlinkCount() const
        {
            return unlink_count;
        }

        /*!
         * @brief Returns how many crossings there were in the original planar diagram, before any simplifications.
         */
        
        Int MaxCrossingCount() const
        {
            return max_crossing_count;
        }
        
        /*!
         * @brief The number of crossings in the planar diagram.
         */
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
        
        /*!
         * @brief Returns how many Reidemeister I moves have been performed so far.
         */
        
        Int Reidemeister_I_Counter() const
        {
            return R_I_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister Ia moves have been performed so far.
         */
        
        Int Reidemeister_Ia_Counter() const
        {
            return R_Ia_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister II moves have been performed so far.
         */
        
        Int Reidemeister_II_Counter() const
        {
            return R_II_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister IIa moves have been performed so far.
         */
        
        Int Reidemeister_IIa_Counter() const
        {
            return R_IIa_counter;
        }
        
        /*!
         * @brief Returns how many twist moves have been performed so far.
         *
         * See TwistMove.hpp for details.
         */
        
        Int TwistMove_Counter() const
        {
            return twist_counter;
        }
        
        /*!
         * @brief Returns how many moves that remove 4 crossings at once have been performed so far.
         */
        
        Int FourMove_Counter() const
        {
            return four_counter;
        }
        
        /*!
         * @brief Returns the list of crossings in the internally used storage format as a reference to a `Tensor3` object, which is basically a heap-allocated array of dimension 3.
         *
         * The reference is constant because things can go wild  (segfaults, infinite loops) if we allow the user to mess with this data.
         *
         * The `c`-th crossing is stored in `Crossings()(c,i,j)`, `i = 0,1`, `j = 0,1` as follows; note that we defined Booleans `Out = 0`, `In = 0`, `Left = 0`, `Right = 0` for easing the indexing:
         *
         *    Crossings()(c,0,0)                   Crossings()(c,0,1)
         *            =              O       O             =
         * Crossings()(c,Out,Left)    ^     ^   Crossings()(c,Out,Right)
         *                             \   /
         *                              \ /
         *                               X
         *                              / \
         *                             /   \
         *    Crossings()(c,1,0)      /     \      Crossings()(c,1,1)
         *            =              O       O             =
         *  Crossings()(c,In,Left)               Crossings()(c,In,Right)
         *
         *  Beware that a crossing can have various states, such as `CrossingState_T::LeftHanded`, `CrossingState_T::RightHanded`, or `CrossingState_T::Deactivated`. This information is stored in the corresponding entry of `CrossingStates()`.
         */
        
        cref<CrossingContainer_T> Crossings() const
        {
            return C_arcs;
        }
        
        /*!
         * @brief Returns the states of the crossings.
         *
         *  The states that a crossing can have are:
         *
         *  - `CrossingState_T::RightHanded`
         *  - `CrossingState_T::LeftHanded`
         *  - `CrossingState_T::Inactive`
         *
         * `CrossingState_T::Inactive` means that the crossing has been deactivated by topological manipulations.
         */
        
        cref<CrossingStateContainer_T> CrossingStates() const
        {
            return C_state;
        }
        
    public:
        
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
        
        template<bool verboseQ = true>
        bool EulerCharacteristicValidQ() const
        {
            TOOLS_PTIMER(timer,MethodName("EulerCharacteristicValidQ"));
            const Int euler_char  = EulerCharacteristic();
            const Int euler_char0 = Int(2) * DiagramComponentCount();
            
            const bool validQ = (euler_char == euler_char0);
            
            if constexpr ( verboseQ )
            {
                if( !validQ )
                {
                    wprint(ClassName()+"::EulerCharacteristicValidQ: Computed Euler characteristic is " + ToString(euler_char) + " != 2 * DiagramComponentCount() = " + ToString(euler_char0) + ". The processed diagram cannot be planar.");
                }
            }
            
            return validQ;
        }
        
        
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
        
        
        /*!@ Sets all entries of all deactivated crossings and arcs to `Uninitialized`.
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
        
    public:

#include "PlanarDiagram/ReadFromLink.hpp"
#include "PlanarDiagram/Traverse.hpp"
#include "PlanarDiagram/CreateCompressed.hpp"
#include "PlanarDiagram/Reconnect.hpp"
#include "PlanarDiagram/Checks.hpp"
#include "PlanarDiagram/R_I.hpp"

#include "PlanarDiagram/Crossings.hpp"
#include "PlanarDiagram/Arcs.hpp"
#include "PlanarDiagram/Darcs.hpp"
#include "PlanarDiagram/Faces.hpp"
#include "PlanarDiagram/Certificates.hpp"
#include "PlanarDiagram/LinkComponents.hpp"
#include "PlanarDiagram/DiagramComponents.hpp"
#include "PlanarDiagram/Strands.hpp"
#include "PlanarDiagram/DisconnectSummands.hpp"
        
#include "PlanarDiagram/Simplify1.hpp"
#include "PlanarDiagram/Simplify2.hpp"
#include "PlanarDiagram/Simplify3.hpp"
#include "PlanarDiagram/Simplify4.hpp"
#include "PlanarDiagram/Simplify5.hpp"
#include "PlanarDiagram/Simplify6.hpp"
        
#include "PlanarDiagram/PDCode.hpp"
#include "PlanarDiagram/GaussCode.hpp"
//#include "PlanarDiagram/GaussCode2.hpp"
#include "PlanarDiagram/LongMacLeodCode.hpp"
#include "PlanarDiagram/MacLeodCode.hpp"
        
#include "PlanarDiagram/ResolveCrossing.hpp"
#include "PlanarDiagram/SwitchCrossing.hpp"
        
#include "PlanarDiagram/VerticalSummandQ.hpp"
        
#include "PlanarDiagram/DepthFirstSearch.hpp"
#include "PlanarDiagram/SpanningForest.hpp"
        
#include "PlanarDiagram/Permute.hpp"
//#include "PlanarDiagram/Planarity.hpp"
        
    public:
        
        bool ProvenMinimalQ() const
        {
            return proven_minimalQ;
        }
        
        bool InvalidQ() const
        {
            return (max_crossing_count == Int(0)) && (unlink_count == Int(0));
        }
        
        bool ValidQ() const
        {
            return !InvalidQ();
        }
        
        bool NonTrivialQ()
        {
            return (CrossingCount() > Int(0)) || (UnlinkCount() > Int(1));
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
            TOOLS_LOGDUMP( unlink_count );
        }

/*!@brief A coarse estimator of heap-allocated memory in use for this class instance. Does not account for quantities stored in the class' cache.
*/
        Size_T AllocatedByteCount()
        {
            Size_T byte_count =
                  C_arcs.AllocatedByteCount()
                + C_state.AllocatedByteCount()
                + A_cross.AllocatedByteCount()
                + C_scratch.AllocatedByteCount()
                + A_state.AllocatedByteCount()
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
                + "\t" + TOOLS_MEM_DUMP_STRING(A_cross)
                + "\t" + TOOLS_MEM_DUMP_STRING(C_scratch)
                + "\t" + TOOLS_MEM_DUMP_STRING(A_state)
                + "\t" + TOOLS_MEM_DUMP_STRING(A_scratch);
        }
        
/*!@brief A coarse estimator of memory in use for this class instance. Does not account for quantities stored in the class' cache.
*/
        Size_T ByteCount()
        {
            return sizeof(PlanarDiagram) + AllocatedByteCount();
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
/*!@brief Returns a string that identifies this class with type information. Mostly used for logging and in error messages.
 */
        
        static std::string ClassName()
        {
            return ct_string("PlanarDiagram")
                + "<" + TypeName<Int>
                + ">";
        } 
    };

} // namespace Knoodle



