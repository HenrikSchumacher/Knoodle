#pragma  once

#include <cassert>

namespace KnotTools
{

#ifdef PD_VERBOSE
    #define PD_PRINT( s ) Tools::logprint((s));
    
    #define PD_VALPRINT( key, val ) Tools::logvalprint( (key), (val) )
    
    #define PD_WPRINT( s ) Tools::wprint((s));
    
#else
    #define PD_PRINT( s )
    
    #define PD_VALPRINT( key, val )
    
    #define PD_WPRINT( s )
#endif
    
    
    
#ifdef PD_DEBUG
    #define PD_ASSERT( c ) if(!(c)) { Tools::eprint( "PD_ASSERT failed: " + std::string(#c) ); }
    
    #define PD_DPRINT( s ) Tools::logprint((s));
#else
    #define PD_ASSERT( c )
    
    #define PD_DPRINT( s )
#endif
    
    
    // TODO: Use signs in A_cross entries to indicate whether the arc enters/leaves the corresponding crossing through left or right port, i.e.,
    // TODO: A_cross(a,Tail) = -c if a == C_arc(a,Out,Left )
    // TODO: A_cross(a,Tail) =  c if a == C_arc(a,Out,Right)
    
    

    
    
//    template<typename Int>
//    struct Crossing
//    {
//        std::array<std::array<Int,2>,2> arcs;
//        CrossingState state;
//        
//    }; // struct Crossing
//    
//    
//    template<typename Int>
//    struct Arc
//    {
//        std::array<Int,2> cross;
//        ArcState state;
//        
//    }; // struct Arc
    
    template<typename Int_, bool use_flagsQ_, bool mult_compQ_> class ArcSimplifier;
    template<typename Int_, bool mult_compQ_> class StrandSimplifier;
    
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
        
        
        friend class ArcSimplifier<Int,false,true>;
        friend class ArcSimplifier<Int,true ,true>;
        friend class StrandSimplifier<Int,true>;
        friend class StrandSimplifier<Int,false>;
        
        using ArcSimplifier_T = ArcSimplifier<Int,false,true>;
        
        
        using Arrow_T = std::pair<Int,bool>;
        
        static constexpr bool Tail  = 0;
        static constexpr bool Head  = 1;
        static constexpr bool Left  = 0;
        static constexpr bool Right = 1;
        static constexpr bool Out   = 0;
        static constexpr bool In    = 1;
        
        
    protected:
        
        CrossingContainer_T      C_arcs;
        CrossingStateContainer_T C_state;
        
        ArcContainer_T           A_cross;
        ArcStateContainer_T      A_state;
        
        Int initial_crossing_count  = 0;
        Int initial_arc_count       = 0;
        
        Int crossing_count          = 0;
        Int arc_count               = 0;
        Int unlink_count            = 0;
        
        // Counters for Reidemeister moves.
        Int R_I_counter             = 0;
        Int R_Ia_counter            = 0;
        Int R_II_counter            = 0;
        Int R_IIa_counter           = 0;
        Int twist_counter           = 0;
        Int four_counter            = 0;
        
#ifdef PD_COUNTERS
        Int R_I_check_counter       = 0;
        Int R_Ia_check_counter      = 0;
        Int R_Ia_vertical_counter   = 0;
        Int R_Ia_horizontal_counter = 0;
        Int R_II_check_counter      = 0;
        Int R_II_vertical_counter   = 0;
        Int R_II_horizontal_counter = 0;
        Int R_IIa_check_counter     = 0;
#endif
        
        // Stacks for keeping track of recently modified entities.
#ifdef PD_USE_TOUCHING
        std::vector<Int> touched_crossings;
#endif
        // Data for the faces and the dual graph
        
        Tensor2<Int,Int> A_faces; // Convention: Left face first.
        
        Tensor1<Int,Int> F_arc_idx {0};
        Tensor1<Int,Int> F_arc_ptr {2,0};
        
        Tensor1<Int,Int> comp_arc_idx {0};
        Tensor1<Int,Int> comp_arc_ptr {2,0};
        
        Tensor1<Int,Int> A_comp {0};
        Tensor1<Int,Int> A_pos  {0};
        
        bool faces_initialized = false;
        bool comp_initialized  = false;
        
//        Int connected_sum_counter = 0;
        
    private:
        
        ArcSimplifier_T arc_simplifier;
//        ArcSimplifier<Int,true,true> arc_simplifier4;
        
        std::vector<Int> stack;
        
    public:
        
        PlanarDiagram()
        :   arc_simplifier  { *this }
//        ,   arc_simplifier4 { *this }
        {}
        
        virtual ~PlanarDiagram() override = default;
        
    protected:
        
        class CrossingView
        {
            Int c;
            mptr<Int> C;
            CrossingState & S;
            
        public:
            
            CrossingView( const Int c_, mptr<Int> C_, CrossingState & S_ )
            : c { c_ }
            , C { C_ }
            , S { S_ }
            {}

            ~CrossingView() = default;
            
            // TODO: Value semantics.
            
            Int Idx() const
            {
                return c;
            }
            
            CrossingState & State()
            {
                return S;
            }
            
            const CrossingState & State() const
            {
                return S;
            }
            
