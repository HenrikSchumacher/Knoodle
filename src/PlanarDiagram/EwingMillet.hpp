
public:

std::pair<CrossingContainer_T,CrossingStateContainer_T> CrossingsEwingMillet()
{
    const Int m = initial_arc_count;
    
    CrossingContainer_T C      (crossing_count,2,2);
    CrossingStateContainer_T S (crossing_count);
    
    // We are using C_scratch to keep track of the new crossing's labels.
    C_scratch.Fill(-1);
    
    Int a_counter = 0;
    Int c_counter = 0;
    Int a_ptr     = 0;
    Int a         = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( (A_scratch[a_ptr] >= 0)  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        a = a_ptr;
        
        C_scratch[A_cross(a,Tail)] = c_counter++;
        
        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            const Int c_tail = A_cross(a,Tail);
            const Int c_head = A_cross(a,Head);
            
            if( C_scratch[c_head] < 0 )
            {
                C_scratch[c_head] = c_counter++;
            }

            const Int c_tail_label = C_scratch[c_tail];
            const Int c_head_label = C_scratch[c_head];
            
            S[c_tail_label] = C_state[c_tail];
            S[c_head_label] = C_state[c_head];

            const Int side_out = (C_arcs(c_tail,Out,Right) == a);
            const Int side_in  = (C_arcs(c_head,In ,Right) == a);

            C(c_tail_label,Out,side_out) = (c_head_label << 2) | (In  << 1) | side_in;
            C(c_head_label,In ,side_in ) = (c_tail_label << 2) | (Out << 1) | side_out;
            
            a = NextArc<Head>(a,c_head);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
}
