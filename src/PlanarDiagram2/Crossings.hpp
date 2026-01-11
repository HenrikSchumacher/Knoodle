public:
    
/*!
 * @brief The maximal number of crossings for which memory is allocated in the data structure.
 */

Int MaxCrossingCount() const
{
    return max_crossing_count;
}

/*!
 * @brief The number of crossings in the planar diagram.
 */

Int CrossingCount() const
{
    return crossing_count;
}

/*!
 * @brief Returns the list of crossings in the internally used storage format as a reference to a `Tensor3` object, which is basically a heap-allocated array of dimension 3.
 *
 * The reference is constant because things can go wild  (segfaults, infinite loops) if we allow the user to mess with this data.
 *
 * The `c`-th crossing is stored in `Crossings()(c,i,j)`, `i = 0,1`, `j = 0,1` as follows; note that we defined Booleans `Out = 0`, `In = 0`, `Left = 0`, `Right = 0` for easing the indexing:
 *
 *    Crossings()(c,0,0)                   Crossings()(c,0,1)
 *           ==              O       O            ==
 * Crossings()(c,Out,Left)    ^     ^   Crossings()(c,Out,Right)
 *                             \   /
 *                              \ /
 *                               X
 *                              / \
 *                             /   \
 *    Crossings()(c,1,0)      /     \      Crossings()(c,1,1)
 *           ==              O       O            ==
 *  Crossings()(c,In,Left)               Crossings()(c,In,Right)
 *
 *  Beware that a crossing can have various states, such as `CrossingState_T::LeftHanded()`, `CrossingState_T::RightHanded()`, or `CrossingState_T::Inactive()`. This information is stored in the corresponding entry of `CrossingStates()`.
 */

cref<CrossingContainer_T> Crossings() const
{
    return C_arcs;
}

/*!
 * @brief Returns the states of the crossings.
 *
 *  The states that a crossing can have are:
 *
 *  - `CrossingState_T::RightHanded()`
 *  - `CrossingState_T::LeftHanded()`
 *  - `CrossingState_T::Inactive()`
 *
 * `CrossingState_T::Inactive()` means that the crossing has been deactivated by topological manipulations.
 */

cref<CrossingStateContainer_T> CrossingStates() const
{
    return C_state;
}

/*!@brief Returns the crossing scratch buffer that is used for a couple of algorithms, in particular by transversal routines. Use this with caution as its content depends heavily on which routines have been called before.
*/
cref<Tensor1<Int,Int>> CrossingScratchBuffer() const
{
    return C_scratch;
}

C_Arc_T CopyCrossing( const Int c ) const
{
    return C_Arc_T( C_arcs.data(c) );
}

CrossingState_T CrossingState( const Int c ) const
{
    return C_state[c];
}

/*!
 * @brief Checks whether crossing `c` is still active.
 */

bool CrossingActiveQ( const Int c ) const
{
    return ActiveQ(C_state[c]);
}

std::string CrossingString( const Int c ) const
{
    return "crossing " + Tools::ToString(c) +" = { { "
        + Tools::ToString(C_arcs(c,Out,Left )) + ", "
        + Tools::ToString(C_arcs(c,Out,Right)) + " }, { "
        + Tools::ToString(C_arcs(c,In ,Left )) + ", "
        + Tools::ToString(C_arcs(c,In ,Right)) + " } } ("
        + ToString(C_state[c])      +")";
}


bool CrossingRightHandedQ( const Int c ) const
{
    return RightHandedQ(C_state[c]);
}

bool CrossingLeftHandedQ( const Int c ) const
{
    return LeftHandedQ(C_state[c]);
}

bool OppositeHandednessQ( const Int c_0, const Int c_1 ) const
{
    return OppositeHandednessQ(C_state[c_0],C_state[c_1]);
}

bool SameHandednessQ( const Int c_0, const Int c_1 ) const
{
    return SameHandednessQ(C_state[c_0],C_state[c_1]);
}

Int CountActiveCrossings() const
{
    Int counter = 0;
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        counter += CrossingActiveQ(c);
    }

    return counter;
}

private:
    
/*!
 * @brief Deactivates crossing `c`. Only for internal use.
 */

template<bool assertsQ = true>
void DeactivateCrossing( const Int c )
{
    if( CrossingActiveQ(c) )
    {
        PD_PRINT("Deactivating " + CrossingString(c) + "." );
        --crossing_count;
        C_state[c] = CrossingState_T::Inactive;
    }
    else
    {
#ifdef PD_DEBUG
        if constexpr ( assertsQ )
        {
            wprint(ClassName()+"::Attempted to deactivate already inactive " + CrossingString(c) + ".");
        }
#endif
    }
    
    PD_ASSERT( crossing_count >= Int(0) );
    
    
#ifdef PD_DEBUG
    if constexpr ( assertsQ )
    {
        for( bool io : { In, Out } )
        {
            for( bool side : { Left, Right } )
            {
                const Int a  = C_arcs(c,io,side);
                
                if( ArcActiveQ(a) && (A_cross(a,io) == c) )
                {
                    pd_eprint(ClassName()+"::DeactivateCrossing: active " + ArcString(a) + " is still attached to deactivated " + CrossingString(c) + ".");
                }
            }
        }
    }
#endif
}

public:

/*!
 * @brief Returns the crossing you would get to by starting at crossing `c` and
 * leaving trough the arc specified by `io` and `side`.
 */

Int NextCrossing( const Int c, bool io, bool side ) const
{
    AssertCrossing(c);

    const Int a = C_arcs(c,io,side);

    AssertArc(a);
    
    const Int c_next = A_cross( a, !io );

    AssertCrossing(c_next);
    
    return c_next;
}


private:

// It is in the user's responsibility to correctly connect the arcs to these crossings.

void RotateCrossing( const Int c, const bool dir )
{
    PD_WPRINT( MethodName("RotateCrossing") +": This routine has not been tested, yet." );
/* Before:
 *
 *   C[Out][Left ] O       O C[Out][Right]
 *                  ^     ^
 *                   \   /
 *                    \ /
 *                     X
 *                    / \
 *                   /   \
 *                  /     \
 *   C[In ][Left ] O       O C[In ][Right]
 */
    
    if( dir == Right )
    {
/* After:
 *
 *   C[Out][Left ] O       O C[Out][Right]
 *                  \     ^
 *                   \   /
 *                    \ /
 *                     X
 *                    / \
 *                   /   \
 *                  /     v
 *   C[In ][Left ] O       O C[In ][Right]
 */

        const Int buffer = C_arcs(c,Out,Left );
        
        C_arcs(c,Out,Left ) = C_arcs(c,Out,Right);
        C_arcs(c,Out,Right) = C_arcs(c,In ,Right);
        C_arcs(c,In ,Right) = C_arcs(c,In ,Left );
        C_arcs(c,In ,Left ) = buffer;
    }
    else
    {
/* After:
 *
 *   C[Out][Left ] O       O C[Out][Right]
 *                  ^     /
 *                   \   /
 *                    \ /
 *                     X
 *                    / \
 *                   /   \
 *                  v     \
 *   C[In ][Left ] O       O C[In ][Right]
 */
        
        const Int buffer = C_arcs(c,Out,Left );
        
        C_arcs(c,Out,Left ) = C_arcs(c,In ,Left );
        C_arcs(c,In ,Left ) = C_arcs(c,In ,Right);
        C_arcs(c,In ,Right) = C_arcs(c,Out,Right);
        C_arcs(c,Out,Right) = buffer;
    }
}
