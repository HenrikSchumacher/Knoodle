#pragma once

namespace KnotTools
{
    template<typename Int_, bool mult_compQ_>
    class alignas( ObjectAlignment ) StrandSimplifier
    {
    public:
        
        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
//        using Sint = int;
        
        using PD_T = PlanarDiagram<Int>;
        
        using CrossingContainer_T       = PD_T::CrossingContainer_T;
        using ArcContainer_T            = PD_T::ArcContainer_T;
        using CrossingStateContainer_T  = PD_T::CrossingStateContainer_T;
        using ArcStateContainer_T       = PD_T::ArcStateContainer_T;
        
        static constexpr bool mult_compQ = mult_compQ_;
        
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Out   = PD_T::Out;
        
    private:
        
        PD_T & pd;
        
        CrossingContainer_T      & C_arcs;
        CrossingStateContainer_T & C_state;
        
        ArcContainer_T           & A_cross;
        ArcStateContainer_T      & A_state;
        
    private:
        
        Tensor2<Int,Int> C_data;
        Tensor1<Int,Int> A_colors;
        
        Int color = 0;
        Int a_ptr = 0;

        Int strand_length    = 0;
        Int strand_0_counter = 0;
        Int strand_1_counter = 0;
        
        bool overQ;
        bool resetQ;
        
//        Tensor1<Int,Int> A_time;
//        
//        Tensor1<Int,Int> S_buffer;
//        
//        Int S_ptr = 0;
//        
//        Int a;
//        Int c_0;
//        Int c_1;
//        Int c_2;
//        Int c_3;
//        
//        // For crossing c_0
//        Int n_0;
//        //        Int e_0; // always = a.
//        Int s_0;
//        Int w_0;
//        
//        // For crossing c_1
//        Int n_1;
//        Int e_1;
//        Int s_1;
//        //        Int w_1; // always = a.
//        
//        // Whether the vertical strand at corresponding crossing goes over.
//        bool o_0;
//        bool o_1;
//        
//        // Whether the vertical strand at corresponding crossing goes up.
//        bool u_0;
//        bool u_1;
//        
//        Int test_count = 0;
        
    public:
        
        StrandSimplifier() = delete;
        
        StrandSimplifier( PD_T & pd_ )
        :   pd      { pd_        }
        ,   C_arcs  { pd.C_arcs  }
        ,   C_state { pd.C_state }
        ,   A_cross { pd.A_cross }
        ,   A_state { pd.A_state }
        ,   C_data  { C_arcs.Dimension(0), 3, -1 }
        ,   A_colors{ A_cross.Dimension(0), -1 }
//        ,   A_time  { A_cross.Dimension(0), -1 }
//        ,   S_buffer{ A_cross.Dimension(0), -1 }
        {}
        
        ~StrandSimplifier() = default;
        
        StrandSimplifier( const StrandSimplifier & other ) = delete;
        
        StrandSimplifier( StrandSimplifier && other ) = delete;
        

    private:
        
        bool ArcActiveQ( const Int a_ ) const
        {
            return pd.ArcActiveQ(a_);
        }
        
        bool ArcChangedQ( const Int a_ ) const
        {
            return pd.ArcChangedQ(a_);
        }
        
        void DeactivateArc( const Int a_ )
        {
            pd.DeactivateArc(a_);
        }
        
        template<bool should_be_activeQ>
        void AssertArc( const Int a_ ) const
        {
#ifdef DEBUG
            if constexpr( should_be_activeQ )
            {
                if( !ArcActiveQ(a_) )
                {
                    eprint("AssertArc<1>: " + ArcString(a_) + " is not active.");
                }
                PD_ASSERT(pd.CheckArc(a_));
            }
            else
            {
                if( ArcActiveQ(a_) )
                {
                    eprint("AssertArc<0>: " + ArcString(a_) + " is not inactive.");
                }
            }
#else
            (void)a_;
#endif
        }
        
        std::string ArcString( const Int a_ )  const
        {
            return pd.ArcString(a_);
        }
        
