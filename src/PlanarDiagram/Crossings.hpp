public:

CrossingState_T CrossingState( const Int c ) const
{
    return C_state[c];
}

/*! @brief Checks whether crossing `c` is still active.
 */

bool CrossingActiveQ( const Int c ) const
{
    return Knoodle::ActiveQ(C_state[c]);
}

std::string CrossingString( const Int c ) const
{
    return "crossing " + Tools::ToString(c) +" = { { "
        + Tools::ToString(C_arcs(c,Out,Left )) + ", "
        + Tools::ToString(C_arcs(c,Out,Right)) + " }, { "
        + Tools::ToString(C_arcs(c,In ,Left )) + ", "
        + Tools::ToString(C_arcs(c,In ,Right)) + " } } ("
        + Knoodle::ToString(C_state[c])      +")";
}


bool CrossingRightHandedQ( const Int c ) const
{
    return Knoodle::RightHandedQ(C_state[c]);
}

bool CrossingLeftHandedQ( const Int c ) const
{
    return Knoodle::LeftHandedQ(C_state[c]);
}

bool OppositeHandednessQ( const Int c_0, const Int c_1 ) const
{
    return Knoodle::OppositeHandednessQ(C_state[c_0],C_state[c_1]);
}

bool SameHandednessQ( const Int c_0, const Int c_1 ) const
{
    return Knoodle::SameHandednessQ(C_state[c_0],C_state[c_1]);
}

private:

void RotateCrossing( const Int c, const bool dir )
{
//            Int C [2][2];
//            copy_buffer<4>( C_arcs.data(c), &C[0][0] );
    
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

public:

Int CountActiveCrossings() const
{
    Int counter = 0;
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        counter += CrossingActiveQ(c);
    }

    return counter;
}


/*!@brief Returns the crossing scratch buffer that is used for a couple of algorithms, in particular by transversal routines. Use with caution as its content depends heavily on which routines have been called before.
*/
cref<Tensor1<Int,Int>> CrossingScratchBuffer() const
{
    return C_scratch;
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
 * leaving trough the
 */

Int NextCrossing( const Int c, bool io, bool side ) const
{
    AssertCrossing(c);

    const Int a = C_arcs(c,io,side);

    AssertArc(a);
    
//            const Int c_next = A_cross( a, ( io == Out ) ? Head : Tail );
    const Int c_next = A_cross( a, !io );

    AssertCrossing(c_next);
    
    return c_next;
}
