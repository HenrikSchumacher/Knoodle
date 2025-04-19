public:


enum class Traversal : std::int_fast8_t
{
    BFS = 0,
    DFS = 1
};

static std::string ToString( Traversal traversal )
{
    switch( traversal )
    {
        case Traversal::BFS: return "BFS";
        case Traversal::DFS: return "DFS";
    }
}

enum class Direction : std::int_fast8_t
{
    Up          = 0,
    Down        = 1,
    VCycle      = 2,
    LambdaCycle = 3
};

static std::string ToString( Direction dir )
{
    switch( dir )
    {
        case Direction::Up:          return "Up";
        case Direction::Down:        return "Down";
        case Direction::VCycle:      return "V";
        case Direction::LambdaCycle: return "Λ";
    }
}

FoldFlagCounts_T FoldRandomHierarchical(
    const LInt accepts_per_node,
    const Int  wave_count,
    const Int  max_level,
    const Real reflectP,
    const bool checkQ = true,
    const Traversal traversal = Traversal::BFS,
    const Direction dir       = Direction::Up,
    const bool splitQ = true,
    const bool verboseQ = false
)
{
    return FoldRandomHierarchical(
        [accepts_per_node]( Int node )
        {
            (void)node;
            return accepts_per_node;
        },
        wave_count,max_level,reflectP,checkQ,traversal,dir,splitQ,verboseQ
    );
}

template<typename F>
FoldFlagCounts_T FoldRandomHierarchical(
    F && f,
    const Int  wave_count,
    const Int  max_level,
    const Real reflectP,
    const bool checkQ = true,
    const Traversal traversal = Traversal::BFS,
    const Direction dir       = Direction::Up,
    const bool splitQ = true,
    const bool verboseQ = false
)
{
    FoldFlagCounts_T flag_ctrs ( LInt(0) );
    
    if( verboseQ )
    {
        FoldRandomHierarchical_impl_1<true>(
            flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal, dir, splitQ
        );
    }
    else
    {
        FoldRandomHierarchical_impl_1<false>(
            flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal, dir, splitQ
        );
    }
    
    return flag_ctrs;
}

private:

template<bool verboseQ, typename F>
void FoldRandomHierarchical_impl_1(
    mref<FoldFlagCounts_T> flag_ctrs,
    F && f,
    const Int  wave_count,
    const Int  max_level,
    const Real reflectP,
    const bool checkQ,
    const Traversal traversal,
    const Direction dir,
    const bool splitQ
)
{
    if( splitQ )
    {
        FoldRandomHierarchical_impl_2<true,verboseQ>(
            flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal, dir
        );
    }
    else
    {
        FoldRandomHierarchical_impl_2<false,verboseQ>(
            flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal, dir
        );
    }
}

template<bool splitQ, bool verboseQ, typename F>
void FoldRandomHierarchical_impl_2(
    mref<FoldFlagCounts_T> flag_ctrs,
    F && f,
    const Int  wave_count,
    const Int  max_level,
    const Real reflectP,
    const bool checkQ,
    const Traversal traversal,
    const Direction dir
)
{
    switch( dir )
    {
        case Direction::Up:
        {
            FoldRandomHierarchical_impl_3<Direction::Up,splitQ,verboseQ>(
                flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal
            );
            return;
        }
        case Direction::Down:
        {
            FoldRandomHierarchical_impl_3<Direction::Down,splitQ,verboseQ>(
                flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal
            );
            return;
        }
        case Direction::VCycle:
        {
            FoldRandomHierarchical_impl_3<Direction::VCycle,splitQ,verboseQ>(
                flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal
            );
            return;
        }
        case Direction::LambdaCycle:
        {
            FoldRandomHierarchical_impl_3<Direction::LambdaCycle,splitQ,verboseQ>(
                flag_ctrs, f, wave_count, max_level, reflectP, checkQ, traversal
            );
            return;
        }
    }
}
template<Direction dir, bool splitQ, bool verboseQ, typename F>
void FoldRandomHierarchical_impl_3(
    mref<FoldFlagCounts_T> flag_ctrs,
    F && f,
    const Int  wave_count,
    const Int  max_level,
    const Real reflectP,
    const bool checkQ,
    const Traversal traversal
)
{
    switch( traversal )
    {
        case Traversal::BFS:
        {
            FoldRandomHierarchical<Traversal::BFS,dir,splitQ,verboseQ>(
                flag_ctrs, f, wave_count, max_level, reflectP, checkQ
            );
            return;
        }
        case Traversal::DFS:
        {
            FoldRandomHierarchical<Traversal::BFS,dir,splitQ,verboseQ>(
                flag_ctrs, f, wave_count, max_level, reflectP, checkQ
            );
            return;
        }
    }
}


