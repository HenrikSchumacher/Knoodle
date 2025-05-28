#pragma  once

namespace Knoodle
{
    template<typename Int_, Size_T optimization_level, bool mult_compQ_>
    class ArcSimplifier;
    
    template<typename Int_, bool mult_compQ_> class CrossingSimplifier;
    
    template<typename Int_, bool mult_compQ_> class StrandSimplifier;
    
    template<typename Int_>
    class alignas( ObjectAlignment ) PlanarDiagram : public CachedObject
    {
    public:
        
        static_assert(SignedIntQ<Int_>,"");

        using Int  = Int_;
        
        using UInt = Scalar::Unsigned<Int>;
        
        using Base_T  = CachedObject;
        using Class_T = PlanarDiagram<Int>;

//        using CrossingContainer_T       = Tensor3<Int,Int>;
//        using ArcContainer_T            = Tensor2<Int,Int>;
        
        using CrossingContainer_T       = Tiny::MatrixList_AoS<2,2,Int,Int>;
        using ArcContainer_T            = Tiny::VectorList_AoS<2,  Int,Int>;
        
        using CrossingStateContainer_T  = Tensor1<CrossingState,Int>;
        using ArcStateContainer_T       = Tensor1<ArcState,Int>;
        
        using Multigraph_T              = Multigraph<Int>;
        using ComponentMatrix_T         = Multigraph_T::ComponentMatrix_T;
        
        using PD_List_T                 = std::vector<PlanarDiagram<Int>>;

        
        template<typename I, Size_T lvl, bool mult_compQ_>
        friend class ArcSimplifier;
        
        template<typename I, bool mult_compQ_>
        friend class CrossingSimplifier;
        
        template<typename I, bool mult_compQ_>
        friend class StrandSimplifier;
            
        
        using Arrow_T = std::pair<Int,bool>;
        
        static constexpr bool Tail  = 0;
        static constexpr bool Head  = 1;
        static constexpr bool Left  = 0;
        static constexpr bool Right = 1;
        static constexpr bool Out   = 0;
        static constexpr bool In    = 1;
        
        static constexpr bool always_canonicalizeQ = true;
        
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
        
        // Exposed to user via Arcs().
        ArcContainer_T           A_cross;
        // Exposed to user via ArcStates().
        ArcStateContainer_T      A_state;
        
        // Counters for Reidemeister moves.
        Int R_I_counter   = 0;
        Int R_Ia_counter  = 0;
        Int R_II_counter  = 0;
        Int R_IIa_counter = 0;
        Int twist_counter = 0;
        Int four_counter  = 0;
        
        mutable Tensor1<Int,Int> C_scratch; // Some multi-purpose scratch buffers.
        mutable Tensor1<Int,Int> A_scratch; // Some multi-purpose scratch buffers.

//        Stack<Int,Int> stack;
        
        bool proven_minimalQ = false;
        
        
        
    public:
        
        PlanarDiagram() = default;
        
        virtual ~PlanarDiagram() override = default;
        
        
    protected:
        
        /*! @brief This constructor is supposed to only allocate all relevant buffers.
         *  Data has to be filled in manually. Only for internal use.
         */
        
        PlanarDiagram( const Int crossing_count_, const Int unlink_count_ )
        : crossing_count     { crossing_count_                             }
        , arc_count          { Int(2)*crossing_count                       }
        // TODO: Make this zero.
//        : crossing_count     { Int(0)                                      }
//        , arc_count          { Int(0)                                      }
        , unlink_count       { unlink_count_                               }
        , max_crossing_count { crossing_count_                             }
        , max_arc_count      { Int(2)*crossing_count_                      }
//        , C_arcs             { max_crossing_count, -1                      }
        , C_state            { max_crossing_count, CrossingState::Inactive }
//        , A_cross            { max_arc_count,      -1                      }
        , A_state            { max_arc_count,      ArcState::Inactive      }
        , C_scratch          { max_crossing_count                          }
        , A_scratch          { max_arc_count                               }
        {}
        
