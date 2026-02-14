private:

    
template<typename dummy = void>
void RepairLeftDarc( const Int da )
{
    if constexpr (lutQ )
    {
        PD_PRINT("RepairLeftDarc(" + ToString(da) +  ")");
        auto [a,dir ] = PD_T::FromDarc(da);
        
        if( pd->ArcActiveQ(a) )
        {
            const Int da_l = pd->LeftDarc(da);
            // TODO: We might be able to optimize this a little.
            const Int da_r = ReverseDarc(pd->RightDarc(da));
            
            //                PD_DPRINT("RepairLeftDarc touched a   = " + ArcString(a) + ".");
            //                PD_DPRINT("RepairLeftDarc touched a_l = " + ArcString(a_l) + ".");
            //                PD_DPRINT("RepairLeftDarc touched a_r = " + ArcString(a_r) + ".");
            
            SetLeftDarc(da  ,da_l        );
            SetLeftDarc(da_r,ReverseDarc(da));
        }
    }
    else
    {
        static_assert(DependentFalse<dummy>,"We should never call this function if lutQ == false.");
        (void)da;
    }
}
