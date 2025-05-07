public:

static constexpr int DefaultTraversalMethod = 1;

// TODO: Use this for ArcOverStrands and ArcUnderStrands.

/*!
 * @brief Traverse each component of the link and apply function `arc_fun` to every arc visited.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten.
 * On return `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is inactive, then `C_scratch[c] = -1`.
 * Otherwise, `C_scratch[c]` contains the position of crossing `c` in the traversal.
 * On return `A_scratch` contains the reordering of the crossings if `arclabelsQ == true`:
 * If arc `a` is inactive, then `A_scratch[a] = -1`.
 * Otherwise, if `arclabelsQ == false`, then `A_scratch` containes garbage do not use it.
 *
 * Typically, `A_scratch` and `C_scratch` need not and should not be used externally, because the values `A_scratch[a]`, `C_scratch[c_0]`, and `C_scratch[c_1]` are fed to `fun` as `a_pos`, `c_0_pos`, and `c_1_pos`.
 *
 * @tparam crossingsQ A `bool` that controls whether the information of the crossings at the tip of tail of the current arc shall be fed to `arc_fun`.
 *
 * @tparam arclabelsQ A `bool` that controls whether `A_scratch` shall be populated with the reordering of arcs.
 *
 * @tparam start_arc_ou Controls how the first arc in a link component is chosen: If set to `1`, then the algorithm tries to choose it so that its tail goes over. If set to `-1`, then the algorithm tries to choose it so that its tail goes under. If set to `0`, then just the next unvisted arc is chosen.
 *
 * @param lc_pre A lambda function that is executed at the start of every link component.
 *    `lc_post( const Int lc, const Int lc_begin )`.
 *
 * @param method The method used for traversal. You should typically use the default method.
 *
 * @param arc_fun A function to apply to every visited arc. Its require argument pattern depends on `crossingsQ`:
 * If `crossingsQ == false`, then it must be of the pattern
 *      `arc_fun( Int a, Int a_pos, Int lc )`.
 * If `crossingsQ == false`, then it must be of the pattern
 *      `arc_fun( Int a, Int a_pos, Int lc, Int c_0, Int c_0_pos, bool c_0_visited, Int c_1, Int c_1_pos, bool c_1_visitedQ )`.
 * Here the arguments mean the following:
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
 * @param lc_post A lambda
 *    `lc_post( const Int lc, const Int lc_begin, const Int lc_end )`.
 */

    
template<
    bool crossingsQ, bool arclabelsQ,
    int start_arc_ou = -1, int method = DefaultTraversalMethod,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse( LinkCompPre_T && lc_pre, ArcFun_T && arc_fun, LinkCompPost_T && lc_post )  const
{
    TOOLS_PTIC(ClassName()+"::Traverse"
        + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
        + "," + (arclabelsQ ? "w/ arc labels" : "w/o arc labels")
        + "," + (start_arc_ou == 0 ?  "0" : (start_arc_ou < 0 ? "under" : "over") )
        + "," + ToString(method)
        + ">");
    
    if constexpr ( method == 0 )
    {
        this->template Traverse_impl<crossingsQ,arclabelsQ,start_arc_ou,method>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
            nullptr,
            reinterpret_cast<std::conditional_t<arclabelsQ,Int,bool> *>(A_scratch.data()),
            C_scratch.data()
        );
    }
    else if constexpr ( method == 1 )
    {
        this->template Traverse_impl<crossingsQ,arclabelsQ,start_arc_ou,method>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
            ArcNextArc().data(),
            reinterpret_cast<std::conditional_t<arclabelsQ,Int,bool> *>(A_scratch.data()),
            C_scratch.data()
        );
        this->ClearCache("ArcNextArc");
    }
    else
    {
        static_assert(
            Tools::DependentFalse<LinkCompPre_T>,
            "Unknown traversal method. Using default method."
        );
    }
    
    TOOLS_PTOC(ClassName()+"::Traverse"
        + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
        + "," + (arclabelsQ ? "w/ arc labels" : "w/o arc labels")
        + "," + (start_arc_ou == 0 ?  "0" : (start_arc_ou < 0 ? "under" : "over") )
        + "," + ToString(method)
        + ">");
}