        bool CrossingActiveQ( const Int c_ ) const
        {
            return pd.CrossingActiveQ(c_);
        }
        
        template<bool assertsQ = false>
        void DeactivateCrossing( const Int c_ )
        {
            pd.template DeactivateCrossing<assertsQ>(c_);
        }
        
        template<bool should_be_activeQ>
        void AssertCrossing( const Int c_ ) const
        {
#ifdef DEBUG
            if constexpr( should_be_activeQ )
            {
                if( !CrossingActiveQ(c_) )
                {
                    eprint("AssertCrossing<1>: " + CrossingString(c_) + " is not active.");
                }
                PD_ASSERT(pd.CheckCrossing(c_));
            }
            else
            {
                if( CrossingActiveQ(c_) )
                {
                    eprint("AssertCrossing<0>: " + CrossingString(c_) + " is not inactive.");
                }
            }
#else
            (void)c_;
#endif
        }
        
        std::string CrossingString( const Int c_ ) const
        {
            return pd.CrossingString(c_);
        }
        

        template<bool assertQ = true>
        void Reconnect( const Int a_, const bool headtail, const Int b_ )
        {
            
//            print( std::string("Reconnecting ") + (headtail ? "Head" : "Tail") + " of " + ArcString(a_) + " to " + (headtail ? "Head" : "Tail") + " of " + ArcString(b_) + "." );
            
            pd.template Reconnect<assertQ>(a_,headtail,b_);
            
//            // TODO: Two crossings are touched multiply.
//            if constexpr ( use_flagsQ )
//            {
//                TouchCrossing(A_cross(a,Tail));
//                TouchCrossing(A_cross(a,Head));
//            }
        }
        
    private:
        
        mref<Int> C_color( const Int c )
        {
            return C_data(c,0);
        }
        
        mref<Int> C_arrow( const Int c )
        {
            return C_data(c,1);
        }
        
        mref<Int> C_len( const Int c )
        {
            return C_data(c,2);
        }
        
        

        void MarkCrossing( const int c )
        {
//            print("MarkCrossing: " + CrossingString(c) + "." );
            C_color(c) = color;
            // This indicates that c itself has been visited.
            C_arrow(c) = -2;
            C_len  (c) = strand_length;
        }
        
        void MarkNeighbor(
            const int c, const int c_next, const bool io, const bool side
        )
        {
//            print("MarkNeighbor(" + CrossingString(c) + "," + CrossingString(c_next) + "," + (io ? "In" : "Out") +"," + (side ? "Right" : "Left" ) + ")" );
//            const Int c_1 = pd.NextCrossing(c,io,side);
            
            const Int a_1 = C_arcs(c,io,side);
            const Int c_1 = A_cross(a_1,!io);

            if( c_1 != c_next )
            {
                // We need to avoid that a one written arrow is overwritten.
                if( C_color(c_1) != color )
                {
                    C_color(c_1) = color;
                    // This indicates that the crossing on the other side of a_1 has been visited.
                    C_arrow(c_1) = a_1;
                    C_len  (c_1) = strand_length;
                }
            }
        }
        
    public:

        Int SimplifyStrands( bool overQ_ )
        {
            overQ = overQ_;
            
//            print(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
            ptic(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
            
//            const Int n = C_arcs.Dimension(0);
            const Int m = A_cross.Dimension(0);
//
//            Tensor1<char,Int> A_visisted ( m, false );
            
            C_data.Fill(-1);
            A_colors.Fill(-1);
            
            color = 0;
            a_ptr = 0;

            strand_length    = 0;
            strand_0_counter = 0;
            strand_1_counter = 0;
            
            
            while( a_ptr < m )
            {
                // Search for next arc that is active and has not yet been handled.
                while( ( a_ptr < m ) && ( (A_colors[a_ptr] >= 0 )  || (!ArcActiveQ(a_ptr)) ) )
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
                while( pd.ArcUnderQ(a,Tail) != overQ )
                {
                    a = pd.PrevArc(a);
                }
                
                const Int a_0 = a;
                
                strand_length = 0;
                
                {
                    const Int c = A_cross(a,Tail);
                    
//                    print("Starting new strand " + ToString(color) + " at " + CrossingString(c) + "." );
                    
                    MarkCrossing(c);
                    
                    const bool side = (C_arcs(c,Out,Right) == a);

                    const Int c_next = A_cross(a,Head);
                    
                    MarkNeighbor(c,c_next,Out,!side);
                    MarkNeighbor(c,c_next,In , side);
                    // c is a start vertex, so we have to mark also the vertex at the back.
                    MarkNeighbor(c,c_next,In ,!side);
                }
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
                    ++strand_length;
                    
                    if( color == std::numeric_limits<Int>::max() )
                    {
                        ptoc(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
                        
                        return strand_0_counter + strand_1_counter;
                    }
                    
                    const Int c = A_cross(a,Head);
                    
                    // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
                    // This finds out whether we have to reset.
                    const bool side = (C_arcs(c,In ,Right) == a);
                    
                    resetQ = ((side == pd.CrossingRightHandedQ(c)) == overQ);
                    
                    // Arc gets old color.
                    A_colors[a] = color;
                    
                    if( C_color(c) == color )
                    {
                        // Vertex c has been visted before.
                        
                        if( C_arrow(c) == -2 )
                        {
                            RemoveLoop(a);
                            
                            strand_length = 0;
                            
                            ++color;
                            
                            break;
                        }
                        
                        // One of the 1-neighbors of c has been visisted before.
                        
//                        if( strand_length - C_len(c) > 2 )
//                        {
//                            const bool changedQ = Simplify_1_Strand(a,c);
//
//                            if( changedQ )
//                            {
//                                strand_length = 0;
//                                
//                                ++color;
//                                
//                                break;
//                            }
//                        }
                    }

                    
                    // Create a new strand if necessary.
                    strand_length = resetQ ? 0 : strand_length;
                    color += resetQ;

//                    if( resetQ )
//                    {
//                        print("Starting new strand " + ToString(color) + " at " + CrossingString(c) + "." );
//                    }
                    
                    // Head of arc gets new color.
                    MarkCrossing(c);
                    
                    const Int a_next = C_arcs(c,Out,!side);
                    const Int c_next = A_cross(a_next,Head);

                    MarkNeighbor(c,c_next,Out, side);
                    MarkNeighbor(c,c_next,In ,!side);
                    // If we reset, then c is a start vertex, so we have to mark also the vertex at the back.
                    if( resetQ )
                    {
                        MarkNeighbor(c,c_next,In , side);
                    }

                    a = a_next;
                    
                }
                while( a != a_0 );
                
                ++a_ptr;
            }
            
            ptoc(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
            
            return strand_0_counter;
        }
        
        
    private:
        
        void RemoveStrand( const Int a_begin, const Int a_end )
        {
            // Remove the strand _backwards_ because of how Reconnect uses PD_ASSERT.
            // (It is legal that the head is inactive, but the tail must be active!)
            // Also, this might be a bit cache friendlier.
            Int a = a_end;
            
            do
            {
                const Int c = A_cross(a,Tail);
                const bool side  = (C_arcs(c,Out,Right) == a);
                const Int a_prev = C_arcs(c,In,!side);
                
                // side == Left         side == Right
                //
                //         |                    ^
                //         |                    |
                // a_prev  |   a        a_next  |
                //    ---->X---->          ---->X---->
                //         |c                   |c
                //         |                    |
                //         v                    |
                
                PD_ASSERT( C_arcs(c,In,side) != C_arcs(c,Out,!side) );
                
                // Sometimes we cannot guarantee that the crossing at the intersection of a_begin and a_end is still intact. But that crossing will be deleted anyways. Thus we suppress some assert here.
                Reconnect<false>( C_arcs(c,In,side), Head, C_arcs(c,Out,!side) );
                
                DeactivateArc(a);
                
                // Sometimes we cannot guarantee that all arcs at c are already deactivated. But they will finally be deleted.  Thus we suppress some assert here.
                DeactivateCrossing<false>(c);
                
                a = a_prev;
            }
            while( a != a_begin );
        }
        
        void RemoveLoop( const Int e )
        {
            const Int c_0 = A_cross(e,Head);
            
            if( A_cross(e,Tail) == c_0 )
            {
                Reidemeister_I(c_0,e);
                
                return;
            }
            
            if constexpr( mult_compQ )
            {
                const Int a = pd.NextArc(e);
                // TODO: Check also the case that the strand is a loop
                if( A_color(a) == color )
                {
                    if( (!resetQ) )
                    {
                        // overQ == true;
                        //
                        //                 O
                        //                 |
                        //        e        |        a
                        // ####O----->O--------->O----->O######
                        //                 |c_0
                        //                 |
                        //                 O
                        
                        ++pd.unlink_count;
                        RemoveStrand(e,e);
                        return;
                    }
                    else
                    {
                        // overQ == true;
                        //
                        //                 O                  O
                        //                 |                  |
                        //        e        |        a         |
                        // ####O----->O----|---->O----->O---->X---->O######
                        //                 |c_0               |c_1
                        //                 |                  |
                        //                 O                  O
                        
                        // TODO: We need to insert a new crossing on the vertical strand.
                        
                        eprint(ClassName()+"::RemoveLoop: Case A_color(a_next) == color and resetQ == true not implemented");
                        
                        // The goal should be this:
                        //
                        // overQ == true, upQ == true
                        //
                        //                 O
                        //                 ^
                        //                 |c_1
                        //            O<---------O
                        //            |    |     ^
                        //            |    |     |
                        //          e |    O     | a
                        //            |    ^     |
                        //            v    |     |
                        //            O----|---->O
                        //                 |c_0
                        //                 |
                        //                 O
                        
//                        bool upQ = (C_arcs(c_0,In,Left) == e);
//
//                        const Int c_1 = A_cross(a,Head);
                        
//                        Reconnect(a,Tail,??);

                        return;
                    }
                }
            }
            
            // side == Left
            //
            //                 ^
            //                 |b
            //              e  |
            //            ---->X---->
            //                 |c_0
            //                 |
            //                 |
            //
            // side == Right
            //
            //                 |
            //                 |
            //              e  |
            //            ---->X---->
            //                 |c_0
            //               b |
            //                 v

            PD_ASSERT( (C_arcs(c_0,In,Right) == e) || (C_arcs(c_0,In,Left) == e) );
            
            const bool side = (C_arcs(c_0,In,Right) == e);
                      
            const Int b = C_arcs(c_0,Out,side);
            
            PD_ASSERT( b != e );
            PD_ASSERT( C_arcs(c_0,In,!side) != C_arcs(c_0,Out,!side));

            RemoveStrand(b,e);
            Reconnect( C_arcs(c_0,In,!side), Head, C_arcs(c_0,Out,!side) );
            DeactivateArc(b);
            DeactivateCrossing<false>(c_0);
            ++strand_0_counter;
        }
        
        void Reidemeister_I( const Int c, const Int a )
        {
//            print( "Reidemeister I move at " + ArcString(a) + ".");
            
            const Int a_prev = pd.PrevArc(a);
            const Int a_next = pd.NextArc(a);
            
            if( a_prev == a_next )
            {
                ++pd.unlink_count;
                DeactivateArc(a);
                DeactivateArc(a_prev);
                DeactivateCrossing(c);
                pd.R_I_counter += 2;
                
                AssertArc<0>(a  );
                AssertArc<0>(a_prev);
                AssertArc<0>(a_next);
                AssertCrossing<0>(c);
                
                return;
            }
            
            Reconnect(a_prev,Head,a_next);
            DeactivateArc(a);
            DeactivateCrossing(c);
            ++pd.R_I_counter;
            
            AssertArc<0>(a  );
            AssertArc<1>(a_prev);
            AssertArc<0>(a_next);
            AssertCrossing<0>(c);
            
            return;
        }

        
        bool Simplify_1_Strand( const Int b, const Int c )
        {
            AssertCrossing<1>(c);
            
            // This is the arc that connects c to the visited crossing.
            const Int a = C_arrow(c);
            
            AssertArc<1>(c);
            
            print("Strand-1-Loop detected at " + ArcString(a));
            
            // We may choose c_0 and c_1 so that a points from c_0 to c_1.
            // This removes a couple of case distinctions.
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);
            
            AssertCrossing<1>(c_0);
            AssertCrossing<1>(c_1);
            
            // Copy of code from ArcSimplifier::load_c_0
            
            // Whether the vertical strand at c_0 points upwards.
            const bool u_0 = (C_arcs(c_0,Out,Right) == a);
            
            const Int n_0 = C_arcs( c_0, !u_0,  Left  );
            const Int s_0 = C_arcs( c_0,  u_0,  Right );
            const Int w_0 = C_arcs( c_0,  In , !u_0   );
            
            AssertArc<1>(n_0);
            AssertArc<1>(s_0);
            AssertArc<1>(w_0);
            
            // Copy of code from ArcSimplifier::load_c_1
            
            // Whether the vertical strand at c_1 points upwards.
            const bool u_1 = (C_arcs(c_1,In ,Left ) == a);
            
            const Int n_1 = C_arcs(c_1,!u_1, Left );
            const Int e_1 = C_arcs(c_1, Out, u_1  );
            const Int s_1 = C_arcs(c_1, u_1, Right);
            
            AssertArc<1>(n_1);
            AssertArc<1>(e_1);
            AssertArc<1>(s_1);

            /*              n_0           n_1
             *               O             O
             *               |      a      |
             *       w_0 O---X-->O---->O---X-->O e_1
             *               |c_0          |c_1
             *               O             O
             *              s_0           s_1
             */
            
//            Int count_strands = 0;
//            
//            count_strands += (A_colors[n_1] == color);
//            count_strands += (A_colors[e_1] == color);
//            count_strands += (A_colors[s_1] == color);
//            count_strands += (A_colors[s_0] == color);
//            count_strands += (A_colors[w_0] == color);
//            count_strands += (A_colors[n_0] == color);
//            
//            if( count_strands != 2 )
//            {
//                dump( A_colors[n_1] == color );
//                dump( A_colors[e_1] == color );
//                dump( A_colors[s_1] == color );
//                dump( A_colors[s_0] == color );
//                dump( A_colors[w_0] == color );
//                dump( A_colors[n_0] == color );
//            }
            
            
            // TODO: Figure out in which basic case we are.
            
//            ++strand_1_counter;
//            return true;
            
            return false;
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("StrandSimplifier")
                + "<" + TypeName<Int>
                + ">";
        }

    }; // class StrandSimplifier
            
} // namespace KnotTools








//        bool operator()()
//        {
//
//            ptic(ClassName()+"::operator()");
//
//            const Int n = C_arcs.Dimension(0);
//            const Int m = A_cross.Dimension(0);
////
////            Tensor3<Int ,Int> C_strands  ( n, 2, 2, -1 );
////            Tensor1<char,Int> A_visisted ( m, false );
//
//            Int color = 0;
//            Int a_ptr = 0;
//
//            while( a_ptr < m )
//            {
//                // Search for next arc that is active.
//                while( ( a_ptr < m ) && (!ArcActiveQ(a_ptr)) )
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
//                // Find the beginning of first overstrand.
//                // For this, we go back along a until ArcUnderQ(a,Tail)
//                while( ArcUnderQ(a,Tail) != overQ )
//                {
//                    a = PrevArc(a);
//                }
//
//                S_ptr = 0;
//
//                S_[S_ptr] = a;
//
//                const Int a_0 = a;
//
//                c_0 = A_cross(a,Head);
//                n_0 =
//                w_0 =
//                s_0 =
//
//                C_color[c_0] = color;
//                A_color[n_0] = color;
//                A_color[s_0] = color;
//                A_color[w_0] = color;
//
//                c_1 = A_cross(a,Head);
//                n_1 =
//                e_1 =
//                s_1 =
//
//
//                C_color[a  ] = color;
//                A_color[n_1] = color;
//                A_color[e_1] = color;
//                A_color[s_1] = color;
//
//                // Traverse forward through all arcs in the link component, until we return where we started.
//                do
//                {
//
//                    const bool overQ = (ArcUnderQ(a,Head) != overQ);
//
//                    if( overQ )
//                    {
//                        n_0 = n_1;
//                        w_0 = a  ;
//                        s_0 = s_1;
//                          a = e_1;
//
//                        S_buffer[S_ptr++] = a;
//
//                        c_1 = A_cross(a,Head);
//                        n_1 =
//                        e_1 =
//                        s_1 =
//
//                        if( C_color[c_1] == color )
//                        {
//                            // loop
//                            //                          # s
//                            //                          #
//                            //                          #
//                            //        O n_0             O n_1
//                            //        |                 ^
//                            // s      |        a        |
//                            // ###O-------O-------->O-------O e_1
//                            //        |c_0              |c_1
//                            //        |                 |
//                            //        O s_0             O s_1
//                            //
//                            // Check s_1 == e_1 !
//                            //
//                            // or
//                            //
//                            //        O n_0             O n_1
//                            //        |                 ^
//                            // s      |        a        |
//                            // ###O-------O-------->O-------O#####
//                            //        |c_0              |c_1
//                            //        |                 |
//                            //        O s_0             O s_1
//                            //
//                        }
//
//                        if( A_color[n_1] == color )
//                        {
//
//
//                        }
//
//                        if( A_color[e_1] == color )
//                        {
//
////                                    O n_0             O n_1
////                                    |                 ^
////                             s      |        a        |     e_1
////                             ###O-------O-------->O-------O####
////                                    |c_0              |c_1
////                                    |                 |
////                                    O s_0             O s_1
////
////                             ==> ++i = unlink_count
//                        }
//
//                        if( A_color[s_1] == color )
//                        {
//                            //        O n_0             O n_1
//                            //        |                 ^
//                            // s      |        a        |
//                            // ###O-------O-------->O-------O e_1
//                            //        |c_0              |c_1
//                            //        |                 v
//                            //        O s_0             O
//                            //                          | s_1
//                            //                          |
//                            //                          O
//                            //                          |
//                            //                          |
//                            //                  ####O---X---O
//                            //                          |c_2
//                            //                          |
//                            //                          O
//                            //
//                            //
//                            // or
//                            //
//                            //        O n_0             O n_1
//                            //        |                 ^
//                            // s      |        a        |
//                            // ###O-------O-------->O-------O e_1
//                            //        |c_0              |c_1
//                            //        |                 v
//                            //        O s_0             O
//                            //                          | s_1
//                            //                          |
//                            //                          O
//                            //                          |
//                            //                          |
//                            //                      O---X---O###
//                            //                          |c_2
//                            //                          |
//                            //                          O
//                            //
//                            //
//                            // or
//                            //
//                            //        O n_0             O n_1
//                            //        |                 ^
//                            // s      |        a        |
//                            // ###O-------O-------->O-------O e_1
//                            //        |c_0              |c_1
//                            //        |                 v
//                            //        O s_0             O
//                            //                          | s_1
//                            //                          |
//                            //                          O
//                            //                          |
//                            //                          |
//                            //                      O---X---O
//                            //                          |c_2
//                            //                          |
//                            //                          O
//                            //                          #
//                            //                          #
//
//                        }
//
//                        C_color[a  ] = color;
//                        A_color[n_1] = color;
//                        A_color[e_1] = color;
//                        A_color[s_1] = color;
//                    }
//                    else if( !overQ )
//                    {
//                        color++;
//                    }
//                }
//                while( a != a_0 );
//
//                ++a_ptr;
//            }
//
//            ptoc(ClassName()+"::operator()");
//
//            return false;
//        }
