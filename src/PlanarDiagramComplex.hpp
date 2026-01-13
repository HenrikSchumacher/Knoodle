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

        using Int                   = Int_;
        using UInt                  = ToUnsigned<Int>;
        
        using Base_T                = CachedObject;
        using Class_T               = PlanarDiagramComplex<Int>;
        using PD_T                  = PlanarDiagram2<Int>;
        using PDC_T                 = PlanarDiagramComplex<Int>;
        using PDList_T              = std::vector<PD_T>;
        
//        using ColorPalette_T        = PD_T::ColorPalette_T;
        using ColorCounts_T         = PD_T::ColorCounts_T;
                
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
        
    public:

        
    protected:
        
        // Class data members
        PDList_T pd_list;
        PDList_T pd_list_new;
        
//        Int color_count;
        
        ColorCounts_T colored_unlinkQ;
        
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
            pd_list.reserve( ToSize_T(unlink_count) + Size_T(pd.ValidQ()) );
            Int max_color = 0;
            
            for( auto & x : pd.color_arc_counts )
            {
                max_color = Max( max_color, x.second );
                colored_unlinkQ[x.first] = 0;
            }
            
            PushDiagram( std::move(pd) );
            
            for( Int unlink = 0; unlink < unlink_count; ++unlink )
            {
                (void)CreateUnlink(max_color + unlink + 1);
            }
            
            JoinLists();
        }
        
        
        bool PushDiagram( PD_T && pd )
        {
            if( pd.ValidQ() )
            {
                pd_list.push_back( std::move(pd) );
                return true;
            }
            else
            {
                // DEBUGGING
                wprint(MethodName("PushDiagram")+": Tried to push an invalid diagram. Doing nothing.");
                
                return false;
            }
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
#include "PlanarDiagramComplex/SimplifyLocal.hpp"
#include "PlanarDiagramComplex/SimplifyGlobal.hpp"
            
    public:
        
        Int ColorCount() const
        {
            return static_cast<Int>(colored_unlinkQ.size());
        }
            
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
        
        
        bool ColorExistsQ( const Int color ) const
        {
            return colored_unlinkQ.contains(color);
        }
        
        
    private:
        
        bool CreateUnlink( const Int color )
        {
            if( !ColorExistsQ(color) )
            {
                eprint(MethodName("CreateUnlink") + ": Invalid input color " + ToString(color) + ". Doing nothing");
                
                return false;
            }
            
            // Make sure to create an unlink to `pd_list` only if there is no other unlink of the same color already.
            if( colored_unlinkQ[color] )
            {
                return false;
            }
            else
            {
                // Records unlinks twice: once in pd_list and once in colored_unlinkQ.
                // Not sure whether I like this design.
                colored_unlinkQ[color] = Int(1);
                
                pd_list_new.push_back( PD_T::Unknot(color) );
                return true;
            }
        }
        
        void CreateUnlinkFromArc( PD_T & pd, const Int a )
        {
            TOOLS_PTIMER(timer,MethodName("CreateUnlinkFromArc"));
            
            pd.template AssertArc<0>(a);
            const Int a_color = pd.A_color[a];
            
            PD_ASSERT( pd.color_arc_counts.contains(a_color) );
            
            // If there are no arcs of that color left in pd, then we tell it to forget that color.
            if( pd.color_arc_counts[a_color] == Int(0) )
            {
                pd.color_arc_counts.erase( a_color );
            }
            else
            {
                // DEBUGGING
                wprint(MethodName("CreateUnlinkFromArc") +": There are arcs with the same color left in the same planar diagram; probably something went wrong.");
            }
                
            CreateUnlink(a_color) ;
        }
        
        bool CreateHopfLink( const Int color_0, const Int color_1 )
        {
            if( !ColorExistsQ(color_0) )
            {
                eprint(MethodName("CreateHopfLink") + ": Invalid input color " + ToString(color_0) + ". Doing nothing");
                
                return false;
            }
            
            if( !ColorExistsQ(color_1) )
            {
                eprint(MethodName("CreateHopfLink") + ": Invalid input color " + ToString(color_1) + ". Doing nothing");
                
                return false;
            }
            
            pd_list_new.push_back( PD_T::HopfLink(color_0,color_1) );
            return true;
        }
        
        /*!@brief Create a new Hopf link `in pd_list-new`.
         *
         * @param pd The diagram we are working with.
         *
         * @param a_0 First edge whose color we use. It is assumed to belong to diagram `pd` and to be deactivated.
         *
         * @param a_1 Second edge whose color we use. It is assumed to belong to diagram `pd` and to be deactivated.
         *
         * @param splitQ If `splitQ == true`, then we assume that the Hopf link was not connect to the remainder of the diagram pd. If `splitQ == false`, then we assume that arc `a_0` was connected to the remaining diagram. This is important for the management of color counts.
         */
        void CreateHopfLinkFromArcs(
            PD_T & pd, const Int a_0, const Int a_1, const bool splitQ
        )
        {
            TOOLS_PTIMER(timer,MethodName("CreateHopfLinkFromArcs"));
            
            pd.template AssertArc<0>(a_0);
            pd.template AssertArc<0>(a_1);
            
            const Int a_0_color = pd.A_color[a_0];
            const Int a_1_color = pd.A_color[a_1];
            
            TOOLS_LOGDUMP(pd.A_color);
            logvalprint("pd.color_arc_counts",Knoodle::ToString(pd.color_arc_counts));
            
            PD_ASSERT( pd.color_arc_counts.contains(a_0_color) );
            PD_ASSERT( pd.color_arc_counts.contains(a_1_color) );

            // If there are no arcs of that color left in pd, then we tell it to forget that color.
            if( pd.color_arc_counts[a_0_color] == Int(0) )
            {
                pd.color_arc_counts.erase( a_0_color );
            }
            else
            {
                // DEBUGGING
                
                if( !splitQ )
                {
                    wprint(MethodName("CreateHopfLinkFromArcs") +": There are arcs with the same color left in the same planar diagram; probably something went wrong.");
                }
            }
            if( pd.color_arc_counts[a_1_color] == Int(0) )
            {
                pd.color_arc_counts.erase( a_1_color );
            }
            else
            {
                // DEBUGGING
                wprint(MethodName("CreateHopfLinkFromArcs") +": There are arcs with the same color left in the same planar diagram; probably something went wrong.");
            }
                
            CreateHopfLink(a_0_color,a_1_color) ;
        }
        
    private:
        
        // TODO: Check where this is used. Usually, there is a better way to do this.
        void JoinLists()
        {
            PD_TIMER(timer,MethodName("JoinLists"));
            for( PD_T & pd : pd_list_new )
            {
                if( pd.ValidQ() )
                {
                    pd_list.push_back(std::move(pd));
                }
            }
            
            pd_list_new = std::vector<PD_T>();
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



