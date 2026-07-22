//TODO: Make this available again. (Although barely needed.)


/*! @brief Construct oriented `Link` from a list of tails and from a list of heads.
 *
 *  @param edge_tails_ Array of integers of length `edge_count_`. Entries are treated as tails of edges.
 *
 *  @param edge_heads_ Array of integers of length `edge_count_`. Entries are treated as heads of edges.
 *
 *  @param edge_count_ Number of edges.
 */

template<IntQ I_0, IntQ I_1>
Link( cptr<I_0> edge_tails_, cptr<I_0> edge_heads_, cptr<I_0> edge_colors_, const I_1 edge_count_ )
:   Link( int_cast<Int>(edge_count_), true )
{
    ReadEdges( edge_tails_, edge_heads_, edge_colors_ );
}


//TODO: Make this available again. (Although barely needed.)


/*! @brief Reads edges from the arrays `edge_tails_` and `edge_heads_`.
 *
 *  @param edge_tails_ Integer array of length `EdgeCount()` that contains the list of tails.
 *
 *  @param edge_heads_ Integer array of length `EdgeCount()` that contains the list of heads.
 */


// TODO: Scrutinize this!
template<IntQ ExtInt>
void ReadEdges(
    cptr<ExtInt> edge_tails_, cptr<ExtInt> edge_heads_, cptr<ExtInt> edge_colors_
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ReadEdges");};

    TOOLS_PTIMER(timer,tag());

    // Finding for each e its next e.
    // Caution: Assuming here that link is correctly oriented and that it has no boundaries.

    // using edges.data(0) temporarily as scratch space.
    mptr<ExtInt> tail_of_edge = edges.data(0);

    for( Int e = 0; e < edge_count; ++e )
    {
        const ExtInt tail = edge_tails_[e];

        if( !std::in_range<Int>(tail) )
        {
            error(tag()+": index tail is out of range for type " + TypeName<Int> + " (tail = " + ToString(tail) + ").");
        }
        if( std::cmp_less(tail, ExtInt(0)) )
        {
            error(tag()+": tail < 0 (tail = " + ToString(tail) + ").");
        }
        if( std::cmp_greater_equal(tail,edge_count) )
        {
            error(tag()+": tail >= edge_count (tail = " + ToString(tail) + ", edge_count = " + ToString(edge_count) + ").");
        }

        tail_of_edge[static_cast<Int>(tail)] = e;
    }

    for( Int e = 0; e < edge_count; ++e )
    {
        const ExtInt head = edge_heads_[e];

        if( !std::in_range<Int>(head) )
        {
            error(tag()+": head tail is out of range for type " + TypeName<Int> + " (head = " + ToString(head) + ").");
        }
        if( std::cmp_less(head, ExtInt(0)) )
        {
            error(tag()+": tail < 0 (head = " + ToString(head) + ").");
        }
        if( std::cmp_greater_equal(head,edge_count) )
        {
            error(tag()+": head >= edge_count (head = " + ToString(head) + ", edge_count = " + ToString(edge_count) + ").");
        }

        next_edge[e] = tail_of_edge[static_cast<Int>(head)];
    }

    FindComponents();

    // using edge_ptr temporarily as scratch space.
    cptr<Int> perm       = edge_ptr.data();

    // Reordering edges.
    for( Int e = 0; e < edge_count; ++e )
    {
        const Int from = perm[e];
        edges(e,0) = edge_tails_[from];
        edges(e,1) = edge_heads_[from];
    }

    FinishPreparations(edge_colors_);
}