template<Traversal traversal, Direction dir, bool splitQ, bool verboseQ, typename F>
void FoldRandomHierarchical(
    mref<FoldFlagCounts_T> flag_ctrs,
    F && f,
    const Int  wave_count_,
    const Int  max_level_,
    const Real reflectP_,
    const bool checkQ
)
{
//    const LInt success_count = Ramp(success_count_);
    const Int  wave_count    = Ramp(wave_count_);
    const Int  d             = this->ActualDepth() - Int(3);
    const Int  max_level     = (max_level_ < 0) ? d : Min( max_level_, d );
    const LInt node_count = (LInt(2) << max_level) - LInt(1);
    const Real reflectP      = Clamp(reflectP_, Real(0), Real(1) );
    
//    constexpr bool cycleQ = (dir == Direction::VCycle) || (dir == Direction::LambdaCycle);

    
//    const LInt per_node   = CeilDivide( success_count, wave_count * node_count * (cycleQ ? LInt(2) : LInt(1)) );
    
    if constexpr ( verboseQ )
    {
        TOOLS_DUMP(traversal);
        TOOLS_DUMP(dir);
        TOOLS_DUMP(splitQ);
        TOOLS_DUMP(wave_count);
        TOOLS_DUMP(max_level);
        TOOLS_DUMP(node_count);
//        TOOLS_DUMP(per_node);
        TOOLS_DUMP(reflectP);
        TOOLS_DUMP(checkQ);
    }
    
//    logprint("");
//    logprint("FoldRandomHierarchical");
//    
//    TOOLS_LOGDUMP(VertexCount());
//    
//    TOOLS_LOGDUMP(traversal);
//    TOOLS_LOGDUMP(dir);
//    TOOLS_LOGDUMP(splitQ);
//    TOOLS_LOGDUMP(success_count);
//    TOOLS_LOGDUMP(wave_count);
//    TOOLS_LOGDUMP(max_level);
//    TOOLS_LOGDUMP(node_count);
//    TOOLS_LOGDUMP(per_node);
//    TOOLS_LOGDUMP(reflectP);
//    TOOLS_LOGDUMP(checkQ);

    for( Int wave = 0; wave < wave_count; ++wave )
    {
//        TOOLS_LOGDUMP(wave);
        this->template FoldRandomHierarchicalWave<traversal,dir,splitQ>(
            flag_ctrs, f, max_level, node_count, reflectP, checkQ
        );
    }
    
//    logprint("");
}


