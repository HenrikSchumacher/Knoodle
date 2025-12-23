#pragma once


// DONE: Performing possible Reidemeister II moves an a strand might unlock further moves. More importantly, it will reduce the length of the current strand and thus make Dijkstra faster!

// DONE: Speed this up by reducing the redirecting in NextLeftArc by using and maintaining ArcLeftArc.

// TODO: Move into PlanarDiagram class.

// TODO: It might also be worthwhile to do the overstrand and the understrand pass in the same loop. (But I doubt it.)

namespace Knoodle
{
    
    template<typename Int_, bool mult_compQ_>
    class alignas( ObjectAlignment ) StrandSimplifier final
    {
    public:
        
//        static_assert(SignedIntQ<Int_>,"");
        
        using Int  = Int_;
        
        using PD_T = PlanarDiagram<Int>;
        
        // We need a signed integer type for Color_T because we use "negative" color values to indicate directions in the dual graph.
        using Color_T = ToSigned<Int>;
        
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
        
        static constexpr bool Uninitialized = PD_T::Uninitialized;
        
    private:
        
        PD_T & restrict pd;
        
        CrossingContainer_T      & restrict C_arcs;
        CrossingStateContainer_T & restrict C_state;
        
        ArcContainer_T           & restrict A_cross;
        ArcStateContainer_T      & restrict A_state;

        Tensor1<Int,Int>         & restrict A_source;
        
    private:
        
        // Colors for the crossings and arcs to mark the current strand.
        // We use this to detect loop strands.
        Tensor1<Color_T,Int> C_color;
        Tensor1<Color_T,Int> A_color;
        
        Tensor1<Color_T,Int> A_ccolor;  // colors for the dual arcs
        // A_from[a] is the dual arc from which we visited dual arc a in the pass with color A_ccolor[a].
        Tensor1<Int,Int> A_from;
        
        Int * restrict dA_left;
        
        Color_T color = 1; // We start with 1 so that we can store things in the sign bit of color.
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
        
        StrandSimplifier( PD_T & pd_ )
        :   pd         { pd_                             }
        ,   C_arcs     { pd.C_arcs                       }  
        ,   C_state    { pd.C_state                      }
        ,   A_cross    { pd.A_cross                      }
        ,   A_state    { pd.A_state                      }
        ,   A_source   { pd.A_scratch                    }
        // We initialize by 0 because actual colors will have to be positive to use sign bit.
        ,   C_color    { C_arcs.Dim(0),  Color_T(0)      }
        // We initialize by 0 because actual colors will have to be positive to use sign bit.
        ,   A_color    { A_cross.Dim(0), Color_T(0)      }
        // We initialize by 0 because actual colors will have to be positive to use sign bit.
        ,   A_ccolor   { A_cross.Dim(0), Color_T(0)      }
        ,   A_from     { A_cross.Dim(0), Uninitialized   }
        ,   next_front { A_cross.Dim(0)                  }
        ,   prev_front { A_cross.Dim(0)                  }
        ,   path       { A_cross.Dim(0), Uninitialized   }
        {}
        
        // No default constructor
        StrandSimplifier() = delete;
        
        // Destructor
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
        
        // Copy constructor
        StrandSimplifier( const StrandSimplifier & other ) = default;
        // Copy assignment operator
        StrandSimplifier & operator=( const StrandSimplifier & other ) = default;
        // Move constructor
        StrandSimplifier( StrandSimplifier && other ) = default;
        // Move assignment operator
        StrandSimplifier & operator=( StrandSimplifier && other ) = default;
        

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
#ifdef PD_DEBUG
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
                   + " (" + (pd.template ArcOverQ<Head>(a) ? "over" : "under") + ")\n"
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
                   + " (" + ToString( Sign(A_ccolor(a)) ) + ")\n";
            }
            
