private:

/*! @brief Checks whether crossing `c` is active. In the affirmative case it to resolves the crossing in an orientation-preserving way by (i) reconnecting the two incoming arcs to the corresponding tips of the outgoing arcs, (ii) deactivating the old outgoing arcs, and (iii) deactivating the crossing `c`; the return value will be `true`. Otherwise, this routine just returns `false` (keeping the internal cache as it was).
 *
 * @param c The crossing to be resolved.
 *
 * @tparam warningQ If set to `true`, then a warning is issued whenever `c` is deactivated. Default is `false`.
 */

template<bool warningQ = false>
bool Private_ResolveCrossing( const Int c )
{
    PD_PRINT(ClassName()+"::Private_ResolveCrossing");
    PD_VALPRINT("c",c);
    
    PD_ASSERT(CrossingActiveQ(c));
    
    if( CrossingActiveQ(c) )
    {

/*        ###O----<----O       O---->----O###
 *               e_0    ^     ^    e_1
 *                       \   /
 *                        \ /
 *                       c X
 *                        / \
 *                       /   \
 *               e_2    /     \    e_3
 *        ###O---->----O       O----<----O###
*/
        const Int e_0 = C_arcs(c,Out,Left );
        const Int e_1 = C_arcs(c,Out,Right);
        const Int e_2 = C_arcs(c,In ,Left );
        const Int e_3 = C_arcs(c,In ,Right);
        
        if( e_0 == e_2 )
        {
            ++unlink_count;
            DeactivateArc(e_0);
        }
        else
        {
            Reconnect<Head>(e_2,e_0);
        }
        
        if( e_1 == e_3 )
        {
            ++unlink_count;
            DeactivateArc(e_1);
        }
        else
        {
            Reconnect<Head>(e_3,e_1);
        }
        
        DeactivateCrossing(c);
        
        /* If e_0 != e_2 and e_1 != e_3, then this should look like this now:
        //
        //        ###O----<----+       +---->----O###
        //                     |       |
        //                     |       |
        //                     |       |
        //                 e_0 ^       ^ e_1
        //                     |       |
        //                     |       |
        //                     |       |
        //        ###O---->----+       +----<----O###
        //
        // In the other case, up to two unlinks might have been created.
        */
        
        return true;
    }
    else
    {
        if constexpr ( warningQ )
        {
            wprint(ClassName()+"::Private_ResolveCrossing: Crossing " + CrossingString(c) + " was already deactivated.");
        }
        return false;
    }
}


public:

/*! @brief Checks whether crossing `c` is active. In the affirmative case it to resolves the crossing in an orientation-preserving way by (i) reconnecting the two incoming arcs to the corresponding tips of the outgoing arcs, (ii) deactivating the old outgoing arcs, (iii) deactivating the crossing `c`, and (iv)  clearing the internal cache; the return value will be `true`. Otherwise, this routine just returns `false` (keeping the internal cache as it was).
 * _Use this with extreme caution as this might invalidate some invariants of the PlanarDiagram class._ _Never _ use it in productive code unless you really, really know what you are doing! This feature is highly experimental and we expose it only for debugging purposes and for experiments.
 *
 * @param c The crossing to be resolved.
 *
 * @tparam warningQ If set to `true`, then a warning is issued whenever `c` is deactivated.  Default is `true`.
 */

template<bool warningQ = true>
bool ResolveCrossing( const Int c )
{
    bool changedQ = this->template Private_ResolveCrossing<warningQ>(c);
    
    if( changedQ )
    {
        this->ClearCache();
    }
    
    return changedQ;
}
