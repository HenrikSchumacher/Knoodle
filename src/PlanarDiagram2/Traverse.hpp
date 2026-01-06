public:

/*!
 * @brief Traverse each component of the link and apply function `arc_fun` to every arc visited. This function may leverage the precomputed array `ArcTraversalFlags` by calling `Traverse_ByLinkComponents` if it are already available. Otherwise, it will traverse by calling `Traverse_ByNextArc`.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten as follows:
 *
 * If `labelsQ == true`, then, on return, `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is active, then `C_scratch[c]` is the position of `c` among the visited crossings; otherwise, if `c` is inactive, `C_scratch[c] = Uninitialized`.
 * Otherwise, if `labelsQ == false`, then `C_scratch` may contain garbage; do not use it.
 *
 * If `labelsQ == true`, then, on return, `A_scratch` contains the reordering:
 * If arc `a` is active, then `A_scratch[a]` is the position of `a` among the visited arcs; otherwise, if `a` is inactive, then `A_scratch[a] = Uninitialized`.
 * Otherwise, if `labelsQ == false`, then `A_scratch` contains garbage; do not use it.
 *
 * @tparam crossingsQ A `bool` that controls whether the information of the crossings at the tail and head of the current arc shall be fed to `arc_fun`.
 *
 * @tparam labelsQ A `bool` that controls whether `C_scratch` and `A_scratch` shall be populated with the reordering of arcs. `C_scratch` may or may not be populated, even if `labelsQ == false`.
 *
 * @param lc_pre A lambda function that is executed at the start of every link component. Must have the following signature:
 *    `lc_pre( const Int lc, const Int lc_begin )`.
 *
 * @param arc_fun A function to apply to every visited arc. Its argument pattern depends on `crossingsQ`:
 * If `crossingsQ == false`, then it must be of the pattern
 *      `arc_fun( Int a, Int a_idx, Int lc )`.
 * If `crossingsQ == true`, then it must be of the pattern
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
    bool crossingsQ, bool labelsQ = false,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse(
    LinkCompPre_T && lc_pre, ArcFun_T && arc_fun, LinkCompPost_T && lc_post
)  const
{
    TOOLS_PTIMER(timer,MethodName("Traverse")
                 + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
                 + "," + (labelsQ ? "w/ labels" : "w/o labels")
                 + ">");
    
    if( this->InCacheQ("ArcTraversalFlags") )
    {
        this->template Traverse_ByLinkComponents<crossingsQ,labelsQ>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post)
        );
    }
    else
    {
        this->template Traverse_ByNextArc<crossingsQ,labelsQ>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post)
        );
    }
}


/*!
 * @brief Short version of `Traverse` with only a argument `arc_fun`.
 */

