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
        
        Tensor2<Int,Int> A_prev;
        
        Int color = 0;
        Int a_ptr = 0;

        Int strand_length    = 0;
        Int strand_0_counter = 0;
        Int strand_1_counter = 0;
        
        bool overQ;
        bool resetQ;
                
        Stack<Int,Int> next_front;
        Stack<Int,Int> prev_front;
        
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
        :   pd         { pd_        }
        ,   C_arcs     { pd.C_arcs  }
        ,   C_state    { pd.C_state }
        ,   A_cross    { pd.A_cross }
        ,   A_state    { pd.A_state }
        ,   C_data     { C_arcs.Dimension(0), 2, -1 }
        ,   A_colors   { A_cross.Dimension(0), -1 }
        ,   A_prev     { A_cross.Dimension(0), 2, -1 }
        ,   next_front { A_cross.Dimension(0) }
        ,   prev_front { A_cross.Dimension(0) }
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

        
        mref<Int> C_len( const Int c )
        {
            return C_data(c,1);
        }
        
        //
        //        mref<Int> C_arrow( const Int c )
        //        {
        //            return C_data(c,2);
        //        }
        

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
                while(
                      ( a_ptr < m ) && ( (A_colors[a_ptr] >= 0 )  || (!ArcActiveQ(a_ptr)) )
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
                
                // For this, we move backwards along arcs until ArcUnderQ(a,Tail)/ArcOverQ(a,Tail)
                while( pd.ArcUnderQ(a,Tail) != overQ )
                {
                    a = pd.PrevArc(a);
                }
                
                const Int a_0 = a;
                
                strand_length = 0;
                
                MarkCrossing( A_cross(a,Tail) );
                
                // Traverse forward through all arcs in the link component, until we return where we started.
                do
                {
//                    logdump(a);
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
                        
                        RemoveLoop(a);
                        
                        strand_length = 0;
                        
                        ++color;
                        
                        break;
                    }
                    
                    if( resetQ )
                    {
//                        if( strand_length > 2 )
//                        {
//                            FindShortestPath(a_0,a,strand_length+1,color);
//                        }
                        
                        // Create a new strand.
                        strand_length = 0;
                        
                        ++color;
                    }
                    
                    // Head of arc gets new color.
                    MarkCrossing(c);
                    
                    const Int a_next = C_arcs(c,Out,!side);

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
        
        
    public:
        
        /*!
         * Run Dijkstra's algprithm to find the shortest path from arc `a_begin` to `a_end` in the graph G, where the vertices of G are the arcs and where there is an edge between two such vertices if the corresponding arcd share a common face.
         *
         * @param a_begin  starting arc
         *
         * @param a_end    final arc to which we want to find a path
         *
         * @param max_dist maximal distance we want to travel
         *
         * @param color    indicator that is written to first column of `A_prev`; this avoid have to erase the whole matrix `A_prev` for each new search. When we call this, we assume that `A_prev` contains only values different from `color`.
         *
         */
        
        Int FindShortestPath(
            const Int a_begin, const Int a_end, const Int max_dist, const Int color
        )
        {
            ptic(ClassName()+"::FindShortestPath");
            
            next_front.Reset();
            
            // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
            
            next_front.Push( (a_begin << 1) | Int(1) );
            next_front.Push( (a_begin << 1) | Int(0) );
            
            A_prev(a_begin,0) = color;
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

                    
                    // arc a_0 itself does not have to be processed because that's we are coming from.
                    std::tie(a,dir) = pd.NextLeftArc( a, dir );

                    do
                    {
                        PD_ASSERT( (0 <= a) && (a < pd.initial_arc_count) );
                        
                        if( A_prev(a,0) != color ) // check whether `a` has been visited already.
                        {
                            A_prev(a,0) = color; // mark as visited
                            A_prev(a,1) = a_0;   // remember where we came from
                            
                            if( a == a_end )
                            {
                                goto exit;
                            }
                            
                            next_front.Push( (a << 1) | !dir );
                        }
                        
                        std::tie(a,dir) = pd.NextLeftArc(a,dir);
                    }
                    while( a != a_0 );
                }
            }
            
        exit:
            
            if( d <= max_dist )
            {
                Int a = a_end;
                
                std::vector<Int> path;
                
                path.push_back(a);
                
                for( Int i = 0; i < d; ++i )
                {
                    a = A_prev(a,1);
                    
                    path.push_back(a);
                }
                
                dump(path);
            }
            
            ptoc(ClassName()+"::FindShortestPath");
            
            return d;
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
