public:


/*!
 * @brief Traverse each component of the link from one arc the arc pointed to by it. Apply function `arc_fun` to every arc visited.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten.
 * On return `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is inactive, then `C_scratch[c] = -1`.
 * Otherwise, `C_scratch[c]` contains the position of crossing `c` in the traversal.
 * On return `A_scratch` contains mostly garbage.
 * Typically, `A_scratch` and `C_scratch` won't be used externally, because the  values `A_scratch[a]`, `C_scratch[c_0]`, and `C_scratch[c_1]` are fed to `fun` as `a_pos`, `c_0_pos`, and `c_1_pos`.
 *
 * @param arc_fun A function to apply to every visited arc. It must have argument pattern, `fun( Int a, Int a_pos, Int lc, Int c_0, Int c_0_pos, bool c_0_visited, Int c_1, Int c_1_pos, bool c_1_visitedQ )`, where
 *      - `a` is the current arc within the link;
 *      - `a_pos` is the position of the current arc `a` within the traversal.
 *      - `lc` is the index of a's link component.
 *
 */

template<typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void TraverseWithoutCrossings(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    bool start_with_undercrossingQ = true
)
{
//    this->template Traverse_impl<false>(
//        std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
//        reinterpret_cast<bool *>(A_scratch.data()),
//        C_scratch.data(),
//        start_with_undercrossingQ
//    );
    
    this->template Traverse_impl2<false>(
        std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
        reinterpret_cast<bool *>(A_scratch.data()),
        C_scratch.data(),
        ArcNextArc().data(),
        start_with_undercrossingQ
    );
}


/*!
 * @brief Traverse each component of the link from one arc the arc pointed to by it. Apply function `arc_fun` to every arc visited.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten.
 * On return `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is inactive, then `C_scratch[c] = -1`.
 * Otherwise, `C_scratch[c]` contains the position of crossing `c` in the traversal.
 * On return `A_scratch` contains mostly garbage.
 * Typically, `A_scratch` and `C_scratch` won't be used externally, because the  values `A_scratch[a]`, `C_scratch[c_0]`, and `C_scratch[c_1]` are fed to `fun` as `a_pos`, `c_0_pos`, and `c_1_pos`.
 *
 * @param arc_fun A function to apply to every visited arc. It must have argument pattern, `fun( Int a, Int a_pos, Int lc, Int c_0, Int c_0_pos, bool c_0_visited, Int c_1, Int c_1_pos, bool c_1_visitedQ )`, where
 *      - `a` is the current arc within the link;
 *      - `a_pos` is the position of the current arc `a` within the traversal;
 *      - `lc` is the index of a's link component.
 *      - `c_0` is the crossing at the _tail_ of arc `a`;
 *      - `c_0_pos` is the position of `c_0` within the traversal;
 *      - `c_0_visited` is a `bool` that indicates whether crossing `c_0` is visited for the first (`false`) or the second (`true`) time.
 *      - `c_1` is the crossing at the _tip_ of arc `a`;
 *      - `c_1_pos` is the position of `c_1` within the traversal.
 *      - `c_1_visited` is a `bool` that indicates whether crossing `c_0` is visited for the first (`false`) or the second (`true`) time.
 *
 */

template<typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void TraverseWithCrossings(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    bool start_with_undercrossingQ = true
)
{
//    this->template Traverse_impl<true>(
//        std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
//        reinterpret_cast<bool *>(A_scratch.data()),
//        C_scratch.data(),
//        start_with_undercrossingQ
//    );
    
    this->template Traverse_impl2<true>(
        std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
        reinterpret_cast<bool *>(A_scratch.data()),
        C_scratch.data(),
        ArcNextArc().data(),
        start_with_undercrossingQ
    );
}


private:

template<bool crossingsQ, typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void Traverse_impl(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    mptr<bool> A_visitedQ, mptr<Int> C_pos,
    bool start_with_undercrossingQ
)
{
    if( !ValidQ() )
    {
        eprint(ClassName() + "Traverse_impl:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        return;
    }
    
    
    const Int m = A_cross.Dimension(0);
    const Int n = C_arcs.Dimension(0);

    // Indicate that no arc or crossings are visited, yet.
    fill_buffer( A_visitedQ, false, m );
//    fill_buffer( A_pos, Int(-1), m );
    
    if constexpr ( crossingsQ )
    {
        fill_buffer( C_pos, Int(-1), n );
    }
    
    Int lc_counter = 0; // counter for the link components.
    Int a_counter  = 0; // counter for the arcs.
    Int c_counter  = 0; // counter for the crossings.
    Int a_ptr      = 0;
    
    bool under_crossing_flag = start_with_undercrossingQ;

Loop:
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
//        while( not_starting_arcQ (a_ptr ) )
        while(
            ( a_ptr < m )
            &&
            (
                (A_visitedQ[a_ptr] == true)
                ||
                (!this->ArcActiveQ(a_ptr))
                ||
                (under_crossing_flag ? this->ArcOverQ<Tail>(a_ptr) : false)
            )
        )     
        {
            ++a_ptr;
        }
        if( a_ptr >= m )
        {
            break;
        };
        
        // Now a_ptr points to the beginning of a link component.
        Int a = a_ptr;
        
        const Int lc_begin = a_counter;
        lc_pre( lc_counter, lc_begin );
        
        Int  c_1 = 0;
        bool c_1_visitedQ = 0;
        Int  c_1_pos = 0;

        if constexpr ( crossingsQ )
        {
            c_1 = A_cross(a,Tail);
            
            AssertCrossing(c_1);
            
            c_1_pos = C_pos[c_1];
            c_1_visitedQ = ( c_1_pos >= Int(0) );
            
            if( !c_1_visitedQ )
            {
                c_1_pos = C_pos[c_1] = c_counter;
                ++c_counter;
            }
        }
        
        // Cycle along all arcs in the link component, until we return where we started.
        // Apply fun to every arc.
        do
        {
            A_visitedQ[a] = true;
            
            if constexpr ( crossingsQ )
            {
                const Int c_0          = c_1;
                const Int c_0_visitedQ = c_1_visitedQ;
                const Int c_0_pos    = c_1_pos;
                
                c_1 = A_cross(a,Head);
                AssertCrossing(c_1);
                
                c_1_pos = C_pos[c_1];
                c_1_visitedQ = ( c_1_pos >= Int(0) ); 
                if( !c_1_visitedQ )
                {
                    c_1_pos = C_pos[c_1] = c_counter;
                    ++c_counter;
                }
                
                arc_fun(
                    a,   a_counter, lc_counter,
                    c_0, c_0_pos,   c_0_visitedQ,
                    c_1, c_1_pos,   c_1_visitedQ
                );
            }
            else
            {
                arc_fun( a, a_counter, lc_counter );
            }
            
            if constexpr ( crossingsQ )
            {
                a = NextArc<Head>(a,c_1);
            }
            else
            {
                a = NextArc<Head>(a);
            }
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        const Int lc_end = a_counter;
        
        lc_post( lc_counter, lc_begin, lc_end );
        ++lc_counter;
        ++a_ptr;
    }
    
    if( a_counter < arc_count )
    {
        if( under_crossing_flag )
        {
            under_crossing_flag = false;
            goto Loop;
        }
        else
        {
            eprint(ClassName() + "::Traverse: not all active arcs are traversed or number of active arcs is tracked incorrectly.");
        }
    }
    
    this->template SetCache<false>("LinkComponentCount",lc_counter);
}


template<bool crossingsQ, typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void Traverse_impl2(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    mptr<bool> A_visitedQ, mptr<Int> C_pos, cptr<Int> A_next,
    bool start_with_undercrossingQ
)
{
    if( !ValidQ() )
    {
        eprint(ClassName() + "Traverse_impl:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        return;
    }

    const Int m = A_cross.Dimension(0);
    const Int n = C_arcs.Dimension(0);

    // Indicate that no arc or crossings are visited, yet.
    fill_buffer( A_visitedQ, false, m );
//    fill_buffer( A_pos, Int(-1), m );

    if constexpr ( crossingsQ )
    {
        fill_buffer( C_pos, Int(-1), n );
    }

    Int lc_counter = 0; // counter for the link components.
    Int a_counter  = 0; // counter for the arcs.
    Int c_counter  = 0; // counter for the crossings.
    Int a_ptr      = 0;

    bool under_crossing_flag = start_with_undercrossingQ;

Loop:

    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
//        while( not_starting_arcQ (a_ptr ) )
        while(
            ( a_ptr < m )
            &&
            (
                (A_visitedQ[a_ptr] == true)
                ||
                (!this->ArcActiveQ(a_ptr))
                ||
                (under_crossing_flag ? this->ArcOverQ<Tail>(a_ptr) : false)
                // TODO: Handle over/under in ArcState.
            )
        )
        {
            ++a_ptr;
        }
        if( a_ptr >= m )
        {
            break;
        };

        // Now a_ptr points to the beginning of a link component.
        Int a = a_ptr;

        const Int lc_begin = a_counter;
        lc_pre( lc_counter, lc_begin );

        Int  c_1 = 0;
        bool c_1_visitedQ = 0;
        Int  c_1_pos = 0;

        if constexpr ( crossingsQ )
        {
            c_1 = A_cross(a,Tail);

            AssertCrossing(c_1);

            c_1_pos = C_pos[c_1];
            c_1_visitedQ = ( c_1_pos >= Int(0) );

            if( !c_1_visitedQ )
            {
                c_1_pos = C_pos[c_1] = c_counter;
                ++c_counter;
            }
        }

        // Cycle along all arcs in the link component, until we return where we started.
        // Apply fun to every arc.
        do
        {
            A_visitedQ[a] = true;

            if constexpr ( crossingsQ )
            {
                const Int c_0          = c_1;
                const Int c_0_visitedQ = c_1_visitedQ;
                const Int c_0_pos    = c_1_pos;

                c_1 = A_cross(a,Head);
                AssertCrossing(c_1);

                c_1_pos = C_pos[c_1];
                c_1_visitedQ = ( c_1_pos >= Int(0) );
                if( !c_1_visitedQ )
                {
                    c_1_pos = C_pos[c_1] = c_counter;
                    ++c_counter;
                }

                arc_fun(
                    a,   a_counter, lc_counter,
                    c_0, c_0_pos,   c_0_visitedQ,
                    c_1, c_1_pos,   c_1_visitedQ
                );
            }
            else
            {
                arc_fun( a, a_counter, lc_counter );
            }

            a = A_next[a];

            AssertArc(a);

            ++a_counter;
        }
        while( a != a_ptr );

        const Int lc_end = a_counter;

        lc_post( lc_counter, lc_begin, lc_end );
        ++lc_counter;
        ++a_ptr;
    }

    if( a_counter < arc_count )
    {
        if( under_crossing_flag )
        {
            under_crossing_flag = false;
            goto Loop;
        }
        else
        {
            eprint(ClassName() + "::Traverse: not all active arcs are traversed or number of active arcs is tracked incorrectly.");
        }
    }

    this->template SetCache<false>("LinkComponentCount",lc_counter);
}
