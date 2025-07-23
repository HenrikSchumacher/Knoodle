public:

using unif_int = std::uniform_int_distribution<Int>;
using unif_real = std::uniform_real_distribution<Real>;

private:

unif_real unif_angle { -Scalar::Pi<Real>,Scalar::Pi<Real> };
unif_real unif_prob  { Real(0), Real(1) };

public:

// Generates a random integer in [a,b[.
Int RandomInteger( const Int a, const Int b )
{
    return unif_int(a,b)(random_engine);
}

Real RandomAngle()
{
    return unif_angle(random_engine);
}

bool RandomReflectionFlag( Real P )
{
    if ( P > Real(0) )
    {
        return (unif_prob(random_engine) <= P);
    }
    else
    {
        return false;
    }
}

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
    
    
//// Defect routine!!! Does not sample uniformly!
//std::pair<Int,Int> RandomPivots_Legacy()
//{
//    const Int n = VertexCount();
//
//    unif_int  u_int ( Int(0), n-3 );
//
//    const Int i = u_int                        (random_engine);
//    const Int j = unif_int(i+2,n-1-(i==Int(0)))(random_engine);
//
//    return std::pair<Int,Int>(i,j);
//}
    
//template<bool only_oddQ = false>
//std::pair<Int,Int> RandomPivots()
//{
//    const Int n = VertexCount();
//    
//    Int i;
//    Int j;
//    
//    do
//    {
//        if constexpr ( only_oddQ )
//        {
//            i = Int(2) * RandomInteger(Int(0),n/2) + Int(1);
//            j = Int(2) * RandomInteger(Int(0),n/2) + Int(1);
//        }
//        
//        else
//        {
//            i = RandomInteger(Int(0),n);
//            j = RandomInteger(Int(0),n);
//        }
//    }
//    while( ModDistance(n,i,j) < Int(2) );
//    
//    return MinMax(i,j);
//}


// Chosse i, j randomly in [begin,end[ so that circular distance of i and j is greater than 1.
// Also, i < j is guaranteed.
template<bool only_oddQ = false>
std::pair<Int,Int> RandomPivots( Int begin, Int end )
{
    const Int n = VertexCount();
    
    assert( (begin + Int(2) < end) );
    assert( Int(0) <= begin );
    assert( end <= n );
    
    Int i;
    Int j;
    do
    {
        Int i_ = RandomInteger( begin, end - Int(1) );
        Int j_ = RandomInteger( begin, end - Int(2) );
        
        if( i_ > j_ )
        {
            i = j_;
            j = i_ + Int(1);

        }
        else
        {
            i = i_;
            j = j_ + Int(2);
        }
    }
    while( ModDistance(n,i,j) <= Int(1) );

    return std::pair<Int,Int>( i, j );
}



template<bool only_oddQ = false>
FoldFlagCounts_T FoldRandom(
    const LInt accept_count,
    const Real reflectP,
    const bool check_collisionsQ = true,
    const bool check_jointsQ = false
)
{
    FoldFlagCounts_T flat_ctrs ( LInt(0) );
    ClearWitnesses();
    
    const Real P = Clamp(reflectP,Real(0),Real(1));
    
    while( flat_ctrs[0] < accept_count )
    {
        FoldFlag_T flag = Fold(
            RandomPivots<only_oddQ>( Int(0), VertexCount() ),
            RandomAngle(),
            RandomReflectionFlag(P),
            check_collisionsQ,
            check_jointsQ
        );
        
        ++flat_ctrs[flag];
    }
    
    return flat_ctrs;
}



//####################################################################################
//####          From here on it gets a bit experimental
//####################################################################################

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




//std::pair<Int,Int> RandomPivots( const Int begin, const Int end )
//{
//    const Int n = VertexCount();
//    
//    unif_int u_int ( begin, end - Int(1) );
//    
////    Int i;
////    Int j;
////    
////    do
////    {
////        i = u_int(random_engine);
////        j = u_int(random_engine);
////    }
////    while( ModDistance(n,i,j) < Int(2) );
//    
//    const Int i = u_int(random_engine);
//    const Int j = u_int(random_engine);
//    
//    return MinMax(i,j);
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
        assert( (begin < mid) );
        assert( (mid   < end) );
        
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
        return this->RandomPivots<false>(begin,end);
    }
}

template<bool pull_transformsQ = true, bool splitQ = false>
void SubtreeFoldRandom(
    const Int start_node,
    mref<FoldFlagCounts_T> flag_ctr,
    const LInt accept_count,
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

//    // DEBUGGING
//    
//    // Checking feasiblity for RandomPivots.
//    if constexpr ( splitQ )
//    {
//        if( begin >= mid )
//        {
//            eprint(ClassName()+"::SubtreeFoldRandom: left node " + Tools::ToString(LeftChild(start_node)) + " is too small.");
//            
//            TOOLS_DUMP(begin);
//            TOOLS_DUMP(mid);
//            
//            return;
//        }
//        
//        if( mid>= end )
//        {
//            eprint(ClassName()+"::SubtreeFoldRandom: right node " + Tools::ToString(RightChild(start_node)) + " is too small.");
//            
//            TOOLS_DUMP(mid);
//            TOOLS_DUMP(end);
//            
//            return;
//        }
//    }
//
//    if( begin + Int(2) >= end )
//    {
//        eprint(ClassName()+"::SubtreeFoldRandom: node " + Tools::ToString(start_node) + " is too small.");
//        
//        TOOLS_DUMP(begin);
//        TOOLS_DUMP(end);
//        
//        return;
//    }
    
    const LInt target = flag_ctr[0] + accept_count;
    
    while( flag_ctr[0] < target )
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
    
    unif_int u_int_i ( a, b - Int(1) );
    unif_int u_int_j ( c, d - Int(1) );
    
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


void RectangleFoldRandom(
    const Int root_0, const Int root_1,
    mref<FoldFlagCounts_T> flag_ctr,
    const LInt accept_count,
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
    
    const LInt target = flag_ctr[0] + accept_count;
    
    while( flag_ctr[0] < target )
    {
        auto pivots = RandomPivots(a,b,c,d);
        
        FoldFlag_T flag = Fold(
            std::move(pivots),
            RandomAngle(),
            RandomReflectionFlag(P),
            check_collisionsQ
        );
        
        ++flag_ctr[flag];
    }
}


//FoldFlagCounts_T ExperimentalMove(
//    const Int  level,
//    const LInt accept_count_per_node,
//    const Real reflectP,
//    const bool check_collisionsQ = true
//)
//{
//    FoldFlagCounts_T flag_ctr ( LInt(0) );
//    
//    const Int begin = this->LevelBegin(level);
//    const Int end   = this->LevelEnd  (level);
//    
//    for( Int node_0 = begin; node_0 < end; ++node_0 )
//    {
//        for( Int node_1 = begin; node_1 < end; ++node_1 )
//        {
//            RectangleFoldRandom(
//                node_0, node_1, flag_ctr, accept_count_per_node,
//                reflectP, check_collisionsQ
//            );
//        }
//    }
//    
//    return flag_ctr;
//}
