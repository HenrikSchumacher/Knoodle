public:

/*! @brief The maximal number of arcs for which memory is allocated in the data structure.
 */

Int MaxArcCount() const
{
    return max_arc_count;
}

/*! @brief Returns the number of arcs in the planar diagram.
 */

Int ArcCount() const
{
    return arc_count;
}

/*! @brief Returns the arcs that connect the crossings as a reference to a constant `Tensor2` object, which is basically a heap-allocated matrix.
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

/*! @brief Returns the states of the arcs. The state encodes whether an edge is active and how its it is connected to the crossings at its head and tail.
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



ArcState_T ArcState( const Int a ) const
{
    return A_state[a];
}

/*!
 * @brief Checks whether arc `a` is still active.
 */

bool ArcActiveQ( const Int a ) const
{
    return A_state[a].ActiveQ();
}


std::string ArcString( const Int a ) const
{
    return "arc " +Tools::ToString(a) +" = { "
        + Tools::ToString(A_cross(a,Tail)) + ", "
        + Tools::ToString(A_cross(a,Head)) + " } ("
        + ToString(A_state[a]) + ")";
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
    
/*!
 * @brief Deactivates arc `a`. Only for internal use.
 */

void DeactivateArc( const Int a )
{
    if( ArcActiveQ(a) )
    {
        PD_PRINT("Deactivating " + ArcString(a) + "." );
        
        --arc_count;
        A_state[a] = ArcState_T::Inactive();
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

void RecomputeArcState( Int a )
{
    const Int  c_0 = A_cross          (a,Tail);
    const bool s_0 = ArcSide_Reference(a,Tail);
    const Int  c_1 = A_cross          (a,Head);
    const bool s_1 = ArcSide_Reference(a,Head);

    ArcState_T a_state = ArcState_T::Inactive();
    a_state.Set(Tail,s_0,C_state[c_0]);
    a_state.Set(Head,s_1,C_state[c_1]);
    A_state[a] = a_state;
}

void RecomputeArcStates()
{
    TOOLS_PTIMER(timer,MethodName("RecomputeArcStates"));

    for( Int c = 0; c < MaxCrossingCount(); ++c )
    {
        if( CrossingActiveQ(c) )
        {
            Int A [2][2];
            
            copy_buffer<4>( C_arcs.data(c), &A[0][0] );

            const CrossingState_T c_state = C_state[c];
            
            /* A[Out][Left ]         A[Out][Right]
             *               O     O
             *                ^   ^
             *                 \ /
             *                  X c
             *                 ^ ^
             *                /   \
             *               O     O
             * A[In ][Left ]         A[In ][Right]
             */
            
            A_state[A[Out][Left ]].Set(Tail,Left ,c_state);
            A_state[A[Out][Right]].Set(Tail,Right,c_state);
            A_state[A[In ][Left ]].Set(Head,Left ,c_state);
            A_state[A[In ][Right]].Set(Head,Right,c_state);
        }
    }
}

public:

/*! @brief This tells us whether a giving arc goes left or right into the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcSide( const Int a, const bool headtail )  const
{
    return A_state[a].Side(headtail);
}

bool ArcSide_Reference( const Int a, const bool headtail )  const
{
    const Int c = A_cross(a,headtail);
    return (C_arcs(c,headtail,Right) == a);
}


/*! @brief This tells us whether the crossing at the indicated end of a given arc is right-handed.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcRightHandedQ( const Int a, const bool headtail )  const
{
    return A_state[a].RightHandedQ(headtail);
}

bool ArcRightHandedQ_Reference( const Int a, const bool headtail )  const
{
    return CrossingRightHandedQ(A_cross(a,headtail));
}


/*! @brief This tells us whether the crossing at the indicated end of a given arc is left-handed.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcLeftHandedQ( const Int a, const bool headtail )  const
{
    return A_state[a].LeftHandedQ(headtail);
}

bool ArcLeftHandedQ_Reference( const Int a, const bool headtail )  const
{
    return CrossingLeftHandedQ(A_cross(a,headtail));
}



/*! @brief This tells us whether a giving arc goes under the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcOverQ( const Int a, const bool headtail )  const
{
    return A_state[a].OverQ(headtail);
}

bool ArcOverQ_Reference( const Int a, const bool headtail )  const
{
    const Int  c    = A_cross(a,headtail);
    const bool side = ArcSide_Reference(a,headtail);
    
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
    
    // TODO: Check this!
    return CrossingRightHandedQ(c) == (headtail != side);
}

/*!
 * @brief This tells us whether a giving arc goes over the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcUnderQ( const Int a, const bool headtail )  const
{
    return A_state[a].UnderQ(headtail);
}

bool ArcUnderQ_Reference( const Int a, const bool headtail )  const
{
    return !ArcOverQ_Reference(a,headtail);
}



bool AlternatingQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) && (ArcOverQ(a,Tail) == ArcOverQ(a,Head)) )
        {
            return false;
        }
    }

    return true;
}


/*!
 * @brief Returns the arc next to arc `a`, i.e., the arc reached by going straight through the crossing at the head/tail of `a`.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates the travel diretion" `headtail == true` means forward and `headtail == false` means backward.
 */

Int NextArc( const Int a, const bool headtail ) const
{
    return NextArc(a,headtail,A_cross(a,headtail));
}

Int NextArc( const Int a, const bool headtail, const Int c ) const
{
    AssertArc(a);
    AssertCrossing(c);
    PD_ASSERT( A_cross(a,headtail) == c );
    
    const bool side   = ArcSide(a,headtail);
    const Int  a_next = C_arcs(c,!headtail,!side);
    AssertArc(a_next);
    
    return a_next;
}


Int NextArc_Reference( const Int a, const bool headtail, const Int c ) const
{
    AssertArc(a);
    AssertCrossing(c);
    PD_ASSERT( A_cross(a,headtail) == c );
    
    const bool side   = ArcSide_Reference(a,headtail);
    const Int  a_next = C_arcs(c,!headtail,!side);
    AssertArc(a_next);
    
    return a_next;
}

Int NextArc_Reference( const Int a, const bool headtail ) const
{
    AssertArc(a);
    
    const Int  c      = A_cross          (a,headtail);
    AssertCrossing(c);
    const bool side   = ArcSide_Reference(a,headtail);
    
    const Int  a_next = C_arcs(c,!headtail,!side );
    AssertArc(a_next);
    
    return a_next;
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
                A_next(C_arcs(c,In,Left )) = C_arcs(c,Out,Right);
                A_next(C_arcs(c,In,Right)) = C_arcs(c,Out,Left );
            }
        }
        this->SetCache(tag,std::move(A_next));
    }
    return this->GetCache<Tensor1<Int,Int>>(tag);
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
                A_prev(C_arcs(c,Out,Right)) = C_arcs(c,In,Left );
                A_prev(C_arcs(c,Out,Left )) = C_arcs(c,In,Right);
            }
        }
        
        this->SetCache(tag,std::move(A_prev));
    }
    
    return this->GetCache<Tensor1<Int,Int>>(tag);
}


/*! @brief Computes the arc states, assuming that the activity status of each arc is correct.
 */

void ComputeArcStates()
{
    ClearCache();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);
            
            A_state[a].Set(Tail,ArcSide_Reference(a,Tail),C_state[c_0]);
            A_state[a].Set(Head,ArcSide_Reference(a,Head),C_state[c_1]);
        }
    }
}
