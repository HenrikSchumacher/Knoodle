private:

template<bool headtail, bool deactivateQ = true>
void Reconnect( const Int a, const Int b )
{
#ifdef PD_DEBUG
    std::string tag  (MethodName("Reconnect")+"<" + (headtail ? "Head" : "Tail") +  ", " + BoolString(deactivateQ) + "," ">( " + ArcString(a) + ", " + ArcString(b) + " )" );
#endif
    
    PD_TIMER(timer,tag);
    PD_ASSERT(a != b);
    PD_ASSERT( ArcActiveQ(a) );

    const Int c = A_cross(b,headtail);

    const bool side = (C_arcs(c,headtail,Right) == b);
    
    A_cross(a,headtail) = c;
    C_arcs(c,headtail,side) = a;

    
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
    
    const Int da      = ToDarc(a,headtail);
    const Int da_left = ToDarc(C_arcs(c, side,!headtail),!side);
    const Int da_revr = ToDarc(C_arcs(c,!side, headtail),!side);
    
    DarcLeftDarc(da)       = da_left;
    DarcLeftDarc(da_revr)  = FlipDarc(da);

    if constexpr( deactivateQ )
    {
        DeactivateArc(b);
    }
}
