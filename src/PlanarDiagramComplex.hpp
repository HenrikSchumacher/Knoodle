#pragma  once

namespace Knoodle
{
    /*!@brief A class for storing and manipulating several planar diagrams. Its `Simplify` routine attempts to compute a prime link decomposition. Edge colors are used to track how the links have to be glued back to one connected link diagram.
     *
     * If no color information is submitted, the constructors will color the arcs automatically in a consistent way: each arc gets the color of its link component, and each link components in each diagram gets its own color that is different from all others'.
     *
     * Some public class methods are marked as **UNSAFE** because they may alter the topological class of the represented link. For users who want to modify a `PlanarDiagramComplex` nonetheless, for we provide a locking mechanism: Per default every new created diagram is _locked_. The state of the lock can be queried with `LockedQ()`. The user may use `Unlock()` to unlock and `Lock()` to lock again. Alternatively, a RAII-stylescoped lock can be obtain by initializing a `ScopedUnlock` object:
     *
     *     ScopedUnlock unlocker (pdc);
     *
     * The lock will remain active for the life time of `unlocker` (unless it is altered by `Lock`/`Unlock` or by another instance `ScopedLock`. When destructed, `unlocker` reset the lock state of `pdc` to the state it had when `unlocker` was created.
     *
     * @tparam Int_ Integral type used for all sorts of indices. Needs to be big enough to store the total number of arcs and then some. Best to give it 3-4 extra bits.
     */
    
    template<IntQ Int_ = Int64>
    class PlanarDiagramComplex final : public CachedObject<1,0,0,0>
    {

    public:

        static constexpr bool debugQ = false;
        
        /*!@brief Type used for indices of crossings and arcs.*/
        using Int                   = Int_;
        /*!@brief The unsigned type corresponding to `Int`.*/
        using UInt                  = ToUnsigned<Int>;
        
        using Base_T                = CachedObject<1,0,0,0>;
        using Class_T               = PlanarDiagramComplex<Int>;
        /*!@brief Alias for `PlanarDiagram`.*/
        using PD_T                  = PlanarDiagram<Int>;
        /*!@brief Alias for `PlanarDiagramComplex`.*/
        using PDC_T                 = PlanarDiagramComplex<Int>;
        using PD_List_T             = std::vector<PD_T>;
        
        using C_Arcs_T              = PD_T::C_Arcs_T;
        using A_Cross_T             = PD_T::A_Cross_T;
        /*!@brief Alias for `PD_T::ArcContainer_T`.*/
        using ArcContainer_T        = PD_T::ArcContainer_T;
        using ColorCounts_T         = PD_T::ColorCounts_T;
        
        using PassSimplifier_T      = PassSimplifier<Int>;
        using Dijkstra_T            = PassSimplifier_T::Dijkstra_T;
        
        /*!@brief Alias for `OrthoDraw`.*/
        using OrthoDraw_T           = OrthoDraw<PD_T>;
        using OrthoDrawSettings_T   = OrthoDraw_T::Settings_T;
        using Compaction_T          = OrthoDraw_T::CompactionMethod_T;
        /*!@brief Alias for `Reapr`.*/
        using Reapr_T               = Reapr<double,Int,float>;
        using Energy_T              = Reapr_T::Energy_T;
        using ReaprSettings_T       = Reapr_T::Settings_T;
        /*!@brief Alias for `LinkEmbedding`.*/
        using LinkEmbedding_T       = Reapr_T::LinkEmbedding_T;
        
        
        using PDCode_TArgs_T        = PD_T::PDCode_TArgs_T;
        using FromPDCode_TArgs_T    = PD_T::FromPDCode_TArgs_T;
        
        
//        using LinkEmbedding_T       = LinkEmbedding<double,Int,float>;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        
        static constexpr Int  DoNotVisit    =  PD_T::DoNotVisit;
        static constexpr Int  Uninitialized =  PD_T::Uninitialized;
        
