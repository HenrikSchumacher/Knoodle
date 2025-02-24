#pragma once


// TODO: Performing possible Reidemeister II moves an a strand might unlock further moves. More importantly, it will reduce the length of the current strand and thus make Dijkstra faster!

// TODO: Speed this up by reducing the redirecting in NextLeftArc by using and maintaining ArcLeftArc.

// TODO: It seems to be faster to interleave the over-/understrand passes with recompression. This begs the question whether we should exploit the recompression step to compute some interesting information, e.g. all the over-/understrands.

// TODO: It might also be worthwhile to do the overstrand and the understrand pass in the same loop. (But I doubt it.)

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
        
        using CrossingContainer_T       = typename PD_T::CrossingContainer_T;
        using ArcContainer_T            = typename PD_T::ArcContainer_T;
        using CrossingStateContainer_T  = typename PD_T::CrossingStateContainer_T;
        using ArcStateContainer_T       = typename PD_T::ArcStateContainer_T;
        
        static constexpr bool mult_compQ = mult_compQ_;
        
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Out   = PD_T::Out;
        
    private:
        
        PD_T & restrict pd;
        
        CrossingContainer_T      & restrict C_arcs;
        CrossingStateContainer_T & restrict C_state;
        
        ArcContainer_T           & restrict A_cross;
        ArcStateContainer_T      & restrict A_state;

        Tensor1<Int,Int>         & restrict A_visited_from;
        
    private:
        
        Tensor2<Int,Int> C_data;
        
        Tensor2<Int,Int> A_data;
        Tensor1<Int,Int> A_colors;
        
        Int * restrict A_left;
        
        Int color = 1; // We start with 1 so that we can store things in the sign bit of color.
        Int a_ptr = 0;

        Int strand_length  = 0;
        Int change_counter = 0;
        
        bool overQ;
        bool strand_completeQ;
                
        Stack<Int,Int> next_front;
        Stack<Int,Int> prev_front;
        
        Tensor1<Int,Int> path;
        Int path_length = 0;
        
        std::vector<Int> touched;
        std::vector<Int> duds;
        
        double Time_RemoveLoop            = 0;
        double Time_FindShortestPath      = 0;
        double Time_RerouteToShortestPath = 0;
        double Time_RepairArcLeftArcs     = 0;
        double Time_SimplifyStrands       = 0;
        double Time_CollapseArcRange      = 0;
        
    public:
        
        StrandSimplifier() = delete;
        
        StrandSimplifier( PD_T & pd_ )
        :   pd             { pd_                        }
        ,   C_arcs         { pd.C_arcs                  }
        ,   C_state        { pd.C_state                 }
        ,   A_cross        { pd.A_cross                 }
        ,   A_state        { pd.A_state                 }
        ,   A_visited_from { pd.A_scratch               }
        ,   C_data         { C_arcs.Dimension(0), 2, -1 }
        // We initialize by 0 because actual colors will have to be positive to use sign bit.
        ,   A_data         { A_cross.Dimension(0), 2, 0 }
        // We initialize by 0 because actual colors will have to be positive to use sign bit.
        ,   A_colors       { A_cross.Dimension(0),  0   }
        ,   next_front     { A_cross.Dimension(0)       }
        ,   prev_front     { A_cross.Dimension(0)       }
        ,   path           { A_cross.Dimension(0), -1   }
        {}
        
        ~StrandSimplifier()
        {
#ifdef PD_TIMINGQ
            logprint("");
            logprint(ClassName() + " absolute timings:");
            logvalprint("SimplifyStrands", Time_SimplifyStrands);
            logvalprint("RemoveLoop", Time_RemoveLoop);
            logvalprint("FindShortestPath", Time_FindShortestPath);
            logvalprint("RerouteToShortestPath", Time_RerouteToShortestPath);
            logvalprint("CollapseArcRange", Time_CollapseArcRange);
            logvalprint("RepairArcLeftArcs", Time_RepairArcLeftArcs);
            logprint("");
            logprint(ClassName() + " relative timings:");
            logvalprint("SimplifyStrands", Time_SimplifyStrands/Time_SimplifyStrands);
            logvalprint("RemoveLoop", Time_RemoveLoop/Time_SimplifyStrands);
            logvalprint("FindShortestPath", Time_FindShortestPath/Time_SimplifyStrands);
            logvalprint("RerouteToShortestPath", Time_RerouteToShortestPath/Time_SimplifyStrands);
            logvalprint("CollapseArcRange", Time_CollapseArcRange/Time_SimplifyStrands);
            logvalprint("RepairArcLeftArcs", Time_RepairArcLeftArcs/Time_SimplifyStrands);
            logprint("");
#endif
        }
        
        StrandSimplifier( const StrandSimplifier & other ) = delete;
        
        StrandSimplifier( StrandSimplifier && other ) = delete;
        

    private:
        
        template<bool must_be_activeQ>
        void AssertArc( const Int a_ ) const
        {
#ifdef DEBUG
            if constexpr( must_be_activeQ )
            {
                if( !pd.ArcActiveQ(a_) )
                {
                    pd_eprint("AssertArc<1>: " + ArcString(a_) + " is not active.");
                }
                PD_ASSERT(pd.CheckArc(a_));
//                PD_ASSERT(CheckArcLeftArc(a_));
            }
            else
            {
                if( pd.ArcActiveQ(a_) )
                {
                    pd_eprint("AssertArc<0>: " + ArcString(a_) + " is not inactive.");
                }
            }
#else
            (void)a_;
#endif
        }
        
        std::string ArcString( const Int a_ )  const
        {
            return pd.ArcString(a_) + " (color = " + ToString(A_color(a_)) + ")";
        }
        
        template<bool must_be_activeQ>
        void AssertCrossing( const Int c_ ) const
        {
#ifdef DEBUG
            if constexpr( must_be_activeQ )
            {
                if( !pd.CrossingActiveQ(c_) )
                {
                    pd_eprint("AssertCrossing<1>: " + CrossingString(c_) + " is not active.");
                }
//                PD_ASSERT(pd.CheckCrossing(c_));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,Out,Left )));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,Out,Right)));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,In ,Left )));
//                PD_ASSERT(CheckArcLeftArc(C_arcs(c_,In ,Right)));
            }
            else
            {
                if( pd.CrossingActiveQ(c_) )
                {
                    pd_eprint("AssertCrossing<0>: " + CrossingString(c_) + " is not inactive.");
                }
            }
#else
            (void)c_;
#endif
        }
        
        std::string CrossingString( const Int c_ ) const
        {
            return pd.CrossingString(c_) + " (color = " + ToString(C_color(c_)) + ")";
        }

        bool CheckStrand( const Int a_begin, const Int a_end, const Int max_arc_count )
        {
            bool passedQ = true;
            
            Int arc_counter = 0;
            
            Int a = a_begin;
            
            if( a_begin == a_end )
            {
                wprint(ClassName()+"::CheckStrand<" + (overQ ? "over" : "under" ) + ">: Strand is trivial: a_begin == a_end.");
            }
            
            while( (a != a_end) && (arc_counter < max_arc_count ) )
            {
                ++arc_counter;
                
                passedQ = passedQ && (pd.template ArcOverQ<Head>(a) == overQ);
                
                a = pd.template NextArc<Head>(a);
            }
            
            if( a != a_end )
            {
                pd_eprint(ClassName()+"::CheckStrand<" + (overQ ? "over" : "under" ) + ">: After traversing strand for max_arc_count steps the end ist still not reached.");
            }
            
            if( !passedQ )
            {
                pd_eprint(ClassName()+"::CheckStrand<" + (overQ ? "over" : "under" ) + ">: Strand is not an" + (overQ ? "over" : "under" ) + "strand.");
            }
            
            return passedQ;
        }
        
        std::string ArcRangeString( const Int a_begin, const Int a_end ) const
        {
            std::string s;

            Int a = a_begin;
            Int i = 0;
            
            s += ToString(0) + " : " + CrossingString(A_cross(a,Tail)) + ")\n"
               + ToString(0) + " : " + ArcString(a)
               + " (" + (pd.template ArcOverQ<Head>(a) ? "over" : "under") + ")\n"
            + ToString(1) + " : " + CrossingString(A_cross(a,Head)) + "\n";

            do
            {
                ++i;
                
                a = pd.template NextArc<Head>(a);
                
                s += ToString(i  ) + " : " +  ArcString(a)
                   + " (" << (pd.template ArcOverQ<Head>(a) ? "over" : "under") + ")\n";
                   + ToString(i+1) + " : " +  CrossingString(A_cross(a,Head)) + "\n";
            }
            while( a != a_end );
            
            return s;
        }
        
        std::string PathString()
        {
            if( path_length <= 0 )
            {
                return "{}";
            }
            
            std::string s;
            
            for( Int p = 0; p < path_length; ++p )
            {
                const Int a = path[p];

                s += ToString(p  ) + " : " +  ArcString(a)
                   + " (" + ToString( Sign(A_data(a,0)) ) + ")\n";
            }
            
            return s;
        }
        
        template<bool must_be_activeQ = true, bool silentQ = false>
        bool CheckArcLeftArc( const Int A )
        {
            bool passedQ = true;
            
            const Int a = (A >> 1);
            
            if( !pd.ArcActiveQ(a) )
            {
                if constexpr( must_be_activeQ )
                {
                    eprint(ClassName()+"::CheckArcLeftArc: Inactive " + ArcString(a) + ".");
                    return false;
                }
                else
                {
                    return true;
                }
            }
            
            const Int A_l_true = pd.NextLeftArc(A);
            const Int A_l      = A_left[A];
            
            if( A_l != A_l_true )
            {
                if constexpr (!silentQ)
                {
                    eprint(ClassName()+"::CheckArcLeftArc: Incorrect at " + ArcString(a) );
                    logprint("Head is        {" + ToString(A_l >> 1)   + "," +  ToString(A_l & Int(1))  + "}.");
                    logprint("Head should be {" + ToString(A_l_true >> 1) + "," +  ToString(A_l_true & Int(1)) + "}.");
                }
                passedQ = false;
            }
            
            return passedQ;
        }
    
    public:
        
        bool CheckArcLeftArcs()
        {
            duds.clear();
            
            for( Int a = 0; a < A_cross.Dimension(0); ++a )
            {
                if( pd.ArcActiveQ(a) )
                {
                    const Int A = (a << 1) | Int(Head);
                    
                    if( !CheckArcLeftArc<false,true>(A) )
                    {
                        duds.push_back(A);
                    }
                    
                    const Int B = (a << 1) | Int(Tail);
                    
                    if( !CheckArcLeftArc<false,true>(B) )
                    {
                        duds.push_back(B);
                    }
                }
            }
            
            if( duds.size() > 0 )
            {
                std::sort(duds.begin(),duds.end());
                
                TOOLS_LOGDUMP(duds);
                
                std::sort(touched.begin(),touched.end());
                
                TOOLS_LOGDUMP(touched);
                
                logprint(ClassName()+"::CheckArcLeftArcs failed.");
                
                return false;
            }
            else
            {
//                TOOLS_LOGDUMP(duds);
                
//                logprint(ClassName()+"::CheckArcLeftArcs passed.");
                
                return true;
            }
        }

    private:
        
        mref<Int> C_color( const Int c )
        {
            return C_data(c,0);
        }
        
        cref<Int> C_color( const Int c ) const
        {
            return C_data(c,0);
        }
        
        mref<Int> A_color( const Int a )
        {
            return A_colors[a];
        }
        
        cref<Int> A_color( const Int a ) const
        {
            return A_colors[a];
        }

        
        mref<Int> C_len( const Int c )
        {
            return C_data(c,1);
        }

        void MarkCrossing( const int c )
        {
            C_data(c,0) = color;
            C_data(c,1) = strand_length;
        }
        
    private:

        
        void RepairArcLeftArc( const Int A )
        {
            if( pd.ArcActiveQ(A >> 1) )
            {
                const Int A_l = pd.NextLeftArc(A);
                const Int A_r = pd.NextRightArc(A) ^ Int(1);
                
//                PD_DPRINT("RepairArcLeftArc touched a   = " + ArcString(A   >> 1) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_l = " + ArcString(A_l >> 1) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_r = " + ArcString(A_r >> 1) + ".");
                
                A_left[A]   = A_l;
                A_left[A_r] = A ^ Int(1);
            }
        }
        
        void RepairArcLeftArcs()
        {
            PD_TIC(ClassName() + "::RepairArcLeftArcs");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
//            for( Int A = 0; A < 2 * pd.initial_arc_count; ++A )
//            {
//                RepairArcLeftArc(A);
//            }
            
            for( Int A : touched )
            {
                RepairArcLeftArc(A);
            }
            
            PD_ASSERT(CheckArcLeftArcs());
            
            touched.clear();

#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_RepairArcLeftArcs += Tools::Duration(start_time,stop_time);
#endif
            PD_TOC(ClassName() + "::RepairArcLeftArcs");
            
            
        }

        template<bool headtail>
        void TouchArc( const Int a )
        {

            const Int A = (a << 1) | Int(headtail);
            
            touched.push_back(A);
            touched.push_back(pd.NextRightArc(A) ^ Int(1));
            
//            const Int B = A ^ Int(1);
//            touched.push_back(B);
//            touched.push_back( pd.NextRightArc(B) ^ Int(1));
        }
        
        void TouchArc( const Int a, const bool headtail )
        {
            if( headtail)
            {
                TouchArc<Head>(a);
            }
            else
            {
                TouchArc<Tail>(a);
            }
        }
        
        template<bool headtail, bool deactivateQ = true>
        void Reconnect( const Int a, const Int b )
        {
#ifdef PD_DEBUG
            std::string tag (ClassName() + "::Reconnect " + (headtail ? "Head of " : "Tail of ") + ArcString(a) + " to " + (headtail ? "Head of " : "Tail of ") + ArcString(b) + "." );
#endif
            
            PD_TIC(tag);

//            AssertArc<1>(a);
            
            PD_ASSERT( pd.ArcActiveQ(a) );
            
            const Int c = A_cross(b,headtail);

            const bool side = (C_arcs(c,headtail,Right) == b);

            A_cross(a,headtail) = c;
            C_arcs(c,headtail,side) = a;
            
            TouchArc(a,headtail);
            
            if constexpr( deactivateQ )
            {
                pd.DeactivateArc(b);
            }
            else
            {
                TouchArc(b,headtail);
            }

            PD_TOC(tag);
        }

        
        /*! @brief Attempts to perform a Reidemeister I move to arc `a` and returns `true` if an actual move has been performed. If called as `Reidemeister_I<false>`, the check for being a loop arc will be skipped and the surgery is performed right away. This may cause errors, though. If `Reidemeister_I<true>`, then the move is performed only if the check was successful.
         */

        template<bool checkQ>
        bool Reidemeister_I( const Int a )
        {
            const Int c = A_cross(a,Head);
            
            if constexpr( checkQ )
            {
                if( A_cross(a,Tail) != c )
                {
                    return false;
                }
            }
            
            PD_DPRINT( ClassName() + "::Reidemeister_I at " + ArcString(a) );
            
            // We assume here that we already know that a is a loop arc.
            
            PD_ASSERT( A_cross(a,Tail) == c );
            
            // const bool side = (C_arcs(c,In,Right) == a);
            
            // The allows a 50% chance that we do not have to load `C_arcs(c,In,Left)` again.
            const bool side = (C_arcs(c,In,Left) != a);
            
            const Int a_next = C_arcs(c,Out,!side);
            const Int a_prev = C_arcs(c,In ,!side);
            
            if( a_prev == a_next )
            {
                //
                //             O-----+ a
                //             |     |
                //             |     |
                //       O-----+-----O
                //       |     |c
                //       |     |
                //       +-----O
                //   a_prev = a_next
                
                ++pd.unlink_count;
                pd.DeactivateArc(a);
                pd.DeactivateArc(a_prev);
                pd.DeactivateCrossing(c);
                pd.R_I_counter += 2;
                
                return true;
            }
            
            //             O-----+ a
            //             |     |
            //             |     |
            //       O-----+---->O
            // a_prev      |c
            //             V
            //             O
            //              a_next
            
            Reconnect<Head>(a_prev,a_next);
            pd.DeactivateArc(a);
            pd.DeactivateCrossing(c);
            ++pd.R_I_counter;
            
            AssertArc<1>(a_prev);
            
            return true;
        }
        
        bool Reidemeister_II_Backward(
            mref<Int> a, const Int c_1, const bool side_1, const Int a_next
        )
        {
            // TODO: This checks for a backward Reidemeister II move (and it exploits already that both vertical strands are either overstrands or understrands). But it would maybe help to do a check for a forward Reidemeister II move, because that might stop the strand from ending early. Not sure whether this would be worth it, because we would have to change the logic of `SimplifyStrands` considerably.
            
            const Int c_0 = A_cross(a,Tail);
            
            const bool side_0 = (C_arcs(c_0,Out,Right) == a);
            
            //             a == C_arcs(c_0,Out, side_0)
            //        a_prev == C_arcs(c_0,In ,!side_0)
            const Int a_0_out = C_arcs(c_0,Out,!side_0);
            const Int a_0_in  = C_arcs(c_0,In , side_0);
            
            //             a == C_arcs(c_1,In , side_1)
            //        a_next == C_arcs(c_1,Out,!side_1)
            const Int a_1_out = C_arcs(c_1,Out, side_1);
            const Int a_1_in  = C_arcs(c_1,In ,!side_1);
            
            //             O a_0_out         O a_1_in
            //             ^                 |
            //             |                 |
            //  a_prev     |c_0     a        v     a_next
            //  ----->O-------->O------>O-------->O------>
            //             ^                 |c_1
            //             |                 |
            //             |                 v
            //             O a_0_in          O a_1_out
                
            if( a_0_out == a_1_in )
            {
                PD_DPRINT("Detected Reidemeister II at ingoing port of " + CrossingString(c_1) + ".");
                
                const Int a_prev = C_arcs(c_0,In,!side_0);
                
                PD_ASSERT( a_0_in  != a_next );
                PD_ASSERT( a_1_in  != a_next );
                
                PD_ASSERT( a_0_out != a_prev );
                PD_ASSERT( a_1_out != a_prev );
                
                //              a_0_out == a_1_in
                //             O---------------->O
                //             ^                 |
                //             |                 |
                //  a_prev     |c_0     a        v     a_next
                //  ----->O-------->O------>O-------->O------>
                //             ^                 |c_1
                //             |                 |
                //             |                 v
                //             O a_0_in          O a_1_out
                
                Reconnect<Head>(a_prev,a_next);
                
                if( a_0_in == a_1_out )
                {
                    ++pd.unlink_count;
                    pd.DeactivateArc(a_0_in);
                }
                else
                {
                    Reconnect<Tail>(a_1_out,a_0_in);
                }
                
                pd.DeactivateArc(a);
                pd.DeactivateArc(a_0_out);
                pd.DeactivateCrossing(c_0);
                pd.DeactivateCrossing(c_1);
                ++pd.R_II_counter;
                
                AssertArc<1>(a_prev);
                AssertArc<0>(a_next);
                
                ++change_counter;
                
                // Tell StrandSimplify to move back to previous arc, so that coloring and loop checking are performed correctly.
                strand_length -= 2;
                a = a_prev;
                
                PD_ASSERT(strand_length >= 0 );
                
                return true;
            }
            
            if( a_0_in == a_1_out )
            {
                PD_DPRINT("Detected Reidemeister II at outgoing port of " + CrossingString(c_1) + ".");
                
                const Int a_prev = C_arcs(c_0,In,!side_0);
                
                // This cannot happen because we called RemoveLoop.
                PD_ASSERT( a_0_out != a_prev );
                
                // TODO: Might be impossible if we call Reidemeister_I on a_next.
                // A nasty case that is easy to overlook.
                if( a_1_in == a_next )
                {
                    //             O a_0_out         O<---+ a_1_in == a_next
                    //             ^                 |    |
                    //             |                 |    |
                    //  a_prev     |c_0     a        v    |
                    //  ----->O-------->O------>O-------->O
                    //             ^                 |c_1
                    //             |                 |
                    //             |                 v
                    //             O<----------------O
                    //              a_0_in == a_1_out
                    //
                    
                    Reconnect<Head>(a_prev,a_0_out);
                    pd.DeactivateArc(a_next);
                    pd.R_I_counter+=2;
                }
                else
                {
                    //             O a_0_out         O a_1_in
                    //             ^                 |
                    //             |                 |
                    //  a_prev     |c_0     a        v     a_next
                    //  ----->O-------->O------>O-------->O------>
                    //             ^                 |c_1
                    //             |                 |
                    //             |                 v
                    //             O<----------------O
                    //              a_0_in == a_1_out
                    //
                    
                    Reconnect<Head>(a_prev,a_next);
                    Reconnect<Tail>(a_0_out,a_1_in);
                    ++pd.R_II_counter;
                }
                
                pd.DeactivateArc(a);
                pd.DeactivateArc(a_0_in);
                pd.DeactivateCrossing(c_0);
                pd.DeactivateCrossing(c_1);
                
                AssertArc<1>(a_prev);
                AssertArc<0>(a_next);
                
                ++change_counter;
                
                // Tell StrandSimplify to move back to previous arc, so that coloring and loop checking are performed correctly.
                strand_length -= 2;
                a = a_prev;
                
                PD_ASSERT(strand_length >= 0 );
                
                return true;
            }

            return false;
        }
        
        
        void Prepare()
        {
            PD_TIC(ClassName() + "::Prepare");
            
            PD_ASSERT(pd.CheckAll());
            
            PD_ASSERT(pd.CheckNextLeftArc());
            PD_ASSERT(pd.CheckNextRightArc());
            
            if( color >= std::numeric_limits<Int>::max()/2 )
            {
                C_data.Fill(-1);
                A_data.Fill(0);
                A_colors.Fill(0);
                
                color = 0;
            }
            
            A_left = pd.ArcLeftArc().data();
            
            PD_ASSERT(CheckArcLeftArcs());

            PD_TOC(ClassName() + "::Prepare");
        }
        
    public:
        
        template<bool R_II_Q = true>
        Int SimplifyStrands(
            bool overQ_, const Int max_dist = std::numeric_limits<Int>::max()
        )
        {
            overQ = overQ_;
            
            PD_TIC(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif

            const Int m = A_cross.Dimension(0);

            Prepare();
            
            // We increase `color` for each strand. This ensures that all entries of A_data, A_colors, C_data etc. are invalidated. In addition, `Prepare` resets these whenever color is at least half of the maximal integer of type Int (rounded down).
            // We typically use Int = int32_t (or greater). So we can handle 2^30-1 strands in once call to `SimplifyStrands`. That should really be enough for all feasible applications.
            
            ++color;
            a_ptr = 0;
            
            const Int color_0 = color;

            strand_length = 0;
            change_counter = 0;
            
            while( a_ptr < m )
            {
                // Search for next arc that is active and has not yet been handled.
                while(
                    ( a_ptr < m )
                    &&
                    ( (A_color(a_ptr) >= color_0 ) || (!pd.ArcActiveQ(a_ptr)) )
                )
                {
                    ++a_ptr;
                }
                
                if( a_ptr >= m )
                {
                    break;
                }
                
                // Find the beginning of first strand.
                Int a_begin = WalkBackToStrandStart(a_ptr);
                
                // If we arrive at this point again, we break the do loop below.
                const Int a_0 = a_begin;
                
                // We should make sure that a_0 is not a loop.
                if( Reidemeister_I<true>(a_0) )
                {
                    RepairArcLeftArcs();
                    ++change_counter;
                    continue;
                }
                
                // Might be changed by RerouteToShortestPath.
                // This we indeed a reference, not a copy here.
                ArcState & a_0_state = A_state[a_0];
                
                // Current arc.
                Int a = a_0;
                
                strand_length = 0;
                
                MarkCrossing( A_cross(a,Tail) );
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
                    // TODO: I think we should increment this at the same place where we set a = a_next.
                    ++strand_length;
                    
                    if( color == std::numeric_limits<Int>::max() )
                    {
#ifdef PD_TIMINGQ
                        const Time stop_time = Clock::now();
                        
                        Time_SimplifyStrands += Tools::Duration(start_time,stop_time);
#endif
                        PD_TOC(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
                        
                        // Return a positive number to encourage that this function is called again upstream.
                        return change_counter+1;
                    }
                    
                    Int c_1 = A_cross(a,Head);

                    AssertCrossing<1>(c_1);
                    
                    const bool side_1 = (C_arcs(c_1,In,Right) == a);
                    
                    Int a_next = C_arcs(c_1,Out,!side_1);
                    AssertArc<1>(a_next);
                    
                    Int c_next = A_cross(a_next,Head);
                    AssertCrossing<1>(c_next);
                    
                    // TODO: Not sure whether this adds anything good. RemoveLoop is also able to remove Reidemeister I loops. It just does it in a slightly different ways.
                    if( c_next == c_1 )
                    {
                        // overQ == true
                        //
                        //                  +-----+
                        //      |     |     |     | a_next
                        //      |     |  a  |     |
                        // ---------------->|-----+
                        //      |     |     |c_1
                        //      |     |     |

                        // overQ == true
                        //
                        //                  +-----+
                        //      |     |     |     | a_next
                        //      |     |  a  |     |
                        // ---------------->------+
                        //      |     |     |c_1
                        //      |     |     |
                        
                        // In any case, the loop would prevent the strand from going further.
                        
                        // TODO: We could pass `a` as `a_prev` to `Reidemeister_I`.
                        (void)Reidemeister_I<false>(a_next);
                        
                        PD_ASSERT(a_next != a);
                        
                        ++change_counter;
                        --strand_length;
                        RepairArcLeftArcs();
                        
                        // TODO: I think it would be safe to continue without checking that `a_0` is still active. We should first complete the current strand.
                        
                        if( ActiveQ(a_0_state) )
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    // Arc gets old color.
                    A_color(a) = color;
                    
                    
                    // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
                    // This finds out whether we have to reset.
                    strand_completeQ = ((side_1 == pd.CrossingRightHandedQ(c_1)) == overQ);
                    
                    // TODO: This might require already that there are no possible Reidemeister II moves along the strand.
                    
                    // Check for loops.
                    
                    if( C_color(c_1) == color )
                    {
                        // Vertex c has been visted before.

                        
                        if constexpr ( mult_compQ )
                        {
                            // We must insert this check because we might otherwise get problems when there is a configuration like this:
                            
                            /*
                             *            +-----+
                             *            |     |
                             *         a  |a_ptr|
                             *      +-----|-----------+
                             *      |  c_1|     |     |
                             *      |     |     |     |
                             *      |     +-----+     |
                             *      |                 |
                             *      +-----------------+
                             *
                             */
                            
                            const Int a_next = pd.template NextArc<Head>(a,c_1);
                            
                            if( !strand_completeQ || (A_color(a_next) != color)  )
                            {
                                RemoveLoop(a,c_1);
                                
                                RepairArcLeftArcs();
                                
                                break;
                            }
                        }
                        else
                        {
                            RemoveLoop(a,c_1);
                            
                            RepairArcLeftArcs();
                            
                            break;
                        }
                    }

                    // overQ == true
                    //
                    //                  +--------+
                    //      |     |     |        |
                    //      |     |  a  | a_next |
                    // ---------------->|------->|----
                    //      |     |     |c_1     |
                    //      |     |     |        |
                    
                    // TODO: It might even be worth to do this before RemoveLoop is called.
                    
//                    if( c_1 == A_cross(a_next,Head) )
//                    {
//                        logprint("R_I ahead");
//                    }
                    
                    // TODO: Why not just calling `ArcSimplifier` on `a_next` and in case of changes, revisit `a`? (And don't forget to repair `A_left_buffer`!)

                    if constexpr ( R_II_Q )
                    {
                        if( (!strand_completeQ) && (a != a_begin) )
                        {
                            const bool changedQ = Reidemeister_II_Backward(a,c_1,side_1,a_next);
                            
                            if( changedQ )
                            {
                                RepairArcLeftArcs();
                                
                                if( ActiveQ(a_0_state) )
                                {
                                    // Reidemeister_II_Backward reset `a` to the previous arc.
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                    
                    if( strand_completeQ )
                    {
                        bool changedQ = false;
                        
                        if( (strand_length > 1) && (max_dist > 0) )
                        {
                            changedQ = RerouteToShortestPath(
                                a_begin,a,Min(strand_length-1,max_dist),color
                            );
                            
                            if( changedQ )
                            {
                                RepairArcLeftArcs();
                            }
                        }
                        
                        // TODO: Check this.
//                        if( changedQ && (!ActiveQ(a_0_state)) )
                        if( changedQ || (!ActiveQ(a_0_state)) )
                        {
                            // RerouteToShortestPath might deactivate `a_0`, so that we could never return to it. Hence we rather break the while loop here.
                            break;
                        }
                        
                        // Create a new strand.
                        strand_length = 0;
                        a_begin = a_next;

                        ++color;
                    }
                    
                    // Head of arc gets new color.
                    MarkCrossing(c_1);

                    AssertArc<1>(a_next);
                    AssertArc<1>(a_0);
                    AssertArc<1>(a);
                    
                    a = a_next;
                    
                }
                while( a != a_0 );
                
                // TODO: If the link has multiple components, it can happen that we cycled around a full unlink lying on top (or below) the remaining diagram.
                
//                if( a == a_begin )
//                {
//                    wprint(ClassName()+"::SimplifyStrands: Split unlink detected. We have yet to remove it." );
//                }
                
                ++color;
                strand_length = 0;
                
                ++a_ptr;
            }
            
#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_SimplifyStrands += Tools::Duration(start_time,stop_time);
#endif
            
            PD_TOC(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
            
            return change_counter;
        }
        
        
    private:
        
        void CollapseArcRange( const Int a_begin, const Int a_end, const Int arc_count_ )
        {
//            PD_DPRINT( ClassName() + "::CollapseArcRange" );

            
            // This shall collapse the arcs `a_begin, pd.NextArc<Head>(a_begin),...,a_end` to a single arc.
            // All crossings from the head of `a_begin` up to the tail of `a_end` have to be reconnected and deactivated.
            // Finally, we reconnect the head of arc `a_begin` to the head of `a_end` and deactivate a_end.
            
            // We remove the arcs _backwards_ because of how Reconnect uses PD_ASSERT.
            // (It is legal that the head is inactive, but the tail must be active!)
            // Also, this might be a bit cache friendlier.

            if( a_begin == a_end )
            {
                return;
            }
            
            PD_TIC(ClassName() + "::CollapseArcRange(" + ToString(a_begin) + "," + ToString(a_end) + "," + ToString(arc_count_) +  ")");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
            
            Int a = a_end;
            
            Int iter = 0;
            
            // arc_count_ is just an upper bound to prevent infinite loops.
            while( (a != a_begin) && (iter < arc_count_) )
            {
                ++iter;
                
                const Int c = A_cross(a,Tail);
                
//                const bool side  = (C_arcs(c,Out,Right) == a);
//                const Int a_prev = C_arcs(c,In,!side);
                
                // side == Left         side == Right
                //
                //         |                    ^
                //         |                    |
                // a_prev  |   a        a_prev  |   a
                //    ---->X---->          ---->X---->
                //         |c                   |c
                //         |                    |
                //         v                    |
                
//                PD_ASSERT( C_arcs(c,In,side) != C_arcs(c,Out,!side) );

//                Reconnect<Head>( C_arcs(c,In,side),C_arcs(c,Out,!side) );
                
                Int a_prev;
                
                if( C_arcs(c,Out,Right) == a )
                {
                    a_prev = C_arcs(c,In,Left);
                    
                    // Sometimes we cannot guarantee that the crossing at the intersection of `a_begin` and `a_end` is still active. But that crossing will be deleted anyways. Thus we suppress some asserts here.
                    Reconnect<Head>( C_arcs(c,In,Right),C_arcs(c,Out,Left) );
                }
                else
                {
                    a_prev = C_arcs(c,In,Right);
                    
                    // Sometimes we cannot guarantee that the crossing at the intersection of `a_begin` and `a_end` is still active. But that crossing will be deleted anyways. Thus we suppress some asserts here.
                    Reconnect<Head>( C_arcs(c,In,Left),C_arcs(c,Out,Right) );
                }
                
                pd.DeactivateArc(a);
                
                // Sometimes we cannot guarantee that all arcs at `c` are already deactivated. But they will finally be deleted. Thus we suppress some asserts here.
                pd.template DeactivateCrossing<false>(c);
                
                a = a_prev;
            }
            
            PD_ASSERT( a == a_begin );
            
            PD_ASSERT( iter <= arc_count_ );

            // a_end is already deactivated.
            PD_ASSERT( !pd.ArcActiveQ(a_end) );
            
            Reconnect<Head,false>(a_begin,a_end);
            
            ++change_counter;
            
#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_CollapseArcRange += Tools::Duration(start_time,stop_time);
#endif
            PD_TOC(ClassName() + "::CollapseArcRange(" + ToString(a_begin) + "," + ToString(a_end) + "," + ToString(arc_count_) +  ")");
        }
        
        void RemoveLoop( const Int e, const Int c_0 )
        {
            PD_TIC(ClassName() + "::RemoveLoop");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif

            // We can save the lookup here.
//            const Int c_0 = A_cross(e,Head);
            
            // TODO: If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well.
            if constexpr( mult_compQ )
            {
                const Int a = pd.template NextArc<Head>(e,c_0);

                // TODO: Can this ever happen?
                // TODO: -> Yes!!!
                
                if( A_color(a) == color )
                {
                    const bool u_0 = (C_arcs(c_0,Out,Right) == a);
                    
                    ++pd.unlink_count;
                    CollapseArcRange(a,e,strand_length);
                    
                    pd.DeactivateArc(a);
                    
                    // overQ == true;
                    //                   n_0
                    //                 O
                    //                 |
                    //        e        |        a
                    // ####O----->O--------->O----->O######
                    //                 |c_0
                    //                 |
                    //                 O
                    //                  s_0
                    
                    // We read out n_0 and s_0 only now because CollapseArcRange might change the north and south port of c_0.
                    
                    const Int n_0 = C_arcs(c_0,!u_0,Left );
                    const Int s_0 = C_arcs(c_0, u_0,Right);

                    if( s_0 == n_0 )
                    {
                        ++pd.unlink_count;
                        pd.DeactivateArc(s_0);
                        
                    }
                    else
                    {
                        if( u_0 )
                        {
                            Reconnect<Head,true>(s_0,n_0);
                        }
                        else
                        {
                            Reconnect<Tail,true>(s_0,n_0);
                        }
                    }
                    
                    pd.DeactivateCrossing(c_0);
                    
                    ++change_counter;
                    
                    PD_TOC(ClassName() + "::RemoveLoop");
                    
                    
                    return;
                }
            }
            
            // side == Left             side == Right
            //
            //           ^                        |
            //           |b                       |
            //        e  |                     e  |
            //      ---->X---->              ---->X---->
            //           |c_0                     |c_0
            //           |                      b |
            //           |                        v
            
            const bool side = (C_arcs(c_0,In,Right) == e);
                      
            const Int b = C_arcs(c_0,Out,side);
            
            CollapseArcRange(b,e,strand_length);

            // Now b is guaranteed to be a loop arc. (e == b or e is deactivated.)
            
            (void)Reidemeister_I<false>(b);
            
            ++change_counter;
            
#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_RemoveLoop += Tools::Duration(start_time,stop_time);
#endif
            
            PD_TOC(ClassName() + "::RemoveLoop");
        }
        
        
    public:
        
        /*!
         * Run Dijkstra's algorithm to find the shortest path from arc `a_begin` to `a_end` in the graph G, where the vertices of G are the arcs and where there is an edge between two such vertices if the corresponding arcd share a common face.
         *
         * If an improved path has been found, its length is stored in `path_length` and the actual path is stored in the leading positions of `path`.
         *
         * @param a_begin  Starting arc.
         *
         * @param a_end    Final arc to which we want to find a path
         *
         * @param max_dist Maximal distance we want to travel
         *
         * @param color_    Indicator that is written to first column of `A_data`; this avoid have to erase the whole matrix `A_data` for each new search. When we call this, we assume that `A_data` contains only values different from `color`.
         *
         */
        
        Int FindShortestPath(
            const Int a_begin, const Int a_end, const Int max_dist, const Int color_
        )
        {
            PD_TIC(ClassName()+"::FindShortestPath");
                        
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
            
            PD_ASSERT( color_ > 0 );
            
            PD_ASSERT( a_begin != a_end );
            
            // Instead of a queue we use to stacks: One to hold the next front; and for the previous (or current) front.
            
            next_front.Reset();
            
            // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
            
            next_front.Push( (a_begin << 1) | Int(Head) );
            next_front.Push( (a_begin << 1) | Int(Tail) );
            
            A_data(a_begin,0) = color_;
            A_data(a_begin,1) = a_begin;
            
            // This is needed to prevent us from running in circles when cycling around faces.
            // See comments below.
            A_visited_from[a_begin] =  a_begin;
            
            Int d = 0;
            
            while( (d < max_dist) && (!next_front.EmptyQ()) )
            {
                // Actually, next_front must never become empty. Otherwise something is wrong.
                swap( prev_front, next_front );
                
                next_front.Reset();
                
                // We don't want paths of length max_dist.
                // The elements of prev_front have distance d.
                // So the element we push onto next_front will have distance d+1.
                
                ++d;

                while( !prev_front.EmptyQ() )
                {
                    const Int A_0 = prev_front.Pop();
                    
                    const Int a_0 = (A_0 >> 1);
                    
                    // Now we run though the boundary arcs of the face using `A_left` to turn always left.
                    // There is only one exception and that is when the arc we get to is part of the strand (which is when `A_color(a) == color_`).
                    // Then we simply go straight through the crossing.

                    // arc a_0 itself does not have to be processed because that's where we are coming from.
                    Int A = A_left[A_0];

                    do
                    {
                        Int  a   = (A >> 1);
                        bool dir = (A & Int(1));
                        
                        AssertArc<1>(a);
                        
                        const bool part_of_strandQ = (A_color(a) == color_);

                        // Check whether `a` has not been visited, yet.
                        if( Abs(A_data(a,0)) != color_ )
                        {

                            if( a == a_end )
                            {
                                // Mark as visited.
                                A_data(a,0) = color_;
                                
                                // Remember from which arc we came
                                A_data(a,1) = a_0;
                                
                                goto exit;
                            }
                            
                            PD_ASSERT( a != a_begin );
                            
                            if( part_of_strandQ )
                            {
                                // This arc is part of the strand itself, but neither a_begin nor a_end. We just skip it and do not make it part of the path.
                            }
                            else
                            {
                                // We make A_data(a,0) positive if we cross arc `a` from left to right. Otherwise it is negative.
                                
                                A_data(a,0) = dir ? color_ : -color_;
                                
                                // Remember the arc we came from.
                                A_data(a,1) = a_0;

                                next_front.Push(A ^ Int(1));
                            }
                        }
                        else if constexpr ( mult_compQ )
                        {
                            // When the diagram becomes disconnected by removing the strand, then there can be faces whose boundary has two components. Even weirder, it can happen that we start at `a_0` and then go to the component of the face boundary that is _not_ connected to `a_0`. This is difficult to imagine, so here is a picture:
                            
                            
                            // The == indicates the current overstrand.
                            //
                            // If we start at `a_0 = a_begin`, then we turn left and get to the split link component, which happens to be a trefoil. Since we ignore all arcs that are part of the strand, we cannot leave this trefoil! This is not too bad because the ideal path would certainly not cross the trefoil. We have just to make sure that we do not cycle indefinitely around the trefoil.
                            //
                            //                      +---+
                            //                      |   |
                            //               +------|-------+
                            //   ------+     |      |   |   |   +---
                            //         |     |      +---|---+   |
                            //         | a_0 |          |       |  a_end |
                            //      ---|=================================|---
                            //         |     |          |       |        |
                            //         |     +----------+       |
                            //   ------+                        +---
                            
                            // TODO: Maybe this is needed only with multiple components?
                            if( A_visited_from[a] == a_0 )
                            {
                                break;
                            }
                        }
                        
                        // TODO: Maybe this is needed only with multiple components?
                        if constexpr ( mult_compQ )
                        {
                            A_visited_from[a] = a_0;
                        }

                        if( part_of_strandQ )
                        {
                            A = A_left[A ^ Int(1)];
                        }
                        else
                        {
                            A = A_left[A];
                        }
                    }
                    while( A != A_0 );
                }
            }
            
            // If we get here, then d+1 = max_dist or next_front.EmptyQ().
            
            // next_front.EmptyQ() should actually never happen because we know that there is a path between the arcs!
            
            if( next_front.EmptyQ() )
            {
                wprint(ClassName()+"::ShortestPath: next_front is empty, but shortest path is not found, yet.");
            }
            
            d = max_dist + 1;
            
        exit:
            
            // Write the actual path to array `path`.
            
            if( (0 <= d) && (d <= max_dist) )
            {
                // The only way to get here with `d <= max_dist` is the `goto` above.
                
                if( Abs(A_data(a_end,0)) != color_ )
                {
                    pd_eprint(ClassName() + "::FindShortestPath");
                    TOOLS_LOGDUMP(d);
                    TOOLS_LOGDUMP(max_dist);
                    TOOLS_LOGDUMP(a_end);
                    TOOLS_LOGDUMP(color_);
                    TOOLS_LOGDUMP(color);
                    TOOLS_LOGDUMP(A_data(a_end,0));
                    TOOLS_LOGDUMP(A_data(a_end,1));
                }
                
                Int a = a_end;
                
                path_length = d+1;
                
                for( Int i = 0; i < path_length; ++i )
                {
                    path[path_length-1-i] = a;
                    
                    a = A_data(a,1);
                }
            }
            else
            {
                path_length = 0;
            }

#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_FindShortestPath += Tools::Duration(start_time,stop_time);
#endif
            
            PD_TOC(ClassName()+"::FindShortestPath");
            
            return d;
        }
        
        
        // Only meant for debugging. Don't do this in production!
        Tensor1<Int,Int> GetShortestPath(
            const Int a_begin, const Int a_end, const Int max_dist, const Int color_
        )
        {
            const Int d  = FindShortestPath( a_begin, a_end, max_dist, color_ );

            if( d <= max_dist )
            {
                Tensor1<Int,Int> path_out (path_length);
                
                path_out.Read( path.data() );
                
                return path_out;
            }
            else
            {
                return Tensor1<Int,Int>();
            }
        }
        
 private:
                
        
        /*!
         * @brief Attempts to reroute the strand.
         *
         * @param a_begin The first arc of the strand on entry as well as as on return.
         *
         * @param a_end The last arc of the strand (included) as well as on entry as on return. Note that this values is a reference and that the passed variable will likely be changedQ!
         *
         * @param max_dist The maximal distance Dijkstra's algorithm will wander.
         *
         * @param color_ The color of the path.
         */
        
        bool RerouteToShortestPath(
            const Int a_begin, mref<Int> a_end, const Int max_dist, const Int color_
        )
        {
            PD_ASSERT(CheckArcLeftArcs());
            
#ifdef PD_DEBUG
            const Int Cr_0 = pd.CrossingCount();
            
            TOOLS_LOGDUMP(Cr_0);
#endif
            
            const Int d = FindShortestPath( a_begin, a_end, max_dist, color_ );
            
            if( (d < 0) || (d > max_dist) )
            {
                PD_DPRINT("No improvement detected.");
                
                return false;
            }
            
            PD_TIC(ClassName()+"::RerouteToShortestPath");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
            
            PD_TIC("Prepare reroute loop");
            
            // path[0] == a_begin. This is not to be crossed.
            
            Int p = 1;
            Int a = a_begin;
            
            // At the beginning of the strand we want to avoid inserting a crossing on
            // arc b if b branches directly off from the current strand.
            //
            //      |         |        |        |
            //      |    a    |        |        |    current strand
            //    --|-------->-------->-------->-------->
            //      |         |        |        |
            //      |         |        |        |
            //                b
            //

            MoveWhileBranching<Head>(a,p);
            
            // Now a is the first arc to be rerouted.
            
            // Same at the end.
            
            Int q = path_length-1;
            Int e = a_end;
  
            MoveWhileBranching<Tail>(e,q);

            // Now e is the last arc to be rerouted.
            
            // Handle the rerouted arcs.
            PD_TOC("Prepare reroute loop");
            
            PD_TIC("Reroute loop");
            
            while( p < q )
            {
                const Int c_0 = A_cross(a,Head);
                
                const bool side = (C_arcs(c_0,Head,Right) != a);
                
                const Int a_2 = C_arcs(c_0,Out,side);
                
                const Int b = path[p];
                
                const bool dir = (A_data(b,0) > 0);
                
                const Int c_1 = A_cross(b,Head);
                
                // This is the situation for `a` before rerouting for side == Right;
                //
                //
                //                ^a_1
                //      |         |        |        |
                //      |    a    | a_2    |        |   e
                //    --|-------->-------->--......-->-------->
                //      |         ^c_0     |        |
                //      |         |        |        |
                //                |a_0
                //
                
                // a_0 is the vertical incoming arc.
                const Int a_0 = C_arcs(c_0,In , side);
                
                // a_1 is the vertical outgoing arc.
                const Int a_1 = C_arcs(c_0,Out,!side);
                
                // In the cases b == a_0 and b == a_1, can we simply leave everything as it is!
                PD_ASSERT( b != a_2 )
                PD_ASSERT( b != a )
                
                if( (b == a_0) || (b == a_1) )
                {
                    // Go to next arc.
                    a = a_2;
                    ++p;
                    
                    continue;
                }
                
                PD_ASSERT( a_0 != a_1 );
                
                
                // The case dir == true.
                // We cross arc `b` from left to right.
                //
                // Situation of `b` before rerouting:
                //
                //             ^
                //             | b_next
                //             |
                //             O
                //             ^
                //             |
                //  ------O----X----O------
                //             |c_1
                //             |
                //             O
                //             ^
                //  >>>>>>     | b
                //             |
                //             X
                //
                // We want to change this into the following:
                //
                //             ^
                //             | b_next
                //             |
                //             O
                //             ^
                //             |
                //  ------O----X----O------
                //             |c_1
                //             |
                //             O
                //             ^
                //             | a_1
                //             |
                //             O
                //             ^
                //    a        |     a_2
                //  ----->O----X----O----->
                //             |c_0
                //             |
                //             O
                //             ^
                //             | b
                //             |
                //             X

                Reconnect<Head,false>(a_0,a_1);

                A_cross(b,Head) = c_0;
                
                // Caution: This might turn around a_1!
                A_cross(a_1,Tail) = c_0;
                A_cross(a_1,Head) = c_1;
                
                pd.template SetMatchingPortTo<In>(c_1,b,a_1);

                
                // Recompute c_0. We have to be aware that the handedness and the positions of the arcs relative to c_0 can completely change!
                
                if( dir )
                {
                    C_state[c_0] = overQ
                                 ? CrossingState::RightHanded
                                 : CrossingState::LeftHanded;
                    
                    // overQ == true
                    //
                    //             O
                    //             ^
                    //             | a_1
                    //             |
                    //             O
                    //             ^
                    //    a        |     a_2
                    //  ----->O---------O----->
                    //             |c_0
                    //             |
                    //             O
                    //             ^
                    //             | b == path[p]
                    //             |
                    //             X
                    
//                    // Very likely, `A_left_buffer(a,Head)` and `A_left_buffer(a_2,Tail)` are next to each other in memory.
//                    A_left_buffer(a  ,Head) = ((a_1 << 1) | Head);
//                    A_left_buffer(a_2,Tail) = ((b   << 1) | Tail);
//                    
//                    A_left_buffer(b  ,Head) = ((a   << 1) | Tail);
//                    A_left_buffer(a_1,Tail) = ((a_2 << 1) | Head);
                    
                    // Fortunately, this does not depend on overQ.
                    const Int buffer [4] = { a_1,a_2,a,b };
                    copy_buffer<4>(&buffer[0],C_arcs.data(c_0));
                }
                else // if ( !dir )
                {
                    C_state[c_0] = overQ
                                 ? CrossingState::LeftHanded
                                 : CrossingState::RightHanded;
                    
                    // overQ == true
                    //
                    //             O
                    //             ^
                    //             | a_1
                    //             |
                    //             O
                    //             ^
                    //    a_2      |        a
                    //  <-----O---------O<-----
                    //             |c_0
                    //             |
                    //             O
                    //             ^
                    //             | b == path[p]
                    //             |
                    //             X
                    
//                    // Very likely, `A_left_buffer(a,Head)` and `A_left_buffer(a_2,Tail)` are next to each other in memory.
//                    A_left_buffer(a  ,Head) = ((b   << 1) | Tail);
//                    A_left_buffer(a_2,Tail) = ((a_1 << 1) | Head);
//                    
//                    A_left_buffer(b  ,Head) = ((a_2 << 1) | Head);
//                    A_left_buffer(a_1,Tail) = ((a   << 1) | Tail);
                    
                    // Fortunately, this does not depend on overQ.
                    const Int buffer [4] = { a_2,a_1,b,a };
                    copy_buffer<4>(&buffer[0],C_arcs.data(c_0));
                }
                
                
//                // Mark arcs so that they will be repaired by `RepairArcLeftArcs`;
                TouchArc<Head>(a_0);
                touched.push_back( (a   << 1) | Int(Head) );
                touched.push_back( (b   << 1) | Int(Head) );
                touched.push_back( (a_1 << 1) | Int(Tail) );
                touched.push_back( (a_2 << 1) | Int(Tail) );

//                TouchArc<Head>(a  );
//                TouchArc<Head>(b  );
//
//                TouchArc<Tail>(a_1);
//                TouchArc<Tail>(a_2);
                
                AssertCrossing<1>(c_0);
                AssertCrossing<1>(c_1);
                AssertArc<1>(a);
                AssertArc<1>(a_0);
                AssertArc<1>(a_1);
                AssertArc<1>(a_2);
                AssertArc<1>(b);

                // Go to next of arc.
                a = a_2;
                ++p;
            }
            
            PD_TOC("Reroute loop");
            
            // strand_length is just an upper bound to prevent infinite loops.
            CollapseArcRange(a,e,strand_length);
            
            AssertArc<1>(a);
            AssertArc<0>(e);
            
            TouchArc<Head>(a);

            a_end = a;

#ifdef PD_DEBUG
            const Int Cr_1 = pd.CrossingCount();
#endif
            
            PD_ASSERT(Cr_1 < Cr_0);
            
            PD_DPRINT(ToString(Cr_0 - Cr_1) + " crossings removed.");
            
#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_RerouteToShortestPath += Tools::Duration(start_time,stop_time);
#endif
            
            ++change_counter;
            
            PD_TOC(ClassName()+"::RerouteToShortestPath");
            
            return true;
        }
        
    public:
    
        
        template<typename Int_0, typename Int_1, typename Int_2>
        void ColorArcs( cptr<Int_0> arcs, const Int_1 strand_length_, const Int_2 color_ )
        {
            static_assert( IntQ<Int_0>, "" );
            static_assert( IntQ<Int_1>, "" );
            static_assert( IntQ<Int_2>, "" );
            
            const Int n = int_cast<Int>(strand_length_);
            const Int c = int_cast<Int>(color_);
            
            if( c <= 0 )
            {
                pd_eprint("Argument color_ is <= 0. This is an illegal color.");
                
                return;
            }
            
            for( Int i = 0; i < n; ++i )
            {
                A_color(arcs[i]) = c;
            }
        }
        
        void SetStrandMode( const bool overQ_ )
        {
            overQ = overQ_;
        }
        
    private:

        
        Int WalkBackToStrandStart( const Int a_0 ) const
        {
            Int a = a_0;

            AssertArc<1>(a);

            // TODO: We could maybe save some lookups here if we inline ArcUnderQ and NextArc. However, I tried it and it did not improve anything. Probably because this loop is typically not very long or because the compiler is good at optimizing this on its own. So I decided to keep this for better readibility.
            
            // Take a first step in any case.
            if( pd.template ArcUnderQ<Tail>(a) != overQ  )
            {
                a = pd.template NextArc<Tail>(a);
                AssertArc<1>(a);
            }
            
            // If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well. So we need a guard against cycling around this unlink forever! ----------------+
            //                                                      |
            //                                                      V
            while( (pd.template ArcUnderQ<Tail>(a) != overQ) && (a != a_0) )
            {
                a = pd.template NextArc<Tail>(a);
                AssertArc<1>(a);
            }

            // We could catch the unlink already here, but that would need some change of communication here. Instead, we dealy this to the do-loop in StrandSimplifier. This will be double work, but only in very rare cases.
            
            return a;
        }

        template<bool headtail>
        void MoveWhileBranching( mref<Int> a, mref<Int> p ) const
        {
            Int c = A_cross(a,headtail);
            
            Int side = (C_arcs(c,headtail,Right) == a);
            
            Int b = path[p];
            
            while( (C_arcs(c,headtail,!side) == b) || (C_arcs(c,!headtail,side) == b) )
            {
                a = C_arcs(c,!headtail,!side);
                
                c = A_cross(a,headtail);

                side = (C_arcs(c,headtail,Right) == a);
                
                p = headtail ? p + Int(1) : p - Int(1);
                
                b = path[p];
            }
        }
    
    public:
        
        static std::string ClassName()
        {
            return std::string("StrandSimplifier")
                + "<" + TypeName<Int>
                + "," + ToString(mult_compQ)
                + ">";
        }

    }; // class StrandSimplifier
    
} // namespace KnotTools
