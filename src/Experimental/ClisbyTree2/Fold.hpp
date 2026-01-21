public:

void ClearWitnesses()
{
    if constexpr ( witnessesQ )
    {
        // Witness checking
        witness_collector.clear();
    }
}

void CollectWitnesses()
{
    if constexpr ( witnessesQ )
    {
        witness_collector.push_back(
            Tiny::Vector<4,Int,Int>({p,q,witness[0],witness[1]})
        );
    }
}

void CollectPivots()
{
    if constexpr ( witnessesQ )
    {
        pivot_collector.push_back(
            std::tuple<Int,Int,Real,bool>({p,q,theta,reflectQ})
        );
    }
}

FoldFlag_T Fold(
    std::pair<Int,Int> && pivots,
    const Real theta_,
    const bool reflectQ_,
    const bool check_collisionsQ,
    const bool check_jointsQ
)
{
    FoldFlag_T pivot_flag = LoadPivots(std::move(pivots),theta_,reflectQ_);
    
    if( pivot_flag != FoldFlag_T::Accepted )
    {
        // Folding step aborted because pivots indices are too close.
        CollectWitnesses();
        return pivot_flag;
    }
    
    if ( check_collisionsQ && check_jointsQ )
    {
        FoldFlag_T joint_flag = CheckJoints();

        if( joint_flag != FoldFlag_T::Accepted )
        {
            // Folding step failed because neighbors of pivot touch.
            CollectWitnesses();
            return joint_flag;
        }
    }
    
    Update();

    if( check_collisionsQ && this->template CollisionQ<false>() )
    {
        // Folding step failed; undo the modifications.
        UndoUpdate();
        CollectWitnesses();
        return FoldFlag_T::RejectedByTree;
    }
    else
    {
        // Folding step succeeded.
        CollectPivots();
        return FoldFlag_T::Accepted;
    }
}

/*!@brief Makes `attempt_count` attempts of random pivot moves.
 */

FoldFlagCounts_T FoldRandom(
    const LInt attempt_count,
    const Real reflectP,
    const bool check_collisionsQ = true,
    const bool check_jointsQ     = true
)
{
    FoldFlagCounts_T flag_ctrs ( LInt(0) );
    ClearWitnesses();
    
    const Real P = Clamp(reflectP,Real(0),Real(1));
    
    for( Int attempt = 0; attempt < attempt_count; ++attempt )
    {
        FoldFlag_T flag = Fold(
            RandomPivots(),
            RandomAngle(),
            RandomReflectionFlag(P),
            check_collisionsQ,
            check_jointsQ
        );
        
        ++flag_ctrs[ToUnderlying(flag)];
    }
    
    return flag_ctrs;
}

/*!@brief Performs random pivot moves until `accept_count` steps were accepted.
 */

FoldFlagCounts_T FoldRandomUntil(
    const LInt accept_count,
    const Real reflectP,
    const bool check_collisionsQ = true,
    const bool check_jointsQ     = true
)
{
    FoldFlagCounts_T flag_ctrs ( LInt(0) );
    ClearWitnesses();
    
    const Real P = Clamp(reflectP,Real(0),Real(1));
    
    while( flag_ctrs[0] < accept_count )
    {
        FoldFlag_T flag = Fold(
            RandomPivots(),
            RandomAngle(),
            RandomReflectionFlag(P),
            check_collisionsQ,
            check_jointsQ
        );
        
        ++flag_ctrs[ToUnderlying(flag)];
    }
    
    return flag_ctrs;
}

/*!@brief Creates a string to present `FoldFlagCounts_T` in human-readable form.
 */

std::string FoldFlagCounts_ToString( cref<FoldFlagCounts_T> counts )
{
    std::string s;
    
    const LInt accepted = counts[ ToUnderlying(FoldFlag_T::Accepted)];
    const LInt total    = counts.Total();
    const LInt rejected = total - accepted;

    s += "Fold flag counts: \n";
    
    double value = Percentage<double>(accepted,total);
    s += "\t accepted:                  " + Tools::ToString(accepted) + " (" + std::format("{:.4g}",value) + " %)\n";
    
    value = Percentage<double>(rejected,total);
    s += "\t rejected:                  " + Tools::ToString(rejected) + " (" + std::format("{:.4g}",value) + " %)\n";
    s += "\t total:                     " + Tools::ToString(total)+ "\n";
    s += "Rejection reasons: \n";
    s += "\t pivots invalid:            " + Tools::ToString(counts[ToUnderlying(FoldFlag_T::RejectedByPivots)]) + "\n";
    s += "\t collision at first joint:  " + Tools::ToString(counts[ToUnderlying(FoldFlag_T::RejectedByJoint0)]) + "\n";
    s += "\t collision at second joint: " + Tools::ToString(counts[ToUnderlying(FoldFlag_T::RejectedByJoint1)]) + "\n";
    s += "\t other collisions:          " + Tools::ToString(counts[ToUnderlying(FoldFlag_T::RejectedByTree)]) + "\n";
    return s;
}