            Int & operator()( const bool io, const bool side )
            {
                return C[2 * io + side];
            }
            
            const Int & operator()( const bool io, const bool side ) const
            {
                return C[2 * io + side];
            }
            
            bool ActiveQ() const
            {
                return (ToUnderlying(S) % 2);
            }
            
            friend bool operator==( const CrossingView & C_0, const CrossingView & C_1 )
            {
                return C_0.c == C_1.c;
            }
            
            friend bool operator==( const CrossingView & C_0, const Int c_1 )
            {
                return C_0.c == c_1;
            }
            
            friend bool operator==( const Int c_0, const CrossingView & C_1 )
            {
                return c_0 == C_1.c;
            }
            
            std::string String() const
            {
                return "crossing " + Tools::ToString(c) +" = { { " 
                + Tools::ToString(C[0]) + ", "
                + Tools::ToString(C[1]) + " }, { "
                + Tools::ToString(C[2]) + ", "
                + Tools::ToString(C[3]) + " } } (" 
                + KnotTools::ToString(S) +")";
            }
            
            friend std::string ToString( const CrossingView & C )
            {
                return C.String();
            }
            
        }; // class CrossingView
        
        CrossingView GetCrossing( const Int c )
        {
            AssertCrossing(c);

            return CrossingView( c, C_arcs.data(c), C_state[c] );
        }
        
        class ArcView
        {
            Int a;
            mptr<Int> A;
            ArcState & S;
            
        public:
            
            ArcView( const Int a_, mptr<Int> A_, ArcState & S_ )
            : a { a_ }
            , A { A_ }
            , S { S_ }
            {}
            
            ~ArcView() = default;

            // TODO: Value semantics.
            
            Int Idx() const
            {
                return a;
            }
            
            ArcState & State()
            {
                return S;
            }
            
            const ArcState & State() const
            {
                return S;
            }
            
            Int & operator()( const bool headtail )
            {
                return A[headtail];
            }
            
            const Int & operator()( const bool headtail ) const
            {
                return A[headtail];
            }
            
            bool ActiveQ() const
            {
                return (ToUnderlying(S) > 0);
            }
            
            friend bool operator==( const ArcView & A_0, const ArcView & A_1 )
            {
                return A_0.a == A_1.a;
            }
            
            friend bool operator==( const ArcView & A_0, const Int a_1 )
            {
                return A_0.a == a_1;
            }
            
            friend bool operator==( const Int a_0, const ArcView & A_1 )
            {
                return a_0 == A_1.a;
            }
            
            std::string String() const
            {
                return "arc " +Tools::ToString(a) +" = { " +
                Tools::ToString(A[0])+", "+Tools::ToString(A[1])+" } (" + KnotTools::ToString(S) +")";
            }
            
            friend std::string ToString( const ArcView & A )
            {
                return A.String();
            }
            
        }; // class ArcView
        
        
        
        ArcView GetArc( const Int a )
        {
            AssertArc(a);
            
            return ArcView( a, A_cross.data(a), A_state[a] );
        }
        
        bool HeadQ( const ArcView & A, const CrossingView & C )
        {
            return A(Head) == C;
        }
        
        bool TailQ( const ArcView & A, const CrossingView & C )
        {
            return A(Tail) == C;
        }
        
        
    protected:
        
