#pragma  once

namespace KnotTools
{
    template<typename Int>
    class alignas( ObjectAlignment ) PlanarDiagram
    {
    public:
        ASSERT_INT(Int)
        
        // Specify local typedefs within this class.

        using CrossingContainer_T       = Tiny::MatrixList<2,2,Int,Int>;
        using ArcContainer_T            = Tiny::VectorList<2,  Int,Int>;
        using CrossingStateContainer_T  = Tensor1<Crossing_State,Int>;
        using ArcStateContainer_T       = Tensor1<Arc_State,Int>;
        
        using Arrow_T = std::pair<Int,bool>;
        
    protected:
        
        // It is my habit to make all data members private/protected and to provided accessor references to only those members that the use is allowed to manipulate.
        
        // We try to make access to stored data as fast as possible by using a Structure of Array (SoA) data layout. That crossing data is stored in the heap-allocated array C_arc_cont of size 2 x 2 x crossing_count. The arc data is stored in the heap-allocated array A_cross_cont of size 2 x arc_count.
        
        CrossingContainer_T      C_arc_cont;
        CrossingStateContainer_T C_state;
        Tensor1<Int,Int>         C_label;       // List of vertex labels that stay persistent throughout.
        
        ArcContainer_T           A_cross_cont;
        ArcStateContainer_T      A_state;
        Tensor1<Int,Int>         A_label;       // List of arc labels that stay persistent throughout.
        
        // The constructor will store pointers to the data in the following pointers. We do so because fixed-size array access is no indirection.
        
        Int * restrict C_arcs      [2][2] = {{nullptr}};
        Int * restrict A_crossings [2]    = {nullptr};
        
        // Provide class members for the container sizes as convenience for loops.
        // For example compared to
        //
        //     for( Int i = 0; i < C_arc_cont.Dimension(2); ++i )
        //
        // this
        //
        //     for( Int i = 0; i < initial_crossing_count; ++i )
        //
        // makes it easier for the compiler to figure out that the upper end of the loop does not change.
        
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
        
        Tiny::VectorList<2,Int,Int> arc_faces; // Convention: Left face first.
        
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
        :   C_arc_cont       ( crossing_count_      )
        ,   C_state          ( crossing_count_      )
        ,   C_label          ( iota<Int,Int>(crossing_count_) )
        ,   A_cross_cont     ( I(2)*crossing_count_ )
        ,   A_state          ( I(2)*crossing_count_ )
        ,   A_label          ( iota<Int,Int>(I(2)*crossing_count_) )
        ,   C_arcs {
                {C_arc_cont.data(0,0), C_arc_cont.data(0,1)},
                {C_arc_cont.data(1,0), C_arc_cont.data(1,1)}
            }
        ,   A_crossings {A_cross_cont.data(0), A_cross_cont.data(1)}
        ,   initial_crossing_count ( crossing_count_      )
        ,   initial_arc_count      ( I(2)*crossing_count_ )
        ,   crossing_count         ( crossing_count_      )
        ,   arc_count              ( I(2)*crossing_count_ )
        ,   unlink_count           ( unlink_count_        )
        {
            PushAllCrossings();
        }
        
        // Copy constructor
        PlanarDiagram( const PlanarDiagram & other )
        :   C_arc_cont       ( other.C_arc_cont   )
        ,   C_state          ( other.C_state      )
        ,   C_label          ( other.C_label      )
        ,   A_cross_cont     ( other.A_cross_cont )
        ,   A_state          ( other.A_state      )
        ,   A_label          ( other.A_label      )
        ,   C_arcs {
                {C_arc_cont.data(0,0), C_arc_cont.data(0,1)},
                {C_arc_cont.data(1,0), C_arc_cont.data(1,1)}
            }
        ,   A_crossings {A_cross_cont.data(0), A_cross_cont.data(1)}
        ,   initial_crossing_count ( other.crossing_count    )
        ,   initial_arc_count      ( other.initial_arc_count )
        ,   crossing_count         ( other.crossing_count    )
        ,   arc_count              ( other.arc_count         )
        ,   unlink_count           ( other.unlink_count      )
        ,   touched_crossings      ( other.touched_crossings )
        ,   touched_arcs           ( other.touched_arcs   )
        {
            PushAllCrossings();
        }
        
