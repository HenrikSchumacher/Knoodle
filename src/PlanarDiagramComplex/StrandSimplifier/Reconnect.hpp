private:

template<bool headtail, bool deactivateQ = true>
void Reconnect( const Int a, const Int b )
{
#ifdef PD_DEBUG
    std::string tag  (MethodName("Reconnect")+"<" + (headtail ? "Head" : "Tail") +  ", " + BoolString(deactivateQ) + ">( " + ArcString(a) + ", " + ArcString(b) + " )" );
#endif
    
    PD_TIMER(timer,tag);
    PD_ASSERT(a != b);
    PD_ASSERT( ArcActiveQ(a) );
    
#ifdef PD_DEBUG
    if( A_color[a] != A_color[b] )
    {
        wprint(MethodName("Reconnect")+": Attempting to reconnect arcs of different colors.");
        TOOLS_LOGDUMP(ArcString(a));
        TOOLS_LOGDUMP(ArcString(b));
    }
#endif
    
    const Int c = A_cross(b,headtail);

    const bool side = (C_arcs(c,headtail,Right) == b);

    /*! If headtail == Head && side == Right
     *
     *         ^       ^
     *          \     /
     *           \   /
     *            \ /
     *             X <----c
     *            ^ ^
     *           /   \  a
     *          /     \
     *         /       \
     *
     * da_left  = ToDarc(C_arcs(c,In ,Left ),Tail)
     * da_revr  = ToDarc(C_arcs(c,Out,Right),Tail)
     */
    
    /*! If headtail == Head && side == Left
     *
     *         ^       ^
     *          \     /
     *           \   /
     *            \ /
     *             X <----c
     *            ^ ^
     *        a  /   \
     *          /     \
     *         /       \
     *
     * da_left = ToDarc(C_arcs(c,Out,Left ),Head)
     * da_revr = ToDarc(C_arcs(c,In ,Right),Head)
     */
    
    /*! If headtail == Tail && side == Right
     *
     *         ^       ^
     *          \     /
     *           \   / a
     *            \ /
     *             X <----c
     *            ^ ^
     *           /   \
     *          /     \
     *         /       \
     *
     * da_left = ToDarc(C_arcs(c,In ,Right),Tail)
     * da_revr = ToDarc(C_arcs(c,Out,Left ),Tail)
     */
    
    /*! If headtail == Tail && side == Left
     *
     *         ^       ^
     *          \     /
     *        a  \   /
     *            \ /
     *             X <----c
     *            ^ ^
     *           /   \
     *          /     \
     *         /       \
     *
     * da_left = ToDarc(C_arcs(c,Out,Right),Head)
     * da_revr = ToDarc(C_arcs(c,In ,Left ),Head)
     */
    
    A_cross(a,headtail) = c;
    C_arcs(c,headtail,side) = a;

    const Int da      = ToDarc(a,headtail);
    const Int da_left = ToDarc(C_arcs(c, side,!headtail),!side);
    const Int da_revr = ToDarc(C_arcs(c,!side, headtail),!side);
    
    dA_left[da]       = da_left;
    dA_left[da_revr]  = FlipDarc(da);

    if constexpr( deactivateQ )
    {
        DeactivateArc(b);
    }
}
