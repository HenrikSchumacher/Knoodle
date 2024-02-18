#pragma  once

#include <cassert>

namespace KnotTools
{

#ifdef PD_ASSERTS
    #define PD_print( s ) Tools::print(s);
#else
    #define PD_print( s )
#endif
    
#ifdef PD_ASSERTS
    #define PD_wprint( s ) Tools::wprint(s);
#else
    #define PD_wprint( s )
#endif
    
#ifdef PD_ASSERTS
    #define PD_assert( s ) assert(s);
#else
    #define PD_assert( s )
#endif
    
#ifdef PD_ASSERTS
    #define PD_valprint( key, val ) Tools::valprint( key, val )
#else
    #define PD_valprint( key, val )
#endif
    
    
    template<typename Int_>
    class alignas( ObjectAlignment ) PlanarDiagram : public CachedObject
    {
    public:
        
        ASSERT_SIGNED_INT(Int_)

        using Int  = Int_;
        using Sint = int;
        
        using Class_T = PlanarDiagram<Int>;
        
        using CrossingContainer_T       = Tensor3<Int,Int>;
        using ArcContainer_T            = Tensor2<Int,Int>;
        using CrossingStateContainer_T  = Tensor1<CrossingState,Int>;
        using ArcStateContainer_T       = Tensor1<ArcState,Int>;
        
        using Arrow_T = std::pair<Int,bool>;
        
        static constexpr bool Tip   = true;
        static constexpr bool Tail  = false;
        static constexpr bool Left  = false;
        static constexpr bool Right = true;
        static constexpr bool In    = true;
        static constexpr bool Out   = false;
        
    protected:
        
        CrossingContainer_T      C_arcs;
        CrossingStateContainer_T C_state;
        Tensor1<Int,Int>         C_label; // List of vertex labels that stay persistent throughout.
        
        ArcContainer_T           A_cross;
        ArcStateContainer_T      A_state;
        Tensor1<Int,Int>         A_label; // List of arc labels that stay persistent throughout.
        
        Int initial_crossing_count = 0;
        Int initial_arc_count      = 0;
        
        Int crossing_count         = 0;
        Int arc_count              = 0;
        Int unlink_count           = 0;
        
        // Counters for Reidemeister moves.
        Int R_I_counter  = 0;
        Int R_II_counter = 0;
        Int tangle_move_counter = 0;
        
        // Stacks for keeping track of recently modified entities.
        std::vector<Int> touched_crossings;
        std::vector<Int> touched_arcs;
        std::vector<Int> switch_candidates;
        
        // Data for the faces and the dual graph
        
        // TODO: I might want to replace this data type by a Tensor2.
        Tiny::VectorList<2,Int,Int> A_faces; // Convention: Left face first.
        
        Tensor1<Int,Int> face_arcs {0};
        Tensor1<Int,Int> face_ptr  {2,0};
        
        Tensor1<Int,Int> comp_arcs {0};
        Tensor1<Int,Int> comp_ptr  {2,0};
        
        Tensor1<Int,Int> arc_comp  {0};
        
        bool faces_initialized = false;
        
        Int connected_sum_counter = 0;
        
    public:
        
        PlanarDiagram() = default;
        
        ~PlanarDiagram() = default;
        
        // This constructor is supposed to allocate all relevant buffers.
        // Data has to be filled in manually by using the references provided by Crossings() and Arcs() method.
        PlanarDiagram( const Int crossing_count_, const Int unlink_count_ )
        :   C_arcs                  ( crossing_count_, 2, 2                         )
        ,   C_state                 ( crossing_count_, CrossingState::Unitialized   )
        ,   C_label                 ( iota<Int,Int>(crossing_count_)                )
        ,   A_cross                 ( Int(2)*crossing_count_, 2                     )
        ,   A_state                 ( Int(2)*crossing_count_, ArcState::Inactive    )
        ,   A_label                 ( iota<Int,Int>(Int(2)*crossing_count_)         )
        ,   initial_crossing_count  ( crossing_count_                               )
        ,   initial_arc_count       ( Int(2)*crossing_count_                        )
        ,   crossing_count          ( crossing_count_                               )
        ,   arc_count               ( Int(2)*crossing_count_                        )
        ,   unlink_count            ( unlink_count_                                 )
        {
            PushAllCrossings();
        }
        