        friend class LoopRemover<Int>;
//        friend class ArcCrawler<Int>;
        friend class ArcSimplifier<Int,0,true >;
        friend class ArcSimplifier<Int,1,true >;
        friend class ArcSimplifier<Int,2,true >;
        friend class ArcSimplifier<Int,3,true >;
        friend class ArcSimplifier<Int,4,true >;
        
        friend class PassSimplifier<Int>;
        
    private:
        
        // Class data members
        mutable PD_List_T pd_list;
        mutable PD_List_T pd_todo;
        mutable PD_List_T pd_done;

        PD_T invalid_diagram { PD_T::InvalidDiagram() };
        
        bool lockedQ = true;
        
    public:
  
        // Default constructor
        PlanarDiagramComplex() = default;
        // Destructor (virtual because of inheritance)
        virtual ~PlanarDiagramComplex() override = default;
        // Copy constructor
        PlanarDiagramComplex( const PlanarDiagramComplex & other ) = default;
        // Copy assignment operator
        PlanarDiagramComplex & operator=( const PlanarDiagramComplex & other ) = default;
        // Move constructor
        PlanarDiagramComplex( PlanarDiagramComplex && other ) = default;
        // Move assignment operator
        PlanarDiagramComplex & operator=( PlanarDiagramComplex && other ) = default;
 
        /*!@brief Initialize from a `PlanarDiagram` and a list of unlink colors, taking ownership.*/
        PlanarDiagramComplex( PD_T && pd, Tensor1<Int,Int> && unlink_colors )
        {
            TOOLS_PTIMER(timer,ClassName()+"()");
            const bool validQ = pd.ValidQ();
            
            const Int unlink_count = unlink_colors.Size();
            
            pd_list.reserve( ToSize_T(unlink_count) + Size_T(validQ) );
            pd_done.reserve( ToSize_T(unlink_count) + Size_T(validQ) );

            PushDiagramDone( std::move(pd) );
            
            for( Int unlink = 0; unlink < unlink_count; ++unlink )
            {
                CreateUnlink( unlink_colors[unlink] );
            }
            
            swap(pd_done,pd_list);
        }
        
    
        /*!@brief Initialize from a `PlanarDiagram` and a list of unlink colors, taking ownership.*/
        explicit PlanarDiagramComplex( std::pair<PD_T,Tensor1<Int,Int>> && pd_and_unlink_colors )
        :   PlanarDiagramComplex(
                std::move(pd_and_unlink_colors.first), std::move(pd_and_unlink_colors.second)
            )
        {}
        
        /*!@brief Initialize from a `PlanarDiagram`, taking ownership.*/
        explicit PlanarDiagramComplex( PD_T && pd )
        :   PlanarDiagramComplex( std::move(pd), Tensor1<Int,Int>() )
        {}
        
        /*!@brief Initialize from a `PlanarDiagram`, copying it.*/
        explicit PlanarDiagramComplex( const PD_T & pd )
        :   PlanarDiagramComplex( PD_T(pd), Tensor1<Int,Int>() )
        {}
        
        /*!@brief Initialize from a `LinkEmbedding`, taking ownership.*/
        template<typename Real, typename BReal>
        explicit PlanarDiagramComplex( LinkEmbedding<Real,Int,BReal> && L )
        :   PlanarDiagramComplex( PD_T::FromLinkEmbedding(L) )
        {}
        
        /*!@brief Initialize from a `LinkEmbedding`.*/
        template<typename Real, typename BReal>
        explicit PlanarDiagramComplex( LinkEmbedding<Real,Int,BReal> & L )
        :   PlanarDiagramComplex( PD_T::FromLinkEmbedding(L) )
        {}
        
        /*!@brief Initialize from a `LinkEmbedding_Int`, taking ownership.*/
        template<typename Real, typename Prosector_T>
        explicit PlanarDiagramComplex( LinkEmbedding_Int<Real,Prosector_T> && L )
        :   PlanarDiagramComplex( PD_T::FromLinkEmbedding(L) )
        {}
        
