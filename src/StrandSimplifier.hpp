#pragma once

namespace KnotTools
{
    template<typename Int_>
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
        
        Tensor2<Int,Int> C_colors;
        Tensor1<Int,Int> A_colors;
        
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
        :   pd     { pd_        }
        ,   C_arcs { pd.C_arcs  }
        ,   C_state{ pd.C_state }
        ,   A_cross{ pd.A_cross }
        ,   A_state{ pd.A_state }
        ,   C_colors{ C_arcs.Dimension(0), 2, -1 }
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
        
        void DeactivateCrossing( const Int c_ )
        {
            pd.DeactivateCrossing(c_);
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
        

        void Reconnect( const Int a_, const bool headtail, const Int b_ )
        {
            
//            print( std::string("Reconnecting ") + (headtail ? "Head" : "Tail") + " of " + ArcString(a_) + " to " + (headtail ? "Head" : "Tail") + " of " + ArcString(b_) + "." );
            
            pd.Reconnect(a_,headtail,b_);
            
//            // TODO: Two crossings are touched multiply.
//            if constexpr ( use_flagsQ )
//            {
//                TouchCrossing(A_cross(a,Tail));
//                TouchCrossing(A_cross(a,Head));
//            }
        }
        
    public:
        
//        Int TestCount() const
//        {
//            return test_count;
//        }
        
        template<bool overQ = true>
        Int RemoveStrandLoops()
        {
            ptic(ClassName()+"::Remove" + (overQ ? "Over" : "Under")  + "StrandLoops");
            
//            const Int n = C_arcs.Dimension(0);
            const Int m = A_cross.Dimension(0);
//
//            Tensor1<char,Int> A_visisted ( m, false );
            
            C_colors.Fill(-1);
            A_colors.Fill(-1);
            
            Int color = 0;
            Int a_ptr = 0;

            Int strand_length    = 0;
            Int strand_0_counter = 0;
            Int strand_1_counter = 0;
            
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
                
//                print("Starting new strand " + ToString(color) + " at " + ArcString(a) + "." );
                C_colors(A_cross(a,Tail),0) = color;
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
                    ++strand_length;
                    
                    if( color == std::numeric_limits<Int>::max() )
                    {
                        ptoc(ClassName()+"::Remove" + (overQ ? "Over" : "Under")  + "StrandLoops");
                        
                        return strand_0_counter + strand_1_counter;
                    }
                     
                    const Int c = A_cross(a,Head);
                    
                    if( C_colors(c,0) == color )
                    {
                        if( strand_length == 1)
                        {
                            Reidemeister_I(a);
                        }
                        else
                        {
                            RemoveLoop(a);
                        }
                        
                        strand_length = 0;
                        
                        ++strand_0_counter;
                        
                        ++color;
                        
                        break;
                    }
                    
//                    if( C_colors(c,1) == color )
//                    {
//                        if( strand_length > 1 )
//                        {
//                            //                        RemoveLoop(a);
//                            print("Strand-1-Loop detected at " + CrossingString(c));
//                            
//                            dump(strand_length);
//                            
////                            strand_length = 0;
////                            
////                            ++strand_1_counter;
////                            
////                            ++color;
////                            
////                            break;
//                        }
//                    }

                    
                    // Arc gets old color.
                    A_colors[a] = color;
//                    A_visisted[a] = true;
                    
                    // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to initialize a new strand.
                    
//                    color += (pd.ArcUnderQ(a,Head) == overQ);
                    
                    // We have fetched A_cross(a,Head) already, so instead of using ArcUnderQ we evaluated resetQ by hand.
                    const bool side = (C_arcs(c,In ,Right) == a);
                    
                    const bool resetQ = ((side == pd.CrossingRightHandedQ(c)) == overQ);
                    
                    strand_length = resetQ ? 0 : strand_length;
                    
                    color += resetQ;

                    // Head of arc gets new color.
                    C_colors(c,0) = color;
                    
//                    C_colors(pd.NextCrossing(c,Out, side),1) = color;
//                    C_colors(pd.NextCrossing(c,In ,!side),1) = color;
                    
//                    a = pd.NextArc(a);
                    
                    // We have fetched A_cross(a,Head) already, so by inlining the logic of NextArc we could save a bit of time here.
                    a = C_arcs(c,Out,!side);
                }
                while( a != a_0 );
                
                ++a_ptr;
            }
            
            ptoc(ClassName()+"::Remove" + (overQ ? "Over" : "Under")  + "StrandLoops");
            
            return strand_0_counter;
        }
        
        void Reidemeister_I( const Int a )
        {
            const Int c_0 = A_cross(a,Head);
            
            const Int a_prev = pd.PrevArc(a);
            const Int a_next = pd.NextArc(a);
            
            if( a_prev == a_next )
            {
                ++pd.unlink_count;
                DeactivateArc(a);
                DeactivateArc(a_prev);
                DeactivateCrossing(c_0);
                pd.R_I_counter += 2;
                
                AssertArc<0>(a  );
                AssertArc<0>(a_prev);
                AssertArc<0>(a_next);
                AssertCrossing<0>(c_0);
                
                return;
            }
            
            Reconnect(a_prev,Head,a_next);
            DeactivateArc(a);
            DeactivateCrossing(c_0);
            ++pd.R_I_counter;
            
            AssertArc<0>(a  );
            AssertArc<1>(a_prev);
            AssertArc<0>(a_next);
            AssertCrossing<0>(c_0);
            
            return;
        }
        
        void RemoveLoop( const Int a )
        {
            if( A_cross(a,Tail) == A_cross(a,Head) )
            {
//                print( "Reidemeister I move at " + ArcString(a) + ".");
                
                Reidemeister_I(a);
                
                return;
            }
            
            const Int c_0 = A_cross(a,Head);
            
//            print( "Removing loop at " + CrossingString(c_0) + ".");
            
            // side == Left
            //
            //                 ^
            //                 |b_0
            //              a  |
            //            ---->X---->
            //                 |c_0
            //                 |
            //                 |
            //
            // side == Right
            //
            //                 |
            //                 |
            //              a  |
            //            ---->X---->
            //                 |c_0
            //              b_0|
            //                 v

            PD_ASSERT( (C_arcs(c_0,In,Right) == a) || (C_arcs(c_0,In,Left) == a) );
            
            const bool side_0 = C_arcs(c_0,In,Right) == a;
                      
            const Int b_0 = C_arcs(c_0,Out,side_0);
            
            Int b = b_0;
            
            do
            {
                const Int c = A_cross(b,Head);
                const bool side  = (C_arcs(c,In,Right) == b);
                const Int b_next = C_arcs(c,Out,!side);
                
                // side == Right        side == Left
                //
                //         |                    ^
                //         |                    |
                //      b  |   b_next        b  |   b_next
                //    ---->X---->          ---->X---->
                //         |c                   |c
                //         |                    |
                //         v                    |
                
                Reconnect( C_arcs(c,In,!side), Head, C_arcs(c,Out,side) );
                DeactivateArc(b);
                DeactivateCrossing(c);
                
                b = b_next;
            }
            while( b != a );
            
            Reconnect( C_arcs(c_0,In,!side_0), Head, C_arcs(c_0,Out,!side_0) );
            DeactivateArc(a);
            DeactivateCrossing(c_0);
        }
        
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
        
    private:
    

        
    public:
        
        static std::string ClassName()
        {
            return std::string("StrandSimplifier")
                + "<" + TypeName<Int>
                + ">";
        }

    }; // class StrandSimplifier
            
} // namespace KnotTools
