public:

/*!
 * @brief The maximal number of arcs for which memory is allocated in the data structure.
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

/*!
 * @brief Returns the arcs that connect the crossings as a reference to a constant `Tensor2` object, which is basically a heap-allocated matrix.
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
 * Beware that an arc can have various states such as `CrossingState::Active` or `CrossingState::Deactivated`. This information is stored in the corresponding entry of `ArcStates()`.
 */

cref<ArcContainer_T> Arcs() const
{
    return A_cross;
}

/*!
 * @brief Returns the states of the arcs. The state encodes whether an edge is active and how its it is connected to the crossings at its head and tail.
 *
 * Use the methods `ArcState_T::ActiveQ`, `ArcState_T::Side`, `ArcState_T::OverQ` to find out more about the states.
 *
 * The user is not supposed to manipulate the states.
 */

cref<ArcStateContainer_T> ArcStates() const
{
    return A_state;
}

/*!
 * @brief Returns the colors of the arcs. Each arc has a unique color which indicates to which link component the arc orginially belonged (before various simplification methods were applied).
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
        wprint(ClassName()+"::DeactivateArc: Attempted to deactivate already inactive " + ArcString(a) + ".");
#endif
    }
    
    PD_ASSERT( arc_count >= Int(0) );
}


public:

/*!
 * @brief This tells us whether a giving arc goes left or right into the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcSide( const Int a )  const
{
    return A_state[a].template Side<headtail>();
}

/*!
 * @brief This tells us whether a giving arc goes left or right into the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcSide( const Int a, const bool headtail )  const
{
    return A_state[a].Side(headtail);
}




/*!
 * @brief This tells us whether the crossing at the indicated end of a given arc is right-handed.
 *
 *  @param a The index of the arc in question.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcRightHandedQ( const Int a )  const
{
    return A_state[a].template RightHandedQ<headtail>();
}

/*!
 * @brief This tells us whether the crossing at the indicated end of a given arc is right-handed.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcRightHandedQ( const Int a, const bool headtail )  const
{
    return A_state[a].RightHandedQ(headtail);
}




/*!
 * @brief This tells us whether the crossing at the indicated end of a given arc is left-handed.
 *
 *  @param a The index of the arc in question.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcLeftHandedQ( const Int a )  const
{
    return A_state[a].template LeftHandedQ<headtail>();
}

/*!
 * @brief This tells us whether the crossing at the indicated end of a given arc is left-handed.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcLeftHandedQ( const Int a, const bool headtail )  const
{
    return A_state[a].LeftHandedQ(headtail);
}


/*!
 * @brief This tells us whether a giving arc goes under the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcOverQ( const Int a )  const
{
    return A_state[a].template OverQ<headtail>();
}

/*!
 * @brief This tells us whether a giving arc goes under the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @param headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

bool ArcOverQ( const Int a, const bool headtail )  const
{
    return A_state[a].OverQ(headtail);
}



/*!
 * @brief This tells us whether a giving arc goes over the crossing at the indicated end.
 *
 *  @param a The index of the arc in question.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcUnderQ( const Int a )  const
{
    return A_state[a].template UnderQ<headtail>();
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



bool AlternatingQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) && (ArcOverQ<Tail>(a) == ArcOverQ<Head>(a)) )
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
 *  @tparam headtail Boolean that indicates the travel diretion" `headtail == true` means forward and `headtail == false` means backward.
 */

template<bool headtail>
Int NextArc( const Int a ) const
{
    return this->template NextArc<headtail>(a,A_cross(a,headtail));
}

template<bool headtail>
Int NextArc( const Int a, const Int c ) const
{
    AssertArc(a);
    AssertCrossing(c);
    PD_ASSERT( A_cross(a,headtail) == c );
    
    const Int a_next = C_arcs(c,!headtail,!A_state[a].template Side<headtail>());
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

//#########################################################
//###       Left
//#########################################################

/*!
 * @brief Returns the arc following arc `a` by going to the crossing at the head of `a` and then turning left.
 */

// TODO: Make this obsolete and use NextLeftDarc instead.
Arrow_T NextLeftArc( const Int a, const bool headtail ) const
{
    AssertArc(a);
    
    const Int c = A_cross(a,headtail);
    AssertCrossing(c);
    
    const bool side = A_state[a].Side(headtail);
    
    /*
     *   O     O    headtail == Head  == Right
     *    ^   ^     side     == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,side,!headtail);
    AssertArc(b);
    
    return Arrow_T(b,!side);
}


//#########################################################
//###       Right
//#########################################################

// TODO: Make this obsolete and use NextLeftDarc instead.
Arrow_T NextRightArc( const Int a, const bool headtail ) const
{
    AssertArc(a);
    
    const Int c = A_cross(a,headtail);
    AssertCrossing(c);
    
    const bool side = A_state[a].Side(headtail);
    
    /*
     *   O     O    headtail == Head  == Right
     *    ^   ^     side     == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,!side,headtail);
    AssertArc(b);
    
    return Arrow_T(b,side);
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
            
            const bool side_0 = (C_arcs(c_0,Out,Right) == a);
            const bool side_1 = (C_arcs(c_1,In ,Right) == a);
            
            A_state[a].template Set<Tail>( side_0, C_state[c_0] );
            A_state[a].template Set<Head>( side_1, C_state[c_1] );
        }
    }
}
