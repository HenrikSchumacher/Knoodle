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


//###########################################################
//####          From here on it gets a bit experimental
//###########################################################

//std::pair<Int,Int> SubtreeRandomPivots( Int start_node )
//{
//    const Int n = VertexCount();
//    
//    unif_int u_int ( NodeBegin(start_node), NodeEnd(start_node) - Int(1) );
//    
//    Int pivot_0;
//    Int pivot_1;
//    
//    do
//    {
//        pivot_0 = u_int(random_engine);
//        pivot_1 = u_int(random_engine);
//    }
//    while( ModDistance(n,pivot_0,pivot_1) < Int(2) ); // distance check is redundant
//    
//    return MinMax(pivot_0,pivot_1); // Is redudant if root_0 < root_1 and if the subtrees are disjoint.
//}


template<bool pull_transformsQ = true>
FoldFlag_T SubtreeFold(
    const Int start_node,
    std::pair<Int,Int> && pivots,
    const Real theta_,
    const bool reflectQ_,
    const bool check_collisionsQ,
    const bool check_jointsQ
)
{
    int pivot_flag = LoadPivots(std::move(pivots),theta_,reflectQ_);
    
    if( pivot_flag != 0 )
    {
        // Folding step aborted because pivots indices are too close.
        CollectWitnesses();
        return pivot_flag;
    }
    
    if ( check_collisionsQ && check_jointsQ )
    {
        int joint_flag = CheckJoints();

        if( joint_flag != 0 )
        {
            // Folding step failed because neighbors of pivot touch.
            CollectWitnesses();
            return joint_flag;
        }
    }

    this->template Update<pull_transformsQ>(start_node);

    if( check_collisionsQ && this->template CollisionQ<false>() )
    {
        // Folding step failed; undo the modifications.
        // TODO: Here it should be safe to use pull_transformsQ = false?
        this->template UndoUpdate<pull_transformsQ>(start_node);
        CollectWitnesses();
        return 4;
    }
    else
    {
        // Folding step succeeded.
        CollectPivots();
        return 0;
    }
}





template<bool splitQ>
std::pair<Int,Int> RandomPivots( const Int begin, const Int mid, const Int end )
{
    const Int n = VertexCount();
    
    if constexpr ( splitQ )
    {
        assert( (begin  <  mid  ) );
        assert( (mid    <  end  ) );
        assert( (Int(0) <= begin) );
        assert( (end    <= n    ) );
        
        Int i;
        Int j;
        
        do
        {
            i = RandomInteger(begin,mid);
            j = RandomInteger(mid  ,end);
        }
        while( ModDistance(n,i,j) <= Int(1) );
        
        return std::pair<Int,Int>(i,j);
    }
    else
    {
        return this->RandomPivots_Box(begin,end);
    }
}

/*!@brief Makes `attempt_count` attempts of random pivot moves restricted to the subtree with root `start_node`.
 */

template<bool pull_transformsQ = true, bool splitQ = false>
void SubtreeFoldRandom(
    const Int start_node,
    mref<FoldFlagCounts_T> flag_ctr,
    const LInt attempt_count,
    const Real reflectP,
    const bool check_collisionsQ,
    const bool check_jointsQ
)
{
    const Real P = Clamp(reflectP,Real(0),Real(1));
    
    if constexpr ( pull_transformsQ )
    {
        PullTransforms(Root(),start_node);
    }
    
    const Int begin = NodeBegin(start_node);
    const Int mid   = NodeBegin(RightChild(start_node));
    const Int end   = NodeEnd  (start_node);
    
    for( Int attempt = 0; attempt < attempt_count; ++attempt )
    {
        FoldFlag_T flag = this->template SubtreeFold<false>(
            start_node,
            RandomPivots<splitQ>(begin,mid,end),
            RandomAngle(),
            RandomReflectionFlag(P),
            check_collisionsQ,
            check_jointsQ
        );
        
        ++flag_ctr[flag];
    }
}


// Samples i uniformly in [a,b[ and j uniformly in [c,d[.
std::pair<Int,Int> RandomPivots(
    const Int a, const Int b, const Int c, const Int d
)
{
    const Int n = VertexCount();
    
    assert( a <  b );
    assert( b <= c );
    assert( c <  d );
    
    int_unif u_int_i ( a, b - Int(1) );
    int_unif u_int_j ( c, d - Int(1) );
    
    Int i;
    Int j;
    
    do
    {
        i = u_int_i(random_engine);
        j = u_int_j(random_engine);
    }
    while( ModDistance(n,i,j) <= Int(1) );
    
    return MinMax(i,j);
}



/*!@brief Makes `attempt_count` attempts of random pivot moves restricted to the pivots in the subtrees with roots `root_0` and `root_1`.
 */

void RectangleFoldRandom(
    const Int root_0, const Int root_1,
    mref<FoldFlagCounts_T> flag_ctr,
    const LInt attempt_count,
    const Real reflectP,
    const bool check_collisionsQ = true,
    const bool check_jointsQ = false
)
{
    const Real P = Clamp(reflectP,Real(0),Real(1));
    
    const Int a = NodeBegin(root_0);
    const Int b = NodeEnd  (root_0);
    const Int c = NodeBegin(root_1);
    const Int d = NodeEnd  (root_1);
    
    for( Int attempt = 0; attempt < attempt_count; ++attempt )
    {
        auto pivots = RandomPivots(a,b,c,d);
        
        FoldFlag_T flag = Fold(
            std::move(pivots),
            RandomAngle(),
            RandomReflectionFlag(P),
            check_collisionsQ,
            check_jointsQ
        );
        
        ++flag_ctr[flag];
    }
}