    public:
  
//        // Copy constructor
//        PlanarDiagram( const PlanarDiagram & other ) = default;
        
        // Copy constructor
        PlanarDiagram( const PlanarDiagram & other )
        :   crossing_count          { other.crossing_count          }
        ,   arc_count               { other.arc_count               }
        ,   unlink_count            { other.unlink_count            }
        ,   max_crossing_count      { other.max_crossing_count      }
        ,   max_arc_count           { other.max_arc_count           }
        ,   C_arcs                  { other.C_arcs                  }
        ,   C_state                 { other.C_state                 }
        ,   A_cross                 { other.A_cross                 }
        ,   A_state                 { other.A_state                 }
        ,   R_I_counter             { other.R_I_counter             }
        ,   R_Ia_counter            { other.R_Ia_counter            }
        ,   R_II_counter            { other.R_II_counter            }
        ,   R_IIa_counter           { other.R_IIa_counter           }
        ,   twist_counter           { other.twist_counter           }
        ,   four_counter            { other.four_counter            }
        ,   C_scratch               { other.C_scratch               }
        ,   A_scratch               { other.A_scratch               }
        ,   proven_minimalQ       { other.proven_minimalQ       }
        {}
            
        
        friend void swap(PlanarDiagram & A, PlanarDiagram & B ) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( static_cast<CachedObject &>(A), static_cast<CachedObject &>(B) );
            
            swap( A.crossing_count      , B.crossing_count      );
            swap( A.arc_count           , B.arc_count           );
            swap( A.unlink_count        , B.unlink_count        );
            swap( A.max_crossing_count  , B.max_crossing_count  );
            swap( A.max_arc_count       , B.max_arc_count       );
            
            swap( A.C_arcs              , B.C_arcs              );
            swap( A.C_state             , B.C_state             );
            swap( A.A_cross             , B.A_cross             );
            swap( A.A_state             , B.A_state             );
            
            swap( A.R_I_counter         , B.R_I_counter         );
            swap( A.R_Ia_counter        , B.R_Ia_counter        );
            swap( A.R_II_counter        , B.R_II_counter        );
            swap( A.R_IIa_counter       , B.R_IIa_counter       );
            swap( A.twist_counter       , B.twist_counter       );
            swap( A.four_counter        , B.four_counter        );
            
            swap( A.C_scratch           , B.C_scratch           );
            swap( A.A_scratch           , B.A_scratch           );
            swap( A.proven_minimalQ   , B.proven_minimalQ   );
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
        
        
        
