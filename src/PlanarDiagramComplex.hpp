#pragma  once

namespace Knoodle
{

    // TODO: StrandSimplifier.
    //      - "Big Hopf Link"
    // TODO: SplitDiagramComponents.
    // TODO: DisconnectSummands.
    // TODO: ArcSimplifier: Improve performance of local simplification at opt level 4.
    
    // TODO: Need a way to express invalidity.
    // TODO: LinkComponentCount.
    // TODO: ByteCount.

    template<typename Int_>
    class alignas( ObjectAlignment ) PlanarDiagramComplex final : public CachedObject
    {
        static_assert(IntQ<Int_>,"");

    public:

        static constexpr bool debugQ = false;
        
        using Int                   = Int_;
        using UInt                  = ToUnsigned<Int>;
        
        using Base_T                = CachedObject;
        using Class_T               = PlanarDiagramComplex<Int>;
        using PD_T                  = PlanarDiagram2<Int>;
        using PDC_T                 = PlanarDiagramComplex<Int>;
        using PDList_T              = std::vector<PD_T>;
        
        using C_Arcs_T              = typename PD_T::C_Arcs_T;
        using A_Cross_T             = typename PD_T::A_Cross_T;
        using ArcContainer_T        = typename PD_T::ArcContainer_T;
        using ColorCounts_T         = typename PD_T::ColorCounts_T;
        
        using StrandSimplifier_T    = StrandSimplifier2<Int,true,true>;
        using Strategy_T            = typename StrandSimplifier_T::Strategy_T;
        
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        
        static constexpr Int  DoNotVisit    =  PD_T::DoNotVisit;
        static constexpr Int  Uninitialized =  PD_T::Uninitialized;
        
        friend class LoopRemover<Int>;
        friend class ArcSimplifier2<Int,0,true >;
        friend class ArcSimplifier2<Int,1,true >;
        friend class ArcSimplifier2<Int,2,true >;
        friend class ArcSimplifier2<Int,3,true >;
        friend class ArcSimplifier2<Int,4,true >;
        friend class ArcSimplifier2<Int,0,false>;
        friend class ArcSimplifier2<Int,1,false>;
        friend class ArcSimplifier2<Int,2,false>;
        friend class ArcSimplifier2<Int,3,false>;
        friend class ArcSimplifier2<Int,4,false>;
        
        friend class StrandSimplifier2<Int,true ,true >;
        friend class StrandSimplifier2<Int,true ,false>;
        friend class StrandSimplifier2<Int,false,true >;
        friend class StrandSimplifier2<Int,false,false>;
        
    private:
        
        // Class data members
        mutable PDList_T pd_list;
        mutable PDList_T pd_todo;
        mutable PDList_T pd_done;
//
//        ColorCounts_T colored_unlinkQ;
        
        PD_T invalid_diagram { PD_T::InvalidDiagram() };
        
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
 
        PlanarDiagramComplex( PD_T && pd, Int unlink_count )
        {
            const bool validQ = pd.ValidQ();
            
            pd_list.reserve( ToSize_T(unlink_count) + Size_T(validQ) );
            pd_done.reserve( ToSize_T(unlink_count) );
            
            Int max_color = 0;
            
            if( validQ )
            {
                ColorCounts_T color_arc_counts = pd.ColorArcCounts();
             
                for( auto & x : color_arc_counts )
                {
                    max_color = Max( max_color, x.first );
                }
            }
            
            pd_done.push_back( std::move(pd) );
            
            for( Int unlink = 0; unlink < unlink_count; ++unlink )
            {
                CreateUnlink( max_color + unlink + validQ );
            }
            
            swap(pd_done,pd_list);
        }
        
    
        
        explicit PlanarDiagramComplex( std::pair<PD_T,Int> && pd_and_unlink_count )
        :   PlanarDiagramComplex(
                std::move(pd_and_unlink_count.first), pd_and_unlink_count.second
            )
        {}
        
        explicit PlanarDiagramComplex( PD_T && pd )
        :   PlanarDiagramComplex( std::move(pd), Int(0) )
        {}
        
#include "PlanarDiagramComplex/Constructors.hpp"
#include "PlanarDiagramComplex/Color.hpp"
#include "PlanarDiagramComplex/SimplifyLocal.hpp"
#include "PlanarDiagramComplex/Split.hpp"
#include "PlanarDiagramComplex/Disconnect.hpp"
#include "PlanarDiagramComplex/Simplify.hpp"
#include "PlanarDiagramComplex/LinkingNumber.hpp"
#include "PlanarDiagramComplex/ModifyDiagramList.hpp"
#include "PlanarDiagramComplex/ModifyDiagram.hpp"
#include "PlanarDiagramComplex/Union.hpp"
#include "PlanarDiagramComplex/Checks.hpp"
        
#include "PlanarDiagramComplex/Connect.hpp"
            
    public:
        
        Int DiagramCount() const
        {
            return int_cast<Int>(pd_list.size());
        }
        
