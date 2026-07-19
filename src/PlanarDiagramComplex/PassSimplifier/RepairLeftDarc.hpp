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
            // TODO: We might be able to optimize this a little.
            
            const Int da_l = pd->LeftDarc(da);
            if( LeftDarc(da) != da_l )
            {
                PD_PRINT("LeftDarc(" + ToString(da) + ") is indeed outdated.");
            }
            else
            {
                PD_PRINT("LeftDarc(" + ToString(da) + ") is up to date.");
            }
            
            const Int da_r = ReverseDarc(pd->RightDarc(da));
            
            if( LeftDarc(da_r) != da )
            {
                PD_PRINT("LeftDarc(" + ToString(da_r) + ") is indeed outdated.");
            }
            else
            {
                PD_PRINT("LeftDarc(" + ToString(da_r) + ") is up to date.");
            }
            
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