        /*! @brief This constructor is supposed to only allocate all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
        PlanarDiagram( const Int crossing_count_, const Int unlink_count_ )
        :   C_arcs                  ( crossing_count_, Int(2), Int(2), -1               )
        ,   C_state                 ( crossing_count_, CrossingState::Inactive      )
        ,   A_cross                 ( Int(2)*crossing_count_, Int(2), -1                )
        ,   A_state                 ( Int(2)*crossing_count_, ArcState::Inactive    )
        ,   initial_crossing_count  ( crossing_count_                               )
        ,   initial_arc_count       ( Int(2)*crossing_count_                        )
        ,   crossing_count          ( crossing_count_                               )
        ,   arc_count               ( Int(2)*crossing_count_                        )
        ,   unlink_count            ( unlink_count_                                 )
        ,   arc_simplifier          ( *this )
//        ,   arc_simplifier4         ( *this )
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
            swap( A.A_cross     , B.A_cross      );
            swap( A.A_state     , B.A_state      );
            
            swap( A.initial_crossing_count , B.initial_crossing_count  );
            swap( A.initial_arc_count      , B.initial_arc_count       );
            swap( A.crossing_count         , B.crossing_count          );
            swap( A.arc_count              , B.arc_count               );
            swap( A.unlink_count           , B.unlink_count            );
            swap( A.R_I_counter            , B.R_I_counter             );

            swap( A.R_Ia_counter           , B.R_Ia_counter            );
            swap( A.R_II_counter           , B.R_II_counter            );
            swap( A.R_IIa_counter          , B.R_IIa_counter           );
            swap( A.twist_counter          , B.twist_counter           );
            swap( A.four_counter           , B.four_counter           );
#ifdef PD_COUNTERS
            swap( A.R_I_check_counter      , B.R_I_check_counter       );
            swap( A.R_Ia_check_counter     , B.R_Ia_check_counter      );
            swap( A.R_Ia_horizontal_counter, B.R_Ia_horizontal_counter );
            swap( A.R_Ia_vertical_counter  , B.R_Ia_vertical_counter   );
            swap( A.R_II_check_counter     , B.R_II_check_counter      );
            swap( A.R_II_horizontal_counter, B.R_II_horizontal_counter );
            swap( A.R_II_vertical_counter  , B.R_II_vertical_counter   );
            swap( A.R_IIa_check_counter    , B.R_IIa_check_counter     );
#endif
            
#ifdef PD_USE_TOUCHING
            swap( A.touched_crossings      , B.touched_crossings       );
#endif
    
            swap( A.A_faces                , B.A_faces                 );
            
            swap( A.F_arc_idx              , B.F_arc_idx               );
            swap( A.F_arc_ptr              , B.F_arc_ptr               );
            
            swap( A.comp_arc_idx           , B.comp_arc_idx            );
            swap( A.comp_arc_ptr           , B.comp_arc_ptr            );
            
            swap( A.A_comp                 , B.A_comp                  );
            swap( A.A_pos                  , B.A_pos                  );
            
            swap( A.faces_initialized      , B.faces_initialized       );
            
//            swap( A.connected_sum_counter  , B.connected_sum_counter   );
            
//            swap( A.arc_simplifier         , B.arc_simplifier          );
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
            

            PD_PRINT("Begin of Link");
            PD_PRINT("{");
            for( Int comp = 0; comp < component_count; ++comp )
            {
                PD_PRINT("\tBegin of component " + Tools::ToString(comp));
                PD_PRINT("\t{");
                
                // The range of arcs belonging to this component.
                const Int arc_begin = edge_ptr[component_ptr[comp  ]];
                const Int arc_end   = edge_ptr[component_ptr[comp+1]];
                
                PD_VALPRINT("\t\tarc_begin", arc_begin);
                PD_VALPRINT("\t\tarc_end"  , arc_end  );

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
                    
                    A_cross(a,Head) = c; // c is head of a
                    A_cross(b,Tail) = c; // c is tail of b
                    
                    PD_ASSERT( inter.handedness > SInt(0) || inter.handedness < SInt(0) );
                    
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
                
                PD_PRINT("\t}");
                PD_PRINT("\tEnd   of component " + Tools::ToString(comp));
                
                PD_PRINT("");
                
            }
            PD_PRINT("");
            PD_PRINT("}");
            PD_PRINT("End   of Link");
            PD_PRINT("");
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
                    
                    A_cross(X[0],Head) = c;
                    A_cross(X[1],Tail) = c;
                    A_cross(X[2],Tail) = c;
                    A_cross(X[3],Head) = c;

                    C[Out][Left ] = X[2];
                    C[Out][Right] = X[1];
                    C[In ][Left ] = X[3];
                    C[In ][Right] = X[0];
                }
                else // left-handed
                {
                    C_state[c] = CrossingState::LeftHanded;
                    
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
                    
                    A_cross(X[0],Head) = c;
                    A_cross(X[1],Head) = c;
                    A_cross(X[2],Tail) = c;
                    A_cross(X[3],Tail) = c;
                    
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
         * @brief Returns how many Reidemeister I moves have been performed so far.
         */
        