        /*!@brief Initialize from a `LinkEmbedding_Int`.*/
        template<typename Real, typename Prosector_T>
        explicit PlanarDiagramComplex( LinkEmbedding_Int<Real,Prosector_T> & L )
        :   PlanarDiagramComplex( PD_T::FromLinkEmbedding(L) )
        {}
        
        /*!@brief Initialize from a `KnotEmbedding`, taking ownership.*/
        template<typename Real, typename BReal>
        explicit PlanarDiagramComplex( KnotEmbedding<Real,Int,BReal> && K  )
        :   PlanarDiagramComplex( PD_T::FromKnotEmbedding(K) )
        {}
        
        /*!@brief Initialize from a `KnotEmbedding`.*/
        template<typename Real, typename BReal>
        explicit PlanarDiagramComplex( KnotEmbedding<Real,Int,BReal> & K )
        :   PlanarDiagramComplex( PD_T::FromKnotEmbedding(K) )
        {}
        
#include "PlanarDiagramComplex/Constructors.hpp"
#include "PlanarDiagramComplex/Color.hpp"
#include "PlanarDiagramComplex/RemoveLoops.hpp"
#include "PlanarDiagramComplex/SimplifyLocal.hpp"
#include "PlanarDiagramComplex/Split.hpp"
#include "PlanarDiagramComplex/Disconnect.hpp"
#include "PlanarDiagramComplex/Canonicalize.hpp"
#include "PlanarDiagramComplex/Simplify.hpp"
#include "PlanarDiagramComplex/Rerouting_Experimental.hpp"
//#include "PlanarDiagramComplex/SimplifyLocal2.hpp" // Only for development and debugging.
#include "PlanarDiagramComplex/LinkingNumber.hpp"
#include "PlanarDiagramComplex/ModifyDiagramList.hpp"
#include "PlanarDiagramComplex/ModifyDiagram.hpp"
#include "PlanarDiagramComplex/Unite.hpp"
#include "PlanarDiagramComplex/Checks.hpp"
#include "PlanarDiagramComplex/Connect.hpp"
#include "PlanarDiagramComplex/Subcomplex.hpp"
        
#include "PlanarDiagramComplex/ToFile.hpp"
#include "PlanarDiagramComplex/FromFile.hpp"
#include "PlanarDiagramComplex/PDCode.hpp"
#include "PlanarDiagramComplex/JenkinsCode.hpp"
        
#include "PlanarDiagramComplex/CountTrefoils.hpp"
        
        
    public:
        
        /*!@brief Return the number of diagram in the internal list of `PlanarDiagram`s. Beware, this counts also invalid diagrams and unlinks.*/
        Int DiagramCount() const
        {
            return int_cast<Int>(pd_list.size());
        }
        
