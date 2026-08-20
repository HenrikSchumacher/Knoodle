#pragma once


namespace Knoodle
{
    template<typename Prosector_T_>
    class LineSweep
    {
    public:
        
        using Prosector_T     = Prosector_T_;
        using Idx             = Prosector_T::Idx;
        using Int             = Prosector_T::Int;
        using LInt            = Prosector_T::LInt;
        using LLInt           = Prosector_T::LLInt;
        
        using Intersection_T  = Prosector_T::Intersection;
        using Time_T          = Prosector_T::Time_T;
        
        
        enum class EventFlag_T : Uint8
        {
            Birth             = 0,
            Intersection      = 1,
            Death             = 2
            // This is also the order in which events have to be executed in the case of ties in time.
        };

//        struct Segment
//        {
//            IReal x_0;
//            IReal x_1;
//            IReal y_0;
//            IReal slope;
//
//            Segment( cptr<IReal> X_0, cptr<IReal> X_1 )
//            :   x_0   { X_0[0] }
//            ,   x_1   { X_0[1] }
//            ,   y_0   { X_1[0] }
//            ,   slope { (X_1[1] - X_0[1]) / ()  }
//            {}
//            
//            IReal get_y( cref<IReal> x ) const
//            {
//                return y_0 + slope * (x - x_0);
//            }
//        };

        struct Event
        {
            EventTime_T time;
            Idx         i     = Uninitialized;
            Idx         j     = Uninitialized;
            EventFlag_T flag;
            
            static Birth( cref<EventTime_T> time_, Idx i_ )
            :   time { time_              }
            ,   i    { i_                 }
            ,   j    { Uninitialized      }
            ,   flag { EventFlag_T::Birth }
            
            static Death( cref<EventTime_T> time_, Idx i_ )
            :   time { time_              }
            ,   i    { i_                 }
            ,   j    { Uninitialized      }
            ,   flag { EventFlag_T::Death }
            
            static Intersection( cref<EventTime_T> time_, Idx i_, Idx j_ )
            :   time { time_                     }
            ,   i    { i_                        }
            ,   j    { j_                        }
            ,   flag { EventFlag_T::Intersection }
            
            friend std::partial_ordering operator<=>( cref<Event> a, cref<Event> b )
            {
                auto c = (a.time <=> b.time);
                if( c != std::partial_ordering::equal ) { return c; }
                return (ToUnderlying(a.flag) <=> ToUnderlying(b.flag));
            }
        };
        
    private:
        
        Prosector_T S;
        
        EventTime_T x;
        
//        Tensor1<LineSegment_T,Idx> line_segments;
        
        ??? queue;
        
        // Must be some kind of sorted list, into which we can insert new elements.
        // We must be able to insert an element. Insertion position must be found by some function get_y with respect to which the elements are suppose to be sorted.
        // We must be able to access the two neighbors of an element in the list.
        // We must be able to swap the two neighbors of an element in the list.
        // We must be able to remove an element.
        ??? active_list;
        
        
        
    public:
        
        IReal CurrentTime() { return x; }
        
        void SetCurrentTime( cref<EventTime_T> x_ ) { x = x_; }
        
        void Step()
        {
            Event e = queue.Pop();
            
            SetCurrentTime(e.time);
            
            switch (e.flag)
            {
                case Event_T::Birth        : Bear     (CurrentTime(),e.i    );
                case Event_T::Intersection : Intersect(CurrentTime(),e.i,e.j);
                case Event_T::Death        : Kill     (CurrentTime(),e.i    );
            }
        }
        
        void Bear( Idx i )
        {
            // Insert the line segment to list of active line segments
            active_list.Insert(i);
            // TODO: How to facilitate that? What data need active line segments to  have?
            // TODO: How to break ties here? (Use slope? Does it matter?)
            
            // Check for intersections with direct neighbors on active list; create intersection events (if applicable).
            if( active_list.HasPrevQ(i) )
            {
                CheckIntersection(active_list.Prev(i),i);
            }
            if( active_list.HasNextQ(i) )
            {
                CheckIntersection(active_list.Next(i),i);
            }
            
        }
        