        Int Reidemeister_I_Counter() const
        {
            return R_I_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister I moves have been tried so far.
         */
        
        Int Reidemeister_I_CheckCounter() const
        {
#ifdef PD_COUNTERS
            return R_I_check_counter;
#else
            return -1;
#endif
        }
        
        /*!
         * @brief Returns how many Reidemeister Ia moves have been performed so far.
         */
        
        Int Reidemeister_Ia_Counter() const
        {
            return R_Ia_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister Ia moves have been tried so far.
         */
        
        Int Reidemeister_Ia_CheckCounter() const
        {
#ifdef PD_COUNTERS
            return R_Ia_check_counter;
#else
            return -1;
#endif
        }
        
        /*!
         * @brief Returns how many Reidemeister II moves have been performed so far.
         */
        
        Int Reidemeister_II_Counter() const
        {
            return R_II_counter;
        }
        
        /*!
         * @brief Returns how many Reidemeister II moves have been tried so far.
         */
        
        Int Reidemeister_II_CheckCounter() const
        {
#ifdef PD_COUNTERS
            return R_II_check_counter;
#else
            return -1;
#endif
        }
        
        /*!
         * @brief Returns how many Reidemeister IIa moves have been performed so far.
         */
        
        Int Reidemeister_IIa_Counter() const
        {
            return R_IIa_counter;
        }
        
       /*!
        * @brief Returns how many Reidemeister IIa moves have been tried so far.
        */
       
       Int Reidemeister_IIa_CheckCounter() const
       {
#ifdef PD_COUNTERS
            return R_IIa_check_counter;
#else
            return -1;
#endif
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
         *  - `CrossingState::RightHandedUnchanged`
         *  - `CrossingState::LeftHanded`
         *  - `CrossingState::LeftHandedUnchanged`
         *  - `CrossingState::Inactive`
         *
         * `CrossingState::Inactive` means that the crossing has been deactivated by topological manipulations.
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
         *  - `CrossingState::RightHandedUnchanged`
         *  - `CrossingState::LeftHanded`
         *  - `CrossingState::LeftHandedUnchanged`
         *  - `CrossingState::Inactive`
         *
         * `CrossingState::Inactive` means that the crossing has been deactivated by topological manipulations.
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
        
        /*!
         * @brief Returns the arcs that connect the crossings as a reference to a Tensor2 object.
         *
         * The `a`-th arc is stored in `Arcs()(a,i)`, `i = 0,1`, in the following way:
         *
         *                          a
         *    Arcs()(a,0) X -------------> X Arcs()(a,0)
         *          =                             =
         *    Arcs()(a,Tail)                Arcs()(a,Head)
         *
         * This means that arc `a` leaves crossing `GetArc()(a,0)` and enters `GetArc()(a,1)`.
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
         *       Arcs()(a,0) X -------------> X Arcs()(a,0)
         *            =                              =
         *      Arcs()(a,Tail)                 Arcs()(a,Head)
         *
         * This means that arc `a` leaves crossing `GetArc()(a,0)` and enters `GetArc()(a,1)`.
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
            return "crossing " + Tools::ToString(c) +" = { { "
                + Tools::ToString(C_arcs(c,Out,Left )) + ", "
                + Tools::ToString(C_arcs(c,Out,Right)) + " }, { "
                + Tools::ToString(C_arcs(c,In ,Left )) + ", "
                + Tools::ToString(C_arcs(c,In ,Right)) + " } } ("
                + KnotTools::ToString(C_state[c])      +")";
        }
        
        /*!
         * @brief Checks whether crossing `c` is still active.
         */
        
        bool CrossingActiveQ( const Int c ) const
        {
            return KnotTools::ActiveQ(C_state[c]);
        }
        
        bool CrossingUnchangedQ( const Int c ) const
        {
            return KnotTools::UnchangedQ(C_state[c]);
        }
        
        bool CrossingChangedQ( const Int c ) const
        {
            return KnotTools::ChangedQ(C_state[c]);
        }
        
        bool CrossingRightHandedQ( const Int c ) const
        {
            return KnotTools::RightHandedQ(C_state[c]);
        }
        
        bool CrossingLeftHandedQ( const Int c ) const
        {
            return KnotTools::LeftHandedQ(C_state[c]);
        }

        bool OppositeHandednessQ( const Int c_0, const Int c_1 ) const
        {
            return KnotTools::OppositeHandednessQ(C_state[c_0],C_state[c_1]);
        }
        
        bool OppositeHandednessQ( const CrossingView & C_0, const CrossingView & C_1 ) const
        {
            return KnotTools::OppositeHandednessQ(C_0.State(),C_1.State());
        }
        
        bool SameHandednessQ( const Int c_0, const Int c_1 ) const
        {
            return KnotTools::SameHandednessQ(C_state[c_0],C_state[c_1]);
        }
        
        bool SameHandednessQ( const CrossingView & C_0, const CrossingView & C_1 ) const
        {
            return KnotTools::SameHandednessQ(C_0.State(),C_1.State());
        }
        
        
        void FlipHandedness( const Int c_0 )
        {
            // We intentionally remove the "Unchanged" attribute.
            
            switch( C_state[c_0] )
            {
                case CrossingState::RightHanded:
                {
                    C_state[c_0] = CrossingState::LeftHanded;
                    return;
                }
                case CrossingState::RightHandedUnchanged:
                {
                    C_state[c_0] = CrossingState::LeftHanded;
                    return;
                }
                case CrossingState::LeftHanded:
                {
                    C_state[c_0] = CrossingState::RightHanded;
                    return;
                }
                case CrossingState::LeftHandedUnchanged:
                {
                    C_state[c_0] = CrossingState::RightHanded;
                    return;
                }
                case CrossingState::Inactive:
                {
                    return;
                }
            }
        }
        
        
        void RotateCrossing( const Int c, const bool dir )
        {
//            Int C [2][2];
//            copy_buffer<4>( C_arcs.data(c), &C[0][0] );
            
        // Before:
        //
        //   C[Out][Left ] O       O C[Out][Right]
        //                  ^     ^
        //                   \   /
        //                    \ /
        //                     X
        //                    / \
        //                   /   \
        //                  /     \
        //   C[In ][Left ] O       O C[In ][Right]
            
            if( dir == Right )
            {
        // After:
        //
        //   C[Out][Left ] O       O C[Out][Right]
        //                  \     ^
        //                   \   /
        //                    \ /
        //                     X
        //                    / \
        //                   /   \
        //                  /     v
        //   C[In ][Left ] O       O C[In ][Right]

                const Int buffer = C_arcs(c,Out,Left );
                
                C_arcs(c,Out,Left ) = C_arcs(c,Out,Right);
                C_arcs(c,Out,Right) = C_arcs(c,In ,Right);
                C_arcs(c,In ,Right) = C_arcs(c,In ,Left );
                C_arcs(c,In ,Left ) = buffer;
            }
            else
            {
        // After:
        //
        //   C[Out][Left ] O       O C[Out][Right]
        //                  ^     /
        //                   \   /
        //                    \ /
        //                     X
        //                    / \
        //                   /   \
        //                  v     \
        //   C[In ][Left ] O       O C[In ][Right]
                
                const Int buffer = C_arcs(c,Out,Left );
                
                C_arcs(c,Out,Left ) = C_arcs(c,In ,Left );
                C_arcs(c,In ,Left ) = C_arcs(c,In ,Right);
                C_arcs(c,In ,Right) = C_arcs(c,Out,Right);
                C_arcs(c,Out,Right) = buffer;
            }
        }
        
        void RotateCrossing( CrossingView & C, const bool dir )
        {
        // Before:
        //
        //    C(Out,Left ) O       O C(Out,Right)
        //                  ^     ^
        //                   \   /
        //                    \ /
        //                     X
        //                    / \
        //                   /   \
        //                  /     \
        //    C(In ,Left ) O       O C(In ,Right)
            
            if( dir == Right )
            {
        // After:
        //
        //    C(Out,Left ) O       O C(Out,Right)
        //                  \     ^
        //                   \   /
        //                    \ /
        //                     X
        //                    / \
        //                   /   \
        //                  /     v
        //    C(In ,Left ) O       O C(In ,Right)
                
                const Int buffer = C(Out,Left );
                
                C(Out,Left ) = C(Out,Right);
                C(Out,Right) = C(In ,Right);
                C(In ,Right) = C(In ,Left );
                C(In ,Left ) = buffer;
            }
            else
            {
        // After:
        //
        //    C(Out,Left ) O       O C(Out,Right)
        //                  ^     /
        //                   \   /
        //                    \ /
        //                     X
        //                    / \
        //                   /   \
        //                  v     \
        //    C(In ,Left ) O       O C(In ,Right)
                
                const Int buffer = C(Out,Left );

                C(Out,Left ) = C(In ,Left );
                C(In ,Left ) = C(In ,Right);
                C(In ,Right) = C(Out,Right);
                C(Out,Right) = buffer;
            }
        }
        
        std::string ArcString( const Int a ) const
        {
            return "arc " +Tools::ToString(a) +" = { "
                + Tools::ToString(A_cross(a,Tail)) + ", "
                + Tools::ToString(A_cross(a,Head)) + " } ("
                + KnotTools::ToString(A_state[a]) + ")";
        }
        
        /*!
         * @brief Checks whether arc `a` is still active.
         */
        
        bool ArcActiveQ( const Int a ) const
        {
            return KnotTools::ActiveQ(A_state[a]);
        }
        
        bool ArcUnchangedQ( const Int a ) const
        {
            return KnotTools::UnchangedQ(A_state[a]);
        }
        
        bool ArcChangedQ( const Int a ) const
        {
            return KnotTools::ChangedQ(A_state[a]);
        }
        
        void TouchArc( const Int a )
        {
            if( ArcActiveQ(a) )
            {
                A_state[a] = ArcState::Active;
                
                stack.push_back(a);
            }
        }
        
        /*!
         * @brief Returns the crossing you would get to by starting at crossing `c` and
         * leaving trough the
         */
        
        Int NextCrossing( const Int c, bool io, bool side ) const
        {
            AssertCrossing(c);

            const Int a = C_arcs(c,io,side);

            AssertArc(a);
            
//            const Int c_next = A_cross( a, ( io == Out ) ? Head : Tail );
            const Int c_next = A_cross( a, !io );

            AssertCrossing(c_next);
            
            return c_next;
        }

        
    private:
        
        /*!
         * @brief Deactivates crossing `c`. Only for internal use.
         */
        
        template<bool assertsQ = true>
        void DeactivateCrossing( const Int c )
        {
            if( CrossingActiveQ(c) )
            {
                PD_PRINT("Deactivating " + CrossingString(c) + "." );
                --crossing_count;
                C_state[c] = CrossingState::Inactive;
            }
            else
            {
                
#ifdef PD_DEBUG
                if constexpr ( assertsQ )
                {
                    wprint("Attempted to deactivate already inactive crossing " + CrossingString(c) + ".");
                }
#endif
            }
            
            
#ifdef PD_DEBUG
            if constexpr ( assertsQ )
            {
                for( bool io : { In, Out } )
                {
                    for( bool side : { Left, Right } )
                    {
                        const Int a  = C_arcs(c,io,side);
                        
                        if( ArcActiveQ(a) && (A_cross(a,io) == c) )
                        {
                            eprint(ClassName()+"DeactivateCrossing: active " + ArcString(a) + " is still attached to deactivated " + CrossingString(c) + ".");
                        }
                    }
                }
            }
#endif
        }
        
        /*!
         * @brief Deactivates crossing represented by `CrossingView` object `C`. Only for internal use.
         */
        
        void Deactivate( mref<CrossingView> C )
        {
            if( C.ActiveQ() )
            {
                --crossing_count;
                C.State() = CrossingState::Inactive;
            }
            else
            {
#if defined(PD_DEBUG)
                wprint("Attempted to deactivate already inactive crossing " + C.String() + ".");
#endif
            }
            
#ifdef PD_DEBUG
            for( bool io : { In, Out } )
            {
                for( bool side : { Left, Right } )
                {
                    const Int a  = C(io,side);
                    
                    if( ArcActiveQ(a) && (A_cross(a,io) == C.Idx()) )
                    {
                        eprint(ClassName()+"DeactivateCrossing: active " + ArcString(a) + " is still attached to deactivated " + ToString(C) + ".");
                    }
                }
            }
#endif
        }
        
        /*!
         * @brief Deactivates arc `a`. Only for internal use.
         */
        
        void DeactivateArc( const Int a )
        {
            if( ArcActiveQ(a) )
            {
                PD_PRINT("Deactivating " + ArcString(a) + "." );
                
                --arc_count;
                A_state[a] = ArcState::Inactive;
            }
            else
            {
#if defined(PD_DEBUG)
                wprint("Attempted to deactivate already inactive arc " + ArcString(a) + ".");
#endif
            }
        }
        
        /*!
         * @brief Deactivates arc represented by `ArcView` object `A`. Only for internal use.
         */
        
        void Deactivate( mref<ArcView> A )
        {
            if( A.ArcActiveQ() )
            {
                --arc_count;
                A.State() = ArcState::Inactive;
            }
            else
            {
#if defined(PD_DEBUG)
                wprint("Attempted to deactivate already inactive arc " + A.String() + ".");
#endif
            }
        }
        
    public:
        
#include "PlanarDiagram/Reconnect.hpp"
#include "PlanarDiagram/Checks.hpp"
#include "PlanarDiagram/R_I.hpp"
#include "PlanarDiagram/R_II.hpp"
#include "PlanarDiagram/R_IIa_Vertical.hpp"
#include "PlanarDiagram/R_IIa_Horizontal.hpp"
        
#include "PlanarDiagram/Simplify.hpp"
#include "PlanarDiagram/Simplify3.hpp"
#include "PlanarDiagram/Simplify4.hpp"
        
        
#include "PlanarDiagram/PDCode.hpp"
        
//#include "PlanarDiagram/PassMove.hpp"


//#include "PlanarDiagram/Break.hpp"
//#include "PlanarDiagram/Switch.hpp"
        
#include "PlanarDiagram/Faces.hpp"
#include "PlanarDiagram/Components.hpp"
#include "PlanarDiagram/ConnectedSum.hpp"
        
    public:
        
        void PushAllCrossings()
        {
#ifdef PD_USE_TOUCHING
            touched_crossings.reserve( initial_crossing_count );
            
            for( Int i = initial_crossing_count-1; i >= 0; --i )
            {
                touched_crossings.push_back( i );
            }
#endif
        }
        
        void PushRemainingCrossings()
        {
#ifdef PD_USE_TOUCHING
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
         * @brief Returns the arc following arc `a` by going to the crossing at the head of `a` and then turning left.
         */
        
        Arrow_T NextLeftArc( const Int a, const bool headtail ) const
        {
            // TODO: Signed indexing does not work because of 0!
            
            AssertArc(a);
            
            const Int c = A_cross(a,headtail);
            
            AssertCrossing(c);
            
            // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
            
            if( headtail == Head )
            {
                // Using `C_arcs(c,In ,Left ) != a` instead of `C_arcs(c,In ,Right) == a` gives us a 50% chance that we do not have to read any index again.

                const Int b = C_arcs(c,In,Left);

                if( b != a )
                {
                    //   O     O
                    //    ^   ^
                    //     \ /
                    //      X c
                    //     / \
                    //    /   \
                    //   O     O
                    //   b     a

                    return Arrow_T(b,Tail);
                }
                else // if( b == a )
                {
                    //   O     O
                    //    ^   ^
                    //     \ /
                    //      X c
                    //     / \
                    //    /   \
                    //   O     O
                    // a == b

                    return Arrow_T(C_arcs(c,Out,Left),Head);
                }
            }
            else // if( headtail == Tail )
            {
                // Also here we can make it so that we have to read for a second time only in 50% of the cases.

                const Int b = C_arcs(c,Out,Right);

                if( b != a )
                {
                    //   a     b
                    //   O     O
                    //    ^   ^
                    //     \ /
                    //      X c
                    //     / \
                    //    /   \
                    //   O     O

                    return Arrow_T(b,Head);
                }
                else // if( b == a )
                {
                    //      a == b
                    //   O     O
                    //    ^   ^
                    //     \ /
                    //      X c
                    //     / \
                    //    /   \
                    //   O     O

                    return Arrow_T(C_arcs(c,In,Right),Tail);
                }
            }
        }
        
        /*!
         * @brief Returns the arc next to arc `a`, i.e., the arc reached by going straight through the crossing at the head of `a`.
         */
        
        Int NextArc( const Int a ) const
        {
            AssertArc(a);
            
            const Int c = A_cross(a,Head);
            
            AssertCrossing(c);

            // Find out whether arc a is connected to a <Left> or <Right> port of c.

//            const bool side = (C_arcs(c,In,Right) == a) ? Right : Left;
            const bool side = (C_arcs(c,In,Right) == a);
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the outgoing arc points into the same direction as a.
        
            const Int a_next = C_arcs(c,Out,!side);
            
            AssertArc(a_next);
            
            return a_next;
        }
        
        /*!
         * @brief Returns the ArcView object next to ArcView object `A`, i.e., the ArcView object reached by going straight through the crossing at the head of `A`.
         */
        
        ArcView NextArc( const ArcView & A )
        {
            AssertArc(A.Idx());
            
            const Int c = A(Head);

            AssertCrossing(c);

            // Find out whether arc a is connected to a <Left> or <Right> port of c.
//            const bool side = (C_arcs(c,In,Right) == A.Idx()) ? Right : Left;
            const bool side = (C_arcs(c,In,Right) == A.Idx());
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the outgoing arc points into the same direction as a.
            
            return GetArc( C_arcs(c,Out,!side) );
        }
        
        /*!
         * @brief Returns the arc preceding arc `a`, i.e., the arc reached by going straight through the crossing at the tail of `a`.
         */
        
        Int PrevArc( const Int a ) const
        {
            AssertArc(a);
            
            const Int c = A_cross(a,Tail);

            AssertCrossing(c);
            
            // Find out whether arc a is connected to a <Left> or <Right> port of c.
//            const bool side = (C_arcs(c,Out,Right) == a) ? Right : Left;
            const bool side = (C_arcs(c,Out,Right) == a);
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the ourgoing arc points into the same direction as a.
            
            const Int a_prev = C_arcs(c,In,!side);
            
            AssertArc(a_prev);
            
            return a_prev;
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
                while( ( a_ptr < m ) && ( A_visisted[a_ptr] || (!ArcActiveQ(a_ptr)) ) )
                {
                    ++a_ptr;
                }
                
                if( a_ptr >= m )
                {
                    break;
                }
                
                ++comp_counter;
                
                a = a_ptr;
                
                AssertArc(a);
                
                {
                    const Int c_prev = A_cross(a,Tail);
                    
                    AssertCrossing(c_prev);
                    
                    const Int c = C_labels[c_prev] = c_counter;
                    
                    C_state_new[c] = C_state[c_prev];
                    
                    ++c_counter;
                }
                
                // Traverse trough all arcs in the link component, until we return where we started.
                do
                {
                    const Int c_prev = A_cross(a,Tail);
                    const Int c_next = A_cross(a,Head);
                    
                    AssertCrossing(c_prev);
                    AssertCrossing(c_next);
                    
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
                        const Int  c  = C_labels[c_prev];
                        const bool side = (C_arcs(c_prev,Out,Right) == a);
                        
                        C_arcs_new(c,Out,side) = a_counter;
                        
                        A_cross_new(a_counter,Tail) = c;
                    }
                    
                    {
                        const Int  c  = C_labels[c_next];
                        const bool side = (C_arcs(c_next,In,Right) == a);
                        
                        C_arcs_new(c,In,side) = a_counter;
                        
                        A_cross_new(a_counter,Head) = c;
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
         * @brief This tells us whether a giving arc goes over the crossing at the indicated end.
         *
         *  @param a The index of the arc in equations.
         *
         *  @param headtail Boolean indicating whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
         */
        
        bool ArcUnderQ( const Int a, const bool headtail )  const
        {
            AssertArc(a);

            const Int c = A_cross(a,headtail);

            AssertCrossing(c);
            
            // Tail == 0 == Out
            // Head == 1 == In
            // const side = C_arcs(c_0,headtail,Right) == a ? Right : Left;
            const bool side = (C_arcs(c,headtail,Right) == a);
            
//            // Long version of code for documentation and debugging.
//            if( headtail == Head )
//            {
//                
//                PD_ASSERT( A_cross(a,Head) == c );
//                PD_ASSERT( C_arcs(c,In,side) == a );
//                
//                return (side == CrossingRightHandedQ(c));
//
//                // (side == Right) && CrossingRightHandedQ(c)
//                //
//                //         O     O
//                //          ^   ^
//                //           \ /
//                //            / c
//                //           / \
//                //          /   \
//                //         O     O
//                //                ^
//                //                 \
//                //                  \ a
//                //                   \
//                //                    X
//                //
//                //  (side == Left) && CrossingLeftHandedQ(c)
//                //
//                //         O     O
//                //          ^   ^
//                //           \ /
//                //            \ c
//                //           / \
//                //          /   \
//                //         O     O
//                //        ^
//                //       /
//                //      / a
//                //     /
//                //    X
//            }
//            else // if( headtail == Tail )
//            {
//                PD_ASSERT( A_cross(a,Tail) == c );
//                PD_ASSERT( C_arcs(c,Out,side) == a );
//                
//                return (side == CrossingLeftHandedQ(c));
//
//                // Positive cases:
//                //
//                // (side == Right) && CrossingLeftHandedQ(c)
//                //
//                //
//                //                   ^
//                //                  /
//                //                 / a
//                //                /
//                //         O     O
//                //          ^   ^
//                //           \ /
//                //            \ c
//                //           / \
//                //          /   \
//                //         O     O
//                //
//                // (side == Left) && CrossingRightHandedQ(c)
//                //
//                //     ^
//                //      \
//                //       \ a
//                //        \
//                //         O     O
//                //          ^   ^
//                //           \ /
//                //            / c
//                //           / \
//                //          /   \
//                //         O     O
//                //
//            }
            
//            // Short version of code for performance.
            return side == ( headtail == CrossingRightHandedQ(c) );
        }
        
        /*!
         * @brief This tells us whether a giving arc goes under the crossing at the indicated end.
         *
         *  @param a The index of the arc in equations.
         *
         *  @param headtail Boolean indicating whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
         */
        
        bool ArcOverQ( const Int a, const bool headtail ) const
        {
            return !ArcUnderQ(a,headtail);
        }
        
        /*!
         * @brief Returns an array that tells every arc to which over-strand it belongs.
         *
         *  More precisely, arc `a` belongs to over-strand number `ArcOverStrands()[a]`.
         *
         *  (An over-strand is a maximal consecutive sequence of arcs that pass over.)
         */
        
        Tensor1<Int,Int> ArcOverStrands() const
        {
            return ArcStrands<true>();
        }
        
        /*!
         * @brief Returns an array that tells every arc to which under-strand it belongs.
         *
         *  More precisely, arc `a` belongs to under-strand number `ArcUnderStrands()[a]`.
         *
         *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
         */
        
        Tensor1<Int,Int> ArcUnderStrands() const
        {
            return ArcStrands<false>();
        }
        
    private:
        
        template<bool overQ>
        Tensor1<Int,Int> ArcStrands() const
        {
            ptic(ClassName()+"::" + (overQ ? "Over" : "Under")  + "StrandIndices");
            
            const Int m = A_cross.Dimension(0);
            
            Tensor1<Int ,Int> A_colors    ( m, -1 );
            Tensor1<char,Int> A_visisted ( m, false );
            
            Int color = 0;
            Int a_ptr = 0;
            
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
                // Find the beginning of first strand.
                
                // For this, we move backwards along arcs until ArcUnderQ(a,Tail)/ArcOverQ(a,Tail)
                while( ArcUnderQ(a,Tail) != overQ )
                {
                    a = PrevArc(a);
                }
                
                const Int a_0 = a;
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
                    A_visisted[a] = true;
                    
                    A_colors[a] = color;

                    // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to initialize a new strand.
                    
                    color += (ArcUnderQ(a,Head) == overQ);
                    
                    a = NextArc(a);
                }
                while( a != a_0 );
                
                ++a_ptr;
            }
            
            ptoc(ClassName()+"::" + (overQ ? "Over" : "Under")  + "StrandIndices");
            
            return A_colors;
        }
        
    public:
        
        
        /*!
         * @brief Returns an array of that tells every crossing which over-strand end or start in is.
         *
         *  More precisely, crossing `c` has the outgoing over-strands `CrossingOverStrands()(c,0,0)` and `CrossingOverStrands()(c,0,1)`
         *  and the incoming over-strands `CrossingOverStrands()(c,1,0)` and `CrossingOverStrands()(c,1,1)`.
         *
         *  (An over-strand is a maximal consecutive sequence of arcs that pass over.)
         */
        
        
        Tensor3<Int,Int> CrossingOverStrands() const
        {
            return CrossingStrands<true>();
        }
        
        /*!
         * @brief Returns an array of that tells every crossing which under-strand end or start in is.
         *
         *  More precisely, crossing `c` has the outgoing under-strands `CrossingUnderStrands()(c,0,0)` and `CrossingUnderStrands()(c,0,1)`
         *  and the incoming under-strands `CrossingUnderStrands()(c,1,0)` and `CrossingUnderStrands()(c,1,1)`.
         *
         *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
         */
        
        Tensor3<Int,Int> CrossingUnderStrands() const
        {
            return CrossingStrands<false>();
        }
        
        template<bool overQ>
        Tensor3<Int,Int> CrossingStrands() const
        {
            ptic(ClassName()+"::Crossing" + (overQ ? "Over" : "Under") + "Strands");
            
            const Int n = C_arcs.Dimension(0);
            const Int m = A_cross.Dimension(0);
            
            Tensor3<Int ,Int> C_strands  ( n, 2, 2, -1 );
            Tensor1<char,Int> A_visisted ( m, false );
            
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
                // Find the beginning of first overstrand.
                // For this, we go back along a until ArcUnderQ(a,Tail)
                while( ArcUnderQ(a,Tail) != overQ )
                {
                    a = PrevArc(a);
                }
                
                const Int a_0 = a;
                
                // Traverse forward trhough all arcs in the link component, until we return where we started.
                do
                {
                    A_visisted[a] = true;
                    
                    const Int c_0 = A_cross(a,Tail);
                    const Int c_1 = A_cross(a,Head);

                    C_strands(c_0,Out,(C_arcs(c_0,Out,Right) == a)) = counter;
                    C_strands(c_1,In ,(C_arcs(c_1,In ,Right) == a)) = counter;
                    
                    counter += (ArcUnderQ(a,Head) == overQ);
                    
                    a = NextArc(a);
                }
                while( a != a_0 );
                
                ++a_ptr;
            }
            
            ptoc(ClassName()+"::Crossing" + (overQ ? "Over" : "Under") + "Strands");
            
            return C_strands;
        }
        
        /*!
         * @brief Computes the writhe = number of right-handed crossings - number of left-handed crossings.
         */
        
        Int Writhe() const
        {
            const Int writhe = 0;
            
            for( Int c = 0; c < C_state.Size(); ++c )
            {
                if( CrossingRightHandedQ(C_state[c]) )
                {
                    ++writhe;
                }
                else
                {
                    --writhe;
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



