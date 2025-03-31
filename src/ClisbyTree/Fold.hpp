public:

FoldFlag_T Fold(
    const Int p_,
    const Int q_,
    const Real theta_,
    const bool reflectQ_,
    const bool check_overlapsQ
)
{
    int pivot_flag = LoadPivots(p_,q_,theta_,reflectQ_);
    
    if( pivot_flag != 0 )
    {
        // Folding step aborted because pivots indices are too close.
        return pivot_flag;
    }
    
    if ( check_overlapsQ )
    {
        int joint_flag = CheckJoints();

        if( joint_flag != 0 )
        {
            if constexpr ( witnessesQ )
            {
                // Witness checking
                witness_collector.push_back(
                    Tiny::Vector<4,Int,Int>({p,q,witness[0],witness[1]})
                );
            }
            
            // Folding step failed because neighbors of pivot touch.
            return joint_flag;
        }
    }
    
    Update();

    if( check_overlapsQ && OverlapQ() )
    {
        // Folding step failed; undo the modifications.
        UndoUpdate();
        
        if constexpr ( witnessesQ )
        {
            // Witness checking
            witness_collector.push_back(
                Tiny::Vector<4,Int,Int>({p,q,witness[0],witness[1]})
            );
        }
        return 4;
    }
    else
    {
        if constexpr ( witnessesQ )
        {
            // Witness checking
            pivot_collector.push_back(
                std::tuple<Int,Int,Real,bool>({p,q,theta,reflectQ})
            );
        }
        
        // Folding step succeeded.
        return 0;
    }
}
    
    
//// Defect routine!!! Does not sample uniformly!
//std::pair<Int,Int> RandomPivots_Legacy()
//{
//    using unif_int  = std::uniform_int_distribution<Int>;
//
//    const Int n = VertexCount();
//
//    unif_int  u_int ( Int(0), n-3 );
//
//    const Int i = u_int                        (random_engine);
//    const Int j = unif_int(i+2,n-1-(i==Int(0)))(random_engine);
//
//    return std::pair<Int,Int>(i,j);
//}
    
std::pair<Int,Int> RandomPivots()
{
    const Int n = VertexCount();
    
    using unif_int = std::uniform_int_distribution<Int>;
    
    unif_int u_int ( Int(0), n - Int(1) );
    
    Int i = u_int(random_engine);
    Int j = u_int(random_engine);

    while( ModDistance(n,i,j) < 2 )
    {
        i = u_int(random_engine);
        j = u_int(random_engine);
    }
    
    return MinMax(i,j);
}

FoldFlagCounts_T FoldRandom(
    const LInt success_count,
    const Real reflectP,
    const bool check_overlapsQ = true
)
{
    FoldFlagCounts_T counters;
    
    counters.SetZero();
    
    using unif_real = std::uniform_real_distribution<Real>;
    
    unif_real unif_angle (- Scalar::Pi<Real>,Scalar::Pi<Real> );
    unif_real unif_prob ( Real(0), Real(1) );
    
    if constexpr ( witnessesQ )
    {
        // Witness checking
        witness_collector.clear();
    }
    
    const Real P = Clamp(reflectP,Real(0),Real(1));
    
    while( counters[0] < success_count )
    {
        const Real angle = unif_angle(random_engine);
        
        auto [i,j] = RandomPivots();
        
        bool reflectQ_ = false;
        
        if ( reflectP > Real(0) )
        {
            reflectQ_ = (unif_prob( random_engine ) <= P);
        }
        
        FoldFlag_T flag = Fold(i,j,angle,reflectQ_,check_overlapsQ);
        
        ++counters[flag];
    }
    
    return counters;
}
