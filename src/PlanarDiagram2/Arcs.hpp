public:

/*!@brief The maximal number of arcs for which memory is allocated in the data structure.
 */

Int MaxArcCount() const
{
    return max_arc_count;
}

/*!@brief Returns the number of arcs in the planar diagram.
 */

Int ArcCount() const
{
    return arc_count;
}

/*!@brief Returns the arcs that connect the crossings as a reference to a constant `Tensor2` object, which is basically a heap-allocated matrix.
 *
 * This reference is constant because things can go wild (segfaults, infinite loops) if we allow the user to mess with this data.
 *
 * The `a`-th arc is stored in `Arcs()(a,i)`, `i = 0,1`, in the following way; note that we defined Booleans `Tail = 0` and `Head = 1` for easing the indexing:
 *
 *                        a
 *    Arcs()(a,0) X -------------> X Arcs()(a,0)
 *        ==                             ==
 * Arcs()(a,Tail)                    Arcs()(a,Head)
 *
 * This means that arc `a` leaves crossing `GetArc()(a,Tail)` and enters `GetArc()(a,Head)`.
 *
 * Beware that an arc can have various states such as `CrossingState_T::Active` or `CrossingState_T::Deactivated`. This information is stored in the corresponding entry of `ArcStates()`.
 */

cref<ArcContainer_T> Arcs() const
{
    return A_cross;
}

/*!@brief Returns the states of the arcs. The state encodes whether an edge is active and how its it is connected to the crossings at its head and tail.
 *
 * Use the methods `ArcState_T::ActiveQ`, `ArcState_T::Side`, `ArcState_T::OverQ` to find out more about the states.
 *
 * The user is not supposed to manipulate the states.
 */

cref<ArcStateContainer_T> ArcStates() const
{
    return A_state;
}

/*! @brief Returns the colors of the arcs. Each arc has a unique color which indicates to which link component the arc orginially belonged (before various simplification methods were applied).
 */

cref<ArcColorContainer_T> ArcColors() const
{
    return A_color;
}


/*!@brief Returns the arc scratch buffer that is used for a couple of algorithms, in particular by transversal routines.  Use with caution as its content depends heavily on which routines have been called before.
 */
cref<Tensor1<Int,Int>> ArcScratchBuffer() const
{
    return A_scratch;
}


A_Cross_T CopyArc( const Int a ) const
{
    return A_Cross_T( A_cross.data(a) );
}

ArcState_T ArcState( const Int a ) const
{
    return A_state[a];
}

/*!@brief Checks whether arc `a` is still active.
 */

bool ArcActiveQ( const Int a ) const
{
    return ActiveQ(A_state[a]);
}


std::string ArcString( const Int a ) const
{
    return "arc " +Tools::ToString(a) +" = { "
        + Tools::ToString(A_cross(a,Tail)) + ", "
        + Tools::ToString(A_cross(a,Head)) + " } ("
        + ToString(A_state[a]) + ", color = " + ToString(A_color[a]) + ")";
}

Int CountActiveArcs() const
{
    Int counter = 0;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        counter += ArcActiveQ(a);
    }
    
    return counter;
}


private:
    
/*!@brief Deactivates arc `a`. Only for internal use.
 */

void DeactivateArc( const Int a )
{
    if( ArcActiveQ(a) )
    {
        PD_PRINT("Deactivating " + ArcString(a) + "." );
        
        --arc_count;
        last_color_deactivated = A_color[a];
        A_state[a] = ArcState_T::Inactive;
    }
    else
    {
#if defined(PD_DEBUG)
        wprint(MethodName("DeactivateArc")+": Attempted to deactivate already inactive " + ArcString(a) + ".");
#endif
    }
    
    PD_ASSERT( arc_count >= Int(0) );
}

public:

/*!@brief This tells us whether arc `a` goes into a left or right slot of the crossing at the indicated end.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcSide( const Int a, const bool headtail )  const
{
    return ArcSide(a,headtail,A_cross(a,headtail));
}

/*!@brief This tells us whether arc `a` goes into a left or right slot of the crossing `c`. Warning: This really assumes that `c` is the end point at the end indicated by `headtail`. This function is meant to save a look-up if `c` is already known.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 *
 * @param c The index of the arc in question.
 */

bool ArcSide( const Int a, const bool headtail, const Int c  )  const
{
    return (C_arcs(c,headtail,Right) == a);
}


/*!@brief This tells us whether the crossing at the indicated end of a given arc is right-handed.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcRightHandedQ( const Int a, const bool headtail )  const
{
    return CrossingRightHandedQ(A_cross(a,headtail));
}

/*!@brief This tells us whether the crossing at the indicated end of a given arc is left-handed.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcLeftHandedQ( const Int a, const bool headtail )  const
{
    return CrossingLeftHandedQ(A_cross(a,headtail));
}


/*!@brief This tells us whether the arc `a` goes under the crossing at the indicated end.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcUnderQ( const Int a, const bool headtail )  const
{
    AssertArc(a);
    return ArcUnderQ(a,headtail,A_cross(a,headtail));
}

/*!@brief This tells us whether the arc `a` goes under the crossing `c`. Warning: This really assumes that `c` is the end point at the end indicated by `headtail`. This function is meant to save a look-up if `c` is already known.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 *
 * @param c The index of the arc in question.
 */