        /*!@brief Expose the `i`-th diagram in the internal list of `PlanarDiagram`s. Read-only.*/
        cref<PD_T> Diagram( Int i ) const
        {
            if( i < Int(0) )
            {
                eprint(MethodName("Diagram") + ": Index  i < 0. Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            if( i >= DiagramCount() )
            {
                eprint(MethodName("Diagram") + ": Index  i = " +ToString(i) + " is greater equal DiagramCount() = " + ToString(DiagramCount()) + " . Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            return pd_list[Size_T(i)];
        }
        
        /*!@brief Expose the `i`-th diagram in the internal list of `PlanarDiagram`s. Read-only.*/
        cref<PD_T> operator[]( Int i ) const
        {
            return Diagram(i);
        }
        
        /*!@brief Expose the last diagram in the list of `PlanarDiagram`s. Read-only.*/
        cref<PD_T> LastDiagram() const
        {
            if( !pd_list.empty() )
            {
                return pd_list.back();
            }
            else
            {
                eprint(MethodName("LastDiagram") + ": List of diagrams is empty. Returning invalid diagram.");
                
                return invalid_diagram;
            }
        }
        
        /*!@brief Expose the internal list of `PlanarDiagram`s. Read-only.*/
        cref<PD_List_T> Diagrams() const
        {
            return pd_list;
        }
        
        /*!@brief Return true if at least one diagram in the internal list of `PlanarDiagram`s is valid.*/
        bool ValidQ() const
        {
            bool contains_validQ = false;
            
            for( PD_T & pd : pd_list )
            {
                contains_validQ = contains_validQ || pd.ValidQ();
            }
            
            return (DiagramCount() > Int(0)) && contains_validQ;
        }
        
        
        /*!@brief Return true there are no valid diagrams in the internal list of  `PlanarDiagram`s.*/
        bool InvalidQ() const
        {
            return !ValidQ();
        }
        
        /*!@brief Compress all diagrams in the internal list of `PlanarDiagram`s.*/
        void Compress()
        {
            for( PD_T & pd : pd_list ) { pd.Compress(); }
        }
        
        /*!@brief Compress all diagrams in the internal list of `PlanarDiagram`s.*/
        void CompressDiagrams()
        {
            TOOLS_PTIMER(timer,MethodName("CompressDiagrams"));

            Compress();
        }
        
        /*!@brief Clear the caches of all diagrams in the internal list of  `PlanarDiagram`s.*/
        void ClearCaches()
        {
            for( PD_T & pd : pd_list )
            {
                pd.ClearCache();
            }
            this->ClearCache();
        }
        
        /*!@brief Convert the complex to a single PlanarDiagram in which color information still persists, but is irrelevant for the topology. Also, anelli are transformed to farfalle in this process because a PlanarDiagram cannot represent diagrams that contain proper subdiagrams that are anelli. Use this to convert a PlanarDiagramComplex to a format that can be understood by other packages, e.g., Regina or SnapPea.
         */
        
        PD_T ToSingleDiagram() const
        {
            if( InvalidQ() ) { return PD_T::InvalidDiagram(); }
            
            // We need to split first, otherwise, Connect() is not guaranteed to work.
            PDC_T PDC = this->Splitting();
            PDC.AnelliToFarfalle();
            PDC.Connect();
            
            if( PDC.DiagramCount() > Int(1) )
            {
                eprint(MethodName("ToSingleDiagram") + ": Merged complex contains more than one diagram. Something must have gone wrong. Returning invalid complex.");
                
                return PD_T();
            }
            
            return std::move(PDC.pd_list[0]);
        }
        
        /*!@brief Compute the writhe = number of right-handed crossings - number of left-handed crossings. */

        ToSigned<Int> Writhe() const
        {
            ToSigned<Int> writhe = 0;
            
            for( PD_T & pd : pd_list ) { writhe += pd.Writhe(); }
            
            return writhe;
        }
        
    private:
        
        mref<PD_T> Diagram_Private( Int i )
        {
            if( i < Int(0) )
            {
                eprint(MethodName("Diagram") + ": Index  i < 0. Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            if( i >= DiagramCount() )
            {
                eprint(MethodName("Diagram") + ": Index i = " +ToString(i) + " is greater equal DiagramCount() = " + ToString(DiagramCount()) + ". Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            return pd_list[Size_T(i)];
        }
        
    public:
        
        /*!@brief Return the total number of crossings of all diagram in the internal list of `PlanarDiagrams`. */
        Int TotalCrossingCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.CrossingCount(); }
            return count;
        }
        
        /*!@brief Return the total number of crossings of all diagram in the internal list of `PlanarDiagrams`. */
        Int CrossingCount() const
        {
            return TotalCrossingCount();
        }
        
        /*!@brief Return the highest number of crossings among all diagram in the internal list of `PlanarDiagrams`. */
        Int HighestCrossingCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count = Max(count,pd.CrossingCount()); }
            return count;
        }
        
        Int TotalMaxCrossingCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.MaxCrossingCount(); }
            return count;
        }
        
        Int MaxMaxCrossingCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count = Max(count,pd.MaxCrossingCount()); }
            return count;
        }
        
        
        /*!@brief Return the total number of arcs of all diagram in the internal list of `PlanarDiagrams`. */
        Int TotalArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.ArcCount(); }
            return count;
        }
        
        /*!@brief Return the total number of arcs of all diagram in the internal list of `PlanarDiagrams`. */
        Int ArcCount() const
        {
            return TotalArcCount();
        }

        /*!@brief Return the highest number of arcs among all diagram in the internal list of `PlanarDiagrams`. */
        Int HighestArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count = Max(count,pd.ArcCount()); }
            return count;
        }
        
        Int MaxMaxArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count = Max(count,pd.MaxArcCount()); }
            return count;
        }
        
        Int TotalMaxArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.MaxArcCount(); }
            return count;
        }
        
        
    private:

        // We must be careful not to push to pd_list, because we may otherwise invalidate references to elements in pd_list; this would bork the simplification loops.
        void CreateUnlink( const Int color )
        {
            PD_TIMER(timer,MethodName("CreateUnlink"));
            PD_VALPRINT("color",color);
            pd_done.push_back( PD_T::Unknot(color) );
        }
        
        void CreateUnlinkFromArc( PD_T & pd, const Int a )
        {
            PD_TIMER(timer,MethodName("CreateUnlinkFromArc"));
            PD_ASSERT( pd.ValidQ() );
            pd.template AssertArc<0>(a);
            CreateUnlink(pd.A_color[a]) ;
        }
        
        /*!@brief Create a new Hopf link `in pd_list-new`.
         *
         * @param pd The diagram we are working with.
         *
         * @param a_0 First edge whose color we use. It is assumed to belong to diagram `pd` and to be deactivated.
         *
         * @param a_1 Second edge whose color we use. It is assumed to belong to diagram `pd` and to be deactivated.
         */
        void CreateHopfLinkFromArcs(
            PD_T & pd, const Int a_0, const Int a_1, const CrossingState_T handedness
        )
        {
            PD_TIMER(timer,MethodName("CreateHopfLinkFromArcs"));
            PD_VALPRINT("color_0",pd.A_color[a_0]);
            PD_VALPRINT("color_1",pd.A_color[a_1]);
            PD_VALPRINT("handedness",ToString(handedness));
            pd.template AssertArc<0>(a_0);
            pd.template AssertArc<0>(a_1);
            pd_done.push_back( PD_T::HopfLink(pd.A_color[a_0],pd.A_color[a_1],handedness) );
        }
        
        
        /*!@brief Create a new trefoil knot `in pd_list-new`.
         *
         * @param pd The diagram we are working with.
         *
         * @param a Edge whose color we use. It is assumed to belong to diagram `pd` and to be deactivated.
         *
         * @param handedness The handedness of the trefoil to create.
         */
        void CreateTrefoilKnotFromArc( PD_T & pd, const Int a, const CrossingState_T handedness )
        {
            PD_TIMER(timer,MethodName("CreateTrefoilKnotFromArc"));
            PD_VALPRINT("color",pd.A_color[a]);
            PD_VALPRINT("handedness",ToString(handedness));
            pd.template AssertArc<0>(a);
            pd_done.push_back( PD_T::TrefoilKnot(pd.A_color[a],handedness) );
        }
        
        /*!@brief Create a new figure-eight knot `in pd_list-new`.
         *
         * @param pd The diagram we are working with.
         *
         * @param a Edge whose color we use. It is assumed to belong to diagram `pd` and to be deactivated.
         */
        void CreateFigureEightKnotFromArc( PD_T & pd, const Int a )
        {
            PD_TIMER(timer,MethodName("CreateFigureEightKnotFromArc"));
            PD_VALPRINT("color",pd.A_color[a]);
            pd.template AssertArc<0>(a);
            pd_done.push_back( PD_T::FigureEightKnot(pd.A_color[a]) );
        }
        
    private:
        
        /*!@brief **UNSAFE.** Push a new diagram to the internal list.
         */
        void PushDiagram( PD_T && pd )
        {
            if( pd.ValidQ() )
            {
                pd_list.push_back( std::move(pd) );
            }
            else
            {
#ifdef PD_DEBUG
                wprint(MethodName("PushDiagram")+": Tried to push an invalid diagram to pd_list. Doing nothing.");
#endif
            }
        }
        
        void PushDiagramDone( PD_T && pd )
        {
            if( pd.ValidQ() )
            {
                // It is maybe a good idea to compress here so save memory.
                if( pd.crossing_count < pd.max_crossing_count ) { pd.Compress(); }
                
                pd_done.push_back( std::move(pd) );
            }
            else
            {
#ifdef PD_DEBUG
                wprint(MethodName("PushDiagramDone")+": Tried to push an invalid diagram to pd_done. Doing nothing.");
#endif
            }
        }
        
        void PushDiagramToDo( PD_T && pd )
        {
            if( pd.ValidQ() )
            {
                pd_todo.push_back( std::move(pd) );
            }
            else
            {
#ifdef PD_DEBUG
                wprint(MethodName("PushDiagramTodo")+": Tried to push an invalid diagram to pd_todo. Doing nothing.");
#endif
            }
        }
        
    public:

        static Int ToDarc( const Int a, const bool d )
        {
            return PD_T::ToDarc(a,d);
        }
        
        static std::pair<Int,bool> FromDarc( const Int da )
        {
            return PD_T::FromDarc(da);
        }
        
        static Int ArcOfDarc( const Int da )
        {
            return PD_T::ArcOfDarc(da);
        }
        
        static Int ReverseDarc( const Int da )
        {
            return PD_T::ReverseDarc(da);
        }
        
        
        
        void SortByCrossingCount()
        {
            if( DiagramCount() <= Int(0) ) { return; }
            
            std::sort(
                pd_list.begin(),
                pd_list.end(),
                []( cref<PD_T> pd_0, cref<PD_T> pd_1 )
                {
                    return pd_0.CrossingCount() > pd_1.CrossingCount();
                }
            );
            
            this->ClearCache();
        }
        
    
        Size_T RemoveLoopArcs()
        {
            TOOLS_PTIMER(timer,MethodName("RemoveLoopArcs"));
            
            Size_T total_counter = 0;
            
            PD_ASSERT(pd_done.empty());
            PD_ASSERT(pd_todo.empty());
            
            using std::swap;
            pd_done.reserve(pd_list.size());
            pd_todo.reserve(pd_list.size());
          
            swap(pd_list,pd_todo);
            
            while( !pd_todo.empty() )
            {
                PD_T pd = std::move(pd_todo.back());
                pd_todo.pop_back();
                
                if( pd.InvalidQ() ) { continue; }
                
                if(  pd.ProvenMinimalQ() )
                {
                    if( pd.ArcCount() < pd.MaxArcCount() )
                    {
                        PushDiagramDone( pd.CreateCompressed() );
                    }
                    else
                    {
                        pd.ClearCache();
                        PushDiagramDone( std::move(pd) );
                    }
                    continue;
                }
                
                Size_T old_counter = 0;
                Size_T counter = 0;
                
                do
                {
                    old_counter = counter;
                    
                    for( Int a = 0; a < pd.MaxArcCount(); ++a )
                    {
                        if( pd.ArcActiveQ(a) )
                        {
                            LoopRemover<Int> R (*this,pd,a);
                            while( R.Step() ) { ++counter; }
                        }
                    }
                }
                while( counter > old_counter );
                
                total_counter += counter;

                if( pd.InvalidQ() ) { continue; }
            
                if( pd.CrossingCount() <= Int(1) )
                {
                    CreateUnlink( pd.last_color_deactivated );
                    continue;
                }
                
                if( pd.CrossingCount() < pd.MaxCrossingCount() )
                {
                    PushDiagramDone( pd.CreateCompressed() );
                    continue;
                }
                else
                {
                    pd.ClearCache();
                    PushDiagramDone( std::move(pd) );
                    continue;
                }
                
            }  // while( !pd_todo.empty() )
            
            swap( pd_list, pd_done );
            
            // Sort big diagrams in front.
            std::sort(
                &pd_list[0],
                &pd_list[pd_list.size()],
                []( cref<PD_T> pd_0, cref<PD_T> pd_1 )
                {
                    return pd_0.CrossingCount() > pd_1.CrossingCount();
                }
            );
            
            if( total_counter > Size_T(0) ) { this->ClearCache(); }
            
            return total_counter;
        }


        mref<PassSimplifier_T> GetPassSimplifier( const Dijkstra_T strategy = Dijkstra_T::Bidirectional )
        {
            if( !this->InCacheQ("PassSimplifier") )
            {
                this->SetCache("PassSimplifier", PassSimplifier_T(*this,strategy));
            }

            return this->template GetCache<PassSimplifier_T>("PassSimplifier").SetDijkstraStrategy(strategy);
        }
        
        /*!@brief **EXPERIMENTAL:** Attempts to find the shortest path between the faces created by merging the two faces of arc`a` and the faces created by merging the two faces of arc `b`.
         *
         * CAUTION: This assumes that `a` and `b` lie on the same link component!
         *
         *  @param idx Index of the subdiagram in which the path shall be found.
         *
         *  @param a The one end arc of the shortest path we are looking for.
         *
         *  @param b The other end arc of the shortest path we are looking for.
         *
         *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
         *
         *  @param strategy The search strategy.
         */
        
        PassSimplifier_T::Path_T FindShortestPath(
            const Int idx, const Int a, const Int b, const Int max_dist, const Dijkstra_T strategy
        )
        {
            return GetPassSimplifier(strategy).FindShortestPath( pd_list[idx], a, b, max_dist );
        }
        
        /*!@brief **EXPERIMENTAL:** Attempts to find the arcs that make up a minimally rerouted strand, neglecting the arcs from `a` to `b` when traversed in natural order. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow! (It has to find and mark a the currect path between `a` and `b`, if existent.
         *
         *  @param idx Index of the subdiagram in which the path shall be found.
         *
         *  @param a The first arc of the input strand.
         *
         *  @param b The last arc of the input strand (included).
         *
         *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
         *
         *  @param strategy The search strategy.
         */
        PassSimplifier_T::Path_T FindShortestRerouting(
            const Int idx, const Int a, const Int b, const Int max_dist, const Dijkstra_T strategy
        )
        {
            return GetPassSimplifier(strategy).FindShortestRerouting( pd_list[idx], a, b, max_dist );
        }
       
    public:
        
        /*!@brief Return whether this diagram is _locked_. Public UNSAFE operations cannot be called while this is being locked to prohibit unintended modifications that break topological invariance.*/
        bool LockedQ() const
        {
            return lockedQ;
        }
        
        /*!@brief Unlock diagram complex to allow topological modifications.*/
        void Unlock()
        {
            lockedQ = false;
        }
        
        /*!@brief Lock diagram complex to prohibit topological modifications.*/
        void Lock()
        {
            lockedQ = true;
        }
        
        void LockMessage( const std::string & tag ) const
        {
            wprint(MethodName(tag) + ": This method is considered **UNSAFE**, and the diagram is currently locked to prevent break of topological invariance. If you want to perform this operation anyways, call `Unlock()` first. (Don't forget to `Lock()` it again.)");
        }
        
    public:
        
        /*!@brief Return a string that identifies a class method specified by `tag`. Mostly used for logging and in error messages.*/
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*!@brief Return a string that identifies this class with type information. Mostly used for logging and in error messages.*/
        static constexpr std::string ClassName()
        {
            return std::string("PlanarDiagramComplex")
                + "<" + TypeName<Int>
                + ">";
        }
    };

} // namespace Knoodle



