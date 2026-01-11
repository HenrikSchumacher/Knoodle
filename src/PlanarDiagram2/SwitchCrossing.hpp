public:

/*! @brief Checks whether crossing `c` is active. In the affirmative case it flips the handedness of that crossing, clears the internal cache, and returns `true`. Otherwise, this routine just returns `false` (keeping the internal cache as it was).
 * _Use this with extreme caution as this might invalidate some invariants of the PlanarDiagram class._ _Never _ use it in productive code unless you really, really know what you are doing! This feature is highly experimental and we expose it only for debugging purposes and for experiments.
 *
 * @param c The crossing to be switched.
 *
 * @tparam warningQ If set to `true`, then a warning is issued whenever `c` is deactivated. Default is `true`.
 */

template<bool warningQ = true>
bool SwitchCrossing( const Int c )
{
    bool changedQ = this->template Private_SwitchCrossing<warningQ>(c);
    
    if( changedQ )
    {
        this->ClearCache();
    }
    
    return changedQ;
}


private:

/*! @brief Checks whether crossing `c` is active. In the affirmative case it flips the handedness of that crossing and returns `true`. Otherwise, this routine just returns `false` (keeping the internal cache as it was).
 *
 * @param c The crossing to be switched.
 *
 * @tparam warningQ If set to `true`, then a warning is issued whenever `c` is deactivated. Default is `false`.
 */

template<bool warningQ = false>
bool Private_SwitchCrossing( const Int c )
{
    if( !ActiveQ(C_state[c]) )
    {
        if constexpr ( warningQ )
        {
            wprint(MethodName("Private_SwitchCrossing")+": Crossing " + CrossingString(c) + " was already deactivated. Doing nothing.");
        }
        return false;
    }
    else
    {
        C_state[c] = Switch(C_state[c]);
        return true;
    }
}