        friend void swap(PlanarDiagram &A, PlanarDiagram &B) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( A.C_arc_cont  , B.C_arc_cont   );
            swap( A.C_state     , B.C_state      );
            swap( A.C_label     , B.C_label      );
            swap( A.A_cross_cont, B.A_cross_cont );
            swap( A.A_state     , B.A_state      );
            swap( A.A_label     , B.A_label      );
            
            swap( A.C_arcs[0][0]  , B.C_arcs[0][0]   );
            swap( A.C_arcs[0][1]  , B.C_arcs[0][1]   );
            swap( A.C_arcs[1][0]  , B.C_arcs[1][0]   );
            swap( A.C_arcs[1][1]  , B.C_arcs[1][1]   );
            
            swap( A.A_crossings[0], B.A_crossings[0] );
            swap( A.A_crossings[1], B.A_crossings[1] );
            
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
   
        Tensor1<Int, Int> & CrossingLabels()
        {
            return C_label;
        }
        
        CrossingContainer_T & Crossings()
        {
            return C_arc_cont;
        }
        
        const CrossingContainer_T& Crossings() const
        {
            return C_arc_cont;
        }
        
        CrossingStateContainer_T & CrossingStates()
        {
            return C_state;
        }
        
        const CrossingStateContainer_T& CrossingStates() const
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
        
        Tensor1<Int, Int> & ArcLabels()
        {
            return A_label;
        }
        
        ArcContainer_T & Arcs()
        {
            return A_cross_cont;
        }
        
        const ArcContainer_T & Arcs() const
        {
            return A_cross_cont;
        }

        ArcStateContainer_T & ArcStates()
        {
            return A_state;
        }
        
        const ArcStateContainer_T& ArcStates() const
        {
            return A_state;
        }
        
        //Modifiers
        
    public:
        
        std::string CrossingString( const Int c ) const
        {
            return "crossing " + ToString(c) +" = { { " +
               ToString(C_arcs[Out][Left ][c])+", "+ToString(C_arcs[Out][Right][c])+" }, { "+
               ToString(C_arcs[In ][Left ][c])+", "+ToString(C_arcs[In ][Right][c])+" } } ";
        }
        
        
        bool CrossingActive( const Int c ) const
        {
            return (C_state[c] == _plus) || (C_state[c] == _minus);
        }

        bool OppositeCrossingSigns( const Int c_0, const Int c_1 ) const
        {
            return ( SI(C_state[c_0]) == -SI(C_state[c_1]) );
        }
        
        void DeactivateCrossing( const Int c )
        {
            if( CrossingActive(c) )
            {
                --crossing_count;
            }
            C_state[c] = Crossing_State::Inactive;
        }
        
        
        std::string ArcString( const Int a ) const
        {
            return "arc " +ToString(a) +" = { " +
               ToString(A_crossings[Tail][a])+", "+ToString(A_crossings[Tip ][a])+" } ";
        }
        
        bool ArcActive( const Int a ) const
        {
            return A_state[a] == Arc_State::Active;
        }
        
        void DeactivateArc( const Int a )
        {
            if( ArcActive(a) )
            {
                --arc_count;
            }
            
            A_state[a] = Arc_State::Inactive;
        }
        

        
    private:
        
        Int NextCrossing( const Int c, bool io, bool lr ) const
        {
            const bool tiptail = ( io == In ) ? Tail : Tip;
            return A_crossings[tiptail][C_arcs[io][lr][c]];
        }

        inline void Reconnect( const Int a, const bool tiptail, const Int b )
        {
            // Read: "Reconnect arc a with its tip/tail to where b pointed/started. Then deactivates b.
            // Also keeps track of crossings that got touched and might be interesting for further simplication.
            
            const bool io = (tiptail==Tip) ? In : Out;
            
            PD_assert( a != b );
            
            PD_assert( ArcActive(a) );
            PD_assert( ArcActive(b) );
            
            const Int c = A_crossings[ tiptail][b];
            
#ifdef PD_ASSERTS
            const Int d = A_crossings[ tiptail][a];
            const Int p = A_crossings[!tiptail][a];
#endif
            
            PD_assert( (C_arcs[io][Left][c] == b) || (C_arcs[io][Right][c] == b) );
            
            PD_assert(CheckArc(b));
            PD_assert(CheckCrossing(c));
            
            PD_assert( CrossingActive(c) );
            PD_assert( CrossingActive(d) );
            PD_assert( CrossingActive(p) );

            A_crossings[tiptail][a] = c;

            const bool lr = (C_arcs[io][Left][c] == b) ? Left : Right;
            
            C_arcs[io][lr][c] = a;
            
            touched_crossings.push_back(c);
//            touched_crossings.push_back(A_crossings[Tip ][a]);
        }

