public:

std::pair<Int,Int> CountTrefoils( cref<PD_T> pd ) const
{
    TOOLS_PTIMER(timer,MethodName("CountTrefoils"));
    
    // An empty list is ordered, of course.
    if( pd.InvalidQ() ) { return {0,0}; }
    
    Int right_handed_count = 0;
    Int left_handed_count = 0;
    
    std::deque<Int> queue;
    AssociativeContainer<Int,Int8> counters;
    
    pd.template Traverse_ByNextArc<true,false,0>(
        [&counters,&queue]( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
            counters.clear();
            queue.clear();
        },
        [&counters,&queue,&right_handed_count,&left_handed_count,&pd](
            const Int a,   const Int a_label, const Int lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)a;
            (void)a_label;
            (void)lc;
            (void)c_0_pos;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
//            TOOLS_LOGDUMP(c_0);
            
            if( queue.size() >= 6 )
            {
                Decrement(counters,queue.front());
                queue.pop_front();
            }
            
            Increment(counters,c_0);
            queue.push_back(c_0);
            
            Tiny::Vector<6,Int,Size_T> v;

            if( (queue.size() >= 6) )
            {
                
                for( Size_T i = 0; i < 6; ++i )
                {
                    v[i] = queue[i];
                }
                TOOLS_LOGDUMP(v);
                logvalprint("counters",ToString(counters));
            }
            
            if( (queue.size() >= 6) && (counters.size() <= 3) )
            {
                if( pd.CrossingRightHandedQ(c_0) )
                {
                    ++right_handed_count;
                }
                else
                {
                    ++left_handed_count;
                }
            }
        },
        []( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
        }
     );
    
    return {left_handed_count,right_handed_count};
}