        void Kill( Idx i )
        {
            // Check for intersections between direct neighbors on active list; create intersection events (if applicable).
            if( active_list.HasPrevQ(i) && active_list.HasNextQ(i) )
            {
                auto i_prev = active_list.Prev(i);
                auto i_next = active_list.Next(i);
                // TODO: (Optional) Check that i_prev and i_next are correctly ordered.
                CheckIntersection(i_prev,i_next);
            }
            
            // Delete the line segment from active list.
            active_list.Delete(i);
            
            // TODO: (Maybe) sort the intersections on this line segment and discard the intersection times to safe storage.
        }
        
        void Intersect( const Idx i, const Idx j, const bool swapQ )
        {
            // Make sure that segment i must be put to active_list before segment j.

            // (optional) Check that the two intersecting line segments are neigbors in active list
    
            if( !active_list.NeigborsQ(i,j) )
            {
                eprint(MethodName("Intersect") + ": Requested line segments are not neighbors in active list.");
            }
            
            // Swap ordering of these two line segments in active list.
            if( swapQ ) { active_list.Swap(i,j); }
            
            // Test each of them with their _new_ neighbor and create new intersection events (if applicable).
            
            if( active_list.HasPrevQ(j) )
            {
                CheckIntersection(active_list.Prev(j),j);
            }
            
            if( active_list.HasNextQ(i) )
            {
                CheckIntersection(active_list.Next(i),b);
            }
            
        }
        
        void CheckIntersection( Idx i, Idx j )
        {
            // TODO: Check whether intersection {i,j} already exists.
            // If yes, then silently abort.
            
            // Check whether line segments i and j intersect.
            // If not, then abort.
            
            using Flag_T = Prosector_T::Flag_T;
            
            Flag_T flag = S.ComputeIntersection(
                i, L.EdgeData(i,Idx(0)), L.EdgeData(i,Idx(1)),
                j, L.EdgeData(j,Idx(0)), L.EdgeData(j,Idx(1))
            );
            
            switch (flag)
            {
                case Flag_T::Empty:         return;
                case Flag_T::Intersection:  break;
                case Flag_T::Error:
                {
                    eprint(tag() +": Edges " + ToString(k) + " and " + ToString(l) + " intersect in 3D.");
                    // Prevent overflow by min - function.
                    L.intersection_count_3D = std::min(
                         L.intersection_count_3D,
                         std::numeric_limits<Size_T>::max() - Size_T(1)
                     ) + Size_T(1);
                    // TODO: LineSweep won't work from here on if we return here. Maybe we better return an error flag?
                    return;
                }
                default:
                {
                    eprint(tag() + ": This should never happen.");
                    return;
                }
            }

            // If we arrive here, then flag == Flag_T::Intersection.
            
            // Create `Intersection` object and record it (increment edge_ptr, push to `intersections`).
            
            ++L.edge_ptr[i + Idx(1)];
            ++L.edge_ptr[j + Idx(1)];
            
            L.intersections.push_back( S.GetIntersection() );

            // TODO: We must make sure that the segment i is below segment j right before the intersection! This seems to be a matter of slope.
            
            Idx k = S.GetIntersection().edges[0];
            Idx l = S.GetIntersection().edges[1];
            
            auto slope_k = ???;
            auto slope_l = ???;
            if( slope_i >= slope_j )

            // Create intersection event and push it to queue.
            if( slope_i >= slope_j )
            {
                // TODO: Compute intersection time. What data type to use? Just a ratio of Ints?
                EventTime_T time = {
                    S.GetIntersection().times[0].c_0,
                    S.GetIntersection().times[1].c_0
                };
                
                queue.push( Event::Intersection(time,k,l,Event_T::Intersection);
            }
            else
            {
                // TODO: Compute intersection time. What data type to use? Just a ratio of Ints?
                EventTime_T time = {
                    S.GetIntersection().times[1].c_0,
                    S.GetIntersection().times[0].c_0
                };
                
                queue.push( Event::Intersection(time,l,k,Event_T::Intersection);
            }
        }
        
    }
} // namespace Knoodle
