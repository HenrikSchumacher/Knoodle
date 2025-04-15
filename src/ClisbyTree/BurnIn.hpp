public:

FoldFlagCounts_T HierarchicalBurnIn(
    const Int burnin_,
    const Int wave_count_,
    const Int max_level_,
    const Real reflectP_,
    const bool check_collisionsQ = true,
    const bool verboseQ = false
)
{
    FoldFlagCounts_T flag_counters;
    flag_counters.SetZero();

    const Int d          = this->ActualDepth() - Int(3);
    const Int burnin     = Ramp(burnin_);
    const Int wave_count = Ramp(wave_count_);
    const Int max_level  = (max_level_ < 0) ? d : Min( max_level_, d );
    const Real reflectP  = Clamp(reflectP_, Real(0), Real(1) );
    const Int burnin_per_level = CeilDivide( burnin, wave_count * (max_level + Int(1)) );

    if( verboseQ )
    {
        TOOLS_DUMP(burnin);
        TOOLS_DUMP(wave_count);
        TOOLS_DUMP(max_level);
        TOOLS_DUMP(reflectP);
        TOOLS_DUMP(burnin_per_level);
    }

    Tensor1<Int,Int> level_step_count ( max_level + 1 );

    for( Int level = max_level + Int(1); level --> 0;  )
    {
        const Int node_begin = (Int(1) << level) - Int(1);
        const Int node_end   = Int(2) * node_begin + Int(1);
        const Int step_count = CeilDivide( burnin_per_level, node_end - node_begin );
        level_step_count[level] = step_count;

        if( verboseQ )
        {
            TOOLS_DUMP(level);
            TOOLS_DUMP(node_begin);
            TOOLS_DUMP(node_end);
            TOOLS_DUMP(step_count);
        }
    }

    for( Int wave = 0; wave < wave_count; ++wave )
    {
        PushAllTransforms();
        for( Int level = max_level + Int(1); level --> 0;  )
        {
            const Int node_begin = (Int(1) << level) - Int(1);
            const Int node_end   = Int(2) * node_begin + Int(1);
            const Int step_count = level_step_count[level];

            for( Int node = node_begin; node < node_end; ++node )
            {
                flag_counters += SubtreeFoldRandom(
                    node, node, step_count, reflectP, check_collisionsQ
                );
            }
        }
    }

    return flag_counters;
}


FoldFlagCounts_T HierarchicalBurnIn2(
    const Int burnin_,
    const Int wave_count_,
    const Int max_level_,
    const Real reflectP_,
    const bool check_collisionsQ = true,
    const bool verboseQ = false
)
{
    FoldFlagCounts_T flag_counters;
    flag_counters.SetZero();

    const Int d          = this->ActualDepth() - Int(3);
    const Int burnin     = Ramp(burnin_);
    const Int wave_count = Ramp(wave_count_);
    const Int max_level  = (max_level_ < 0) ? d : Min( max_level_, d );
    const Int max_node   = (Int(2) << max_level) - Int(2);

    const Real reflectP  = Clamp(reflectP_, Real(0), Real(1) );
    const Int burnin_per_node = CeilDivide( burnin, wave_count * (max_node + Int(1)) );
    
    if( verboseQ )
    {
        TOOLS_DUMP(burnin);
        TOOLS_DUMP(wave_count);
        TOOLS_DUMP(max_level);
        TOOLS_DUMP(max_node);
        TOOLS_DUMP(reflectP);
        TOOLS_DUMP(burnin_per_node);
    }

    for( Int wave = 0; wave < wave_count; ++wave )
    {
        PushAllTransforms();
        for( Int level = max_level + Int(1); level --> 0;  )
        {
            const Int node_begin = (Int(1) << level) - Int(1);
            const Int node_end   = Int(2) * node_begin + Int(1);

            for( Int node = node_begin; node < node_end; ++node )
            {
                flag_counters += SubtreeFoldRandom(
                    node, node, burnin_per_node, reflectP, check_collisionsQ
                );
            }
        }
    }

    return flag_counters;
}
