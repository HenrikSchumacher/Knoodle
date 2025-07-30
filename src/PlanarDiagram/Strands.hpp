public:

/*!
 * @brief Returns an array that tells every arc to which over-strand it belongs.
 *
 *  More precisely, arc `a` belongs to over-strand number `ArcOverStrands()[a]`.
 *
 *  (An over-strand is a maximal consecutive sequence of arcs that pass over.)
 */

Tensor1<Int,Int> ArcOverStrands() const
{
    return ArcStrands<true>();
}

/*!
 * @brief Returns an array that tells every arc to which under-strand it belongs.
 *
 *  More precisely, arc `a` belongs to under-strand number `ArcUnderStrands()[a]`.
 *
 *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
 */

Tensor1<Int,Int> ArcUnderStrands() const
{
    return ArcStrands<false>();
}

private:

template<bool overQ>
Tensor1<Int,Int> ArcStrands() const
{
    TOOLS_PTIMER(timer,ClassName()+"::" + (overQ ? "Over" : "Under")  + "StrandIndices");
    
    Tensor1<Int,Int> A_colors ( max_arc_count, Uninitialized );
    Int color = 0;
    
    this->template Traverse<false,false,(overQ ? -1 : 1 )>(
        [&color,&A_colors,this]
        ( const Int a, const Int a_pos, const Int lc )
        {
            (void)a_pos;
            (void)lc;

            A_colors[a] = color;

            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to initialize a new strand.
            color += (ArcOverQ<Head>(a) != overQ);
        }
    );

    return A_colors;
}

public:


/*!
 * @brief Returns an array of that tells every crossing which over-strand end or start in is.
 *
 *  More precisely, crossing `c` has the outgoing over-strands `CrossingOverStrands()(c,0,0)` and `CrossingOverStrands()(c,0,1)`
 *  and the incoming over-strands `CrossingOverStrands()(c,1,0)` and `CrossingOverStrands()(c,1,1)`.
 *
 *  (An over-strand is a maximal consecutive sequence of arcs that pass over.)
 */


Tensor3<Int,Int> CrossingOverStrands() const
{
    return CrossingStrands<true>();
}

/*!
 * @brief Returns an array of that tells every crossing which under-strand end or start in is.
 *
 *  More precisely, crossing `c` has the outgoing under-strands `CrossingUnderStrands()(c,0,0)` and `CrossingUnderStrands()(c,0,1)`
 *  and the incoming under-strands `CrossingUnderStrands()(c,1,0)` and `CrossingUnderStrands()(c,1,1)`.
 *
 *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
 */

Tensor3<Int,Int> CrossingUnderStrands() const
{
    return CrossingStrands<false>();
}

template<bool overQ>
Tensor3<Int,Int> CrossingStrands() const
{
    TOOLS_PTIMER(timer,ClassName()+"::Crossing" + (overQ ? "Over" : "Under") + "Strands");
    
    Tensor3<Int,Int> C_strands ( max_crossing_count, 2, 2, -1 );
    
    Int strand_counter = 0;
    
    this->template Traverse<false,false,(overQ ? -1 : 1)>(
        [&strand_counter,&C_strands,this]
        ( const Int a, const Int a_pos, const Int lc )
        {
            (void)a_pos;
            (void)lc;
            
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);
            
            // TODO: Putting side information into A_state would be very useful here.

            C_strands(c_0,Out,(C_arcs(c_0,Out,Right) == a)) = strand_counter;
            C_strands(c_1,In ,(C_arcs(c_1,In ,Right) == a)) = strand_counter;

            strand_counter += (ArcUnderQ<Head>(a) == overQ);
        }
    );
    
    return C_strands;
}