        // Copy constructor
        PlanarDiagram( const PlanarDiagram & other )
        :   C_arcs                  ( other.C_arcs              )
        ,   C_state                 ( other.C_state             )
        ,   C_label                 ( other.C_label             )
        ,   A_cross                 ( other.A_cross             )
        ,   A_state                 ( other.A_state             )
        ,   A_label                 ( other.A_label             )
        ,   initial_crossing_count  ( other.crossing_count      )
        ,   initial_arc_count       ( other.initial_arc_count   )
        ,   crossing_count          ( other.crossing_count      )
        ,   arc_count               ( other.arc_count           )
        ,   unlink_count            ( other.unlink_count        )
        ,   touched_crossings       ( other.touched_crossings   )
        ,   touched_arcs            ( other.touched_arcs        )
        {
            PushAllCrossings();
        }
        
        friend void swap(PlanarDiagram &A, PlanarDiagram &B) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( A.C_arcs      , B.C_arcs       );
            swap( A.C_state     , B.C_state      );
            swap( A.C_label     , B.C_label      );
            swap( A.A_cross     , B.A_cross      );
            swap( A.A_state     , B.A_state      );
            swap( A.A_label     , B.A_label      );
            
            swap( A.initial_crossing_count, B.initial_crossing_count );
            swap( A.initial_arc_count     , B.initial_arc_count      );
            swap( A.crossing_count        , B.crossing_count         );
            swap( A.arc_count             , B.arc_count              );
            swap( A.unlink_count          , B.unlink_count           );
            swap( A.touched_crossings     , B.touched_crossings      );
            swap( A.touched_arcs          , B.touched_arcs           );
        }
        
        // Move constructor
        PlanarDiagram( PlanarDiagram && other ) noexcept
        :   PlanarDiagram()
        {
            swap(*this, other);
        }

        /* Copy assignment operator */
        PlanarDiagram & operator=( const PlanarDiagram & other )
        {
            // Use the copy constructor.
            swap( *this, PlanarDiagram(other) );
            return *this;
        }

