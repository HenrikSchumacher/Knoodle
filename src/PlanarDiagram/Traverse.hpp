public:

// TODO: Precompute ArcOverQ -- or actually pack that into A_state.

// TODO: Use this for ArcOverStrands and ArcUnderStrands.

/*!
 * @brief Traverse each component of the link and apply function `arc_fun` to every arc visited.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten.
 * On return `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is inactive, then `C_scratch[c] = Uninitialized`.
 * Otherwise, `C_scratch[c]` contains the position of crossing `c` in the traversal.
 * On return `A_scratch` contains the reordering of the crossings if `arclabelsQ == true`:
 * If arc `a` is inactive, then `A_scratch[a] = Uninitialized`.
 * Otherwise, if `arclabelsQ == false`, then `A_scratch` containes garbage do not use it.
 *
 * Typically, `A_scratch` and `C_scratch` need not and should not be used externally, because the values `A_scratch[a]`, `C_scratch[c_0]`, and `C_scratch[c_1]` are fed to `fun` as `a_idx`, `c_0_idx`, and `c_1_idx`.
 *
 * @tparam crossingsQ A `bool` that controls whether the information of the crossings at the tip of tail of the current arc shall be fed to `arc_fun`.
 *
 * @tparam arclabelsQ A `bool` that controls whether `A_scratch` shall be populated with the reordering of arcs.
 *
 * @tparam start_arc_ou Controls how the first arc in a link component is chosen: If set to `0` (default), then just the next unvisited arc is chosen. If set to `1`, then the algorithm tries to choose it so that its tail goes over. If set to `-1`, then the algorithm tries to choose it so that its tail goes under. This feature is useful to traverse over/understrands.
 *
 * @param lc_pre A lambda function that is executed at the start of every link component. Must have the following signature:
 *    `lc_pre( const Int lc, const Int lc_begin )`.
 *
 * @param arc_fun A function to apply to every visited arc. Its require argument pattern depends on `crossingsQ`:
 * If `crossingsQ == false`, then it must be of the pattern
 *      `arc_fun( Int a, Int a_idx, Int lc )`.
 * If `crossingsQ == false`, then it must be of the pattern
 *      `arc_fun( Int a, Int a_idx, Int lc, Int c_0, Int c_0_idx, bool c_0_visited, Int c_1, Int c_1_idx, bool c_1_visitedQ )`.
 * Here the arguments mean the following:
 *      - `a` is the current arc within the link;
 *      - `a_idx` is the position of the current arc `a` within the traversal;
 *      - `lc` is the index of a's link component.
 *      - `c_0` is the crossing at the _tail_ of arc `a`;
 *      - `c_0_idx` is the position of `c_0` within the traversal;
 *      - `c_0_visited` is a `bool` that indicates whether crossing `c_0` is visited for the first (`false`) or the second (`true`) time.
 *      - `c_1` is the crossing at the _tip_ of arc `a`;
 *      - `c_1_idx` is the position of `c_1` within the traversal.
 *      - `c_1_visited` is a `bool` that indicates whether crossing `c_0` is visited for the first (`false`) or the second (`true`) time.
 *
 * @param lc_post A lambda function that is executed at the end of every link component. Must have the following signature:
 *    `lc_post( const Int lc, const Int lc_begin, const Int lc_end )`.
 */

template<
    bool crossingsQ, bool arclabelsQ,
    int start_arc_ou = 0, bool use_lutQ = true,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse(
    LinkCompPre_T && lc_pre, ArcFun_T && arc_fun, LinkCompPost_T && lc_post
)  const
{
    TOOLS_PTIMER(timer,ClassName()+"::Traverse"
        + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
        + "," + (arclabelsQ ? "w/ arc labels" : "w/o arc labels")
        + "," + (start_arc_ou == 0 ?  "0" : (start_arc_ou < 0 ? "under" : "over") )
        + "," + ToString(use_lutQ)
        + ">");
    
//    cptr<Int>  A_next,
//    mptr<std::conditional_t<arclabelsQ,Int,bool>> A_flag,
//    mptr<Int>  C_idx
    
    auto * A_data = reinterpret_cast<std::conditional_t<arclabelsQ,Int,bool> *>(A_scratch.data());
    
    auto * C_data = C_scratch.data();

    if constexpr ( use_lutQ )
    {
        this->template Traverse_impl<crossingsQ,arclabelsQ,start_arc_ou,use_lutQ>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
            ArcNextArc().data(), A_data, C_data
        );
        
        this->ClearCache("ArcNextArc");
    }
    else
    {
        this->template Traverse_impl<crossingsQ,arclabelsQ,start_arc_ou,use_lutQ>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
            nullptr, A_data, C_data
        );
    }
}


template<
    bool crossingsQ, bool arclabelsQ,
    int start_arc_ou = 0, bool use_lutQ = true,
    typename ArcFun_T
