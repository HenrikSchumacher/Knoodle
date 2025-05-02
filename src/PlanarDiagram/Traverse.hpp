public:

/*!
 * @brief Traverse each component of the link from one arc the arc pointed to by it. Apply function `arc_fun` to every arc visited.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten. After on, `A_scratch` contains the reordering of the arcs:
 * If arc `a` is inactive, then `A_scratch[a] = -1`.
 * Otherwise, `A_scratch[a]` contains the position of arc `a` in the traversal.
 * Likewise, on return, `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is inactive, then `C_scratch[c] = -1`.
 * Otherwise, `C_scratch[c]` contains the position of crossing `c` in the traversal.
 * Typically, `A_scratch` and `C_scratch` won't be used externally, because the  values `A_scratch[a]`, `C_scratch[c_0]`, and `C_scratch[c_1]` are fed to `fun` as `a_label`, `c_0_label`, and `c_1_label`.
 *
 * @param arc_fun A function to apply to every visited arc. It must have argument pattern, `fun( Int a, Int a_label, Int c_0, Int c_0_label, bool c_0_visited, Int c_1, Int c_1_label, bool c_1_visitedQ )`, where
 *      - `a` is the current arc within the link;
 *      - `a_label` is the position of the current arc `a` within the traversal;
 *      - `c_0` is the crossing at the _tail_ of arc `a`;
 *      - `c_0_label` is the position of `c_0` within the traversal;
 *      - `c_0_visited` is a `bool` that indicates whether crossing `c_0` is visited for the first (`false`) or the second (`true`) time.
 *      - `c_1` is the crossing at the _tip_ of arc `a`;
 *      - `c_1_label` is the position of `c_1` within the traversal.
 *      - `c_1_visited` is a `bool` that indicates whether crossing `c_0` is visited for the first (`false`) or the second (`true`) time.
 *
 */

template<typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void Traverse(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post
)
{
    if( !ValidQ() )
    {
        eprint(ClassName() + "Traverse:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        return;
    }
    
    Traverse_impl(
        std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
        A_scratch.data(), C_scratch.data()
    );
}

private:

template<typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void Traverse_impl(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    mptr<Int> A_labels, mptr<Int> C_labels
)
{
    const Int m = A_cross.Dimension(0);
    const Int n = C_arcs.Dimension(0);

    // Indicate that no arc or crossings are visited, yet.
    fill_buffer( A_labels, Int(-1), m );
    fill_buffer( C_labels, Int(-1), n );
    
    Int lc_counter = 0;
    Int a_counter  = 0;
    Int c_counter  = 0;
    Int a_ptr      = 0;
    
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while(
            ( a_ptr < m ) && ( (A_labels[a_ptr] >= Int(0))  || (!ArcActiveQ(a_ptr)) )
        )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        const Int lc_begin = a_counter;
        lc_pre( lc_counter, lc_begin );
        Int a = a_ptr;
        

        Int  c_1;
        bool c_1_visitedQ;
        Int  c_1_label;

        {
            c_1 = A_cross(a,Tail);
            
            AssertCrossing(c_1);
            
            c_1_label = C_labels[c_1];
            c_1_visitedQ = ( c_1_label >= Int(0) );
            
            if( !c_1_visitedQ )
            {
                c_1_label = C_labels[c_1] = c_counter++;
            }
        }
        
        // Cycle along all arcs in the link component, until we return where we started.
        // Apply fun to every arc.
        do
        {
            A_labels[a] = a_counter;
            
            const Int c_0          = c_1;
            const Int c_0_visitedQ = c_1_visitedQ;
            const Int c_0_label    = c_1_label;
            
            c_1 = A_cross(a,Head);
            AssertCrossing(c_1);
        
            c_1_label = C_labels[c_1];
            c_1_visitedQ = ( c_1_label >= Int(0) );
            
            if( !c_1_visitedQ )
            {
                c_1_label = C_labels[c_1] = c_counter++;
            }
            
            arc_fun( a,   a_counter,
                 c_0, c_0_label, c_0_visitedQ,
                 c_1, c_1_label, c_1_visitedQ
            );
            
            a = NextArc<Head>(a,c_1);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        const Int lc_end = a_counter;
        
        lc_post( lc_counter, lc_begin, lc_end );
        ++lc_counter;
        ++a_ptr;
    }
}
