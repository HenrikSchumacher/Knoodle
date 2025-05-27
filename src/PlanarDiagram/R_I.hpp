private:
    
/*!@brief Checks whether arc `a` is active and a loop arc. In the affirmative case it performs a Reidemeister I move and returns `true`. Otherwise, it returns `false`. Only for internal use.
 *
 * @param a The arc to check as possibly, to remove.
 *
 * @tparam checkQ Whether the check for `a` being an active loop is to be performed (`true`). The default is `true`.
 *
 * @tparam warningQ Whether a warning should be issued if arc `a` is not active. The default is `false`.
 */

template<bool checkQ = true, bool warningQ = false>
bool Private_Reidemeister_I( const Int a )
{
    if constexpr ( checkQ )
    {
        if( !ArcActiveQ(a) )
        {
            if constexpr ( warningQ )
            {
                wprint(ClassName()+"::Private_Reidemeister_I: Arc " + ArcString(a) + " is deactivated.");
            }
            return false;
        }
    }
    
    const Int c = A_cross(a,Head);
    
    if constexpr ( checkQ )
    {
        if( A_cross(a,Tail) != c )
        {
            return false;
        }
    }
    
    PD_PRINT( ClassName() + "::Reidemeister_I at " + ArcString(a) );
    
    // We assume here that we already know that a is a loop arc.
    
    PD_ASSERT( A_cross(a,Tail) == c );
    
    //    const bool side = (C_arcs(c,In,Right) == a);
    
    // This allows a 50% chance that we do not have to load `C_arcs(c,In,Left)` again.
    const bool side = (C_arcs(c,In,Left) != a);
    
    const Int a_next = C_arcs(c,Out,!side);
    const Int a_prev = C_arcs(c,In ,!side);
    
    if( a_prev == a_next )
    {
        //
        //             O-----+ a
        //             |     |
        //             |     |
        //       O-----+-----O
        //       |     |c
        //       |     |
        //       +-----O
        //   a_prev = a_next
        
        ++unlink_count;
        DeactivateArc(a);
        DeactivateArc(a_prev);
        DeactivateCrossing(c);
        R_I_counter += 2;
        
        return true;
    }
    
    //             O-----+ a
    //             |     |
    //             |     |
    //       O-----+---->O
    // a_prev      |c
    //             V
    //             O
    //              a_next
    
    Reconnect<Head>(a_prev,a_next);
    DeactivateArc(a);
    DeactivateCrossing(c);
    ++R_I_counter;
    
    AssertArc(a_prev);
    
    return true;
}

public:

/*! @brief Checks whether arc `a` is a loop arc. In the affirmative case it performs a Reidemeister I move, clears the internal cache, and returns `true`. Otherwise, it returns `false` (keeping the internal cache as it was).
 * _Use this with extreme caution as this might invalidate some invariants of the PlanarDiagram class!_ _Never _ use it in productive code unless you really, really know what you are doing! We expose this feature only for debugging purposes and for experiments. If you want to remove many loop arcs, then use `Simplify1`, `Simplify2`,... etc. instead.
 *
 * @param a The arc to check as possibly, to remove.
 *
 * @tparam warningQ Whether a warning should be issued if arc `a` is not active. The default is `true`.
 */

template<bool warningQ = true>
bool Reidemeister_I( const Int a )
{
    bool changedQ = this->template Private_Reidemeister_I<true,true>(a);
    
    if( changedQ )
    {
        this->ClearCache();
    }
    
    return changedQ;
}
