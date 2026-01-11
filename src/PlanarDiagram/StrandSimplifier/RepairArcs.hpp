private:

    
void RepairArcLeftArc( const Int da )
{
    auto [a,dir ] = PD_T::FromDarc(da);
    
    if( pd.ArcActiveQ(a) )
    {
        const Int da_l = pd.LeftDarc(da);
        const Int da_r = FlipDarc(pd.RightDarc(da));
        
//                PD_DPRINT("RepairArcLeftArc touched a   = " + ArcString(a) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_l = " + ArcString(a_l) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_r = " + ArcString(a_r) + ".");
        
        dA_left[da]   = da_l;
        dA_left[da_r] = FlipDarc(da);
    }
}

//        void RepairArcLeftArcs()
//        {
//            TOOLS_PTIMER(timer,ClassName()+"::RepairArcLeftArcs");
//
//#ifdef PD_TIMINGQ
//            const Time start_time = Clock::now();
//#endif
//
//            for( Int da : touched )
//            {
//                RepairArcLeftArc(da);
//            }
//
//            PD_ASSERT(CheckArcLeftArcs());
//
//            touched.clear();
//
//#ifdef PD_TIMINGQ
//            const Time stop_time = Clock::now();
//
//            Time_RepairArcLeftArcs += Tools::Duration(start_time,stop_time);
//#endif
//        }
//
//        template<bool headtail>
//        void TouchArc( const Int a )
//        {
//            const Int da = ToDarc(a,headtail);
//
//            touched.push_back(da);
//            touched.push_back(FlipDarc(pd.NextRightArc(da)));
//        }
//
//        void TouchArc( const Int a, const bool headtail )
//        {
//            if( headtail)
//            {
//                TouchArc<Head>(a);
//            }
//            else
//            {
//                TouchArc<Tail>(a);
//            }
//        }

