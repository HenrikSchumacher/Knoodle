#pragma once

// TODO: Speed this up by reducing the redirecting in NextArc, NextLeftArc, and NextRightArc with simple array lookups.

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
        Tensor1<Int,Int> A_colors; // TODO: Could use pd.A_scratch instead.
        
        Tensor2<Int,Int> A_prev;
        
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
        
    public:
        
        StrandSimplifier() = delete;
        
        StrandSimplifier( PD_T & pd_ )
        :   pd         { pd_        }
        ,   C_arcs     { pd.C_arcs  }
        ,   C_state    { pd.C_state }
        ,   A_cross    { pd.A_cross }
        ,   A_state    { pd.A_state }
        ,   C_data     { C_arcs.Dimension(0), 2, -1 }
        // We initialize 0 because actual colors will have to be positive to use sign bit.
        ,   A_colors   { A_cross.Dimension(0), 0 }
        // We initialize 0 because actual colors will have to be positive to use sign bit.
        ,   A_prev     { A_cross.Dimension(0), 2, 0 }
        ,   next_front { A_cross.Dimension(0) }
        ,   prev_front { A_cross.Dimension(0) }
        ,   path       { A_cross.Dimension(0) }
        ,   path_length{ 0 }
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
            return pd.ArcString(a_) + " has color " + ToString(A_color(a_)) + ".";
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
            return pd.CrossingString(c_) + " has color " + ToString(C_color(c_)) + ".";
        }

        std::string StrandString( const Int a_begin, const Int a_end) const
        {
            std::stringstream s;

            Int a = a_begin;
            Int i = 0;
            
            s << ToString(0) << " : " <<  CrossingString(A_cross(a,Tail)) << "\n";
            s << ToString(0) << " : " <<  ArcString(a)                    << "\n";
            s << ToString(1) << " : " <<  CrossingString(A_cross(a,Head)) << "\n";

            do
            {
                ++i;
                a = pd.template NextArc<Head>(a);
                s << ToString(i  ) << " : " <<  ArcString(a)                    << "\n";
                s << ToString(1+1) << " : " <<  CrossingString(A_cross(a,Head)) << "\n";

            }
            while( a != a_end );
            
            return s.str();
        }
        
        template<bool headtail, bool deactivateQ = true>
        void Reconnect( const Int a_, const Int b_ )
        {
            pd.template Reconnect<headtail,deactivateQ>(a_,b_);
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
            C_color(c) = color;
            C_len  (c) = strand_length;
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
            
            // TODO: Make these fills obsolete.
            C_data.Fill(-1);
            A_colors.Fill(0);
            
            color = 1;
            a_ptr = 0;

            strand_length = 0;
            change_counter = 0;
            
            while( a_ptr < m )
            {
                // Search for next arc that is active and has not yet been handled.
                while(
                    ( a_ptr < m ) && ( (A_color(a_ptr) != 0 ) || (!ArcActiveQ(a_ptr)) )
                )
                {
                    ++a_ptr;
                }
                
                if( a_ptr >= m )
                {
                    break;
                }
                
                Int a = a_ptr;
                // Find the beginning of first strand.
                
                AssertArc<1>(a);
                
                // For this, we move backwards along arcs until ArcUnderQ(a,Tail)/ArcOverQ(a,Tail)
                while( pd.ArcUnderQ(a,Tail) != overQ )
                {
                    a = pd.template NextArc<Tail>(a);
                    AssertArc<1>(a);
                }

                Int a_0 = a;
                
                // The starting arc of the current strand.
                Int a_begin = a_0;
                
                strand_length = 0;
                
                MarkCrossing( A_cross(a,Tail) );
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
                    ++strand_length;
                    
                    if( color == std::numeric_limits<Int>::max() )
                    {
                        ptoc(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
                        
                        return change_counter;
                    }
                    
                    const Int c = A_cross(a,Head);

                    AssertCrossing<1>(c);
                    
                    // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
                    // This finds out whether we have to reset.
                    const bool side = (C_arcs(c,In ,Right) == a);
                    
                    strand_completeQ = ((side == pd.CrossingRightHandedQ(c)) == overQ);
                    
                    // Arc gets old color.
                    A_color(a) = color;
                    
                    const Int a_next = C_arcs(c,Out,!side);
                    
                    if( C_color(c) == color )
                    {
                        // Vertex c has been visted before.
                        
                        RemoveLoop(a);
                        
                        // TODO: We might not have to break here;
                        // TODO: Instead we could move on to the next arc.
                        break;
                    }
                    
                    if( strand_completeQ )
                    {
                        bool changedQ = false;
                        
                        if( strand_length > 1 )
                        {
                            PD_ASSERT( a != a_begin );
                            
                            changedQ = RerouteToShortestPath(a_begin,a,strand_length-1,color);
                            
                            change_counter += changedQ;
                        }
                        
                        // TODO: We might not have to break here;
                        // TODO: Instead we could move on to the next arc.
                        if( changedQ )
                        {
                            break;
                        }
                        
                        // Create a new strand.
                        strand_length = 0;
                        
                        a_begin = a_next;

                        ++color;
                    }
                    
                    // Head of arc gets new color.
                    MarkCrossing(c);

                    AssertArc<1>(a_next);
                    
                    a = a_next;
                    
                }
                while( a != a_0 );
                
                ++color;
                strand_length = 0;
                
                ++a_ptr;
            }
            
            ptoc(ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands");
            
            return change_counter;
        }
        
        
    private:
        
        void CollapseArcRange( const Int a_begin, const Int a_end, const Int arc_count )
        {
            PD_DPRINT( ClassName() + "::CollapseArcRange" );
            
            ptic(ClassName() + "::CollapseArcRange");
            
//            logvalprint( "a_begin", ArcString(a_begin) );
//            logvalprint( "a_end",   ArcString(a_end  ) );
//            logvalprint( "arc_count",   arc_count );
            
            // This shall collapse the arcs `a_begin, pd.NextArc<Head>(a_begin),...,a_end` to a single arc.
            // All crossings from the head of `a_begin` up to the tail of `a_end` have to be reconnected and deactivated.
            // Finally, we reconnect the head of arc `a_begin` to the head of `a_end` and deactivate a_end.
            
            // We remove the arcs _backwards_ because of how Reconnect uses PD_ASSERT.
            // (It is legal that the head is inactive, but the tail must be active!)
            // Also, this might be a bit cache friendlier.

            if( a_begin == a_end )
            {
                ptoc(ClassName() + "::CollapseArcRange");
                return;
            }
            
//            logprint("Arc before collapse:");
//            logprint( StrandString(a_begin, a_end) );
//            logprint("Starting the collapse:");
            
            Int a = a_end;
            
            Int iter = 0;
            
            while( (a != a_begin) && (iter < arc_count) )
            {
                ++iter;
                
                const Int c = A_cross(a,Tail);
                const bool side  = (C_arcs(c,Out,Right) == a);
                const Int a_prev = C_arcs(c,In,!side);

                
//                logvalprint( "a", ArcString(a) );
//                logvalprint( "c", CrossingString(c) );
                
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
                
                // Sometimes we cannot guarantee that the crossing at the intersection of a_begin and a_end is still active. But that crossing will be deleted anyways. Thus we suppress some asserts here.
                Reconnect<Head>( C_arcs(c,In,side),C_arcs(c,Out,!side) );
                
                DeactivateArc(a);
                
                // Sometimes we cannot guarantee that all arcs at `c` are already deactivated. But they will finally be deleted. Thus we suppress some asserts here.
                DeactivateCrossing<false>(c);
                
                a = a_prev;
            }
            
            
            if( iter >= arc_count )
            {
                eprint(ClassName()+"::CollapseArcRange: iter >= arc_count.");
            }
                
            PD_ASSERT( a == a_begin );
            
            // a_end should already be deactivated.
            
            Reconnect<Head,false>(a_begin,a_end);
            
            PD_ASSERT( !ArcActiveQ(a_end) );
            
            ptoc(ClassName() + "::CollapseArcRange");
        }
        
        void RemoveLoop( const Int e )
        {
            PD_DPRINT( ClassName() + "::RemoveLoop at " + ArcString(e) );
            
            ptic(ClassName() + "::RemoveLoop");
            const Int c_0 = A_cross(e,Head);
            
            if( A_cross(e,Tail) == c_0 )
            {
                pd.Reidemeister_I(e);
                
                ++change_counter;
                
                ptoc(ClassName() + "::RemoveLoop");
                return;
            }
            
            
            // TODO: Implement this!
            
//            if constexpr( mult_compQ )
//            {
//                const Int a = pd.template NextArc<Head>(e);
//
//                // TODO: Check also the case that the strand is a loop
//                
//                if( A_color(a) == color )
//                {
//                    if( (!strand_completeQ) )
//                    {
//                        // overQ == true;
//                        //
//                        //                 O
//                        //                 |
//                        //        e        |        a
//                        // ####O----->O--------->O----->O######
//                        //                 |c_0
//                        //                 |
//                        //                 O
//                        
//                        ++pd.unlink_count;
//                        //RemoveStrand(e,e);
//                        return;
//                    }
//                    else
//                    {
//                        // overQ == true;
//                        //
//                        //                 O                  O
//                        //                 |                  |
//                        //        e        |        a         |
//                        // ####O----->O----|---->O----->O---->X---->O######
//                        //                 |c_0               |c_1
//                        //                 |                  |
//                        //                 O                  O
//                        
//                        // TODO: We need to insert a new crossing on the vertical strand.
//                        
//                        eprint(ClassName()+"::RemoveLoop: Case A_color(a_next) == color and resetQ == true not implemented");
//                        
//                        // The goal should be this:
//                        //
//                        // overQ == true, upQ == true
//                        //
//                        //                 O
//                        //                 ^
//                        //                 |c_1
//                        //            O<---------O
//                        //            |    |     ^
//                        //            |    |     |
//                        //          e |    O     | a
//                        //            |    ^     |
//                        //            v    |     |
//                        //            O----|---->O
//                        //                 |c_0
//                        //                 |
//                        //                 O
//                        
////                        bool upQ = (C_arcs(c_0,In,Left) == e);
////
////                        const Int c_1 = A_cross(a,Head);
//                        
////                        Reconnect<Tail>(a,??);
//
//                        return;
//                    }
//                }
//            }
            
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

            CollapseArcRange(b,e,strand_length);
            Reconnect<Head>( C_arcs(c_0,In,!side), C_arcs(c_0,Out,!side) );
            DeactivateArc(b);
            DeactivateCrossing(c_0);
            
            ++change_counter;
            
            ptoc(ClassName() + "::RemoveLoop");
        }
        
        
    public:
        
        /*!
         * Run Dijkstra's algorithm to find the shortest path from arc `a_begin` to `a_end` in the graph G, where the vertices of G are the arcs and where there is an edge between two such vertices if the corresponding arcd share a common face.
         *
         * @param a_begin  starting arc
         *
         * @param a_end    final arc to which we want to find a path
         *
         * @param max_dist maximal distance we want to travel
         *
         * @param color_    indicator that is written to first column of `A_prev`; this avoid have to erase the whole matrix `A_prev` for each new search. When we call this, we assume that `A_prev` contains only values different from `color`.
         *
         */
        
        Int FindShortestPath(
            const Int a_begin, const Int a_end, const Int max_dist, const Int color_
        )
        {
//            ptic(ClassName()+"::FindShortestPath");
            
//            if( color_ <= 0 )
//            {
//                eprint(ClassName()+"::FindShortestPath: Argument color_ is nonpositive. That is illegal.");
//                
//                ptoc(ClassName()+"::FindShortestPath");
//                
//                return -1;
//            }
            
            PD_ASSERT( color_ > 0 );
            
            next_front.Reset();
            
            // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
            
            next_front.Push( (a_begin << 1) | Int(1) );
            next_front.Push( (a_begin << 1) | Int(0) );
            
            A_prev(a_begin,0) = color_;
            A_prev(a_begin,1) = a_begin;
            
            Int d = 0; // Distance of the elements of `next_front` to the origin `a_begin`.
            
            while( (d <= max_dist) && (!next_front.EmptyQ()) )
            {
                swap( prev_front, next_front );
                
                next_front.Reset();

                ++d;
                
                while( !prev_front.EmptyQ() )
                {
                    const Int A_0 = prev_front.Pop();
                    
                    const Int a_0 = (A_0 >> 1);
                    
                    Int a = a_0;
                    
                    bool dir = (A_0 & Int(1));

                    
                    // arc a_0 itself does not have to be processed because that's where we are coming from.
                    std::tie(a,dir) = pd.NextLeftArc( a, dir );

                    do
                    {
                        PD_ASSERT( (0 <= a) && (a < pd.initial_arc_count) );
                        
                        const bool part_of_strandQ = (A_color(a) == color_);
                        
                        // Check whether `a` has been visited already.
                        if( Abs(A_prev(a,0)) != color_ )
                        {
                            if( a == a_end )
                            {
                                // Mark as visited.
                                A_prev(a,0) = color_;
                                
                                // Remember from which arc we came
                                A_prev(a,1) = a_0;
                                
                                goto exit;
                            }
                            
                            if( part_of_strandQ )
                            {
                                // This arc is part of the strand itself, but neither a_begin nor a_end. We just skip it and do not make it part of the path.
                            }
                            else
                            {
                                // We make A_prev(a,0) positive if we cross arc `a` from left to right. Otherwise it is negative.
                                
                                A_prev(a,0) = dir ? color_ : -color_;
                                
                                // Remember from which arc we came.
                                A_prev(a,1) = a_0;
                                
                                next_front.Push( (a << 1) | !dir );
                            }
                        }

                        std::tie(a,dir) = pd.NextLeftArc(a, part_of_strandQ ? !dir : dir);
                    }
                    while( a != a_0 );
                }
            }
            
        exit:
            
            if( d <= max_dist )
            {
                path_length = d+1;
                
                Int a = a_end;
                
                path[d] = a;
                
                for( Int i = 0; i < d; ++i )
                {
                    a = A_prev(a,1);
                    
                    path[d-1-i] = a;
                }
            }
            
//            ptoc(ClassName()+"::FindShortestPath");
            
            return d;
        }
        
        
        Tensor1<Int,Int> GetShortestPath(
            const Int a_begin, const Int a_end, const Int max_dist, const Int color_
        )
        {
            const Int d = FindShortestPath( a_begin, a_end, max_dist, color_ );

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
        
// private:
        
        bool RerouteToShortestPath(
            const Int a_begin, const Int a_end, const Int max_dist, const Int color_
        )
        {
//            ptic(ClassName()+"::RerouteToShortestPath");
            
//            logdump(max_dist);
//            
//            logdump(color);
//            logdump(color_);
//            
//            logvalprint( "a_begin", ArcString(a_begin) );
//            logvalprint( "a_end  ", ArcString(a_end  ) );
            
            ptic("Find path");
            const Int d = FindShortestPath( a_begin, a_end, max_dist, color_ );
            ptoc("Find path");
            
//            logdump(d);
            
            
            if( (d < 0) || (d > max_dist) )
            {
                PD_DPRINT("No improvement detected: (d < 0) || (d > max_dist).");
                
//                ptoc(ClassName()+"::RerouteToShortestPath");
                
                return false;
            }
            
//            logprint("The old strand:");
//            logprint( StrandString(a_begin, a_end) );

            
//            logvalprint( "path", ArrayToString( path.data(), {path_length} ) );
            
            // path[0] == a_begin. This is not to be crossed.
            
            
            ptic("FindStart");
            
            Int p = 1;
            Int a = a_begin;
            
//            logdump(p);
//            logvalprint("a",ArcString(a));
//            logvalprint("path[p]",ArcString(path[p]));
            
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

//            logprint("Finding first branching arc.");
            
            // Finding first branching arc
            // TODO: Optimize away the call to NextArc by fusing this with ArcBranchesFromArcQ.

            
            while( ArcBranchesFromArcQ<Head>(a,path[p]) )
            {
                ++p;
                a = pd.template NextArc<Head>(a);
            }
            
//            logdump(p);
//            logvalprint("a",ArcString(a));
//            logvalprint("path[p]",ArcString(path[p]));
            
            // Now a is the first arc to be rerouted.
            
            // Same at the end.
            
            Int q = path_length-1;
            Int e = a_end;
            
//            logdump(q);
//            logvalprint("e",ArcString(e));
//            logvalprint("path[q]",ArcString(path[q]));
//            
//            logprint("Finding last branching arc.");
            
            // TODO: Optimize away the call to NextArc by fusing this with ArcBranchesFromArcQ.
            
            while( ArcBranchesFromArcQ<Tail>(e,path[q]) )
            {
                --q;
                e = pd.template NextArc<Tail>(e);
            }
            
            ptoc("FindStart");

//            logdump(q);
//            logvalprint("e",ArcString(e));
//            logvalprint("path[q]",ArcString(path[q]));
            
            // Now e is the last arc to be rerouted.
            
            
            PD_DPRINT("========== Rerouting ==========");
            
            // Handle the rerouted arcs.
            
            ptic("Reroute");

            
            while( p < q )
            {
//                logprint("==============================");
//                
//                logdump(p);
//                logvalprint( "a", ArcString(a)  );
                
                const Int c_0 = A_cross(a,Head);
                
                AssertCrossing<1>(c_0);
                
//                const Int a_next = pd.template NextArc<Head>(a);
                
                const bool side_0 = (C_arcs(c_0,In,Right) != a);
                const Int a_next = C_arcs(c_0,Out,side_0);

                AssertArc<1>(a_next);
                
//                logvalprint("c_0",CrossingString(c_0));
                
                const Int b = path[p];
                
                AssertArc<1>(b);
                
//                logvalprint("b",ArcString(b));
//                
                const bool dir = (A_prev(b,0) > 0);
                
                const Int c_1 = A_cross(b,Head);
                
                AssertCrossing<1>(c_1);
//                logvalprint("c_1",CrossingString(c_1));
                
                const bool u = (overQ == RightHandedQ(C_state[c_0]));
                
                // This is the situation for `a` before rerouting for u == true;
                //
                //
                //                ^a_1
                //      |         |        |        |
                //      |    a    | a_next |        |   e
                //    --|-------->-------->--......-->-------->
                //      |         ^c_0     |        |
                //      |         |        |        |
                //                |a_0
                //
                
                // a_0 is the vertical incoming arc.
//                const Int a_0 = u ? C_arcs(c_0,In ,Right) : C_arcs(c_0,In ,Left );
                
                const Int a_0 = C_arcs(c_0,In , u);
                
                // a_1 is the vertical outgoing arc.
//                const Int a_1 = u ? C_arcs(c_0,Out,Left ) : C_arcs(c_0,Out,Right);
                
                const Int a_1 = C_arcs(c_0,Out,!u);

                
                PD_ASSERT( A_cross(a_0,Head) == c_0 );
                PD_ASSERT( A_cross(a_1,Tail) == c_0 );
                
                
                // In the cases b == a_0 and b == a_1, can we simply leave everything as it is!
                
                if( (b == a_0) || (b == a_1) )
                {
                    // Go to next arc.
                    a = a_next;
                    ++p;
                    continue;
                }
                
//                PD_ASSERT( b != a_next );
//                PD_ASSERT( b != a_0 );
//                PD_ASSERT( b != a_1 );
                
                AssertArc<1>(a_0);
                AssertArc<1>(a_1);
                
//                logvalprint("a_0",ArcString(a_0));
//                logvalprint("a_1",ArcString(a_1));
//                logvalprint("a_next",ArcString(a_next));
                
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
                //    a        |     a_next
                //  ----->O----X----O----->
                //             |c_0
                //             |
                //             O
                //             ^
                //             | b
                //             |
                //             X

//                logprint("------------------------------");
                
                Reconnect<Head,false>(a_0,a_1);

                A_cross(b,  Head) = c_0;
                
                // Caution: This might turn around a_1!
                A_cross(a_1,Tail) = c_0;
                A_cross(a_1,Head) = c_1;

                if( C_arcs(c_1,In,Right) == b )
                {
                    C_arcs(c_1,In,Right) = a_1;
                }
                else
                {
                    C_arcs(c_1,In,Left ) = a_1;
                }
                
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
                    //    a        |     a_next
                    //  ----->O---------O----->
                    //             |c_0
                    //             |
                    //             O
                    //             ^
                    //             | b == path[p]
                    //             |
                    //             X
                    
                    // Fortunately, this does not depend on overQ.
                    C_arcs(c_0,Out,Left ) = a_1;
                    C_arcs(c_0,Out,Right) = a_next;
                    C_arcs(c_0,In ,Left ) = a;
                    C_arcs(c_0,In ,Right) = b;
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
                    //  a_next     |        a
                    //  <-----O---------O<-----
                    //             |c_0
                    //             |
                    //             O
                    //             ^
                    //             | b == path[p]
                    //             |
                    //             X
                    
                    // Fortunately, this does not depend on overQ.
                    C_arcs(c_0,Out,Left ) = a_next;
                    C_arcs(c_0,Out,Right) = a_1;
                    C_arcs(c_0,In ,Left ) = b;
                    C_arcs(c_0,In ,Right) = a;
                }
                
                AssertCrossing<1>(c_0);
                AssertCrossing<1>(c_1);
                AssertArc<1>(a);
                AssertArc<1>(a_0);
                AssertArc<1>(a_1);
                AssertArc<1>(a_next);
                AssertArc<1>(b);
                
                
//                logvalprint("a",ArcString(a));
//                logvalprint("b",ArcString(b));
//                
//                logvalprint("a_0",ArcString(a_0));
//                logvalprint("a_1",ArcString(a_1));
//                logvalprint("a_next",ArcString(a_next));
//                
//                logvalprint("c_0",CrossingString(c_0));
//                logvalprint("c_1",CrossingString(c_1));
//                
                // Go to next of arc.
                a = a_next;
                ++p;
            
//                logprint("==============================");
            }
            
            ptoc("Reroute");
            
//            logprint("========== Rerouting done. ==========");
//            
//            logdump(p);
//            logdump(q);
//            
//            PD_ASSERT( p == q );
//            PD_ASSERT( path[p] == e );
            
//            logvalprint("a",ArcString(a));
//            logvalprint("e",ArcString(e));
            
            // max_dist+1 is just an upper bound.
            CollapseArcRange(a,e,max_dist+1);
            
            AssertArc<1>(a);
            AssertArc<0>(e);

//            logprint("The new strand:");
//            logprint( StrandString(a,e) );
    
//            ptoc(ClassName()+"::RerouteToShortestPath");
            
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
                eprint("Argument color_ is <= 0. This is an illegal color.");
                
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
        

        

        
        template<bool headtail>
        bool ArcBranchesFromArcQ( const Int a, const Int b ) const
        {
            const Int c = A_cross(a,headtail);
            
            cptr<Int> C = C_arcs.data(c);

            if( C[2 * headtail + Right] == a )
            {
                return (C[2 * headtail + Left ] == b) or (C[2 * (!headtail) + Right] == b);
            }
            else
            {
                return (C[2 * headtail + Right] == b) or (C[2 * (!headtail) + Left ] == b);
            }

//            const Int side = (C_arcs(c,headtail,Right) == a);
//            return (C_arcs(c,headtail,!side) == b) or (C_arcs(c,!headtail,side) == b);
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
