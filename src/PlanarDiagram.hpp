#pragma  once

#include <cassert>

namespace KnotTools
{

#ifdef PD_VERBOSE
    #define PD_print( s ) Tools::print(s);
#else
    #define PD_print( s )
#endif
    
#ifdef PD_VERBOSE
    #define PD_valprint( key, val ) Tools::valprint( key, val )
#else
    #define PD_valprint( key, val )
#endif
    
#ifdef PD_VERBOSE
    #define PD_wprint( s ) Tools::wprint(s);
#else
    #define PD_wprint( s )
#endif
    
    
#ifdef PD_DEBUG
    #define PD_assert( s ) if(!(s)) { Tools::eprint( "PD_assert failed: " + std::string(#s) ); }
#else
    #define PD_assert( s )
#endif
    
    
    template<typename Int_>
    class alignas( ObjectAlignment ) PlanarDiagram : public CachedObject
    {
    public:
        
        static_assert(SignedIntQ<Int_>,"");

        using Int  = Int_;
        using Sint = int;
        
        using Base_T  = CachedObject;
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
#if defined(PD_USE_TOUCHING)
        std::vector<Int> touched_crossings;
//        std::vector<Int> touched_arcs;
//        std::vector<Int> switch_candidates;
#endif
        // Data for the faces and the dual graph
        
        Tensor2<Int,Int> A_faces; // Convention: Left face first.
        
        Tensor1<Int,Int> face_arcs    {0};
        Tensor1<Int,Int> face_arc_ptr {2,0};
        
        Tensor1<Int,Int> comp_arcs    {0};
        Tensor1<Int,Int> comp_arc_ptr {2,0};
        
        Tensor1<Int,Int> arc_comp     {0};
        
        bool faces_initialized = false;
        
        Int connected_sum_counter = 0;
        
    public:
        
        PlanarDiagram() = default;
        
        virtual ~PlanarDiagram() override = default;
        
        
    protected:
        
        /*! @brief This constructor is supposed to only allocate all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
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
        
    public:
  
        // Copy constructor
        PlanarDiagram( const PlanarDiagram & other ) = default;
        
        friend void swap(PlanarDiagram & A, PlanarDiagram & B ) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( static_cast<CachedObject &>(A), static_cast<CachedObject &>(B) );
            
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
            
            swap( A.R_I_counter           , B.R_I_counter            );
            swap( A.R_II_counter          , B.R_II_counter           );
            swap( A.twist_move_counter    , B.twist_move_counter     );
            
#if defined(PD_USE_TOUCHING)
            swap( A.touched_crossings     , B.touched_crossings      );
//            swap( A.touched_arcs          , B.touched_arcs           );
//            swap( A.switch_candidates     , B.switch_candidates      );
#endif
    
            swap( A.A_faces               , B.A_faces                );
            
            swap( A.face_arcs             , B.face_arcs              );
            swap( A.face_arc_ptr          , B.face_arc_ptr           );
            
            swap( A.comp_arcs             , B.comp_arcs              );
            swap( A.comp_arc_ptr          , B.comp_arc_ptr           );
            
            swap( A.arc_comp              , B.arc_comp               );
            
            swap( A.faces_initialized     , B.faces_initialized      );
            
            swap( A.connected_sum_counter , B.connected_sum_counter  );
        }
        
        // Move constructor
        PlanarDiagram( PlanarDiagram && other ) noexcept
        :   PlanarDiagram()
        {
            swap(*this, other);
        }

        /* Copy assignment operator */
        PlanarDiagram & operator=( PlanarDiagram other ) noexcept
        {   //                                     ^
            //                                     |
            // Use the copy constructor     -------+
            swap( *this, other );
            return *this;
        }
        
        
        /*! @brief Construction from `Link_2D` object.
         */
        
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
        
        /*! @brief Construction from PD codes and handedness of crossings.
         *
         *  @param pd_codes_ Integer array of length `5 * crossing_count_`.
         *  There is one 5-tuple for each crossing.
         *  The first 4 entries in each 5-tuple store the actual PD code.
         *  The last entry gives the handedness of the crossing:
         *    >  0 for a right-handed crossing
         *    <= 0 for a left-handed crossing
         *
         *  @param crossing_count_ Number of crossings in the diagram.
         *
         *  @param unlink_count_ Number of unlinks in the diagram. (This is necessary as PD codes cannot track trivial unlinks.
         *
         */
        