        template<typename ExtInt>
        PlanarDiagram(
            cptr<ExtInt> crossings, cptr<ExtInt> crossing_states,
            cptr<ExtInt> arcs     , cptr<ExtInt> arc_states,
            const ExtInt crossing_count_,
            const ExtInt unlink_count_,
            const bool proven_minimalQ_ = false
        )
        :   PlanarDiagram( crossing_count_, unlink_count_ )
        {
            static_assert(IntQ<ExtInt>,"");
            
            C_arcs.Read(crossings);
            C_state.Read(crossing_states);
            A_cross.Read(arcs);
            A_state.Read(arc_states);
            proven_minimalQ = proven_minimalQ_;
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
        
    private:
        
        template<typename Real, typename BReal>
        void ReadFromLink(
            const Int  component_count,
            cptr<Int>  component_ptr,
            cptr<Int>  edge_ptr,
            cptr<Int>  edge_intersections,
            cptr<bool> edge_overQ,
            cref<std::vector<typename Link_2D<Real,Int,BReal>::Intersection_T>> intersections
        )
        {
            static_assert(FloatQ<Real>,"");
            static_assert(FloatQ<BReal>,"");
            
            using Intersection_T = typename Link_2D<Real,Int,BReal>::Intersection_T;
            using Sign_T         = typename Intersection_T::Sign_T;
            // TODO: Handle over/under in ArcState.
//            using F_T            = Underlying_T<ArcState>;
            
            
            if( intersections.size() <= Size_T(0) )
            {
                unlink_count = component_count;
                proven_minimalQ = true;
                return;
            }
            
            unlink_count = 0;
            
            C_scratch.Fill(Int(-1));
            
            mptr<Int> C_label = C_scratch.data();
            Int C_counter = 0;
            
            // TODO: If we want to canonicalize here, then we also need arc labels!
            
            auto process = [&,C_label,edge_intersections,edge_overQ,this]( const Int a, const Int b
            )
            {
                const Int c_pos = edge_intersections[b];
                
                if( C_label[c_pos] < 0 )
                {
                    C_label[c_pos] = C_counter++;
                }
                
                const Int c = C_label[c_pos];
                
                const bool overQ = edge_overQ[b];
                
                cref<Intersection_T> inter = intersections[static_cast<Size_T>(c_pos)];
                
                A_cross(a,Head) = c; // c is head of a
                A_cross(b,Tail) = c; // c is tail of b
                
                PD_ASSERT( (inter.handedness > Sign_T(0)) || (inter.handedness < Sign_T(0)) );
                
                bool righthandedQ = inter.handedness > Sign_T(0);
                
                /*
                 *
                 *    negative         positive
                 *    right-handed     left-handed
                 *    .       .        .       .
                 *    .       .        .       .
                 *    O       O        O       O
                 *     ^     ^          ^     ^
                 *      \   /            \   /
                 *       \ /              \ /
                 *        /                \
                 *       / \              / \
                 *      /   \            /   \
                 *     /     \          /     \
                 *    O       O        O       O
                 *    .       .        .       .
                 *    .       .        .       .
                 *
                 */
                
                C_state[c] = righthandedQ ? CrossingState::RightHanded : CrossingState::LeftHanded;
                
                // TODO: Handle over/under in ArcState.
                A_state[a] = ArcState::Active;
//                A_state[a] = ToUnderlying(A_state[a]) | F_T(1) | (F_T(overQ) << 2);
//                A_state[b] = ToUnderlying(A_state[b]) | F_T(1) | (F_T(overQ) << 1);
                
                /*
                * righthandedQ == true and overQ == true:
                *
                *         C_arcs(c,Out,Left) |       | C_arcs(c,Out,Right) = b
                *                            |       |
                *                            O       O
                *                             ^     ^
                *                              \   /
                *                               \ /
                *                                /
                *                               / \
                *                              /   \
                *                             /     \
                *                            O       O
                *                            |       |
                *      a = C_arcs(c,In,Left) |       | C_arcs(c,In,Right)
                */
                
                const bool over_in_side = (righthandedQ == overQ) ? Left : Right ;
                
                C_arcs(c,In , over_in_side) = a;
                C_arcs(c,Out,!over_in_side) = b;
            };
            
            
            // TODO: Extract LinkComponentArcPointers, LinkComponentIndices, ArcLinkComponents from here (only if needed)?
            
            // Now we go through all components
            //      then through all edges of the component
            //      then through all intersections of the edge
            // and generate new vertices, edges, crossings, and arcs in one go.
            
            
            for( Int comp = 0; comp < component_count; ++comp )
            {
                // The range of arcs belonging to this component.
                const Int b_begin = edge_ptr[component_ptr[comp  ]];
                const Int b_end   = edge_ptr[component_ptr[comp+1]];

                if( b_begin == b_end )
                {
                    // Component is an unlink. Just skip it.
                    ++unlink_count;
                    continue;
                }
                
                // If we arrive here, then there is definitely a crossing in the first edge.
                
                
                // TODO: Do the traversal so that the resulting `PlanarDiagram` is in canonical ordering in the sense of `CanonicallyOrderedQ`. For this, we have to go forward along the component until reach the first edge whose tail goes under. If if there is no such edge, we start at `b_begin`.
                
                for( Int b = b_begin, a = b_end-Int(1); b < b_end; a = (b++) )
                {
                    process(a,b);
                }
            }
            
            // TODO: For some reason, the resulting `PlanarDiagram` is not canonically ordered in the sense of `CanonicallyOrderedQ`. This may be a big issue when using `PlanarDiagram` in conjunction of the external visualization tools as `PlanarDiagram` returns PDCode always in canonically ordered form. So we do here a maybe unnecessary recanonicalization here. Maybe we can create the diagram in canonical form already?
            
            if constexpr ( always_canonicalizeQ )
            {
                CanonicalizeInPlace();
            }
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
        
        Int InitialCrossingCount() const
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
         *  Beware that a crossing can have various states, such as `CrossingState::LeftHanded`, `CrossingState::RightHanded`, or `CrossingState::Deactivated`. This information is stored in the corresponding entry of `CrossingStates()`.
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
        
        cref<CrossingStateContainer_T> CrossingStates() const
        {
            return C_state;
        }
        
        CrossingState GetCrossingState( const Int c ) const
        {
            return C_state[c];
        }
        
        /*!
         * @brief Returns how many arcs there were in the original planar diagram, before any simplifications.
         */
        
        Int InitialArcCount() const
        {
            return max_arc_count;
        }
        
        /*!
         * @brief Returns the number of arcs in the planar diagram.
         */
        
        Int ArcCount() const
        {
            return arc_count;
        }
        
        /*!
         * @brief Returns the arcs that connect the crossings as a reference to a constant `Tensor2` object, which is basically a heap-allocated matrix.
         *
         * This reference is constant because things can go wild (segfaults, infinite loops) if we allow the user to mess with this data.
         *
         * The `a`-th arc is stored in `Arcs()(a,i)`, `i = 0,1`, in the following way; note that we defined Booleans `Tail = 0` and `Head = 1` for easing the indexing:
         *
         *                          a
         *    Arcs()(a,0) X -------------> X Arcs()(a,0)
         *          =                             =
         *    Arcs()(a,Tail)                Arcs()(a,Head)
         *
         * This means that arc `a` leaves crossing `GetArc()(a,0)` and enters `GetArc()(a,1)`.
         *
         * Beware that an arc can have various states such as `CrossingState::Active` or `CrossingState::Deactivated`. This information is stored in the corresponding entry of `ArcStates()`.
         */
        
        cref<ArcContainer_T> Arcs()
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
        
        cref<ArcStateContainer_T> ArcStates() const
        {
            return A_state;
        }
        
        ArcState GetArcState( const Int a ) const
        {
            return A_state[a];
        }
        
        
//        void SanitizeArcStates()
//        {
//            for( Int a = 0; a < max_arc_count; ++a )
//            {
//                if( ArcActiveQ(a) )
//                {
//                    A_state[a] = ArcState::Active;
//                }
//            }
//        }

        
    public:
        
        std::string CrossingString( const Int c ) const
        {
            return "crossing " + Tools::ToString(c) +" = { { "
                + Tools::ToString(C_arcs(c,Out,Left )) + ", "
                + Tools::ToString(C_arcs(c,Out,Right)) + " }, { "
                + Tools::ToString(C_arcs(c,In ,Left )) + ", "
                + Tools::ToString(C_arcs(c,In ,Right)) + " } } ("
                + Knoodle::ToString(C_state[c])      +")";
        }
        
        /*!
         * @brief Checks whether crossing `c` is still active.
         */
        
        bool CrossingActiveQ( const Int c ) const
        {
            return Knoodle::ActiveQ(C_state[c]);
        }
        
//        bool CrossingUnchangedQ( const Int c ) const
//        {
//            return Knoodle::UnchangedQ(C_state[c]);
//        }
//        
//        bool CrossingChangedQ( const Int c ) const
//        {
//            return Knoodle::ChangedQ(C_state[c]);
//        }
        
        bool CrossingRightHandedQ( const Int c ) const
        {
            return Knoodle::RightHandedQ(C_state[c]);
        }
        
        bool CrossingLeftHandedQ( const Int c ) const
        {
            return Knoodle::LeftHandedQ(C_state[c]);
        }

        bool OppositeHandednessQ( const Int c_0, const Int c_1 ) const
        {
            return Knoodle::OppositeHandednessQ(C_state[c_0],C_state[c_1]);
        }
        
        bool SameHandednessQ( const Int c_0, const Int c_1 ) const
        {
            return Knoodle::SameHandednessQ(C_state[c_0],C_state[c_1]);
        }
        
        void RotateCrossing( const Int c, const bool dir )
        {
//            Int C [2][2];
//            copy_buffer<4>( C_arcs.data(c), &C[0][0] );
            
        /* Before:
         *
         *   C[Out][Left ] O       O C[Out][Right]
         *                  ^     ^
         *                   \   /
         *                    \ /
         *                     X
         *                    / \
         *                   /   \
         *                  /     \
         *   C[In ][Left ] O       O C[In ][Right]
         */
            
            if( dir == Right )
            {
        /* After:
         *
         *   C[Out][Left ] O       O C[Out][Right]
         *                  \     ^
         *                   \   /
         *                    \ /
         *                     X
         *                    / \
         *                   /   \
         *                  /     v
         *   C[In ][Left ] O       O C[In ][Right]
         */

                const Int buffer = C_arcs(c,Out,Left );
                
                C_arcs(c,Out,Left ) = C_arcs(c,Out,Right);
                C_arcs(c,Out,Right) = C_arcs(c,In ,Right);
                C_arcs(c,In ,Right) = C_arcs(c,In ,Left );
                C_arcs(c,In ,Left ) = buffer;
            }
            else
            {
        /* After:
         *
         *   C[Out][Left ] O       O C[Out][Right]
         *                  ^     /
         *                   \   /
         *                    \ /
         *                     X
         *                    / \
         *                   /   \
         *                  v     \
         *   C[In ][Left ] O       O C[In ][Right]
         */
                
                const Int buffer = C_arcs(c,Out,Left );
                
                C_arcs(c,Out,Left ) = C_arcs(c,In ,Left );
                C_arcs(c,In ,Left ) = C_arcs(c,In ,Right);
                C_arcs(c,In ,Right) = C_arcs(c,Out,Right);
                C_arcs(c,Out,Right) = buffer;
            }
        }
        
        std::string ArcString( const Int a ) const
        {
            return "arc " +Tools::ToString(a) +" = { "
                + Tools::ToString(A_cross(a,Tail)) + ", "
                + Tools::ToString(A_cross(a,Head)) + " } ("
                + Knoodle::ToString(A_state[a]) + ")";
        }
        
        /*!
         * @brief Checks whether arc `a` is still active.
         */
        
        bool ArcActiveQ( const Int a ) const
        {
            return Knoodle::ActiveQ(A_state[a]);
        }
        
//        bool ArcUnchangedQ( const Int a ) const
//        {
//            return Knoodle::UnchangedQ(A_state[a]);
//        }
//        
//        bool ArcChangedQ( const Int a ) const
//        {
//            return Knoodle::ChangedQ(A_state[a]);
//        }
        
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
                    wprint(ClassName()+"::Attempted to deactivate already inactive " + CrossingString(c) + ".");
                }
#endif
            }
            