        /* Move assignment operator */
        PlanarDiagram & operator=( PlanarDiagram && other ) noexcept
        {
            swap( *this, other );
            return *this;
        }
        
        
        
        
        template<typename Real, typename SInt>
        PlanarDiagram( cref<Link_2D<Real,Int,SInt>> L )
        :   PlanarDiagram( L.CrossingCount(), L.UnlinkCount() )
        {
            using Link_T         = Link_2D<Real,Int>;
            using Intersection_T = Link_T::Intersection_T;

            const Int component_count     = L.ComponentCount();
            
            cptr<Int>  component_ptr      = L.ComponentPointers().data();
            cptr<Int>  edge_ptr           = L.EdgePointers().data();
            cptr<Int>  edge_intersections = L.EdgeIntersections().data();
            cptr<bool> edge_overQ         = L.EdgeOverQ().data();
            
            cref<std::vector<Intersection_T>> intersections = L.Intersections();
            
            // Now we go through all components
            //      then through all edges of the component
            //      then through all intersections of the edge
            // and generate new vertices, edges, crossings, and arcs in one go.
            

            PD_print("Begin of Link");
            PD_print("{");
            for( Int comp = 0; comp < component_count; ++comp )
            {
                PD_print("\tBegin of component " + ToString(comp));
                PD_print("\t{");
                
                // The range of arcs belonging to this component.
                const Int arc_begin = edge_ptr[component_ptr[comp  ]];
                const Int arc_end   = edge_ptr[component_ptr[comp+1]];
                
                PD_valprint("\t\tarc_begin", arc_begin);
                PD_valprint("\t\tarc_end"  , arc_end  );

                if( arc_begin == arc_end )
                {
                    // Component is an unlink. Just skip it.
                    continue;
                }
                
                // If we arrive here, then there is definitely a crossing in the first edge.

                for( Int b = arc_begin, a = arc_end-1; b < arc_end; a = (b++) )
                {
                    const Int c = edge_intersections[b];
                    
                    const bool overQ = edge_overQ[b];
                    
                    cref<Intersection_T> inter = intersections[c];
                    
                    A_cross(a,Tip ) = c; // c is tip  of a
                    A_cross(b,Tail) = c; // c is tail of b
                    
                    PD_assert( inter.sign > SInt(0) || inter.sign < SInt(0) );
                    
                    bool positiveQ = inter.sign > SInt(0);

                    
//                 negative         positive
//                 right-handed     left-handed
//                 .       .        .       .
//                 .       .        .       .
//                 +       +        +       +
//                  ^     ^          ^     ^
//                   \   /            \   /
//                    \ /              \ /
//                     /                \
//                    / \              / \
//                   /   \            /   \
//                  /     \          /     \
//                 +       +        +       +
//                 .       .        .       .
//                 .       .        .       .
                    
                    C_state[c] = positiveQ ? CrossingState::Positive : CrossingState::Negative;
                    A_state[a] = ArcState::Active;
                    
                    /*
                        positiveQ == true and overQ == true:

                            C_arcs(c,Out,Left)  .       .  C_arcs(c,Out,Right) = b
                                                .       .
                                                +       +
                                                 ^     ^
                                                  \   /
                                                   \ /
                                                    /
                                                   / \
                                                  /   \
                                                 /     \
                                                +       +
                                                .       .
                         a = C_arcs(c,In,Left)  .       .  C_arcs(c,In,Right)
                    */
                    const bool over_in_side = (positiveQ == overQ) ? Left : Right ;
                    
                    
                    C_arcs(c,In , over_in_side) = a;
                    C_arcs(c,Out,!over_in_side) = b;
                }
                
                PD_print("\t}");
                PD_print("\tEnd   of component " + ToString(comp));
                
                PD_print("");
                
            }
            PD_print("");
            PD_print("}");
            PD_print("End   of Link");
            PD_print("");
        }
        
        
        
        Int UnlinkCount() const
        {
            return unlink_count;
        }
        
        // Crossings

        Int InitialCrossingCount() const
        {
            return initial_crossing_count;
        }
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
        
        Int Reidemeister_I_Counter() const
        {
            return R_I_counter;
        }
        
        Int Reidemeister_II_Counter() const
        {
            return R_II_counter;
        }
   
        mref<Tensor1<Int,Int>> CrossingLabels()
        {
            return C_label;
        }
        
        cref<Tensor1<Int,Int>> CrossingLabels() const
        {
            return C_label;
        }
        
        mref<CrossingContainer_T> Crossings()
        {
            return C_arcs;
        }
        
        cref<CrossingContainer_T> Crossings() const
        {
            return C_arcs;
        }
        
        mref<CrossingStateContainer_T> CrossingStates()
        {
            return C_state;
        }
        
        cref<CrossingStateContainer_T> CrossingStates() const
        {
            return C_state;
        }
        
        
        Int InitialArcCount() const
        {
            return initial_arc_count;
        }
        
        Int ArcCount() const
        {
            return arc_count;
        }
        
        mref<Tensor1<Int,Int>> ArcLabels()
        {
            return A_label;
        }
        
        cref<Tensor1<Int,Int>> ArcLabels() const
        {
            return A_label;
        }
        
        mref<ArcContainer_T> Arcs()
        {
            return A_cross;
        }
        
        cref<ArcContainer_T> Arcs() const
        {
            return A_cross;
        }

        mref<ArcStateContainer_T> ArcStates()
        {
            return A_state;
        }
        
        cref<ArcStateContainer_T> ArcStates() const
        {
            return A_state;
        }
        
        //Modifiers
        
    public:
        