        template<typename ExtInt, typename ExtInt2, typename ExtInt3>
        PlanarDiagram(
            cptr<ExtInt> pd_codes_,
            const ExtInt2 crossing_count_,
            const ExtInt3 unlink_count_
        )
        :   PlanarDiagram(
                int_cast<Int>(crossing_count_),
                int_cast<Int>(unlink_count_)
            )
        {
            static_assert( IntQ<ExtInt>, "" );
            
            if( crossing_count_ <= 0 )
            {
                return ;
            }
            
            A_cross.Fill(-1);

            // The maximally allowed arc index.
            const Int max_a = 2 * crossing_count_ - 1;
            
            for( Int c = 0; c < crossing_count; ++c )
            {
                Int X [5];
                copy_buffer<5>( &pd_codes_[5*c], &X[0] );
                
                if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
                {
                    eprint( ClassName()+"(): There is a pd code entry that is greater than 2 * number of crosssings - 1." );
                    return;
                }
                
                bool righthandedQ = (X[4] > 0);
                
                Int C [2][2];
                
                if( righthandedQ )
                {
                    C_state[c] = CrossingState::RightHanded;
                    
//                    if( A_cross(X[0],Tip  ) != -1 )
//                    {
//                        dump(c);
//                        dump(A_cross(X[0],Tip  ));
//                        dump(X[0]);
//                    }
//                    
//                    if( A_cross(X[1],Tail ) != -1 )
//                    {
//                        dump(c);
//                        dump(A_cross(X[1],Tail ));
//                        dump(X[1]);
//                    }
//                    
//                    if( A_cross(X[2],Tail ) != -1 )
//                    {
//                        dump(c);
//                        dump(A_cross(X[2],Tail ));
//                        dump(X[2]);
//                    }
//                    
//                    if( A_cross(X[3],Tip  ) != -1 )
//                    {
//                        dump(c);
//                        dump(A_cross(X[3],Tip  ));
//                        dump(X[3]);
//                    }
                    
                    /*
                     *    X[2]           X[1]
                     *          ^     ^
                     *           \   /
                     *            \ /
                     *             / <--- c
                     *            ^ ^
                     *           /   \
                     *          /     \
                     *    X[3]           X[0]
                     */
                    
                    A_cross(X[0],Tip  ) = c;
                    A_cross(X[1],Tail ) = c;
                    A_cross(X[2],Tail ) = c;
                    A_cross(X[3],Tip  ) = c;

                    C[Out][Left ] = X[2];
                    C[Out][Right] = X[1];
                    C[In ][Left ] = X[3];
                    C[In ][Right] = X[0];
                }
                else // left-handed
                {
                    C_state[c] = CrossingState::LeftHanded;
                    
                    if( A_cross(X[0],Tip  ) != -1 )
                    {
                        dump(c);
                        dump(A_cross(X[0],Tip  ));
                        dump(X[0]);
                    }
                    
                    if( A_cross(X[1],Tip  ) != -1 )
                    {
                        dump(c);
                        dump(A_cross(X[1],Tip  ));
                        dump(X[1]);
                    }
                    
                    if( A_cross(X[2],Tail ) != -1 )
                    {
                        dump(c);
                        dump(A_cross(X[2],Tail ));
                        dump(X[2]);
                    }
                    
                    if( A_cross(X[3],Tail ) != -1 )
                    {
                        dump(c);
                        dump(A_cross(X[3],Tail ));
                        dump(X[3]);
                    }
                    
                    /*
                     *    X[3]           X[2]
                     *          ^     ^
                     *           \   /
                     *            \ /
                     *             \ <--- c
                     *            ^ ^
                     *           /   \
                     *          /     \
                     *    X[0]           X[1]
                     */
                    
                    A_cross(X[0],Tip  ) = c;
                    A_cross(X[1],Tip  ) = c;
                    A_cross(X[2],Tail ) = c;
                    A_cross(X[3],Tail ) = c;
                    
                    C[Out][Left ] = X[3];
                    C[Out][Right] = X[2];
                    C[In ][Left ] = X[0];
                    C[In ][Right] = X[1];
                }
                
                copy_buffer<4>( &C[0][0], C_arcs.data(c) );
            }
            
            fill_buffer( A_state.data(), ArcState::Active, arc_count );
        }
        
        
        /*!
         * @brief Returns the number of trivial unlinks in the diagram, i.e., unknots that do not share any crossings with other link components.
         */
        
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
        