template<bool crossingsQ, bool labelsQ = false, typename ArcFun_T>
void Traverse( ArcFun_T && arc_fun )  const
{
    this->template Traverse<crossingsQ,labelsQ>(
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

// TODO: Use this for ArcOverStrands and ArcUnderStrands.

/*!
 * @brief Traverse each component of the link and apply function `arc_fun` to every arc visited. 
 *
 * For the explanation of the template parameters and arguments see `Traverse`.
 *
 * Beware that `A_scratch` and `C_scratch` are overwritten as follows:
 *
 * On return, `C_scratch` contains the reordering of the crossings:
 * If crossing `c` is active, then `C_scratch[c]` is the position of `c` among the visited crossings; otherwise, if `c` is inactive, `C_scratch[c] = Uninitialized`.
 * Otherwise, if `labelsQ == false`, then `C_scratch` contains garbage; do not use it.
 *
 * If `arclabelsQ == true`, then, on return, `A_scratch` contains the reordering:
 * If arc `a` is active, then `A_scratch[a]` is the position of `a` among the visited arcs; otherwise, if `a` is inactive, then `A_scratch[a] = Uninitialized`.
 * Otherwise, if `labelsQ == false`, then `A_scratch` contains garbage; do not use it.
 *
 * Typically, `A_scratch` and `C_scratch` need not and should not be used externally. Because the values `A_scratch[a]`, `C_scratch[c_0]`, and `C_scratch[c_1]` are fed to `fun` as `a_idx`, `c_0_idx`, and `c_1_idx`, this could otherwise interfere with the traversal.
 *
 * The following template parameters come on top of those used in `Traverse`.
 *
 * @tparam start_arc_ou Controls how the first arc in a link component is chosen: If set to `0` (default), then just the next unvisited arc is chosen. If set to `1`, then the algorithm tries to choose it so that its tail goes over. If set to `-1`, then the algorithm tries to choose it so that its tail goes under. This feature is useful to traverse over/understrands.
 *
 * @tparam lutQ If set to `true`, then the traversal uses the precomputed array `ArcNextArc`; otherwise `NextArc<Head>` is used to determine the next arc to visit. The latter s typically slower.
 */

template<
    bool crossingsQ, bool arclabelsQ,
    int start_arc_ou = 0, bool lutQ = true,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse_ByNextArc(
    LinkCompPre_T && lc_pre, ArcFun_T && arc_fun, LinkCompPost_T && lc_post
)  const
{
    TOOLS_PTIMER(timer,MethodName("Traverse_ByNextArc")
                 + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
                 + "," + (arclabelsQ ? "w/ arc labels" : "w/o arc labels")
                 + "," + (start_arc_ou == 0 ?  "0" : (start_arc_ou < 0 ? "under" : "over") )
                 + "," + ToString(lutQ)
                 + ">");
    
    auto * A_data = reinterpret_cast<std::conditional_t<arclabelsQ,Int,bool> *>(A_scratch.data());
    
    auto * C_data = C_scratch.data();

    if constexpr ( lutQ )
    {
        this->template Traverse_ByNextArc_impl<crossingsQ,arclabelsQ,start_arc_ou,lutQ>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
            ArcNextArc().data(), A_data, C_data
        );
        
        // TODO: Is it really necessary to delete this cache?
//        this->ClearCache("ArcNextArc");
    }
    else
    {
        this->template Traverse_ByNextArc_impl<crossingsQ,arclabelsQ,start_arc_ou,lutQ>(
            std::move(lc_pre), std::move(arc_fun), std::move(lc_post),
            nullptr, A_data, C_data
        );
    }
}

/*!
 * @brief Short version of `Traverse_ByNextArc` with only a single argument `arc_fun`.
 */

template<
    bool crossingsQ, bool arclabelsQ,
    int start_arc_ou = 0, bool lutQ = true,
    typename ArcFun_T
>
void Traverse_ByNextArc( ArcFun_T && arc_fun )  const
{
    this->template Traverse_ByNextArc<crossingsQ,arclabelsQ,start_arc_ou,lutQ>(
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
    bool crossingsQ, bool arclabelsQ, int start_arc_ou, bool lutQ,
    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
>
void Traverse_ByNextArc_impl(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post,
    cptr<Int>  A_next,
    mptr<std::conditional_t<arclabelsQ,Int,bool>> A_flag,
    mptr<Int>  C_idx
)  const
{

    if constexpr ( !lutQ )
    {
        (void)A_next;
    }
    
    if( InvalidQ() )
    {
        eprint(MethodName("Traverse_ByNextArc_impl")
               + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
               + "," + (arclabelsQ ? "w/ arc labels" : "w/o arc labels")
               + "," + (start_arc_ou == 0 ?  "0" : (start_arc_ou < 0 ? "under" : "over") )
               + "," + ToString(lutQ)
               + " >: Trying to traverse an invalid planar diagram. Aborting.");
        
        // Other methods might assume that this is set.
        // In particular, calls to `LinkComponentCount` might go into an infinite loop.
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
            if( ArcOverQ<Tail>(a) != overQ )
            {
                do
                {
                    // Move to next arc.
                    if constexpr ( lutQ )
                    {
                        a = A_next[a];
                    }
                    else
                    {
                        a = this->template NextArc<Head>(a);
                    }
                }
                while( (a != a_0) && (ArcOverQ<Tail>(a) != overQ) );
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
            if constexpr ( lutQ )
            {
                a = A_next[a];
            }
            else
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
