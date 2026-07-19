public:

/*!@brief Returns a RaggedList that contains the arcs that belong to every overstrand. The arcs are ordered as they are traversed in forward direction.
 */

RaggedList<Int,Int> OverStrandArcs() const
{
    return this->StrandArcs<true>();
}

/*!@brief Returns an array that tells every arc to which over-strand it belongs.
 *
 *  More precisely, arc `a` belongs to over-strand number `ArcOverStrands()[a]`.
 *  (An overstrand is a maximal consecutive sequence of arcs that pass over.)
 */

Tensor1<Int,Int> ArcOverStrands() const
{
    return this->ArcStrands<true>();
}

/*!@brief Returns a RaggedList that contains the arcs that belong to every understrand. The arcs are ordered as they are traversed in forward direction.
 */

RaggedList<Int,Int> UnderStrandArcs() const
{
    return this->StrandArcs<false>();
}

/*!@brief Returns an array that tells every arc to which under-strand it belongs.
 *
 *  More precisely, arc `a` belongs to under-strand number `ArcUnderStrands()[a]`.
 *
 *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
 */

Tensor1<Int,Int> ArcUnderStrands() const
{
    return this->ArcStrands<false>();
}

private:

template<bool overQ>
RaggedList<Int,Int> StrandArcs() const
{
    TOOLS_PTIMER(timer, MethodName(overQ ? "Over" : "Under") + "StrandArcs");
    
    // Are these sizes optimal?
    RaggedList<Int,Int> strand_arcs ( crossing_count, arc_count );
    
    this->template Traverse_ByNextArc<false,false,(overQ ? -1 : 1 )>(
        [&strand_arcs,this]
        ( const Int a, const Int a_pos, const Int lc )
        {
            (void)a_pos;
            (void)lc;

            strand_arcs.Push(a);

            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to initialize a new strand.
            if( ArcOverQ(a,Head) != overQ ) { strand_arcs.FinishSublist(); }
        }
    );
    
    return strand_arcs;
}


template<bool overQ>
Tensor1<Int,Int> ArcStrands() const
{
    TOOLS_PTIMER(timer,ClassName()+"::" + (overQ ? "Over" : "Under")  + "StrandIndices");
    
    Tensor1<Int,Int> A_strand ( MaxArcCount(), Uninitialized );
    Int strand = 0;
    
    this->template Traverse_ByNextArc<false,false,(overQ ? -1 : 1 )>(
        [&strand,&A_strand,this]
        ( const Int a, const Int a_pos, const Int lc )
        {
            (void)a_pos;
            (void)lc;

            A_strand[a] = strand;

            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to initialize a new strand.
            strand += (ArcOverQ(a,Head) != overQ);
        }
    );

    return A_strand;
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


CrossingContainer_T CrossingOverStrands() const
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

CrossingContainer_T CrossingUnderStrands() const
{
    return CrossingStrands<false>();
}

template<bool overQ>
CrossingContainer_T CrossingStrands() const
{
    TOOLS_PTIMER(timer,ClassName()+"::Crossing" + (overQ ? "Over" : "Under") + "Strands");
    
//    CrossingContainer_T C_strand ( max_crossing_count, 2, 2, -1 );
    
    CrossingContainer_T C_strand ( max_crossing_count, -1 );
    
    Int strand = 0;
    
    this->template Traverse_ByNextArc<false,false,(overQ ? -1 : 1)>(
        [&strand,&C_strand,this]
        ( const Int a, const Int a_pos, const Int lc )
        {
            (void)a_pos;
            (void)lc;
            
            const Int  c_0    = A_cross(a,Tail);
            const bool side_0 = ArcSide(a,Tail,c_0);
            const Int  c_1    = A_cross(a,Head);
            const bool side_1 = ArcSide(a,Head,c_0);
            
            C_strand(c_0,Out,side_0) = strand;
            C_strand(c_1,In ,side_1) = strand;

            strand += (ArcOverQ(a,Head) != overQ);
        }
    );
    
    return C_strand;
}