        /*!
         * @brief Returns the states of the crossings.
         *
         *  The states that a crossing can have are:
         *
         *  - `CrossingState::RightHanded`
         *  - `CrossingState::LeftHanded`
         *  - `CrossingState::Unitialized`
         *
         * `CrossingState::Unitialized` means that the crossing has been deactivated by topological manipulations.
         */
        
        mref<CrossingStateContainer_T> CrossingStates()
        {
            return C_state;
        }
        
        /*!
         * @brief Returns the states of the crossings.
         *
         *  The states that a crossing can have are:
         *
         *  - `CrossingState::RightHanded`
         *  - `CrossingState::LeftHanded`
         *  - `CrossingState::Unitialized`
         *
         * `CrossingState::Unitialized` means that the crossing has been deactivated by topological manipulations.
         */
        
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

        /*!
         * @brief Returns the states of the arcs.
         *
         *  The states that an arc can have are:
         *
         *  - `ArcState::Active`
         *  - `ArcState::Inactive`
         *
         * `CrossingState::Inactive` means that the arc has been deactivated by topological manipulations.
         */
        
        mref<ArcStateContainer_T> ArcStates()
        {
            return A_state;
        }
        
        /*!
         * @brief Returns the states of the arcs.
         *
         *  The states that an arc can have are:
         *
         *  - `ArcState::Active`
         *  - `ArcState::Inactive`
         *
         * `CrossingState::Inactive` means that the arc has been deactivated by topological manipulations.
         */
        
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
        
        /*!
         * @brief Checks whether crossing `c` is still active.
         */
        
        bool CrossingActiveQ( const Int c ) const
        {
            return (
                (C_state[c] == CrossingState::RightHanded)
                ||
                (C_state[c] == CrossingState::LeftHanded)
            );
        }

        bool OppositeCrossingSignsQ( const Int c_0, const Int c_1 ) const
        {
            return ( ToUnderlying(C_state[c_0]) == -ToUnderlying(C_state[c_1]) );
        }
        
        
        std::string ArcString( const Int a ) const
        {
            return "arc " +ToString(a) +" = { " +
               ToString(A_cross(a,Tail))+", "+ToString(A_cross(a,Tip))+" } ";
        }
        
        /*!
         * @brief Checks whether arc `a` is still active.
         */
        
        bool ArcActiveQ( const Int a ) const
        {
            return A_state[a] == ArcState::Active;
        }
        
        /*!
         * @brief Returns the crossing you would get to by starting at crossing `c` and
         * leaving trough the
         */
        
        Int NextCrossing( const Int c, bool io, bool lr ) const
        {
            PD_assert( CrossingActiveQ(c) );

            const Int a = C_arcs(c,io,lr);

            PD_assert( ArcActiveQ(a) );
            
            const Int c_next = A_cross( a, ( io == In ) ? Tail : Tip );

            PD_assert( CrossingActiveQ(c_next) );
            
            return c_next;
        }

        
    protected:
        
        /*!
         * @brief Deactivates crossing `c`. Only for internal use.
         */
        
        void DeactivateCrossing( const Int c )
        {
            if( CrossingActiveQ(c) )
            {
                --crossing_count;
                C_state[c] = CrossingState::Unitialized;
            }
            else
            {
#if defined(PD_DEBUG)
                wprint("Attempted to deactivate already inactive crossing " + ToString(c) + ".");
#endif
            }
        }
        
        /*!
         * @brief Deactivates arc `a`. Only for internal use.
         */
        