            return s;
        }
        
        template<bool must_be_activeQ = true, bool silentQ = false>
        bool CheckArcLeftArc( const Int da )
        {
            bool passedQ = true;
            
            auto [a,dir] = PD_T::FromDarc(da);
            
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
            
            const Int da_l_true = pd.NextLeftArc(da);
            const Int da_l      = dA_left[da];
            
            if( da_l != da_l_true )
            {
                if constexpr (!silentQ)
                {
                    eprint(ClassName()+"::CheckArcLeftArc: Incorrect at " + ArcString(a) );
                    
                    auto [a_l_true,dir_l_true] = PD_T::FromDarc(da_l_true);
                    auto [a_l     ,dir_l     ] = PD_T::FromDarc(da_l);
                    
                    logprint("Head is        {" + ToString(a_l)   + "," +  ToString(dir_l)  + "}.");
                    logprint("Head should be {" + ToString(a_l_true) + "," +  ToString(dir_l_true) + "}.");
                }
                passedQ = false;
            }
            
            return passedQ;
        }
    
    public:
        
        bool CheckArcLeftArcs()
        {
            duds.clear();
            
            const Int m = A_cross.Dim(0);
            
            for( Int a = 0; a < m; ++a )
            {
                if( pd.ArcActiveQ(a) )
                {
                    const Int da = PD_T::template ToDarc<Head>(a);
                    
                    if( !CheckArcLeftArc<false,true>(da) )
                    {
                        duds.push_back(da);
                    }
                    
                    const Int db = PD_T::template ToDarc<Tail>(a);
                    
                    if( !CheckArcLeftArc<false,true>(db) )
                    {
                        duds.push_back(db);
                    }
                }
            }
            
            if( duds.size() > Size_T(0) )
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
        
//        mref<Color_T> C_color( const Int c )
//        {
//            return C_colors(c);
//        }
//        
//        cref<Color_T> C_color( const Int c ) const
//        {
//            return C_colors(c);
//        }
        
//        mref<Int> C_len( const Int c )
//        {
//            return C_colors(c,1);
//        }
//        
//        cref<Int> C_len( const Int c ) const
//        {
//            return C_colors(c,1);
//        }
//        
//        
//        mref<Color_T> A_color( const Int a )
//        {
//            return A_colors[a];
//        }
//        
//        cref<Color_T> A_color( const Int a ) const
//        {
//            return A_colors[a];
//        }

        void MarkCrossing( const Int c )
        {
            C_color(c) = color;
        }
        
    private:

        
        void RepairArcLeftArc( const Int da )
        {
            auto [a,dir ] = PD_T::FromDarc(da);
            
            if( pd.ArcActiveQ(a) )
            {
                const Int da_l = pd.NextLeftArc(da);
                const Int da_r = PD_T::FlipDarc(pd.NextRightArc(da));
                
//                PD_DPRINT("RepairArcLeftArc touched a   = " + ArcString(a) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_l = " + ArcString(a_l) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_r = " + ArcString(a_r) + ".");
                
                dA_left[da]   = da_l;
                dA_left[da_r] = PD_T::FlipDarc(da);
            }
        }
        
        void RepairArcLeftArcs()
        {
            PD_TIC(ClassName()+"::RepairArcLeftArcs");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
            
            for( Int da : touched )
            {
                RepairArcLeftArc(da);
            }
            
            PD_ASSERT(CheckArcLeftArcs());
            
            touched.clear();

#ifdef PD_TIMINGQ
            const Time stop_time = Clock::now();
            
            Time_RepairArcLeftArcs += Tools::Duration(start_time,stop_time);
#endif
            PD_TOC(ClassName()+"::RepairArcLeftArcs");
        }

        template<bool headtail>
        void TouchArc( const Int a )
        {
            const Int da = PD_T::ToDarc(a,headtail);
            
            touched.push_back(da);
            touched.push_back(PD_T::FlipDarc(pd.NextRightArc(da)));
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
            std::string tag  (ClassName()+"::Reconnect<" + (headtail ? "Head" : "Tail") +  ", " + BoolString(deactivateQ) + "," ">( " + ArcString(a) + ", " + ArcString(b) + " )" );
#endif
            
            PD_TIC(tag);
            
            PD_ASSERT( pd.ArcActiveQ(a) );
            
            const Int c = A_cross(b,headtail);

            const bool side = (C_arcs(c,headtail,Right) == b);

            A_cross(a,headtail) = c;
            C_arcs(c,headtail,side) = a;
            
            // TODO: Handle over/under in ArcState.
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

        /*! @brief Checks whether arc `a` is a loop arc. In the affirmative case it performs a Reidemeister I move and returns `true`. Otherwise, it returns `false`.
         *  _Caution:_ This routine is equivalent to `PlanarDiagram<Int>::Private_Reidemeister_I`, except that it uses the modified routine `StrandSimplifier<Int>::Reconnect` that is equivalent to `PlanarDiagram<Int>::Reconnect` except that it makes modified arcs by `PlanarDiagram<Int>::TouchArc`!
         *
         * @param a The arc to check as possibly, to remove.
         *
         * @tparam checkQ Whether the check for `a` being a loop is activated (`true`), which is the default.
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
            
            PD_DPRINT( ClassName()+"::Reidemeister_I at " + ArcString(a) );
            
            // We assume here that we already know that a is a loop arc.
            
            PD_ASSERT( A_cross(a,Tail) == c );
            
            // const bool side = (C_arcs(c,In,Right) == a);
            
            // This allows a 50% chance that we do not have to load `C_arcs(c,In,Left)` again.
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
                
                PD_ASSERT(strand_length >= Int(0) );
                
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
                
                PD_ASSERT(strand_length >= Int(0) );
                
                return true;
            }

            return false;
        }
        
        
    private:
        
        void Prepare()
        {
            PD_TIC(ClassName()+"::Prepare");
            
            PD_ASSERT(pd.CheckAll());
            
            PD_ASSERT(pd.CheckNextLeftArc());
            PD_ASSERT(pd.CheckNextRightArc());
            
            if( color >= std::numeric_limits<Color_T>::max()/2 )
            {
                C_color.Fill(Color_T(0));
                A_color.Fill(Color_T(0));
                
                A_ccolor.Fill(Color_T(0));
                A_from.Fill(Int(0));

                color = Color_T(0);
            }
            
            // We do not have to erase A_source, because it is only written from when a A_ccolor check is successful.
//            pd.A_source.Fill(Uninitialized);
            dA_left = pd.ArcLeftArc().data();
            
            PD_ASSERT(CheckArcLeftArcs());

            PD_TOC(ClassName()+"::Prepare");
        }
        
        void Cleanup()
        {
            PD_TIC(ClassName()+"::Cleanup");
            
            dA_left = nullptr;
            
            pd.ClearCache("ArcLeftArc");
            
            PD_TOC(ClassName()+"::Cleanup");
        }
        
    public:
        
        void SetStrandMode( const bool overQ_ )
        {
            overQ = overQ_;
        }
        
        /*!@brief This is the main routine of the class. It is supposed to reroute all over/understrands to shorter strands, if possible. It does so by traversing the diagram and looking for over/understrand. When a complete strand is detected, it runs Dijkstra's algorithm in the dual graph of the diagram _without the currect strand_. If a shorter path is detected, the strand is rerouted. The returned integer is a rough(!) indicator of how many changes accoured. 0 is returned only of no changes have been made and if there is no need to call this function again. A positive value indicates that it would be worthwhile to call this function again (maybe after some further simplifications).
         *
         * @param overQ_ Whether to simplify overstrands (true) or understrands(false).
         *
         * @param max_dist Maximal lengths of rerouted strands. If no rerouting is found that makes the end arcs this close to each other, the rerouting attempt is aborted. Typically, we should set this to "infinity".
         *
         * @tparam R_II_Q Whether to apply small Reidemeister II moves on the way.
         */
        
        template<bool R_II_Q = true>
        Int SimplifyStrands(
            bool overQ_, const Int max_dist = std::numeric_limits<Int>::max()
        )
        {
            // TODO: Maybe I should break this function into a few smaller pieces.
            
            SetStrandMode(overQ_);
            
            TOOLS_PTIMER(timer,ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands<" + ToString(R_II_Q) + ">(" + ToString(max_dist) + ")" );
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
//            if( max_dist <= Int(0) ) { return 0; }
            
            if( max_dist <= Int(0) ) { return 0; }
            
            Prepare();
            
            const Int m = A_cross.Dim(0);
            
            // We increase `color` for each strand. This ensures that all entries of A_ccolor,A_from, A_visited_from, A_color, C_color etc. are invalidated. In addition, `Prepare` resets these whenever color is at least half of the maximal integer of type Int (rounded down).
            // We typically use Int = int32_t (or greater). So we can handle 2^30-1 strands in one call to `SimplifyStrands`. That should really be enough for all feasible applications.
            
            ++color;
            a_ptr = 0;
            
            const Color_T color_0 = color;

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
                
                if( a_ptr >= m ) { break; }
                
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
                
                // Might be changed by RerouteToShortestPath_impl.
                // Thus, we need indeed a reference, not a copy here.
                ArcState & a_0_state = A_state[a_0];
                
                // Current arc.
                Int a = a_0;
                
                strand_length = 0;
                
                MarkCrossing( A_cross(a,Tail) );
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
                    ++strand_length;
                    
                    // Safe guard against integer overflow.
                    if( color == std::numeric_limits<Color_T>::max() )
                    {
#ifdef PD_TIMINGQ
                        const Time stop_time = Clock::now();
                        
                        Time_SimplifyStrands += Tools::Duration(start_time,stop_time);
#endif
                        
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
                    
                    // Arc gets current color.
                    A_color(a) = color;
                    
                    
                    // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
                    // This finds out whether we have to reset.
                    strand_completeQ = ((side_1 == pd.CrossingRightHandedQ(c_1)) == overQ);
                    
                    // TODO: This might require already that there are no possible Reidemeister II moves along the strand. (?)
                    
                    // Check for loops, i.e., a over/understrand that starts and ends at the same crossing. This is akin to a "big Reidemeister I" move on top (or under) the diagram.
                    
                    if( C_color(c_1) == color )
                    {
                        // Vertex c has been visted before.
                        
                        
                        /*            |
                         *            |
                         *         a  | a_ptr
                         *     -------|-------
                         *         c_1|
                         *            |
                         *            |
                         */

                        // TODO: Test this.
                        if constexpr ( mult_compQ )
                        {
                            // We must insert this check because we might otherwise get problems when there is a configuration like this:
                            
                            /* "Big Hopf Loop"
                             *
                             *            #       #
                             *            #       #
                             *            |       |
                             *            |       |
                             *         a  | a_ptr |
                             *      +-----|-------------+
                             *      |  c_1|       |     |
                             *      |     |       |     |
                             *      |     |       |     |
                             *      |     |       |  +--|--##
                             *  ##--|--+  +-------+  |  |
                             *      |  |             |  |
                             *      +-------------------+
                             *         |             |
                             *         #             #
                             */


                            if( !strand_completeQ || (A_color(a_next) != color) )
                            {
                                RemoveLoop(a,c_1);
                                RepairArcLeftArcs();
                                break;
                            }
                            else
                            {
                                // TODO: What we have here could be a "big Hopf link", no? It should still be possible to reroute this to get rid of all but one crossings on the strand. We could reconnect it like this or increment a "Hopf loop" counter of the corresponding link component.
                                
                                /* "Big Hopf Loop"
                                 *
                                 *            #       #
                                 *            #       #
                                 *            |       |
                                 *            |       |
                                 *         a  | a_ptr |
                                 *      +-----|-------------+
                                 *      |  c_1|       |     |
                                 *      |     |       |     |
                                 *      |     |       |     |
                                 *      |     |       |  +--|--##
                                 *  ##--|--+  +-------+  |  |
                                 *      |  |             |  |
                                 *      +-------------------+
                                 *         |             |
                                 *         #             #
                                 */
                                
                                /* Simplified "Big Hopf Loop"
                                 *
                                 *            #       #
                                 *            #       #
                                 *            |       |
                                 *            |       |
                                 *         a  | a_ptr |
                                 *      +-----|---+   |
                                 *      |  c_1|   |   |
                                 *      |     |c_2|   |
                                 *      +---------+   |
                                 *            |       |  +-----##
                                 *  ##-----+  +-------+  |
                                 *         |             |
                                 *         |             |
                                 *         #             #
                                 */
                                
                                // The Dijkstra algorithm should be able to simplify this. We could of course provide a shorter, slightly more efficient implementation. But I Dijkstra should terminate quite quickly, so we should be happy about getting rid of this mental load. We we just do not break here and let the algorithm continue.
                                
                                
                                // TODO: CAUTION: Also this is possible:
                                
                                /* "Over Unknot"
                                 *
                                 *
                                 *              +-------+
                                 *              |       |
                                 *       ##-----|-------|----##
                                 *              |       |
                                 *          a   | a_ptr |
                                 *      +-------|-------+
                                 *      |    c_1|
                                 *      |       |
                                 *  ##--|-------|------##
                                 *      |       |
                                 *      +-------+
                                 *
                                 */

                                // TODO: I am not entirely sure whether Dijkstra will resolve this correctly.
                            }
                    
                        }
                        else // single component link
                        {
                            // The only case I can imagine where `RemoveLoop` won't wotk is this unknot:
                            
                            /*            +-------+
                             *            |       |
                             *            |       |
                             *         a  | a_ptr |
                             *    +-------|-------+
                             *    |    c_1|
                             *    |       |
                             *    |       |
                             *    +-------+
                             */
                            
                            // However, the earlier call(s) to Reidemeister_I should rule this case out. So we have the case of a "Big Reidemeister I" here, and that is what `RemoveLoop` is made for.
                            
                            RemoveLoop(a,c_1);
                            RepairArcLeftArcs();
                            break;
                        }
                    }

                    // Checking for Reidemeister II moves like this:
                    
                    // overQ == true
                    //
                    //                  +--------+
                    //      |     |     |        |
                    //      |     |  a  | a_next |
                    // ---------------->|------->|----
                    //      |     |     |c_1     |
                    //      |     |     |        |
                    
                    // TODO: It might even be worth to do this before RemoveLoop is called.

                    if constexpr ( R_II_Q )
                    {
                        if( (!strand_completeQ) && (a != a_begin) )
                        {
                            // Caution: This might change `a`.
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

                        if( (strand_length > Int(1)) && (max_dist > Int(0)) )
                        {
                            changedQ = RerouteToShortestPath_impl(
                                a_begin,a,
                                Min(strand_length-Int(1),max_dist),
                                color
                            );
                            
                            if( changedQ ) { RepairArcLeftArcs(); }
                        }
                        
                        // TODO: Check this.

                        // RerouteToShortestPath might deactivate `a_0`, so that we could never return to it. Hence, we rather break the while loop here.
                        if( changedQ || (!ActiveQ(a_0_state)) ) { break; }
                        
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
            
            Cleanup();
            
            return change_counter;
            
        } // Int SimplifyStrands(
        
        
    private:
        
        /*!@brief Collapse the strand made by the arcs `a_begin, pd.NextArc<Head>(a_begin),...,a_end` to a single arc. Finally, we reconnect the head of arc `a_begin` to the head of `a_end` and deactivate `a_end`. All the crossings from the head of `a_begin` up to the tail of `a_end`. This routine must only be called, if some precautions have be carried out: all arcs that crossed this strand have to reconnected or deactivated.
         *
         * @param a_begin First arc on strand to be collapsed.
         *
         * @param a_end Last arc on strand to be collapsed.
         *
         * @param arc_count_ An upper bound for the number of arcs on the strand from `a_begin` to `a_end`. This merely serves as fallback to prevent infinite loops.
         */
        
        void CollapseArcRange(
            const Int a_begin, const Int a_end, const Int arc_count_
        )
        {
//            PD_DPRINT( ClassName()+"::CollapseArcRange" );
            
            // We remove the arcs _backwards_ because of how Reconnect uses PD_ASSERT.
            // (It is legal that the head is inactive, but the tail must be active!)
            // Also, this might be a bit cache friendlier.

            if( a_begin == a_end )
            {
                return;
            }
            
            PD_TIC(ClassName()+"::CollapseArcRange(" + ToString(a_begin) + "," + ToString(a_end) + "," + ToString(arc_count_) +  ")");
            
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
                
                const bool side  = (C_arcs(c,Out,Right) == a);
                
                // side == Left         side == Right
                //
                //         |                    ^
                //         |                    |
                // a_prev  |   a        a_prev  |   a
                //    ---->X---->          ---->X---->
                //         |c                   |c
                //         |                    |
                //         v                    |

                Int a_prev;
                
//                const Int a_prev = C_arcs(c,In,!side);
//                PD_ASSERT( C_arcs(c,In,side) != C_arcs(c,Out,!side) );
//                Reconnect<Head>( C_arcs(c,In,side),C_arcs(c,Out,!side) );
                
                
                // I do an if check here so that all the indexing into `C_arcs` is mostly by constexpr numbers.
                if( side == Right )
                {
                    a_prev = C_arcs(c,In,Left);
                    
                    // Sometimes we cannot guarantee that the crossing at the intersection of `a_begin` and `a_end` is still active. But that crossing will be deleted anyways. Thus, we suppress some asserts here.
                    Reconnect<Head>( C_arcs(c,In,Right),C_arcs(c,Out,Left) );
                }
                else
                {
                    a_prev = C_arcs(c,In,Right);
                    
                    // Sometimes we cannot guarantee that the crossing at the intersection of `a_begin` and `a_end` is still active. But that crossing will be deleted anyways. Thus, we suppress some asserts here.
                    Reconnect<Head>( C_arcs(c,In,Left),C_arcs(c,Out,Right) );
                }
                
                pd.DeactivateArc(a);
                
                // Sometimes we cannot guarantee that all arcs at `c` are already deactivated. But they will finally be deleted. Thus, we suppress some asserts here.
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
            PD_TOC(ClassName()+"::CollapseArcRange(" + ToString(a_begin) + "," + ToString(a_end) + "," + ToString(arc_count_) +  ")");
        }
        
        
        void RemoveLoop( const Int e, const Int c_0 )
        {
            PD_TIC(ClassName()+"::RemoveLoop");
            
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
                    
                    PD_TOC(ClassName()+"::RemoveLoop");
                    
                    
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
            
            PD_TOC(ClassName()+"::RemoveLoop");
        }
        
        
    private:
        
        /*!
         * Run Dijkstra's algorithm to find the shortest path from arc `a_begin` to `a_end` in the graph G, where the vertices of G are the arcs and where there is an edge between two such vertices if the corresponding arcs share a common face.
         *
         * If an improved path has been found, its length is stored in `path_length` and the actual path is stored in the leading positions of `path`.
         *
         * @param a_begin  Starting arc.
         *
         * @param a_end    Final arc to which we want to find a path
         *
         * @param max_dist Maximal distance we want to travel
         *
         * @param color_    Indicator that is written to `A_ccolor`; this avoids having to erase the whole vector `A_from` for each new search. When we call this, we assume that `A_ccolor` contains only values different from `color`.
         *
         */
        
        Int FindShortestPath_impl(
            const Int a_begin, const Int a_end, const Int max_dist, const Color_T color_
        )
        {
            PD_TIC(ClassName()+"::FindShortestPath_impl");
         
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
            
            PD_ASSERT( color_ > Color_T(0) );
            
            PD_ASSERT( a_begin != a_end );
            
            // Instead of a queue we use two stacks: One to hold the next front; and one for the previous (or current) front.
            
            next_front.Reset();
            
            // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
            
            next_front.Push( PD_T::template ToDarc<Head>(a_begin) );
            next_front.Push( PD_T::template ToDarc<Tail>(a_begin) );
            
            A_ccolor(a_begin) = color_;
            A_from(a_begin)   = a_begin;
            
            // This is needed to prevent us from running in circles when cycling around faces.
            // See comments below.
            A_source[a_begin] = a_begin;
            
            Int d = 0;
            
            while( (d < max_dist) && (!next_front.EmptyQ()) )
            {
                // Actually, next_front must never become empty. Otherwise something is wrong.
                std::swap( prev_front, next_front );
                
                next_front.Reset();
                
                // We don't want paths of length max_dist.
                // The elements of prev_front have distance d.
                // So the elements we push onto next_front will have distance d+1.
                
                ++d;

                while( !prev_front.EmptyQ() )
                {
                    const Int da_0 = prev_front.Pop();
                    
                    auto [a_0,d_0] = PD_T::FromDarc(da_0);
                    
                    // Now we run through the boundary arcs of the face using `dA_left` to turn always left.
                    // There is only one exception and that is when the arc we get to is part of the strand (which is when `A_color(a) == color_`).
                    // Then we simply go straight through the crossing.

                    // arc a_0 itself does not have to be processed because that's where we are coming from.
                    Int da = dA_left[da_0];

                    do
                    {
                        auto [a,dir] = PD_T::FromDarc(da);
                        
                        AssertArc<1>(a);
                        
                        const bool part_of_strandQ = (A_color(a) == color_);

                        // Check whether `a` has not been visited, yet.
                        if( Abs(A_ccolor(a)) != color_ )
                        {
                            if( a == a_end )
                            {
                                // Mark as visited.
                                A_ccolor(a) = color_;
                                
                                // Remember from which arc we came
                                A_from(a) = a_0;
                                
                                goto Exit;
                            }
                            
                            PD_ASSERT( a != a_begin );
                            
                            if( part_of_strandQ )
                            {
                                // This arc is part of the strand itself, but neither a_begin nor a_end. We just skip it and do not make it part of the path.
                            }
                            else
                            {
                                // We make A_ccolor(a) positive if we cross arc `a` from left to right. Otherwise it is negative.
                                
                                A_ccolor(a) = dir ? color_ : -color_;
                                
                                // Remember the arc we came from.
                                A_from(a) = a_0;

                                next_front.Push(PD_T::FlipDarc(da));
                            }
                        }
                        else if constexpr ( mult_compQ )
                        {
                            // When the diagram becomes disconnected by removing the strand, then there can be a face whose boundary has two components. Even weirder, it can happen that we start at `a_0` and then go to the component of the face boundary that is _not_ connected to `a_0`. This is difficult to imagine, so here is a picture:
                            
                            
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
                            if( A_source[a] == a_0 )
                            {
                                break;
                            }
                        }
                        
                        // TODO: Maybe this is needed only with multiple components?
                        if constexpr ( mult_compQ )
                        {
                            A_source[a] = a_0;
                        }

                        if( part_of_strandQ )
                        {
                            // If da is part of the current strand, we ignore i
                            da = dA_left[PD_T::FlipDarc(da)];
                        }
                        else
                        {
                            da = dA_left[da];
                        }
                    }
                    while( da != da_0 );
                }
            }
            
            // If we get here, then d+1 = max_dist or next_front.EmptyQ().
            
            // next_front.EmptyQ() should actually never happen because we know that there is a path between the arcs!
            
            if( next_front.EmptyQ() )
            {
                wprint(ClassName()+"::ShortestPath: next_front is empty, but shortest path is not found, yet.");
            }
            
            d = max_dist + 1;
            
        Exit:
            
            // Write the actual path to array `path`.
            
            if( (Int(0) <= d) && (d <= max_dist) )
            {
                // The only way to get here with `d <= max_dist` is the `goto` above.
                
                if( Abs(A_ccolor(a_end)) != color_ )
                {
                    pd_eprint(ClassName()+"::FindShortestPath_impl");
                    TOOLS_LOGDUMP(d);
                    TOOLS_LOGDUMP(max_dist);
                    TOOLS_LOGDUMP(a_end);
                    TOOLS_LOGDUMP(color_);
                    TOOLS_LOGDUMP(color);
                    TOOLS_LOGDUMP(A_ccolor(a_end));
                    TOOLS_LOGDUMP(A_from(a_end));
                }
                
                Int a = a_end;
                
                path_length = d+1;
                
                for( Int i = 0; i < path_length; ++i )
                {
                    path[path_length-1-i] = a;
                    
                    a = A_from(a);
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
            
            PD_TOC(ClassName()+"::FindShortestPath_impl");
            
            return d;
        }
        
//    private:
//        
//        // Only meant for debugging. Don't do this in production!
//        Tensor1<Int,Int> GetShortestPath_impl(
//            const Int a_first, const Int a_last, const Int max_dist, const Int color_
//        )
//        {
//            const Int d = FindShortestPath_impl( a_first, a_last, max_dist, color_ );
//
//            if( d <= max_dist )
//            {
//                Tensor1<Int,Int> path_out (path_length);
//                
//                path_out.Read( path.data() );
//                
//                return path_out;
//            }
//            else
//            {
//                return Tensor1<Int,Int>();
//            }
//        }
        
        
    public:
        
        /*!
         * @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
         *
         * @param a_first The first arc of the input strand.
         *
         * @param a_last The last arc of the input strand (included).
         *
         * @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
         */
        
        Tensor1<Int,Int> FindShortestPath(
            const Int a_first, const Int a_last, const Int max_dist
        )
        {
            Prepare();
            
            color = 1;
            
            Int a = a_first;
            Int b = a_last;
            
            strand_length = ColorArcs(a,b,color);

            Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
            
            const Int d = FindShortestPath_impl(a,b,max_dist_,color);
            
            Tensor1<Int,Int> p;
            
            if( d <= max_dist )
            {
                p = Tensor1<Int,Int> (path_length);
                
                p.Read( path.data() );
            }
            
            Cleanup();
            
            return p;
        }
        
 private:
                
        /*!
         * @brief Attempts to reroute the strand. It is typically not save to call this function without the invariants guaranteed by `SimplifyStrands`. Some of the invariants are correct coloring of the arcs and crossings in the current strand and the absense of possible Reidemeister I and II moves along that strand.
         * This is why we make this function private.
         *
         * @param a_first The first arc of the strand on entry as well as as on return.
         *
         * @param a_last The last arc of the strand (included) as well as on entry as on return. Note that this values is a reference and that the passed variable will likely be changedQ!
         *
         * @param max_dist The maximal distance Dijkstra's algorithm will wander.
         *
         * @param color_ The color of the path.
         */
        
        bool RerouteToShortestPath_impl(
            const Int a_first, mref<Int> a_last,
            const Int max_dist, const Color_T color_
        )
        {
            PD_ASSERT(CheckArcLeftArcs());
            
#ifdef PD_DEBUG
            const Int Cr_0 = pd.CrossingCount();
            
            TOOLS_LOGDUMP(Cr_0);
#endif

            const Int d = FindShortestPath_impl(
                a_first, a_last, max_dist, color_
            );

            if( (d < Int(0)) || (d > max_dist) )
            {
                PD_DPRINT("No improvement detected.");
                
                return false;
            }
            
            PD_TIC(ClassName()+"::RerouteToShortestPath_impl");
            
#ifdef PD_TIMINGQ
            const Time start_time = Clock::now();
#endif
            
            PD_TIC("Prepare reroute loop");
            
            // path[0] == a_first. This is not to be crossed.
            
            Int p = 1;
            Int a = a_first;
            
            // At the beginning of the strand we want to avoid inserting a crossing on arc `b` if `b` branches directly off from the current strand.
            //
            //      |         |        |        |
            //      |    a    |        |        |    current strand
            //    --|-------->-------->-------->-------->
            //      |         |        |        |
            //      |         |        |        |
            //            b = path[p]?
            //

            MoveWhileBranching<Head>(a,p);
            
            // Now `a` is the first arc to be rerouted.
            
            // Same at the end.
            
            Int q = path_length - Int(1);
            Int e = a_last;

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
                
                // TODO: Use FromDarc
                const Int b = path[p];

                const bool dir = (A_ccolor(b) > Color_T(0));
                
                const Int c_1 = A_cross(b,Head);
                
                // This is the situation for `a` before rerouting for `side == Right`;
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
                
                
                // The case `dir == true`.
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
                
                // Caution: This might turn around `a_1`!
                A_cross(a_1,Tail) = c_0;
                A_cross(a_1,Head) = c_1;
                
                pd.template SetMatchingPortTo<In>(c_1,b,a_1);

                
                // Recompute `c_0`. We have to be aware that the handedness and the positions of the arcs relative to `c_0` can completely change!
                
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
                    
                    // Fortunately, this does not depend on overQ.
                    const Int buffer [4] = { a_2,a_1,b,a };
                    copy_buffer<4>(&buffer[0],C_arcs.data(c_0));
                }
                
                
//                // Mark arcs so that they will be repaired by `RepairArcLeftArcs`;
                TouchArc<Head>(a_0);

                touched.push_back( PD_T::template ToDarc<Head>(a)   );
                touched.push_back( PD_T::template ToDarc<Head>(b)   );
                touched.push_back( PD_T::template ToDarc<Tail>(a_1) );
                touched.push_back( PD_T::template ToDarc<Tail>(a_2) );
                
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

            a_last = a;

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
            
            PD_TOC(ClassName()+"::RerouteToShortestPath_impl");
            
            return true;
        }
        
    public:
        
        /*!
         * @brief Attempts to reroute the input strand. It returns the first and last arcs of the rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
         *
         * @param a_first The first arc of the input strand.
         *
         * @param a_last The last arc of the input strand (included).
         *
         * @param overQ_ Whether the input strand is over (`true`) or under (`true`). Caution: The routine won't work correctly, if the input arc range is not a over/understrand according to this flag!
         */
        
        std::array<Int,2> RerouteToShortestPath(
            const Int a_first, const Int a_last, bool overQ_
        )
        {
            Prepare();
            SetStrandMode(overQ_);
            
            color = 1;
            
            Int a = a_first;
            Int b = a_last;
            
            strand_length = ColorArcs(a,b,color);
            
            RerouteToShortestPath_impl(a,b,strand_length-Int(1),color);
            
            Cleanup();
            
            return {a,b};
        }
        
    public:
        
        template<typename Int_0, typename Int_1>
        Int ColorArcs(
            const Int_0 a_first, const Int_0 a_last, const Int_1 color_
        )
        {
            static_assert( IntQ<Int_0>, "" );
            static_assert( IntQ<Int_1>, "" );
            
            const Color_T c   = int_cast<Color_T>(color_ );
            const Int a_begin = int_cast<Int>(a_first);
            const Int a_end   = int_cast<Int>(pd.template NextArc<Head>(a_last));
            Int a = a_begin;
            
            Int counter = 0;
            
            do
            {
                ++counter;
                A_color(a) = c;
                a = pd.template NextArc<Head>(a);
            }
            while( (a != a_end) && (a != a_begin) );
            
            return counter;
        }
        
        template<typename Int_0, typename Int_1, typename Int_2>
        void ColorArcs( cptr<Int_0> arcs, const Int_1 strand_length_, const Int_2 color_ )
        {
            static_assert( IntQ<Int_0>, "" );
            static_assert( IntQ<Int_1>, "" );
            static_assert( IntQ<Int_2>, "" );
            
            const Int n     = int_cast<Int>(strand_length_);
            const Color_T c = int_cast<Color_T>(color_);
            
            if( c <= Color_T(0) )
            {
                pd_eprint("Argument color_ is <= 0. This is an illegal color.");
                
                return;
            }
            
            for( Int i = 0; i < n; ++i )
            {
                A_color(arcs[i]) = c;
            }
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
            // `a` is an arc.
            // `p` points to a position in `path`.
            
            Int c = A_cross(a,headtail);
            
            Int side = (C_arcs(c,headtail,Right) == a);
            
            Int b = path[p];
            
            while(
                (C_arcs(c,headtail,!side) == b)
                ||
                (C_arcs(c,!headtail,side) == b)
            )
            {
                a = C_arcs(c,!headtail,!side);
                
                c = A_cross(a,headtail);

                side = (C_arcs(c,headtail,Right) == a);
                
                p = headtail ? p + Int(1) : p - Int(1);
                
                b = path[p];
            }
        }
    
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("StrandSimplifier")
                + "<" + TypeName<Int>
                + "," + ToString(mult_compQ)
                + ">";
        }

    }; // class StrandSimplifier
    
} // namespace Knoodle
