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
        
        using ColorMap_T            = std::unordered_map<Int,Int>;
                
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        
        static constexpr Int  Uninitialized =  PD_T::Uninitialized;
        
    public:

        
    protected:
        
        // Class data members
        std::vector<PD_T> pd_list;
        std::vector<PD_T> pd_list_new;
        
        Int color_count;
        
        ColorMap_T colored_unlinkQ;
        
        
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
        :   color_count     { pd.ColorCount() + Ramp(unlink_count) }
        {
            pd_list.reserve( ToSize_T(unlink_count) + Size_T(1) );
            Int max_color = 0;
            
            for( Int color : pd.color_palette )
            {
                max_color = Max( color, max_color );
                colored_unlinkQ[color] = false;
            }
            
            pd_list.push_back( std::move(pd) );
            for( Int lc = 0; lc < unlink_count; ++lc )
            {
                CreateUnlink(max_color + lc + 1);
            }
            
            
        }
        
        PlanarDiagramComplex( std::pair<PD_T,Int> && pd_and_unlink_count )
        :   PlanarDiagramComplex( std::move(pd_and_unlink_count.first), pd_and_unlink_count.second )
        {}
        
        
#include "PlanarDiagramComplex/Constructors.hpp"
#include "PlanarDiagramComplex/SimplifyLocal.hpp"
#include "PlanarDiagramComplex/SimplifyGlobal.hpp"
            
    public:
        
        Int ColorCount() const
        {
            return color_count;
        }
            
        Int DiagramCount() const
        {
            return int_cast<Int>(pd_list.size());
        }
        
        mref<PD_T> Diagram( Int i )
        {
            // TODO: Check range of i?
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
            return colored_unlinkQ.count(color) > Size_T(0);
        }
        
        void CreateUnlink( const Int color )
        {
            if( !ColorExistsQ(color) )
            {
                eprint(MethodName("CreateUnlink") + ": Invalid input color " + ToString(color) + ". Doing nothing.");
            }
            
            // Make sure to push an unlink to `pd_list` only if there is no other unlink of the same color already.
            if( !colored_unlinkQ[color] )
            {
                // Records unlinks twice: once in pd_list and once in colored_unlinkQ.
                // Not sure whether I like this design.
                colored_unlinkQ[color] = true;
                pd_list_new.push_back( PD_T::Unknot(color) );
            }
        }
        
    private:
        
        void JoinLists()
        {
            PD_TIMER(timer,MethodName("JoinLists"));
            for( PD_T & pd : pd_list_new )
            {
                pd_list.push_back(std::move(pd));
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



