public:

FoldFlag_T Fold(
    const Int p_,
    const Int q_,
    const Real theta_,
    const bool reflectQ_,
    const bool check_collisionsQ
)
{
    int pivot_flag = LoadPivots(p_,q_,theta_,reflectQ_);
    
    if( pivot_flag != 0 )
    {
        // Folding step aborted because pivots indices are too close.
        return pivot_flag;
    }
    
    if ( check_collisionsQ )
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

    if( check_collisionsQ && CollisionQ() )
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
    
template<bool only_oddQ = false>
std::pair<Int,Int> RandomPivots()
{
    const Int n = VertexCount();
    
    using unif_int = std::uniform_int_distribution<Int>;
    
    unif_int u_int ( Int(0), (only_oddQ ? n/2 : n) - Int(1) );
    
    Int i;
    Int j;
    
    do
    {
        if constexpr ( only_oddQ )
        {
            i = Int(2) * u_int(random_engine) + 1;
            j = Int(2) * u_int(random_engine) + 1;
        }
        else
        {
            i = u_int(random_engine);
            j = u_int(random_engine);
        }
    }
    while( ModDistance(n,i,j) < Int(2) );
    
    return MinMax(i,j);
}


template<bool only_oddQ = false>
FoldFlagCounts_T FoldRandom(
    const LInt success_count,
    const Real reflectP,
    const bool check_collisionsQ = true
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
        
        auto [i,j] = RandomPivots<only_oddQ>();
        
        bool reflectQ_ = false;
        
        if ( reflectP > Real(0) )
        {
            reflectQ_ = (unif_prob( random_engine ) <= P);
        }
        
        FoldFlag_T flag = Fold(i,j,angle,reflectQ_,check_collisionsQ);
        
        ++counters[flag];
    }
    
    return counters;
}



//####################################################################################
//####          From here on it gets a bit experimental
//####################################################################################

std::pair<Int,Int> SubtreeRandomPivots( Int root_0, Int root_1 )
{
    const Int n = VertexCount();
    
    using unif_int = std::uniform_int_distribution<Int>;
    
    unif_int u_int_0 ( NodeBegin(root_0), NodeEnd(root_0) - Int(1) );
    unif_int u_int_1 ( NodeBegin(root_1), NodeEnd(root_1) - Int(1) );
    
    Int pivot_0;
    Int pivot_1;
    
    do
    {
        pivot_0 = u_int_0(random_engine);
        pivot_1 = u_int_1(random_engine);
    }
    while( ModDistance(n,pivot_0,pivot_1) < Int(2) ); // distance check is redundant
    
    return MinMax(pivot_0,pivot_1); // Is redudant if root_0 < root_1 and if the subtrees are disjoint.
}


FoldFlag_T SubtreeFold(
    const Int root_0, const Int pivot_0,
    const Int root_1, const Int pivot_1,
    const Real theta_,
    const bool reflectQ_,
    const bool check_collisionsQ
)
{
    int pivot_flag = LoadPivots(pivot_0,pivot_1,theta_,reflectQ_);
    
    if( pivot_flag != 0 )
    {
        // Folding step aborted because pivots indices are too close.
        return pivot_flag;
    }
    
    if ( check_collisionsQ )
    {
        int joint_flag = CheckJoints();

        if( joint_flag != 0 )
        {
            if constexpr ( witnessesQ )
            {
                // Witness checking
                witness_collector.push_back(
                    Tiny::Vector<4,Int,Int>({pivot_0,pivot_1,witness[0],witness[1]})
                );
            }
            
            // Folding step failed because neighbors of pivot touch.
            return joint_flag;
        }
    }
    
    Update(root_0,root_1);

    if( check_collisionsQ && CollisionQ() )
    {
        // Folding step failed; undo the modifications.
        
        UndoUpdate(root_0,root_1);

        if constexpr ( witnessesQ )
        {
            // Witness checking
            witness_collector.push_back(
                Tiny::Vector<4,Int,Int>({pivot_0,pivot_1,witness[0],witness[1]})
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
                std::tuple<Int,Int,Real,bool>({pivot_0,pivot_1,theta,reflectQ})
            );
        }
        
        // Folding step succeeded.
        return 0;
    }
}

FoldFlagCounts_T SubtreeFoldRandom(
    const Int root_0, const Int root_1,
    const LInt success_count,
    const Real reflectP,
    const bool check_collisionsQ = true
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
        
        auto [p_0,p_1] = SubtreeRandomPivots(root_0,root_1);
        
        bool reflectQ_ = false;
        
        if ( reflectP > Real(0) )
        {
            reflectQ_ = (unif_prob( random_engine ) <= P);
        }
        
        FoldFlag_T flag = SubtreeFold(
            root_0, p_0,
            root_1, p_1,
            angle, reflectQ_, check_collisionsQ
        );
        
        ++counters[flag];
    }
    
    return counters;
}
