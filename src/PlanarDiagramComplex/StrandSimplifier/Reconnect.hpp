private:

template<bool headtail, bool deactivateQ = true>
void Reconnect( const Int a, const Int b )
{
#ifdef PD_DEBUG
    std::string tag  (ClassName()+"::Reconnect<" + (headtail ? "Head" : "Tail") +  ", " + BoolString(deactivateQ) + "," ">( " + ArcString(a) + ", " + ArcString(b) + " )" );
#endif
    
    PD_TIC(tag);
    PD_ASSERT( pd.ArcActiveQ(a) );
    
    const Int  c    = A_cross(b,headtail);
    const bool side = ArcSide(b,headtail);
    
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
        pd.DeactivateArc(b);
    }

    PD_TOC(tag);
}


/*!@brief We use this to indicate that the darc `da` has been modified.
 */
void TouchDarc( const Int da )
{
    touched.push_back(da);
    touched.push_back(PD_T::FlipDarc(pd.RightArc(da)));
}


/*!@brief We use this to indicate that the darc `2 * a + headtail` has been modified.
 */

template<bool headtail>
void TouchArc( const Int a )
{
    const Int da = PD_T::ToDarc(a,headtail);
    
    touched.push_back(da);
    touched.push_back(PD_T::FlipDarc(pd.NextRightArc(da)));
}

    
void RepairArcLeftArc( const Int da )
{
    auto [a,dir ] = PD_T::FromDarc(da);
    
    if( pd.ArcActiveQ(a) )
    {
        const Int da_l = pd.LeftDarc(da);
        const Int da_r = PD_T::FlipDarc(pd.RightDarc(da));
        
//                PD_DPRINT("RepairArcLeftArc touched a   = " + ArcString(a) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_l = " + ArcString(a_l) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_r = " + ArcString(a_r) + ".");
        
        dA_left[da]   = da_l;
        dA_left[da_r] = PD_T::FlipDarc(da);
    }
}

void RepairArcLeftArcs()
{
    PD_TIMER(timer,MethodName("RepairArcLeftArcs"));
    
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    for( Int da : touched )
    {
        RepairArcLeftArc(da);
    }
    
    PD_ASSERT(CheckDarcLeftDarc());
    
    touched.clear();

#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_RepairArcLeftArcs += Tools::Duration(start_time,stop_time);
#endif
}