        void DeactivateArc( const Int a )
        {
            if( ArcActiveQ(a) )
            {
                --arc_count;
                A_state[a] = ArcState::Inactive;
            }
            else
            {
#if defined(PD_DEBUG)
                wprint("Attempted to deactivate already inactive arc " + ToString(a) + ".");
#endif
            }
        }
        
    public:
        
#include "PlanarDiagram/Reconnect.hpp"
#include "PlanarDiagram/Checks.hpp"
#include "PlanarDiagram/Reidemeister_I.hpp"
#include "PlanarDiagram/Reidemeister_II.hpp"
//#include "PlanarDiagram/PassMove.hpp"
        
//#include "PlanarDiagram/Break.hpp"
//#include "PlanarDiagram/Switch.hpp"
        
#include "PlanarDiagram/Faces.hpp"
//#include "PlanarDiagram/ConnectedSum.hpp"
        
        void PushAllCrossings()
        {
#if defined(PD_USE_TOUCHING)
            touched_crossings.reserve( initial_crossing_count );
            
            for( Int i = initial_crossing_count-1; i >= 0; --i )
            {
                touched_crossings.push_back( i );
            }
#endif
        }
        
        void PushRemainingCrossings()
        {
#if defined(PD_USE_TOUCHING)
            Int counter = 0;
            for( Int i = initial_crossing_count-1; i >= 0; --i )
            {
                if( CrossingActiveQ(i) )
                {
                    ++counter;
                    touched_crossings.push_back( i );
                }
            }
#endif
        }
        

        /*!
         * @brief Returns the arc following arc `a` by going to the crossing at the tip of `a` and then turning left.
         */
        
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
        
        /*!
         * @brief Returns the arc preceding arc `a`, i.e., the arc reached by going straight through the crossing at the tip of `a`.
         */
        
        Int NextArc( const Int a ) const
        {
            PD_assert( ArcActiveQ(a) );
            
            const Int c = A_cross( a, Tip );
            
            PD_assert( CrossingActiveQ(c) );

            // Find out whether arc a is connected to a <Left> or <Right> port of c.
            const bool lr = (C_arcs(c,In,Left) == a) ? Left : Right;
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the outgoing arc points into the same direction as a.
        
            const Int a_next = C_arcs(c,Out,!lr);
            
            PD_assert( ArcActiveQ(a_next) );
            
            return a_next;
        }
        
        /*!
         * @brief Returns the arc preceding arc `a`, i.e., the arc reached by going straight through the crossing at the tail of `a`.
         */
        
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
        
//        Tensor1<Int,Int> ExportSwitchCandidates()
//        {
//            return Tensor1<Int,Int>( &switch_candidates[0], I(switch_candidates.size() ) );
//        }
//        
//        Tensor1<Int,Int> ExportTouchedCrossings()
//        {
//            return Tensor1<Int,Int>( &touched_crossings[0], I(touched_crossings.size() ) );
//        }
        
        
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
        
        /*!
         * @brief Attempts to simplify the planar diagram by applying some standard moves.
         *
         *  So far, Reidemeister I and Reidemeister II moves are support as
         *  well as a move we call "twist move". See the ASCII-art in
         *  TwistMove.hpp for more details.
         *
         */
        
        void Simplify()
        {
            ptic(ClassName()+"::Simplify");
            
//            pvalprint( "Number of crossings  ", crossing_count      );
//            pvalprint( "Number of arcs       ", arc_count           );
            
            R_I_counter  = 0;
            R_II_counter = 0;
            twist_move_counter = 0;
            
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
            
#if defined(PD_USE_TOUCHING)
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
#else
            Int old_counter = -1;
            Int counter = 0;
            
            while( counter != old_counter )
            {
                old_counter = counter;
                
                for( Int c = 0; c < initial_crossing_count; ++c )
                {
                    const bool R_I = Reidemeister_I(c);
                    
                    counter += R_I;
                    
                    if( !R_I )
                    {
                        counter += Reidemeister_II(c);
                    }
                }
            }
#endif
            
#pragma clang diagnostic pop
            
//            pvalprint( "Reidemeister I  moves", R_I_counter         );
//            pvalprint( "Reidemeister II moves", R_II_counter        );
//            pvalprint( "Twist           moves", twist_move_counter  );
//            
//            pvalprint( "Number of crossings  ", crossing_count      );
//            pvalprint( "Number of arcs       ", arc_count           );
            
//            const bool connected_sum_Q = ConnectedSum();
//
//            if( connected_sum_Q )
//            {
//                print("A");
//                Simplify();
//            }

            if( (R_I_counter > 0) || (R_II_counter > 0) || (twist_move_counter > 0) )
            {
                faces_initialized = false;
                
                this->ClearCache();
            }
            
            ptoc(ClassName()+"::Simplify");
        }
        
