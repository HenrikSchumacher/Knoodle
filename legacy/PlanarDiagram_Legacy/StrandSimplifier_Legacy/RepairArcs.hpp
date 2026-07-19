private:

    
void RepairDarcLeftDarc( const Int da )
{
    PD_PRINT("RepairDarcLeftDarc(" + ToString(da) +  ")");
    auto [a,dir ] = PD_T::FromDarc(da);
    
    if( pd.ArcActiveQ(a) )
    {
        const Int da_l = pd.LeftDarc(da);
        const Int da_r = ReverseDarc(pd.RightDarc(da));
        
//                PD_DPRINT("RepairArcLeftArc touched a   = " + ArcString(a) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_l = " + ArcString(a_l) + ".");
//                PD_DPRINT("RepairArcLeftArc touched a_r = " + ArcString(a_r) + ".");
        
        SetDarcLeftDarc(da, da_l);
        SetDarcLeftDarc(da_r, ReverseDarc(da));
    }
}