template<Traversal traversal, Direction dir, bool splitQ, typename F>
void FoldRandomHierarchicalWave(
    mref<FoldFlagCounts_T> flag_ctrs,
    F && f, // f(node) specifies number of successes required on this node
    const Int  max_level,
    const Int  node_count,
    const Real reflectP,
    const bool checkQ
)
{
    constexpr Direction Up          = Direction::Up;
    constexpr Direction Down        = Direction::Down;
    constexpr Direction VCycle      = Direction::VCycle;
    constexpr Direction LambdaCycle = Direction::LambdaCycle;
    
    if constexpr ( traversal == Traversal::BFS )
    {
        if constexpr ( dir == Down )
        {
            for( Int node = Int(0); node < node_count; ++node  )
            {
                this->template SubtreeFoldRandom<true,splitQ>(
                    node, flag_ctrs, f(node), reflectP, checkQ
                );
            }
        }
        else if constexpr ( dir == Up )
        {
            for( Int node = node_count; node --> Int(0);  )
            {
                this->template SubtreeFoldRandom<true,splitQ>(
                    node, flag_ctrs, f(node), reflectP, checkQ
                );
            }
        }
        else if constexpr ( dir == VCycle )
        {
            this->template FoldRandomHierarchicalWave<Traversal::BFS,Down,splitQ>(
                flag_ctrs, f, max_level, node_count, reflectP, checkQ
            );
            this->template FoldRandomHierarchicalWave<Traversal::BFS,Up  ,splitQ>(
                flag_ctrs, f, max_level, node_count, reflectP, checkQ
            );
        }
        else if constexpr ( dir == LambdaCycle )
        {
            this->template FoldRandomHierarchicalWave<Traversal::BFS,Up  ,splitQ>(
                flag_ctrs, f, max_level, node_count, reflectP, checkQ
            );
            this->template FoldRandomHierarchicalWave<Traversal::BFS,Down,splitQ>(
                flag_ctrs, f, max_level, node_count, reflectP, checkQ
            );
        }
    }
    else if constexpr ( traversal == Traversal::DFS )
    {
        (void)node_count;
        
        if constexpr ( dir == Down )
        {
            this->template DepthFirstScan<DFS::BreakEarly>(
                [=,this,&flag_ctrs]( Int node )
                {
                    this->template SubtreeFoldRandom<true,splitQ>(
                        node, flag_ctrs, f(node), reflectP, checkQ
                    );
                    return (this->Depth(node) < max_level );
                },
                []( Int node ){ (void)node; },
                []( Int node ){ (void)node; }
            );
        }
        else if constexpr ( dir == Up )
        {
            this->template DepthFirstScan<DFS::PostVisitAlways>(
                [max_level,this]( Int node )
                {
                    return (this->Depth(node) < max_level );
                },
                [=,this,&flag_ctrs]( Int node )
                {
                    this->template SubtreeFoldRandom<true,splitQ>(
                        node, flag_ctrs, f(node), reflectP, checkQ
                    );
                },
                []( Int node ){ (void)node; }
            );
        }
        else if constexpr ( dir == VCycle )
        {
            this->template DepthFirstScan<DFS::PostVisitAlways>(
                [=,this,&flag_ctrs]( Int node )
                {
                    this->template SubtreeFoldRandom<true,splitQ>(
                        node, flag_ctrs, f(node), reflectP, checkQ
                    );
                    return (this->Depth(node) < max_level );
                },
                [=,this,&flag_ctrs]( Int node )
                {
                    this->template SubtreeFoldRandom<true,splitQ>(
                        node, flag_ctrs, f(node), reflectP, checkQ
                    );
                },
                []( Int node ){ (void)node; }
            );
        }
        else if constexpr ( dir == LambdaCycle )
        {
            this->template FoldRandomHierarchicalWave<Traversal::DFS,Up  ,splitQ>(
                flag_ctrs, f, max_level, node_count, reflectP, checkQ
            );
            this->template FoldRandomHierarchicalWave<Traversal::DFS,Down,splitQ>(
                flag_ctrs, f, max_level, node_count, reflectP, checkQ
            );
        }
    }
}



public:

FoldFlagCounts_T HierarchicalMove(
    const LInt iter_count,
    const Real reflectP,
    const bool checkQ
)
{
    const Traversal traversal = Traversal::BFS;
    const Direction direction = Direction::Up;
    
    FoldFlagCounts_T flag_ctrs( LInt(0) );
    
    for( LInt iter = 0; iter < iter_count; ++iter )
    {
        this->template FoldRandomHierarchical<traversal,direction,true,false>(
            flag_ctrs,
            [](Int node)
            {
                (void)node;
                return LInt(1);
            },
            Int(1), Int(-1), reflectP, checkQ
        );
    }
    
    return flag_ctrs;
}



FoldFlagCounts_T StratifiedMove(
    const Int  level,
    const LInt wave_count,
    const LInt accepts_per_box,
//    const LInt accepts_per_diagonal_box,
//    const LInt accepts_per_offdiagonal_box,
    const Real reflectP,
    const bool checkQ
)
{
    FoldFlagCounts_T flag_ctrs( LInt(0) );
    
    auto [begin,end] = LevelRange(level);
    
    for( LInt wave = 0; wave < wave_count; ++wave )
    {
        for( Int node_0 = begin; node_0 < end; ++node_0 )
        {
            for( Int node_1 = begin + Int(1); node_1 < end; ++node_1 )
            {
                this->template SubtreeFoldRandom<true>(
                    node_0, node_1, flag_ctrs, accepts_per_box, reflectP, checkQ
                );
            }
        }
    }
    
    return flag_ctrs;
}
