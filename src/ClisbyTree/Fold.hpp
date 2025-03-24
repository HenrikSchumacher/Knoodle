public:

template<bool allow_reflectionsQ, bool check_overlapsQ = true>
FoldFlag_T Fold( const Int p_, const Int q_, const Real theta_, const bool reflectQ_ )
{
    int pivot_flag = LoadPivots<allow_reflectionsQ>(p_,q_,theta_,reflectQ_);
    
    if( pivot_flag != 0 )
    {
        // Folding step aborted because pivots indices are too close.
        return pivot_flag;
    }
    
    if constexpr ( check_overlapsQ )
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

    if constexpr ( check_overlapsQ )
    {
        if( OverlapQ() )
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
    else
    {
        if constexpr ( witnessesQ )
        {
            // Witness checking
            pivot_collector.push_back(
                std::tuple<Int,Int,Real,bool>({p,q,theta,reflectQ})
            );
        }
        
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


template<bool allow_reflectionsQ, bool check_overlapsQ = true>
FoldFlagCounts_T FoldRandom( const LInt success_count )
{
    FoldFlagCounts_T counters;
    
    counters.SetZero();
    
    using unif_real = std::uniform_real_distribution<Real>;
    
    unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
    
    if constexpr ( witnessesQ )
    {
        // Witness checking
        witness_collector.clear();
    }
    
    while( counters[0] < success_count )
    {
        const Real angle = u_real(random_engine);
        
        auto [i,j] = RandomPivots();
        
        bool mirror_bit = false;
        
        if constexpr ( allow_reflectionsQ )
        {
            using unif_bool = std::uniform_int_distribution<int>;

            mirror_bit = unif_bool(0,1)(random_engine);
        }
        
        FoldFlag_T flag = Fold<allow_reflectionsQ,check_overlapsQ>(i,j,angle,mirror_bit);
        
        ++counters[flag];
    }
    
    return counters;
}