    public:
        
        
#include "PD_Checks.hpp"
#include "PD_Reidemeister_I.hpp"
#include "PD_Reidemeister_II.hpp"
#include "PD_Break.hpp"
#include "PD_Switch.hpp"
        
#include "PD_Faces.hpp"
#include "PD_ConnectedSum.hpp"
        

        void PushAllCrossings()
        {
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
                if( CrossingActive(i) )
                {
                    ++counter;
                    touched_crossings.push_back( i );
                }
            }
        }
        
        void Simplify()
        {
            faces_initialized = false;
            
            R_I_counter  = 0;
            R_II_counter = 0;
            tangle_move_counter = 0;
            
//            switch_candidates.clear();

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
            
            PD_print( "Performed Reidemeister I  moves = " + ToString(R_I_counter ));
            PD_print( "Performed Reidemeister II moves = " + ToString(R_II_counter));
            PD_print( "Performed Tangle          moves = " + ToString(tangle_move_counter));
            
            const bool connected_sum_Q = ConnectedSum();

            if( connected_sum_Q )
            {
                Simplify();
            }
        }
        
        void RemoveLoops()
        {
            faces_initialized = false;
            
            R_I_counter  = 0;
            R_II_counter = 0;
            tangle_move_counter = 0;
            
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
            
//            valprint("a",a);
            PD_assert( ArcActive(a));
            
            
            const Int c = A_crossings[tiptail][a];
            
//            valprint("c",c);
            PD_assert( CrossingActive(c));
            
            
            const bool io = (tiptail == Tip) ? In : Out;

            const bool lr = (C_arcs[io][Left][c] == a) ? Left : Right;
            
            
            
            if( io == In )
            {
                if( lr == Left )
                {
                    return Arrow_T(C_arcs[Out][Left][c], Tip);
                }
                else
                {
                    return Arrow_T(C_arcs[_in][Left][c], Tail);
                }
            }
            else
            {
                if( lr == Left )
                {
                    return Arrow_T(C_arcs[Out][Right][c],Tip);
                }
                else
                {
                    return Arrow_T(C_arcs[_in][Right][c],Tail);
                }
            }
        }
        
        Arrow_T NextArc( const Int a, const bool tiptail )
        {
            PD_assert( ArcActive(a));
            
            const Int c = A_crossings[tiptail][a];
            
            PD_assert( CrossingActive(c));
            
            // Find out whether arc a is connected to an <_in>going or <Out>going port of c.
            const bool io = (tiptail == Tip) ? In : Out;

            // Find out whether arc a is connected to a <Left> or <Right> port of c.
            const bool lr = (C_arcs[io][Left][c] == a) ? Left : Right;
            
            // We leave through the arc at the opposite port.
            //If everything is set up correctly, the ourgoing arc points into the same direction as a.
            return Arrow_T(C_arcs[!io][!lr][c], tiptail );
        }
        
        Int NextCrossing( const Int a, const bool tiptail )
        {
            return A_crossings[tiptail][a];
        }
        
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
            
            valprint("label",label);
            Int pos = std::distance(
                    C_label.begin(),
                    std::find( C_label.begin(), C_label.end(), label )
            );
            
            valprint("pos",pos);
            
