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
        
        static_assert(SignedIntQ<Int_>,"");

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
        Int twist_move_counter = 0;
        
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
                    
                    PD_assert( inter.handedness > SInt(0) || inter.handedness < SInt(0) );
                    
                    bool righthandedQ = inter.handedness > SInt(0);

                    
                    
                    /*
                    *
                    *    negative         positive
                    *    right-handed     left-handed
                    *    .       .        .       .
                    *    .       .        .       .
                    *    +       +        +       +
                    *     ^     ^          ^     ^
                    *      \   /            \   /
                    *       \ /              \ /
                    *        /                \
                    *       / \              / \
                    *      /   \            /   \
                    *     /     \          /     \
                    *    +       +        +       +
                    *    .       .        .       .
                    *    .       .        .       .
                    *
                    */
                    
                    C_state[c] = righthandedQ ? CrossingState::RightHanded : CrossingState::LeftHanded;
                    A_state[a] = ArcState::Active;
                    
                    /*
                    * righthandedQ == true and overQ == true:
                    *
                    *        C_arcs(c,Out,Left)  .       .  C_arcs(c,Out,Right) = b
                    *                            .       .
                    *                            +       +
                    *                             ^     ^
                    *                              \   /
                    *                               \ /
                    *                                /
                    *                               / \
                    *                              /   \
                    *                             /     \
                    *                            +       +
                    *                            .       .
                    *     a = C_arcs(c,In,Left)  .       .  C_arcs(c,In,Right)
                    */
                    
                    const bool over_in_side = (righthandedQ == overQ) ? Left : Right ;
                    
                    
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

        /*!
         * @brief Returns how many crossings there were in the original planar diagram, before any simplifications.
         */
        
        Int InitialCrossingCount() const
        {
            return initial_crossing_count;
        }
        
        /*!
         * @brief The number of crossings in the planar diagram.
         */
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
        
        /*!
         * @brief Returns how many Reidemeister I moves have performed so far.
         */
        
        Int Reidemeister_I_Counter() const
        {
            return R_I_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister II moves have performed so far.
         */
        
        Int Reidemeister_II_Counter() const
        {
            return R_II_counter;
        }
        
        /*!
         * @brief Returns how many twist moves have performed so far.
         *
         * See TwistMove.hpp for details.
         */
        
        Int TwistMoveCounter() const
        {
            return twist_move_counter;
        }
   
        mref<Tensor1<Int,Int>> CrossingLabels()
        {
            return C_label;
        }
        
        cref<Tensor1<Int,Int>> CrossingLabels() const
        {
            return C_label;
        }
        
        /*!
         * @brief Returns the list of crossings as a reference to a Tensor3 object.
         *
         * The `c`-th crossing is stored in `Crossings()(c,i,j)`, `i = 0,1`, `j = 0,1` as follows:
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
         */
        
        mref<CrossingContainer_T> Crossings()
        {
            return C_arcs;
        }
        
        /*!
         * @brief Returns the list of crossings as a reference to a Tensor3 object.
         *
         * The `c`-th crossing is stored in `Crossings()(c,i,j)`, `i = 0,1`, `j = 0,1` as follows:
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
         */
        
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
        
        
        /*!
         * @brief Returns how many arcs there were in the original planar diagram, before any simplifications.
         */
        
        Int InitialArcCount() const
        {
            return initial_arc_count;
        }
        
        /*!
         * @brief Returns the number of arcs in the planar diagram.
         */
        
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
        
        /*!
         * @brief Returns the arcs that connect the crossings as a reference to a Tensor2 object.
         *
         * The `a`-th arc is stored in `Arcs()(a,i)`, `i = 0,1`, in the following way:
         *
         *                          a
         *       Arc()(a,0) X -------------> X Arc()(a,0)
         *           =                             =
         *      Arc()(a,Tail)                 Arc()(a,Tip)
         *
         * This means that arc `a` leaves crossing `Arc()(a,0)` and enters `Arc()(a,1)`.
         */
        
        mref<ArcContainer_T> Arcs()
        {
            return A_cross;
        }
        
        /*!
         * @brief Returns the arcs that connect the crossings as a reference to a Tensor2 object.
         *
         * The `a`-th arc is stored in `Arcs()(a,i)`, `i = 0,1`, in the following way:
         *
         *                          a
         *       Arc()(a,0) X -------------> X Arc()(a,0)
         *           =                             =
         *      Arc()(a,Tail)                 Arc()(a,Tip)
         *
         * This means that arc `a` leaves crossing `Arc()(a,0)` and enters `Arc()(a,1)`.
         */
        
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
                (C_state[c] == CrossingState::RightHanded)
                ||
                (C_state[c] == CrossingState::LeftHanded)
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
        
        /*!
         * @brief Returns the crossing you would get to by starting at crossing `c` and
         * leaving trough the
         */
        
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
//            twise_move_counter = 0;
            
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
            
            pvalprint( "Number of crossings  ", crossing_count      );
            pvalprint( "Number of arcs       ", arc_count           );
            
            faces_initialized = false;
            
            R_I_counter  = 0;
            R_II_counter = 0;
            twist_move_counter = 0;
            
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
            
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
            
#pragma clang diagnostic pop
            
            pvalprint( "Reidemeister I  moves", R_I_counter         );
            pvalprint( "Reidemeister II moves", R_II_counter        );
            pvalprint( "Twist           moves", twist_move_counter  );
            
            pvalprint( "Number of crossings  ", crossing_count      );
            pvalprint( "Number of arcs       ", arc_count           );
//            const bool connected_sum_Q = ConnectedSum();
//
//            if( connected_sum_Q )
//            {
//                print("A");
//                Simplify();
//            }
            
            ptoc(ClassName()+"::Simplify()");
        }
        
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
        
        
        /*!
         * @brief Returns the pd-codes of the crossing as Tensor2 object.
         *
         *  The pd-code of crossing `c` is given by
         *
         *    `{ PDCode()(c,0), PDCode()(c,1), PDCode()(c,2), PDCode()(c,3) }`
         *
         *  TODO: What is the convention here? I think I made this compatible with
         *  Dror's _KnotTheory_ package.
         *
         */
       
        
        Tensor2<Int,Int> PDCode() const
        {
            ptic( ClassName()+"::PDCode");
            
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
                        const CrossingState state = C_state[c_prev];
                        const Int           label = C_labels[c_prev];
                        const bool          lr    = C_arcs(c_prev,0,1) == a;
                        
                        if( state == CrossingState::RightHanded )
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
                        else if( state == CrossingState::LeftHanded )
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
                        const CrossingState state = C_state[c_next];
                        const Int           label = C_labels[c_next];
                        const bool          lr    = C_arcs(c_next,1,1) == a;
                        
                        if( state == CrossingState::RightHanded )
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
                        else if( state == CrossingState::LeftHanded )
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
            
            ptoc( ClassName()+"::PDCode");
            
            return pdcode;
        }
        
        /*!
         * @brief Tells every orc to which over-arc it belongs.
         *
         * (An over-arc is a consecutive sequence of arcs that pass over.)
         */
        
        cref<Tensor1<Int,Int>> OverArcIndices() const
        {
            std::string tag ( "OverArcIndices" );
            
            if( !this->InCacheQ(tag) )
            {
                ptic(ClassName()+"::OverArcIndices()");
                
                const Int m = A_cross.Dimension(0);
                
                Tensor1<Int ,Int> over_arc_idx ( m );
                Tensor1<Int ,Int> A_labels     ( m, -1 );
                Tensor1<char,Int> A_visisted   ( m, false );
                
                
                Int counter = 0;
                Int a_ptr   = 0;
                
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
                        (C_state[tail] == CrossingState::RightHanded)
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
                            (C_state[tip] == CrossingState::RightHanded)
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
                
                this->SetCache( tag, std::move( over_arc_idx ) );
                
                ptoc(ClassName()+"::OverArcIndices()");
            }
            
            return this->template GetCache<Tensor1<Int,Int>>(tag);
        }
        
        Int Writhe() const
        {
            const Int writhe = 0;
            
            for( Int c = 0; c < C_state.Size(); ++c )
            {
                switch( C_state[c] )
                {
                    case CrossingState::RightHanded:
                    {
                        ++writhe;
                        break;
                    }
                    case CrossingState::LeftHanded:
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