            PD_ASSERT( crossing_count >= Int(0) );
            
            
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
                            pd_eprint(ClassName()+"::DeactivateCrossing: active " + ArcString(a) + " is still attached to deactivated " + CrossingString(c) + ".");
                        }
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
                wprint(ClassName()+"::DeactivateArc: Attempted to deactivate already inactive " + ArcString(a) + ".");
#endif
            }
            
            PD_ASSERT( arc_count >= Int(0) );
        }
        
    public:
        
        Int CountActiveCrossings() const
        {
            Int counter = 0;
            
            for( Int c = 0; c < max_crossing_count; ++c )
            {
                counter += CrossingActiveQ(c);
            }
            
            return counter;
        }
        
        Int CountActiveArcs() const
        {
            Int counter = 0;
            
            for( Int a = 0; a < max_arc_count; ++a )
            {
                counter += ArcActiveQ(a);
            }
            
            return counter;
        }
        
        /*!
         * @brief Computes the writhe = number of right-handed crossings - number of left-handed crossings.
         */
        
        Int Writhe() const
        {
            Int writhe = 0;
            
            for( Int c = 0; c < C_state.Size(); ++c )
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
        
        
        PlanarDiagram ChiralityTransform( const bool mirrorQ, const bool reverseQ )
        {
            PlanarDiagram pd ( crossing_count, unlink_count );
            
            pd.proven_minimalQ = proven_minimalQ;

            auto & pd_C_arcs  = pd.C_arcs;
            auto & pd_C_state = pd.C_state;
            auto & pd_A_cross = pd.A_cross;
            auto & pd_A_state = pd.A_state;
            
            
            const bool i0 = reverseQ;
            const bool i1 = !reverseQ;
            
            const bool j0 = mirrorQ != reverseQ;
            const bool j1 = mirrorQ == reverseQ;

            for( Int c = 0; c < max_crossing_count; ++c )
            {
                pd_C_arcs(c,0,0) = C_arcs(c,i0,j0);
                pd_C_arcs(c,0,1) = C_arcs(c,i0,j1);
                pd_C_arcs(c,1,0) = C_arcs(c,i1,j0);
                pd_C_arcs(c,1,1) = C_arcs(c,i1,j1);
            }
            
            if( mirrorQ )
            {
                for( Int c = 0; c < max_crossing_count; ++c )
                {
                    pd_C_state(c) = Flip(C_state(c));
                }
            }
            else
            {
                pd_C_state.Read(C_state.data());
            }
            
            
            const bool k0 = reverseQ;
            const bool k1 = !reverseQ;
            
            for( Int a = 0; a < max_arc_count; ++a )
            {
                pd_A_cross(a,0) = A_cross(a,k0);
                pd_A_cross(a,1) = A_cross(a,k1);
            }
            
            pd_A_state.Read(A_state.data());
            
            return pd;
        }
        
    public:

#include "PlanarDiagram/Traverse.hpp"
#include "PlanarDiagram/Canonicalize.hpp"
#include "PlanarDiagram/Reconnect.hpp"
#include "PlanarDiagram/Checks.hpp"
#include "PlanarDiagram/R_I.hpp"
//#include "PlanarDiagram/R_II.hpp"
//#include "PlanarDiagram/R_IIa_Vertical.hpp"
//#include "PlanarDiagram/R_IIa_Horizontal.hpp"
        
#include "PlanarDiagram/Arcs.hpp"
#include "PlanarDiagram/Faces.hpp"
#include "PlanarDiagram/LinkComponents.hpp"
#include "PlanarDiagram/DiagramComponents.hpp"
#include "PlanarDiagram/Strands.hpp"
#include "PlanarDiagram/DisconnectSummands.hpp"
        
#include "PlanarDiagram/Simplify1.hpp"
#include "PlanarDiagram/Simplify2.hpp"
#include "PlanarDiagram/Simplify3.hpp"
#include "PlanarDiagram/Simplify4.hpp"
#include "PlanarDiagram/Simplify5.hpp"
        
#include "PlanarDiagram/PDCode.hpp"
#include "PlanarDiagram/GaussCode.hpp"

#include "PlanarDiagram/ResolveCrossing.hpp"
#include "PlanarDiagram/SwitchCrossing.hpp"
        
#include "PlanarDiagram/VerticalSummandQ.hpp"
        
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
        
        Size_T ByteCount()
        {
            return sizeof(PlanarDiagram) + AllocatedByteCount();
        }
        
        static std::string ClassName()
        {
            return ct_string("PlanarDiagram")
                + "<" + TypeName<Int>
                + ">";
        }
        
    };

} // namespace Knoodle