        std::string CrossingString( const Int c ) const
        {
            return "crossing " + ToString(c) +" = { { " +
               ToString(C_arcs(c,Out,Left ))+", "+ToString(C_arcs(c,Out,Right))+" }, { "+
               ToString(C_arcs(c,In ,Left ))+", "+ToString(C_arcs(c,In ,Right))+" } } ";
        }
        
        
        bool CrossingActiveQ( const Int c ) const
        {
            return (
                (C_state[c] == CrossingState::Positive)
                ||
                (C_state[c] == CrossingState::Negative)
            );
        }

        bool OppositeCrossingSigns( const Int c_0, const Int c_1 ) const
        {
            return ( ToUnderlying(C_state[c_0]) == -ToUnderlying(C_state[c_1]) );
        }
        
        void DeactivateCrossing( const Int c )
        {
            if( CrossingActiveQ(c) )
            {
                --crossing_count;
            }

            C_state[c] = CrossingState::Unitialized;
        }
        
        
        std::string ArcString( const Int a ) const
        {
            return "arc " +ToString(a) +" = { " +
               ToString(A_cross(a,Tail))+", "+ToString(A_cross(a,Tip))+" } ";
        }
        
        bool ArcActiveQ( const Int a ) const
        {
            return A_state[a] == ArcState::Active;
        }
        
        void DeactivateArc( const Int a )
        {
            if( ArcActiveQ(a) )
            {
                --arc_count;
            }
            
            A_state[a] = ArcState::Inactive;
        }
        
        Int NextCrossing( const Int c, bool io, bool lr ) const
        {
            return A_cross( C_arcs(c,io,lr), ( io == In ) ? Tail : Tip );
        }

        

        
    public:
        
#include "PlanarDiagram/Reconnect.hpp"
#include "PlanarDiagram/Checks.hpp"
#include "PlanarDiagram/Reidemeister_I.hpp"
#include "PlanarDiagram/Reidemeister_II.hpp"
//#include "PlanarDiagram/Break.hpp"
//#include "PlanarDiagram/Switch.hpp"
        
#include "PlanarDiagram/Faces.hpp"
#include "PlanarDiagram/ConnectedSum.hpp"
        

        void PushAllCrossings()
        {
            touched_crossings.reserve( initial_crossing_count );
            
            for( Int i = initial_crossing_count-1; i >= 0; --i )
            {
                touched_crossings.push_back( i );
            }
        }
        
        void PushRemainingCrossings()
        {
            Int counter = 0;
            for( Int i = initial_crossing_count-1; i >= 0; --i )
            {
                if( CrossingActiveQ(i) )
                {
                    ++counter;
                    touched_crossings.push_back( i );
                }
            }
        }
        
        void RemoveLoops()
        {
            faces_initialized = false;
            
//            R_I_counter  = 0;
//            R_II_counter = 0;
//            tangle_move_counter = 0;
            
//            switch_candidates.clear();
                        
            while( !touched_crossings.empty() )
            {
                const Int c = touched_crossings.back();
                touched_crossings.pop_back();
                
                PD_assert( CheckCrossing(c) );
                
                (void)Reidemeister_I(c);
            }
            
            PD_print( "Performed Reidemeister I  moves = " + ToString(R_I_counter ));
        }
        


        
        Arrow_T NextLeftArc( const Int a, const bool tiptail )
        {
            // TODO: Signed indexing does not work because of 0!
            
            PD_assert( ArcActiveQ(a) );
            
            
            const Int c = A_cross(a,tiptail);
            
            PD_assert( CrossingActiveQ(c) );
            
            
            const bool io = (tiptail == Tip) ? In : Out;

            const bool lr = (C_arcs(c,io,Left) == a) ? Left : Right;
            
            
            
            if( io == In )
            {
                if( lr == Left )
                {
                    return Arrow_T(C_arcs(c,Out,Left),Tip );
                }
                else
                {
                    return Arrow_T(C_arcs(c,In ,Left),Tail);
                }
            }
            else
            {
                if( lr == Left )
                {
                    return Arrow_T(C_arcs(c,Out,Right),Tip );
                }
                else
                {
                    return Arrow_T(C_arcs(c, In,Right),Tail);
                }
            }
        }
        
//        Arrow_T NextArc( const Int a, const bool tiptail )
//        {
//            PD_assert( ArcActiveQ(a) );
//            
//            const Int c = A_cross(a,tiptail);
//            
//            PD_assert( CrossingActiveQ(c) );
//            
//            // Find out whether arc a is connected to an <In>going or <Out>going port of c.
//            const bool io = (tiptail == Tip) ? In : Out;
//
//            // Find out whether arc a is connected to a <Left> or <Right> port of c.
//            const bool lr = (C_arcs(c,io,Left) == a) ? Left : Right;
//            
//            // We leave through the arc at the opposite port.
//            //If everything is set up correctly, the ourgoing arc points into the same direction as a.
//            return Arrow_T(C_arcs(c,!io,!lr), tiptail );
//        }
        
