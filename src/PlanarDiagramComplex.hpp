#pragma  once

#include "PlanarDiagramComplex/ArcSimplifier2.hpp"
#include "PlanarDiagramComplex/StrandSimplifier2.hpp"

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
        
        using C_Arc_T               = typename PD_T::C_Arc_T;
        using A_Cross_T             = typename PD_T::A_Cross_T;
        using ArcContainer_T        = typename PD_T::ArcContainer_T;
        using ColorCounts_T         = typename PD_T::ColorCounts_T;
                
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        
        static constexpr Int  Uninitialized =  PD_T::Uninitialized;
        
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
#include "PlanarDiagramComplex/SimplifyGlobal.hpp"
#include "PlanarDiagramComplex/LinkingNumber.hpp"
#include "PlanarDiagramComplex/Connect.hpp"
#include "PlanarDiagramComplex/Modify.hpp"
        
#include "PlanarDiagramComplex/Unite.hpp"
            
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
        
        Int CrossingCount() const
        {
            Int crossing_count = 0;
            
            for( const PD_T & pd : pd_list )
            {
                crossing_count += pd.CrossingCount();
            }
            
            return crossing_count;
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
        
        static Int FlipDarc( const Int da )
        {
            return PD_T::FlipDarc(da);
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