template<
    bool crossingsQ, bool arclabelsQ,
    int start_arc_ou = -1, int method = DefaultTraversalMethod,
    typename ArcFun_T
>
void Traverse( ArcFun_T && arc_fun )  const
{
    this->template Traverse<crossingsQ,arclabelsQ,start_arc_ou,method>(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        std::move(arc_fun),
        []( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
        }
    );
}

private:

template<
    bool crossingsQ, bool arclabelsQ, int start_arc_ou, int method,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse_impl(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    cptr<Int>  A_next,
    mptr<std::conditional_t<arclabelsQ,Int,bool>> A_flag,
    mptr<Int>  C_pos
)  const
{
    static_assert(
        (method==0) || (method==1),
        "Unknown traversal method. Using default method."
    );
    
    if constexpr ( method != 1 )
    {
        (void)A_next;
    }
    
    if( !ValidQ() )
    {
        eprint(ClassName() + "Traverse_impl:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        return;
    }

    const Int m = A_cross.Dimension(0);
    const Int n = C_arcs.Dimension(0);

    // Indicate that no arc or crossings are visited, yet.
    if constexpr ( arclabelsQ )
    {
        fill_buffer( A_flag, Int(-1), m );
    }
    else
    {
        fill_buffer( A_flag, false, m );
    }

    if constexpr ( crossingsQ )
    {
        fill_buffer( C_pos, Int(-1), n );
    }

    Int lc_counter = 0; // counter for the link components.
    Int a_counter  = 0; // counter for the arcs.
    Int c_counter  = 0; // counter for the crossings.
    Int a_ptr      = 0;

    constexpr bool ou_flag = (start_arc_ou != 0) ;
    constexpr bool overQ   = (start_arc_ou >  0) ;

    while( a_ptr < m )
    {
        // Search for next active, unvisited arc whose tail is over/under crossing (or arbitary if ou_flag == false).
        while(
            ( a_ptr < m )
            &&
            (
                ( arclabelsQ ? (A_flag[a_ptr] >= Int(0)): (A_flag[a_ptr] == true) )
                ||
                (!this->ArcActiveQ(a_ptr))
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
        
        if constexpr ( ou_flag )
        {
            // We have to find the _beginning_ of first over/understrand.
            // For this, we go forward until its tail goes under/over as needed.
            if( ArcOverQ<Tail>(a) != overQ )
            {
                do
                {
                    // Move to next arc.
                    if constexpr ( method == 0 )
                    {
                        a = NextArc<Head>(a);
                    }
                    else if constexpr ( method == 1 )
                    {
                        a = A_next[a];
                    }
                }
                while( (a != a_ptr) && (ArcOverQ<Tail>(a) != overQ) );
            }
            
            a_ptr = a;
        }
        
        // Now we are at the first arc of an over/understrand or we have `a = a_ptr`å.
        // The latter can only happen if `ou_flag == true` and if the whole link component is a single over/understrand.
        
        // In any case, we just have to traverse forward through all arcs in the link component.

        const Int lc_begin = a_counter;
        lc_pre( lc_counter, lc_begin );

        Int  c_1 = 0;
        bool c_1_visitedQ = 0;
        Int  c_1_pos = 0;

        if constexpr ( crossingsQ )
        {
            // Get the crossing labels right.
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
            if constexpr ( arclabelsQ )
            {
                A_flag[a] = a_counter;
            }
            else
            {
                A_flag[a] = true;
            }

            // Function call.
            if constexpr ( crossingsQ )
            {
                // Get the crossing labels right.
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
            
            // Move to next arc.
            if constexpr ( method == 0 )
            {
                if constexpr ( crossingsQ )
                {
                    a = NextArc<Head>(a,c_1);
                }
                else
                {
                    a = NextArc<Head>(a);
                }
            }
            else if constexpr ( method == 1 )
            {
                a = A_next[a];
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

    this->template SetCache<false>("LinkComponentCount",lc_counter);
}