        Int NextArc( const Int a ) const
        {
            PD_assert( ArcActiveQ(a) );
            
            const Int c = A_cross( a, Tip );
            
            PD_assert( CrossingActiveQ(c) );

            // Find out whether arc a is connected to a <Left> or <Right> port of c.
            const bool lr = (C_arcs(c,In,Left) == a) ? Left : Right;
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the ourgoing arc points into the same direction as a.
            return C_arcs(c,Out,!lr);
        }
        
        Int PrevArc( const Int a ) const
        {
            PD_assert( ArcActiveQ(a) );
            
            const Int c = A_cross( a, Tail );
            
            PD_assert( CrossingActiveQ(c) );

            // Find out whether arc a is connected to a <Left> or <Right> port of c.
            const bool lr = (C_arcs(c,Out,Left) == a) ? Left : Right;
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the ourgoing arc points into the same direction as a.
            return C_arcs(c,In,!lr);
        }
        
//        Int NextCrossing( const Int a, const bool tiptail )
//        {
//            return A_cross(a,tiptail);
//        }
        
        Tensor1<Int,Int> ExportSwitchCandidates()
        {
            return Tensor1<Int,Int>( &switch_candidates[0], I(switch_candidates.size() ) );
        }
        
        Tensor1<Int,Int> ExportTouchedCrossings()
        {
            return Tensor1<Int,Int>( &touched_crossings[0], I(touched_crossings.size() ) );
        }
        
        
        Int CrossingLabelLookUp( const Int label )
        {
            print("CrossingLabelLookUp");
            
            PD_valprint("label",label);
            
            Int pos = std::distance(
                    C_label.begin(),
                    std::find( C_label.begin(), C_label.end(), label )
            );
            
            PD_valprint("pos",pos);
            
            if( pos >= C_label.Size() )
            {
                eprint(ClassName()+"::CrossingCrossingLookUp: Label "+ToString(label)+" not found. Returning 0");
                return 0;
            }
            else
            {
                PD_valprint("C_label[pos]",C_label[pos]);
                return pos;
            }
        }
        