        cref<PD_T> Diagram( Int i ) const
        {
            if( i < Int(0) )
            {
                eprint(MethodName("Diagram") + ": Index  i < 0. Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            if( i >= DiagramCount() )
            {
                eprint(MethodName("Diagram") + ": Index  i >= DiagramCount(). Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            return pd_list[Size_T(i)];
        }
        
        void Compress()
        {
            for( PD_T & pd : pd_list )
            {
                pd.Compress();
            }
        }
        
        void ClearCaches()
        {
            for( PD_T & pd : pd_list )
            {
                pd.ClearCache();
            }
            this->ClearCache();
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
                eprint(MethodName("Diagram") + ": Index  i >= DiagramCount(). Returning invalid diagram.");
                
                return invalid_diagram;
            }
            
            return pd_list[Size_T(i)];
        }
        
    public:
        
        Int CrossingCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.CrossingCount(); }
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
        
        Int ArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.ArcCount(); }
            return count;
        }
        
        Int TotalMaxArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count += pd.MaxArcCount(); }
            return count;
        }
        
        Int MaxMaxArcCount() const
        {
            Int count = 0;
            for( const PD_T & pd : pd_list ) { count = Max(count,pd.MaxArcCount()); }
            return count;
        }
        
        
    private:

        // We must be careful not to push to pd_list, because we may otherwise invalidate references to elements in pd_list; this would bork the simplification loops.
        void CreateUnlink( const Int color )
        {
//            TOOLS_PTIMER(timer,MethodName("CreateUnlink"));
 
            pd_done.push_back( PD_T::Unknot(color) );
        }
        
        void CreateUnlinkFromArc( PD_T & pd, const Int a )
        {
//            TOOLS_PTIMER(timer,MethodName("CreateUnlinkFromArc"));
            
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
//            TOOLS_PTIMER(timer,MethodName("CreateHopfLinkFromArcs"));
            
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
//            TOOLS_PTIMER(timer,MethodName("CreateTrefoilKnotFromArc"));
            
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
//            TOOLS_PTIMER(timer,MethodName("CreateFigureEightKnotFromArc"));
            
            pd.template AssertArc<0>(a);

            pd_done.push_back( PD_T::FigureEightKnot(pd.A_color[a]) );
        }
        
    private:
        
        void PushDiagram( PD_T && pd )
        {
            if( pd.ValidQ() )
            {
                pd_list.push_back( std::move(pd) );
            }
            else
            {
                // DEBUGGING
                wprint(MethodName("PushDiagram")+": Tried to push an invalid diagram. Doing nothing.");
            }
        }
        
//        // TODO: Check where this is used. Usually, there is a better way to do this.
//        void JoinLists()
//        {
//            PD_TIMER(timer,MethodName("JoinLists"));
//            for( PD_T & pd : pd_list_todo )
//            {
//                PushDiagram(std::move(pd));
//            }
//            
//            pd_list_todo.clear();
//        }
        
    public:
        
        void CompressDiagrams()
        {
            TOOLS_PTIMER(timer,MethodName("CompressDiagrams"));

            for( PD_T & pd : pd_list )
            {
                pd.Compress();
            }
        }

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
        
        static Int FlipDarc( const Int da )
        {
            return PD_T::FlipDarc(da);
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
                
                if(  pd.proven_minimalQ )
                {
                    if( pd.arc_count < pd.max_arc_count )
                    {
                        pd_done.push_back( pd.CreateCompressed() );
                    }
                    else
                    {
                        pd.ClearCache();
                        pd_done.push_back( std::move(pd) );
                    }
                    continue;
                }
                
                Size_T old_counter = 0;
                Size_T counter = 0;
                
                do
                {
                    old_counter = counter;
                    
                    for( Int a = 0; a < pd.max_arc_count; ++a )
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
            
                if( pd.CrossingCount() == Int(0) )
                {
                    pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
                    continue;
                }
                
                if( pd.crossing_count < pd.max_crossing_count )
                {
                    pd_done.push_back( pd.CreateCompressed() );
                }
                else
                {
                    pd.ClearCache();
                    pd_done.push_back( std::move(pd) );
                }
            }  // while( !pd_todo.empty() )
            
            swap( pd_list, pd_done );
            
            // Sort big diagrams in front.
            Sort(
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


        mref<StrandSimplifier_T> StrandSimplifier( const Strategy_T strategy = Strategy_T::TwoSided )
        {
            if( !this->InCacheQ("StrandSimplifier") )
            {
                this->SetCache("StrandSimplifier", StrandSimplifier_T(*this,strategy));
            }

            return this->template GetCache<StrandSimplifier_T>("StrandSimplifier").SetStrategy(strategy);
        }
        
        Tensor1<Int,Int> FindShortestPath(
            const Int idx, const Int a, const Int b, const Int max_dist, const Strategy_T strategy
        )
        {
            return StrandSimplifier(strategy).FindShortestPath( pd_list[idx], a, b, max_dist );
        }
        
        Tensor1<Int,Int> FindShortestRerouting(
            const Int idx, const Int a, const Int b, const Int max_dist, const Strategy_T strategy
        )
        {
            return StrandSimplifier(strategy).FindShortestRerouting( pd_list[idx], a, b, max_dist );
        }
       
    public:
        
    /*!@brief Returns a string that identifies a class method specified by `tag`. Mostly used for logging and in error messages.
     */
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
/*!@brief Returns a string that identifies this class with type information. Mostly used for logging and in error messages.
 */
        
        static std::string ClassName()
        {
            return ct_string("PlanarDiagramComplex")
                + "<" + TypeName<Int>
                + ">";
        }
    };

} // namespace Knoodle