bool ArcUnderQ( const Int a, const bool headtail, const Int c )  const
{
    AssertArc(a);
    AssertCrossing(c);
    PD_ASSERT( A_cross(a,headtail) == c );
    
    return CrossingRightHandedQ(c) == (headtail == ArcSide(a,headtail,c));
    
    /* headtail == Tail, side == Right => underQ == false
     *
     *    ^       ^
     *     \     /  a
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *     /     \
     *    /       \
     */

    /* headtail == Tail, side == Left => underQ == true
     *
     *    ^       ^
     *  a  \     /
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *     /     \
     *    /       \
     */

    /* headtail == Head, side == Right => underQ == true
     *
     *    ^       ^
     *     \     /
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *     /     \  a
     *    /       \
     */

    /* headtail == Head, side == Left => underQ == false
     *
     *    ^       ^
     *     \     /
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *  a  /     \
     *    /       \
     */
}

/*!@brief This tells us whether the arc `a` goes over the crossing at the indicated end.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcOverQ( const Int a, const bool headtail )  const
{
    AssertArc(a);
    return ArcOverQ(a,headtail,A_cross(a,headtail));
}


/*!@brief This tells us whether the arc `a` goes over the crossing `c`. Warning: This really assumes that `c` is the end point at the end indicated by `headtail`. This function is meant to save a look-up if `c` is already known.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 *
 * @param c The index of the arc in question.
 */

bool ArcOverQ( const Int a, const bool headtail, const Int c )  const
{
    AssertArc(a);
    AssertCrossing(c);
    PD_ASSERT( A_cross(a,headtail) == c );
    
    return CrossingRightHandedQ(c) != (headtail == ArcSide(a,headtail,c));
    
    /* headtail == Tail, side == Right => overQ == true
     *
     *    ^       ^
     *     \     /  a
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *     /     \
     *    /       \
     */

    /* headtail == Tail, side == Left => overQ == false
     *
     *    ^       ^
     *  a  \     /
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *     /     \
     *    /       \
     */

    /* headtail == Head, side == Right => overQ == false
     *
     *    ^       ^
     *     \     /
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *     /     \  a
     *    /       \
     */

    /* headtail == Head, side == Left => overQ == true
     *
     *    ^       ^
     *     \     /
     *      \   /
     *       \ /
     *        / <--- c
     *       ^ ^
     *      /   \
     *  a  /     \
     *    /       \
     */
}

/*!@brief Returns the arc next to arc `a`, i.e., the arc reached by going straight through the crossing at the head/tail of `a`.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates the travel diretion" `headtail == true` means forward and `headtail == false` means backward.
 */

Int NextArc( const Int a, const bool headtail ) const
{
    return NextArc(a,headtail,A_cross(a,headtail));
}

/*!@brief Returns the arc next to arc `a`, i.e., the arc reached by going straight through the crossing `c` at the head/tail of `a`. Warning: This really assumes that `c` is the end point at the end indicated by `headtail`. This function is meant to save a look-up if `c` is already known.
 *
 * @param a The index of the arc in question.
 *
 * @param headtail Boolean that indicates the travel diretion" `headtail == true` means forward and `headtail == false` means backward.
 *
 * @param c The index of the arc in question.
 */

Int NextArc( const Int a, const bool headtail, const Int c ) const
{
    AssertArc(a);
    AssertCrossing(c);
    PD_ASSERT( A_cross(a,headtail) == c );
    
    const bool side   = ArcSide(a,headtail,c);
    const Int  a_next = C_arcs(c,!headtail,!side);
    AssertArc(a_next);
    
    return a_next;
}


template<bool headtail>
bool CheckNextArc() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        
        const Int a_next = NextArc(a,headtail);

        if( A_cross(a_next,!headtail) != A_cross(a,headtail) ) { return false; }
    }
    return true;
}

cref<Tensor1<Int,Int>> ArcNextArc() const
{
    std::string tag ("ArcNextArc");
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
    if( !this->InCacheQ(tag) )
    {
        Tensor1<Int,Int> A_next ( max_arc_count, Uninitialized );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                const C_Arcs_T C = CopyCrossing(c);
                A_next(C[In][Left ]) = C[Out][Right];
                A_next(C[In][Right]) = C[Out][Left ];
            }
        }
        this->SetCache(tag,std::move(A_next));
    }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

bool CheckArcNextArc() const
{
    auto next = ArcNextArc();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        
        const Int a_next = next[a];

        if( A_cross(a_next,Tail) != A_cross(a,Head) ) { return false; }
    }
    
    return true;
}


cref<Tensor1<Int,Int>> ArcPrevArc() const
{
    std::string tag ("ArcPrevArc");
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
    if( !this->InCacheQ(tag) )
    {
        Tensor1<Int,Int> A_prev ( max_arc_count, Uninitialized );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                const C_Arcs_T C = CopyCrossing(c);
                A_prev(C[Out][Right]) = C[In][Left ];
                A_prev(C[Out][Left ]) = C[In][Right];
            }
        }
        
        this->SetCache(tag,std::move(A_prev));
    }
    
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

bool CheckArcPrevArc() const
{
    auto prev = ArcPrevArc();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        
        const Int a_prev = prev[a];

        if( A_cross(a_prev,Head) != A_cross(a,Tail) ) { return false; }
    }
    
    return true;
}