        /*!
         * @brief Creates a copy of the planar diagram with all inactive crossings and arcs removed.
         *
         * Relabeling is done as follows:
         * First active arc becomes first arc in new planar diagram.
         * The _tail_ of this arc becomes the new first crossing.
         * Then we follow all arcs in the component with `NextArc(a)`.
         * The new labels increase by one for each invisited arc.
         * Same for the crossings.
         */
        
        PlanarDiagram CreateCompressed()
        {
            ptic( ClassName()+"::CreateCompressed");
            
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
         *    `{ PDCode()(c,0), PDCode()(c,1), PDCode()(c,2), PDCode()(c,3), PDCode()(c,4) }`
         *
         *  The first 4 entries are the arcs attached to crossing c.
         *  `PDCode()(c,0)` is the incoming arc that goes under.
         *  This should be compatible with Dror Bar-Natan's _KnotTheory_ package.
         *
         *  The last entry stores the handedness of the crossing:
         *    +1 for a right-handed crossing,
         *    -1 for a left-handed crossing.
         *
         *
         */
        
        Tensor2<Int,Int> PDCode() const
        {
            ptic( ClassName()+"::PDCode" );
            
            const Int m = A_cross.Dimension(0);
            
            Tensor2<Int ,Int> pdcode     ( crossing_count, 5 );
//            Tensor1<Int ,Int> C_labels   ( A_cross.Max()+1, -1 );
            Tensor1<Int ,Int> C_labels   ( m/2, -1 );
            Tensor1<char,Int> A_visisted ( m, false           );
            
            Int a_counter = 0;
            Int c_counter = 0;
            Int a_ptr     = 0;
            Int a         = 0;
            
//            logdump( C_arcs  );
//            logdump( C_state );
//            logdump( A_cross );
//            logdump( A_state );
            
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
                
                a = a_ptr;
                
                
                C_labels[A_cross(a,Tail)] = c_counter++;
                
                
                // Cycle along all arcs in the link component, until we return where we started.
                do
                {
                    const Int c_prev = A_cross(a,Tail);
                    const Int c_next = A_cross(a,Tip );
                    
                    A_visisted[a] = true;
                    
                    if( C_labels[c_next] < 0 )
                    {
                        C_labels[c_next] = c_counter++;
                    }
                    
                    {
                        const CrossingState state = C_state[c_prev];
                        const Int           c     = C_labels[c_prev];

                        pdcode( c, 4 ) = (state == CrossingState::RightHanded) ? 1 : -1;
                        
                        const bool side  = (C_arcs(c_prev,Out,Right) == a) ? Right : Left;
        
                        if( state == CrossingState::RightHanded )
                        {
                            if( side == Left )
                            {
                                /*
                                 *
                                 * a_counter
                                 *     =
                                 *    X[2]           X[1]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             / <--- c = C_labels[c_prev]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[3]           X[0]
                                 *
                                 */
                                
                                pdcode( c, 2 ) = a_counter;
                            }
                            else // if( side == Right )
                            {
                                /*
                                 *
                                 *                a_counter
                                 *                    =
                                 *    X[2]           X[1]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             / <--- c = C_labels[c_prev]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[3]           X[0]
                                 *
                                 */
                                
                                pdcode( c, 1 ) = a_counter;
                            }
                        }
                        else if( state == CrossingState::LeftHanded )
                        {
                            if( side == Left )
                            {
                                /*
                                 *
                                 * a_counter
                                 *     =
                                 *    X[3]           X[2]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             \ <--- c = C_labels[c_prev]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[0]           X[1]
                                 *
                                 */
                                
                                pdcode( c, 3 ) = a_counter;
                            }
                            else // if( side == Right )
                            {
                                /*
                                 *
                                 *                a_counter
                                 *                    =
                                 *    X[3]           X[2]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             \ <--- c = C_labels[c_prev]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[0]           X[1]
                                 *
                                 */
                                
                                pdcode( c, 2 ) = a_counter;
                            }
                        }
                        
                    }
                    
                    {
                        const CrossingState state = C_state[c_next];
                        const Int           c     = C_labels[c_next];
                        
                        pdcode( c, 4 ) = (state == CrossingState::RightHanded) ? 1 : -1;
                        
                        const bool side  = (C_arcs(c_next,In,Right)) == a ? Right : Left;
                        
                        if( state == CrossingState::RightHanded )
                        {
                            if( side == Left )
                            {
                                /*
                                 *
                                 *    X[2]           X[1]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             / <--- c = C_labels[c_next]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[3]           X[0]
                                 *     =
                                 * a_counter
                                 *
                                 */
                                
                                pdcode( c, 3 ) = a_counter;
                            }
                            else // if( side == Right )
                            {
                                /*
                                 *
                                 *    X[2]           X[1]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             / <--- c = C_labels[c_next]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[3]           X[0]
                                 *                    =
                                 *                a_counter
                                 *
                                 */
                                
                                pdcode( c, 0 ) = a_counter;
                            }
                        }
                        else if( state == CrossingState::LeftHanded )
                        {
                            if( side == Left )
                            {
                                /*
                                 *
                                 *    X[3]           X[2]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             \ <--- c = C_labels[c_next]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[0]           X[1]
                                 *     =
                                 * a_counter
                                 *
                                 */
                                
                                pdcode( c, 0 ) = a_counter;
                            }
                            else // if( lr == Right )
                            {
                                /*
                                 *
                                 *    X[3]           X[2]
                                 *          ^     ^
                                 *           \   /
                                 *            \ /
                                 *             \ <--- c = C_labels[c_next]
                                 *            ^ ^
                                 *           /   \
                                 *          /     \
                                 *    X[0]           X[1]
                                 *                    =
                                 *                a_counter
                                 *
                                 */
                                
                                pdcode( c, 1 ) = a_counter;
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
         * @brief Returns an array of that tells every cross which over-arcs end or start in it.
         *
         *  More precisely, crossing `c` has the outgoing over-arcs `CrossingOverArcs()(c,0,0)` and `CrossingOverArcs()(c,0,1)`
         *  and the incoming over-arcs `CrossingOverArcs()(c,1,0)` and `CrossingOverArcs()(c,1,1)`.
         *
         *  (An over-arc is a maximal consecutive sequence of arcs that pass over.)
         */
        
        Tensor3<Int,Int> CrossingOverArcs() const
        {
            ptic(ClassName()+"::CrossingOverArcs");
            
            const Int n = C_arcs.Dimension(0);
            const Int m = A_cross.Dimension(0);
            
            Tensor3<Int ,Int> C_over_arcs  ( n, 2, 2, -1 );
            Tensor1<Int ,Int> A_labels     ( m, -1 );
            Tensor1<char,Int> A_visisted   ( m, false );
            
            Int oa_counter = 0;
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
                    const Int tail = A_cross(a,0);
                    const Int tip  = A_cross(a,1);
                    
                    A_visisted[a] = true;
                    
                    if( C_arcs(tail,Out,Left) == a )
                    {
                        C_over_arcs(tail,Out,Left ) = oa_counter;
                    }
                    else
                    {
                        C_over_arcs(tail,Out,Right) = oa_counter;
                    }
                    
                    const bool b = (C_arcs(tip,In,Left) == a);
                    
                    const bool goes_underQ =
                        (C_state[tip] == CrossingState::RightHanded)
                        ==
                        b;
                    
                    if( b )
                    {
                        C_over_arcs(tip,In,Left ) = oa_counter;
                    }
                    else
                    {
                        C_over_arcs(tip,In,Right) = oa_counter;
                    }
                    
                    oa_counter += goes_underQ;
                    
                    a = NextArc(a);
                }
                while( a != a_0 );
                
                ++a_ptr;
            }
            
            ptoc(ClassName()+"::CrossingOverArcs");
            
            return C_over_arcs;
        }
        
        /*!
         * @brief Returns an array of that tells every cross which under-arcs end or start in it.
         *
         *  More precisely, crossing `c` has the outgoing under-arcs `CrossingOverArcs()(c,0,0)` and `CrossingOverArcs()(c,0,1)`
         *  and the incoming under-arcs `CrossingOverArcs()(c,1,0)` and `CrossingOverArcs()(c,1,1)`.
         *
         *  (An under-arc is a maximal consecutive sequence of arcs that pass under.)
         */
        
        Tensor3<Int,Int> CrossingUnderArcs() const
        {
            ptic(ClassName()+"::CrossingUnderArcs");
            
            const Int n = C_arcs.Dimension(0);
            const Int m = A_cross.Dimension(0);
            
            Tensor3<Int ,Int> C_under_arcs ( n, 2, 2, -1 );
            Tensor1<Int ,Int> A_labels     ( m, -1 );
            Tensor1<char,Int> A_visisted   ( m, false );
            
            Int oa_counter = 0;
            Int a_ptr   = 0;
            
            eprint( ClassName()+"::CrossingUnderArcs: Not implemented yet. Returning garbage." );
            
//            while( a_ptr < m )
//            {
//                // Search for next arc that is active and has not yet been handled.
//                while( ( a_ptr < m ) && ( A_visisted[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
//                {
//                    ++a_ptr;
//                }
//                
//                if( a_ptr >= m )
//                {
//                    break;
//                }
//                
//                Int a = a_ptr;
//                
//                Int tail = A_cross(a,0);
//                
//                // Go backwards until a goes under crossing tail.
//                while(
//                    (C_state[tail] == CrossingState::RightHanded)
//                    ==
//                    (C_arcs(tail,Out,0) == a)
//                )
//                {
//                    a = PrevArc(a);
//                    
//                    tail = A_cross(a,0);
//                }
//                
//                const Int a_0 = a;
//                
//                // Cycle along all arcs in the link component, until we return where we started.
//                do
//                {
//                    const Int tail = A_cross(a,0);
//                    const Int tip  = A_cross(a,1);
//                    
//                    A_visisted[a] = true;
//                    
//                    if( C_arcs(tail,Out,Left) == a )
//                    {
//                        C_over_arcs(tail,Out,Left ) = oa_counter;
//                    }
//                    else
//                    {
//                        C_over_arcs(tail,Out,Right) = oa_counter;
//                    }
//                    
//                    const bool b = (C_arcs(tip,In,Left) == a);
//                    
//                    const bool goes_underQ =
//                        (C_state[tip] == CrossingState::RightHanded)
//                        ==
//                        b;
//                    
//                    if( b )
//                    {
//                        C_over_arcs(tip,In,Left ) = oa_counter;
//                    }
//                    else
//                    {
//                        C_over_arcs(tip,In,Right) = oa_counter;
//                    }
//                    
//                    oa_counter += goes_underQ;
//                    
//                    a = NextArc(a);
//                }
//                while( a != a_0 );
//                
//                ++a_ptr;
//            }
            
            ptoc(ClassName()+"::CrossingUnderArcs");
            
            return C_under_arcs;
        }
        
        /*!
         * @brief Returns an array that tells every arc to which over-arc it belongs.
         *
         *  More precisely, arc `a` belongs to over-arc number `OverArcIndices()[a]`.
         *
         *  (An over-arc is a maximal consecutive sequence of arcs that pass over.)
         */
        
        Tensor1<Int,Int> OverArcIndices() const
        {
            ptic(ClassName()+"::OverArcIndices");
            
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
            
            ptoc(ClassName()+"::OverArcIndices");
            
            return over_arc_idx;
        }
        
        /*!
         * @brief Computes the writhe = number of right-handed crossings - number of left-handed crossings.
         */
        
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
            return std::string("PlanarDiagram") + "<" + TypeName<Int> + ">";
        }
        
    };

} // namespace KnotTools