>
void Traverse( ArcFun_T && arc_fun )  const
{
    this->template Traverse<crossingsQ,arclabelsQ,start_arc_ou,use_lutQ>(
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
    bool crossingsQ, bool arclabelsQ, int start_arc_ou, bool use_lutQ,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse_impl(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    cptr<Int>  A_next,
    mptr<std::conditional_t<arclabelsQ,Int,bool>> A_flag,
    mptr<Int>  C_idx
)  const
{

    if constexpr ( !use_lutQ )
    {
        (void)A_next;
    }
    
    if( InvalidQ() )
    {
        eprint(ClassName() + "Traverse_impl"
               + "<" + BoolString(crossingsQ)
               + "," + BoolString(arclabelsQ)
               + "," + ToString(start_arc_ou)
               + "," + ToString(use_lutQ)
               + " >:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        
        // Other methods might assume that this is set.
        // In particular, calls to `LinkComponentCount` might go into a infinite loop.
        this->template SetCache<false>("LinkComponentCount",Int(0));
        
        return;
    }
    
    // Indicate that no arc or crossings are visited, yet.
    if constexpr ( arclabelsQ )
    {
        fill_buffer( A_flag, Uninitialized, max_arc_count );
    }
    else
    {
        fill_buffer( A_flag, false, max_arc_count );
    }
    
    if constexpr ( crossingsQ )
    {
        fill_buffer( C_idx, Uninitialized, max_crossing_count );
    }
    
    Int lc_counter = 0; // counter for the link components.
    Int a_counter  = 0; // counter for the arcs.
    Int c_counter  = 0; // counter for the crossings.
    
    constexpr bool ou_flag = (start_arc_ou != 0) ;
    constexpr bool overQ   = (start_arc_ou >  0) ;
    
    // TODO: Simply to a for-loop?
    
    for( Int a_0 = 0; a_0 < max_arc_count; ++a_0 )
    {
        if(
           ( arclabelsQ ? ValidIndexQ(A_flag[a_0]) : A_flag[a_0] )
           ||
           (!this->ArcActiveQ(a_0))
        )
        {
            continue;
        }
        
        // Now a_0 points to the beginning of a link component.
        Int a = a_0;
        
        if constexpr ( ou_flag )
        {
            // We have to find the _beginning_ of first over/understrand.
            // For this, we go forward until its tail goes under/over as needed.
            if( ArcOverQ(a,Tail) != overQ )
            {
                do
                {
                    // Move to next arc.
                    if constexpr ( use_lutQ )
                    {
                        a = A_next[a];
                    }
                    else
                    {
                        a = NextArc(a,Head);
                    }
                }
                while( (a != a_0) && (ArcOverQ(a,Tail) != overQ) );
            }
            
            a_0 = a;
        }
        
        // Now we are at the first arc of an over/understrand or we have `a = a_0`.
        // The latter can only happen if `ou_flag == true` and if the whole link component is a single over/understrand.
        
        // In any case, we just have to traverse forward through all arcs in the link component.
        
        const Int lc_begin = a_counter;
        lc_pre( lc_counter, lc_begin );

        Int  c_1 = 0;
        bool c_1_visitedQ = 0;
        Int  c_1_idx = 0;

        if constexpr ( crossingsQ )
        {
            // Get the crossing labels right.
            c_1 = A_cross(a,Tail);

            AssertCrossing(c_1);

            c_1_idx = C_idx[c_1];
            c_1_visitedQ = ValidIndexQ(c_1_idx);

            if( !c_1_visitedQ )
            {
                c_1_idx = C_idx[c_1] = c_counter;
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
                const Int c_0_idx      = c_1_idx;

                c_1 = A_cross(a,Head);
                AssertCrossing(c_1);

                c_1_idx = C_idx[c_1];
                c_1_visitedQ = ValidIndexQ(c_1_idx);
                if( !c_1_visitedQ )
                {
                    c_1_idx = C_idx[c_1] = c_counter;
                    ++c_counter;
                }

                arc_fun(
                    a,   a_counter, lc_counter,
                    c_0, c_0_idx,   c_0_visitedQ,
                    c_1, c_1_idx,   c_1_visitedQ
                );
            }
            else
            {
                arc_fun( a, a_counter, lc_counter );
            }
            
            // Move to next arc.
            if constexpr ( use_lutQ )
            {
                a = A_next[a];
            }
            else
            {
                if constexpr ( crossingsQ )
                {
                    a = NextArc(a,Head,c_1);
                }
                else
                {
                    a = NextArc(a,Head);
                }
            }
            
            AssertArc(a);

            ++a_counter;
        }
        while( a != a_0 );

        const Int lc_end = a_counter;

        lc_post( lc_counter, lc_begin, lc_end );
        ++lc_counter; 
    }

    this->template SetCache<false>("LinkComponentCount",lc_counter);
}

public:

template<bool use_lutQ = true, typename ArcFun_T>
void TraverseComponent(const Int a_0, ArcFun_T && arc_fun)  const
{
    TOOLS_PTIMER(timer,ClassName()+"::Traverse"
        + "<" + ToString(use_lutQ)
        + ">");
    

    if constexpr ( use_lutQ )
    {
        this->template TraverseComponent_impl<use_lutQ>(
            a_0, std::move(arc_fun),ArcNextArc().data()
        );
        
        this->ClearCache("ArcNextArc");
    }
    else
    {
        this->template Traverse_impl<use_lutQ>(
            a_0, std::move(arc_fun), nullptr
        );
    }
}

private:

template< bool use_lutQ, typename ArcFun_T>
void TraverseComponent_impl(
    const Int a_0,
    ArcFun_T && arc_fun,
    cptr<Int> A_next
)  const
{
    
    if constexpr ( !use_lutQ )
    {
        (void)A_next;
    }
    
    if( InvalidQ() )
    {
        eprint(ClassName() + "TraverseComponent_impl"
               + "<" + ToString(use_lutQ)
               + " >:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        return;
    }
    
    Int a = a_0;
    
    do
    {
        arc_fun(a);
        
        // Move to next arc.
        if constexpr ( use_lutQ )
        {
            a = A_next[a];
        }
        else
        {
            a = NextArc(a,Head);
        }
    }
    while( a != a_0 );
}

