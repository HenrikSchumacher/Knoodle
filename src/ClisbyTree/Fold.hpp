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
    
    Update();

    if( check_collisionsQ && CollisionQ() )
    {
        // Folding step failed; undo the modifications.
        UndoUpdate();
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

/*!@brief Makes `attempt_count` attempts of random pivot moves.
 */

FoldFlagCounts_T FoldRandom(
    const LInt attempt_count,
    const Real reflectP,
    const bool check_collisionsQ = true,
    const bool check_jointsQ = false
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
        
        ++flag_ctrs[flag];
    }
    
    return flag_ctrs;
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

    if( check_collisionsQ && CollisionQ() )
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
    
    int_unif int_u_i ( a, b - Int(1) );
    int_unif int_u_j ( c, d - Int(1) );
    
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