        Int ArcLabelLookUp( const Int label )
        {
            Int pos = std::distance(
                    A_label.begin(),
                    std::find( A_label.begin(), A_label.end(), label)
            );
            
            if( pos >= A_label.Size() )
            {
                eprint(ClassName()+"::CrossingLabelLookUp: Label "+ToString(label)+" not found. Returning 0");
                return 0;
            }
            else
            {
                return pos;
            }
        }
        
        
        void Simplify()
        {
            ptic(ClassName()+"::Simplify()");
            
            faces_initialized = false;
            
            R_I_counter  = 0;
            R_II_counter = 0;
            tangle_move_counter = 0;
            
            while( !touched_crossings.empty() )
            {
                const Int c = touched_crossings.back();
                touched_crossings.pop_back();
                
                PD_assert( CheckCrossing(c) );
                
                const bool R_I = Reidemeister_I(c);
                
                if( !R_I )
                {
                    (void)Reidemeister_II(c);
                }
            }
            
            logprint( "Performed Reidemeister I  moves = " + ToString(R_I_counter ));
//            logprint( "Performed Reidemeister II moves = " + ToString(R_II_counter));
//            logprint( "Performed Tangle          moves = " + ToString(tangle_move_counter));
            
//            const bool connected_sum_Q = ConnectedSum();
//
//            if( connected_sum_Q )
//            {
//                print("A");
//                Simplify();
//            }
            
            ptoc(ClassName()+"::Simplify()");
        }
        
        
//        PlanarDiagram CreateCompressed()
//        {
//            // Creates a copy of the planar diagram with all inactive crossings and arcs removed.
//
//            PlanarDiagram pd ( crossing_count, unlink_count );
//
//            mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
//            mptr<CrossingState>       C_state_new = pd.C_state.data();
//            mptr<Int>                 C_label_new = pd.C_label.data();
//
//            mref<ArcContainer_T>      A_cross_new = pd.A_cross;
//            mptr<ArcState>            A_state_new = pd.A_state.data();
//            mptr<Int>                 A_label_new = pd.A_label.data();
//
//
//            // Lookup tables to translate the connectivity data.
//
//            // Will tell each old crossing where it has to go into the new array of crossings.
//            Tensor1<Int,Int> C_lookup ( initial_crossing_count );
//            // Will tell each old arc where it has to go into the new array of arcs.
//            Tensor1<Int,Int> A_lookup ( initial_arc_count );
//
//            // TODO: Rename the crossings and arcs so that they are ordered by first appearance during a curve traversal.
//
//            Int C_counter = 0;
//            for( Int c = 0; c < initial_crossing_count; ++c )
//            {
//                if( CrossingActiveQ(c) )
//                {
//                    // We abuse C_label_new for the moment in order to store where each crossing came from.
//                    C_label_new[C_counter] = c;
//                    // We have to remember for each crossing what its new position is.
//                    C_lookup[c] = C_counter;
//                    ++C_counter;
//                }
//            }
//
//            Int A_counter = 0;
//            for( Int a = 0; a < initial_arc_count; ++a )
//            {
//                if( ArcActiveQ(a) )
//                {
//                    // We abuse A_label_new for the moment in order to store where each arc came from.
//                    A_label_new[A_counter] = a;
//
//                    // We have to remember for each arc what its new position is.
//                    A_lookup[a] = A_counter;
//                    ++A_counter;
//                }
//            }
//
//            for( Int c = 0; c < crossing_count; ++c )
//            {
//                const Int c_from = C_label_new[c];
//
//                C_arcs_new(c,0,0) = A_lookup[ C_arcs(c_from,0,0) ];
//                C_arcs_new(c,0,1) = A_lookup[ C_arcs(c_from,0,1) ];
//                C_arcs_new(c,1,0) = A_lookup[ C_arcs(c_from,1,0) ];
//                C_arcs_new(c,1,1) = A_lookup[ C_arcs(c_from,1,1) ];
//
//                C_state_new[c] = C_state[c_from];
//
//                // Finally we overwrite C_label_new with the actual labels.
//                C_label_new[c] = C_label[c_from];
//            }
//
//            for( Int a = 0; a < arc_count; ++a )
//            {
//                const Int a_from = A_label_new[a];
//
//                PD_assert( ArcActiveQ(a_from) );
//
//                A_cross_new(a,0) = C_lookup[ A_cross(a_from,0) ];
//                A_cross_new(a,1) = C_lookup[ A_cross(a_from,1) ];
//
//                A_state_new[a]   = A_state[a_from];
//
////                PD_assert( pd.ArcActiveQ(a) );
//
//                // Finally we overwrite A_label_new with the actual labels.
//                A_label_new[a] = A_label[a_from];
//            }
//
//            pd.connected_sum_counter = connected_sum_counter;
//
//            return pd;
//        }
    
        
        PlanarDiagram CreateCompressed()
        {
            ptic( ClassName()+"::CreateCompressed");
            // Creates a copy of the planar diagram with all inactive crossings and arcs removed.
            
            // Relabeling is done as follows:
            // First active arc becomes first arc in new planar diagram.
            // The _tail_ of this arc becomes the new first crossing.
            // Then we follow all arcs in the component with NextArc(a); the new labels increase by one for each invisited arc. Same for the crossings.
            
            PlanarDiagram pd ( crossing_count, unlink_count );
            
            mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
            mptr<CrossingState>       C_state_new = pd.C_state.data();
            
            mref<ArcContainer_T>      A_cross_new = pd.A_cross;
            mptr<ArcState>            A_state_new = pd.A_state.data();
            
            Tensor1<Int,Int>  C_labels   ( C_arcs.Size(),  -1 );
            Tensor1<char,Int> A_visisted ( A_cross.Size(), false );
            
            const Int m = A_cross.Dimension(0);
            
            Int a_counter = 0;
            Int c_counter = 0;
            Int a_ptr     = 0;
            Int a         = 0;
            
            Int comp_counter = 0;
            
            while( a_ptr < m )
            {
                // Search for next arc that is active and has not yet been handled.
                while( ( a_ptr < m ) && ( A_visisted[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
                {
                    ++a_ptr;
                }
                
                if( a_ptr >= m )
                {
                    break;
                }
                
                ++comp_counter;
                
                a = a_ptr;
                
                {
                    const Int c_prev = A_cross(a,0);
                    const Int c      = C_labels[c_prev] = c_counter;
                    
                    C_state_new[c] = C_state[c_prev];
                    
                    ++c_counter;
                }
                
                // Cycle along all arcs in the link component, until we return where we started.
                do
                {
                    const Int c_prev = A_cross(a,0);
                    const Int c_next = A_cross(a,1);
                    
                    if( !A_visisted[a] )
                    {
                        A_state_new[a_counter] = ArcState::Active;
                        A_visisted[a] = true;
                    }
                    
                    
                    
                    if( C_labels[c_next] < 0 )
                    {
                        const Int c = C_labels[c_next] = c_counter;
                        
                        C_state_new[c] = C_state[c_next];
                        
                        ++c_counter;
                    }
                    
                    {
                        const Int  c   = C_labels[c_prev];
                        const bool lr  = C_arcs(c_prev,0,1) == a;
                        
                        C_arcs_new( c, Out, lr ) = a_counter;
                        
                        A_cross_new( a_counter, Tail ) = c;
                    }
                    
                    {
                        const Int  c  = C_labels[c_next];
                        const bool lr = C_arcs(c_next,1,1) == a;
                        
                        C_arcs_new( c, In, lr ) = a_counter;
                        
                        A_cross_new( a_counter, Tip ) = c;
                    }
                    
                    a = NextArc(a);
                    
                    ++a_counter;
                }
                while( a != a_ptr );
                
                ++a_ptr;
            }
            
            ptoc( ClassName()+"::CreateCompressed");
            
            return pd;
        }
        
       
        
        Tensor2<Int,Int> PDCode() const
        {
            const Int m = A_cross.Dimension(0);
            
            Tensor2<Int ,Int> pdcode     ( crossing_count, 4 );
            Tensor1<Int ,Int> C_labels   ( C_arcs.Size(), -1 );
            
            Tensor1<char,Int> A_visisted ( A_cross.Size(), false );
            
            
            Int a_counter = 0;
            Int c_counter = 0;
            Int a_ptr     = 0;
            Int a         = 0;
            
            Int comp_counter = 0;
            
            while( a_ptr < m )
            {
                // Search for next arc that is active and has not yet been handled.
                while( ( a_ptr < m ) && ( A_visisted[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
                {
                    ++a_ptr;
                }
                
                if( a_ptr >= m )
                {
                    break;
                }
                
                ++comp_counter;
                
                a = a_ptr;
                
                
                C_labels[A_cross(a,0)] = c_counter++;
                
                
                // Cycle along all arcs in the link component, until we return where we started.
                do
                {
                    const Int c_prev = A_cross(a,0);
                    const Int c_next = A_cross(a,1);
                    
                    A_visisted[a] = true;
                    
                    if( C_labels[c_next] < 0 )
                    {
                        C_labels[c_next] = c_counter++;
                    }
                    
                    {
                        const CrossingState sign  = C_state[c_prev];
                        const Int           label = C_labels[c_prev];
                        const bool          lr    = C_arcs(c_prev,0,1) == a;
                        
                        if( sign == CrossingState::Positive )
                        {
                            if( lr == 0 )
                            {
                                pdcode( label, 3 ) = a_counter;
                            }
                            else
                            {
                                pdcode( label, 2 ) = a_counter;
                            }
                        }
                        else if( sign == CrossingState::Negative )
                        {
                            if( lr == 0 )
                            {
                                pdcode( label, 2 ) = a_counter;
                            }
                            else
                            {
                                pdcode( label, 1 ) = a_counter;
                            }
                        }
                        
                    }
                    
                    {
                        const CrossingState sign  = C_state[c_next];
                        const Int           label = C_labels[c_next];
                        const bool          lr    = C_arcs(c_next,1,1) == a;
                        
                        if( sign == CrossingState::Positive )
                        {
                            if( lr == 0 )
                            {
                                pdcode( label, 0 ) = a_counter;
                            }
                            else
                            {
                                pdcode( label, 1 ) = a_counter;
                            }
                        }
                        else if( sign == CrossingState::Negative )
                        {
                            if( lr == 0 )
                            {
                                pdcode( label, 3 ) = a_counter;
                            }
                            else
                            {
                                pdcode( label, 0 ) = a_counter;
                            }
                        }
                        
                    }
                    
                    a = NextArc(a);
                    
                    ++a_counter;
                }
                while( a != a_ptr );
                
                ++a_ptr;
            }
            
            
            return pdcode;
        }
        
        Tensor1<Int,Int> OverArcIndices() const
        {
            const Int m = A_cross.Dimension(0);
            
            Tensor1<Int,Int>  over_arc_idx ( A_cross.Size() );
            
            Tensor1<Int,Int>  A_labels     ( A_cross.Size(), -1 );
            Tensor1<char,Int> A_visisted   ( A_cross.Size(), false );
            
            
            Int counter = 0;
            Int a_ptr   = 0;
//            Int a       = 0;
            
            while( a_ptr < m )
            {
                // Search for next arc that is active and has not yet been handled.
                while( ( a_ptr < m ) && ( A_visisted[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
                {
                    ++a_ptr;
                }
                
                if( a_ptr >= m )
                {
                    break;
                }
                
                Int a = a_ptr;
                
                Int tail = A_cross(a,0);
                
                // Go backwards until a goes under crossing tail.
                while(
                    (C_state[tail] == CrossingState::Positive)
                    ==
                    (C_arcs(tail,Out,0) == a)
                )
                {
                    a = PrevArc(a);
                    
                    tail = A_cross(a,0);
                }
                
                const Int a_0 = a;
                
                // Cycle along all arcs in the link component, until we return where we started.
                do
                {
                    const Int tip = A_cross(a,1);
                    
                    A_visisted[a] = true;
                    
                    over_arc_idx[a] = counter;
                    
                    const bool goes_underQ =
                        (C_state[tip] == CrossingState::Positive)
                        ==
                        (C_arcs(tip,In,0) == a);
                    
                    if( goes_underQ )
                    {
                        ++counter;
                    }
                    
                    a = NextArc(a);
                }
                while( a != a_0 );
                
                ++a_ptr;
            }
            
            
            return over_arc_idx;
        }
        
        Int Writhe() const
        {
            const Int writhe = 0;
            
            for( Int c = 0; c < C_state.Size(); ++c )
            {
                switch( C_state[c] )
                {
                    case CrossingState::Positive:
                    {
                        ++writhe;
                        break;
                    }
                    case CrossingState::Negative:
                    {
                        --writhe;
                        break;
                    }
                }
            }
            
            return writhe;
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("PlanarDiagram")+"<"+TypeName<Int>+">";
        }
        
    };

} // namespace KnotTools