            if( pos >= C_label.Size() )
            {
                eprint(ClassName()+"::CrossingCrossingLookUp: Label "+ToString(label)+" not found. Returning 0");
                return 0;
            }
            else
            {
                valprint("C_label[pos]",C_label[pos]);
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
    
        PlanarDiagram CreateCompressed()
        {
            // Creates a copy of the planar diagram with all inactive crossings and arcs removed.
            
            PlanarDiagram pd ( crossing_count, unlink_count );
            
            mptr<Int> C_arcs_new [2][2] =
            {
                { pd.C_arc_cont.data(0,0), pd.C_arc_cont.data(0,1) },
                { pd.C_arc_cont.data(1,0), pd.C_arc_cont.data(1,1) }
            };

            mptr<Crossing_State> C_state_new = pd.C_state.data();
            mptr<Int> C_label_new = pd.C_label.data();

            mptr<Int> A_crossings_new [2] =
                { pd.A_cross_cont.data(0), pd.A_cross_cont.data(1) };
            
            mptr<Arc_State> A_state_new = pd.A_state.data();
            mptr<Int> A_label_new = pd.A_label.data();
            
            
            // Lookup tables to translate the connectivity data.
            
            // Will tell each old crossing where it has to go into the new array of crossings.
            Tensor1<Int,Int> C_lookup ( initial_crossing_count );
            // Will tell each old arc where it has to go into the new array of arcs.
            Tensor1<Int,Int> A_lookup ( initial_arc_count );

            Int C_counter = 0;
            for( Int c = 0; c < initial_crossing_count; ++c )
            {
                if( CrossingActive(c) )
                {
                    // We abuse C_label_new for the moment in order to store where each crossing came from.
                    C_label_new[C_counter] = c;
                    // We have to remember for each crossing what its new position is.
                    C_lookup[c] = C_counter;
                    ++C_counter;
                }
            }
            
            Int A_counter = 0;
            for( Int a = 0; a < initial_arc_count; ++a )
            {
                if( ArcActive(a) )
                {
                    // We abuse A_label_new for the moment in order to store where each arc came from.
                    A_label_new[A_counter] = a;

                    // We have to remember for each arc what its new position is.
                    A_lookup[a] = A_counter;
                    ++A_counter;
                }
            }
            
            for( Int c = 0; c < crossing_count; ++c )
            {
                const Int c_from = C_label_new[c];
                                
                C_arcs_new[0][0][c] = A_lookup[ C_arcs[0][0][c_from] ];
                C_arcs_new[0][1][c] = A_lookup[ C_arcs[0][1][c_from] ];
                C_arcs_new[1][0][c] = A_lookup[ C_arcs[1][0][c_from] ];
                C_arcs_new[1][1][c] = A_lookup[ C_arcs[1][1][c_from] ];
                
                C_state_new[c] = C_state[c_from];
                
                // Finally we overwrite C_label_new with the actual labels.
                C_label_new[c] = C_label[c_from];
            }
            
            for( Int a = 0; a < arc_count; ++a )
            {
                const Int a_from = A_label_new[a];
                
                PD_assert( ArcActive(a_from) );
                
                A_crossings_new[0][a] = C_lookup[ A_crossings[0][a_from] ];
                A_crossings_new[1][a] = C_lookup[ A_crossings[1][a_from] ];
                
                A_state_new[a] = A_state[a_from];
                
//                PD_assert( pd.ArcActive(a) );
                
                // Finally we overwrite A_label_new with the actual labels.
                A_label_new[a] = A_label[a_from];
            }
            
            pd.connected_sum_counter = connected_sum_counter;
            
            return pd;
        }
        
        OrientedLinkDiagram<Int> CreateOrientedLinkDiagram()
        {
            OrientedLinkDiagram<Int> L ( crossing_count );
            
            mptr<Int> C [2][2] = {
                { L.Crossings().data(0,0), L.Crossings().data(0,1) },
                { L.Crossings().data(1,0), L.Crossings().data(1,1) }
            };
            
            std::vector<bool> & C_sign = L.CrossingSigns();
            
            Tensor1<Int,Int> C_lookup ( initial_crossing_count );
            
            Int C_counter = 0;
            for( Int c = 0; c < initial_crossing_count; ++c )
            {
                if( CrossingActive(c) )
                {
                    // We have to remember for each crossing what its new position is.
                    C_lookup[c] = C_counter;
                    ++C_counter;
                }
            }
            
            C_counter = 0;
            for( Int c = 0; c < initial_crossing_count; ++c )
            {
                if( CrossingActive(c) )
                {
                    for( bool io : { _in, Out} )
                    {
                        for( bool lr : { Left, Right} )
                        {
                            const Int a = C_arcs[io][lr][c];
                            
                            const Int d = A_crossings[ (io == Out) ? Tip : Tail ][a];
                            
                            PD_assert( CrossingActive(d) );
                            
                            C[io][lr][C_counter] = C_lookup[d];
                        }
                    }
                    
                    C_sign[C_counter] = (C_state[c] == Crossing_State::Positive) ? _pos : _neg;
                    
                    ++C_counter;
                }
            }
            
            return L;
        }
        
    public:
        
        static std::string ClassName()
        {
            return "PlanarDiagram<"+TypeName<Int>+">";
        }
        
    };
    
} // namespace KnotTools
