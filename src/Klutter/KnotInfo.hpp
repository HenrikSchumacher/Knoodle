public:

Path_T KnotInfoNameFile() const
{
    return working_directory / ("KnotNames_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".txt");
}

Path_T KnotInfoPDCodeFile() const
{
    return working_directory / ("KnotPDCodes_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".txt");
}

void LoadKnotInfo(
    Size_T embedding_trials,
    Size_T rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("LoadKnotInfo"); };
    
    if( knot_info_loadedQ )
    {
        wprint(tag() + ": KnotInfo data has already been loaded. Aborting.");
        return;
    }
    
    buckets.clear();
    names.clear();
    
    Path_T name_file = KnotInfoNameFile();
    
    std::ifstream name_stream ( name_file, std::ios::binary );
    if( !name_stream )
    {
        eprint(tag() + ": Could not open " + name_file.string() +". Aborting with incomplete table." );
        return;
    }
    
    Path_T pd_code_file = KnotInfoPDCodeFile();
    
    {
        Tools::InString s (pd_code_file);
        if( s.EmptyQ() )
        {
            eprint(tag() + ": File  " + pd_code_file.string() + " is empty.");
            return;
        }
        
        Tensor2<Int,Int> pd_code ( crossing_count, Int(4) );
        Name_T  name;
        
//        tic("Reading inputs");
        while( name_stream && !s.EmptyQ() && !s.FailedQ() )
        {
            name_stream >> name;
            names.push_back(name);
            //        s.TakeMatrixFunction(pd_code.WriteAccess(),crossing_count,Int(4),"{",",","}","{",",","}");
            s.TakeMatrixFunction(pd_code.WriteAccess(),crossing_count,Int(4), "","\n","\n", ""," ","");
            
            if( s.FailedQ() )
            {
                wprint(tag() + ": Reading pd code no. " + ToString(buckets.size()) + " failed.");
                break;
            }
            
//            logvalprint("buckets.size()",buckets.size());
//            logvalprint("s.FailedQ()",s.FailedQ());
//            logvalprint("s.EmptyQ()",s.EmptyQ());
//            logvalprint("pd_code",pd_code);
            
            PD_T pd = PD_T::template FromPDCode<{.signQ = false,.colorQ = false}>(
                pd_code.data(), crossing_count
            );
            
            CreateBucket(ToKey(pd));
            
            if( s.EmptyQ() ) { break; }
            
            s.SkipChar('\n');
        }
//        toc("Reading inputs");
    }
    
    Tensor1<int,Size_T> flags( KnotTypeCount(), 0 );
    
    // Randomization helps, but Round Robin might be better.;
    
    auto perm = Permutation<ID_T,Sequential>::RandomPermutation(
        ID_T(KnotTypeCount()), ID_T(1), R.RandomEngine()
    );
    
    tic("Generate parallel");
    ParallelDo(
        [&tag,&flags,&perm,embedding_trials,rotation_trials,this]( Size_T thread )
        {
            TimeInterval thread_timer;
            thread_timer.Tic();
            
            ID_T id_begin = JobPointer( KnotTypeCount(), thread_count, thread     );
            ID_T id_end   = JobPointer( KnotTypeCount(), thread_count, thread + 1 );

            Reapr_T reapr ( R.Settings() );

            for( ID_T id = id_begin; id < id_end; ++id )
            {
                ID_T p = perm.GetPermutation()[id];
                
                flags[p] = Generate2(reapr,buckets[p],crossing_count,embedding_trials,rotation_trials);
            }

            thread_timer.Toc();
            logprint(tag() + ": thread " + ToString(thread) + " time = " + ToString(thread_timer.Duration()) + "; job_count = " + ToString(id_end - id_begin)+ "." );
        },
        thread_count
    );
    toc("Generate parallel");
    
    
    // TODO: We could also apply the chirality transformation to the present codes and classify them accordingly.
    
//    tic("Build the lut");
    for( ID_T id = 0; id < KnotTypeCount(); ++id )
    {
        if( (flags[id] >> 1) & 1 )
        {
            DeleteBucket<true>(id);
        }
        else if( flags[id] & 1 )
        {
            for( const Key_T & key : buckets[id] )
            {
                lut[key] = id;
            }
        }
    }
//    toc("Build the lut");
    
    if( !BucketsOkayQ() )
    {
        eprint(tag() + ": Buckets are corrupted.");
        return;
    }
    
    knot_info_loadedQ = true;
}


int Generate2(
    mref<Reapr_T>  reapr,
    mref<KeySet_T> bucket,
    const Int      max_n,              // Maximal number of crossings of pd diagrams on stack.
    const Size_T   embedding_trials,
    const Size_T   rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Generate2"); };
    TOOLS_PTIMER(timer,tag());
    
    constexpr Size_T max_projection_iter = 10;
    
    int changedQ = 0;

    if( buckets.empty() ) { return 0; }
    
    std::vector<Key_T> stack;

    // We put all keys from the starting key set to the stack.
    for( const Key_T & key : bucket )
    {
        stack.push_back(key);
    }
    
    const Int threshold = Max(crossing_count,max_n);

    while( !stack.empty() )
    {
        Key_T key = std::move(stack.back());
        stack.pop_back();
        
        PD_T pd = FromKey(key);

        for( Size_T iter = 0; iter < embedding_trials; ++iter )
        {
            if constexpr ( debugQ ) { logvalprint("iter",iter); }
            
            LinkEmbedding_T emb = reapr.Embedding(pd,reapr.RandomRotation());
            
            for( Size_T rot = 0; rot < rotation_trials; ++rot )
            {
                if constexpr ( debugQ ) { logvalprint("rot",rot); }
                
                Size_T projection_iter = 0;
                int projection_flag = 0;
                emb.Rotate( reapr.RandomRotation() );
                projection_flag = emb.RequireIntersections();
                
                while( (projection_flag!=0) && (projection_iter < max_projection_iter) )
                {
                    ++projection_iter;
                    // Rotate is a bit expensive do to an extra allocation and extra copying.
                    // But we land here really very, very, very seldomly.
                    emb.Rotate( reapr.RandomRotation() );
                    projection_flag = emb.RequireIntersections();
                }
                
                if( projection_flag != 0 )
                {
                    eprint(tag() + ": " + emb.MethodName("FindIntersections")+ " returned invalid status flag for " + ToString(max_projection_iter) + " random rotation matrices. Something must be wrong. Returning an invalid diagram. Check your results carefully.");
                    return changedQ;
                }
                
                PDC_T pdc( emb );
                
                // We only accept pass-reduced diagrams. But we don't run Reapr.
                pdc.Simplify(reapr, {.embedding_trials = 0});
                
                // TODO: Make sure that pdc does not contain any spurious invalid diagrams.
                
                // We only collect diagrams that can be prime knots with crossing_count crossings.
                if ( (pdc.DiagramCount() > Int(1)) || (pdc[0].CrossingCount() < crossing_count) )
                {
                    // All the key in this bucket belong to diagrams that we don't want.
                    return 2;
                }
                
                if ( pdc[0].CrossingCount() > threshold )
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Key exceeds length. Skipping it.");
                    }
                    continue;
                }

                Key_T key = ToKey(pdc[0]);
                
                if constexpr ( debugQ ) { logvalprint("key",key); }
                
                if( !bucket.contains(key) )
                {
                    changedQ = 1;
                    bucket.insert(key);
                    stack.push_back(key);
                }
            }
        }
    }
    
    return changedQ;
}

// Start with buckets[id_0] and generate new diagrams from all members and add them to this bucket. Whenever a newly found key is found in another bucket, the buckets are merged. The return value is the id of the surviving bucket.
ID_T Generate(
    mref<Reapr_T> reapr,
    ID_T          id_0,
    const Int     max_n,              // Maximal number of crossings of pd diagrams on stack.
    const Size_T  embedding_trials,
    const Size_T  rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Generate"); };
    TOOLS_PTIMER(timer,tag());
    
    constexpr Size_T max_projection_iter = 10;
    
    if( id_0 >= buckets.size() ) { return not_found; }
    
    if( buckets[id_0].empty() ) { return not_found; }
    
    // The current id may change due to merges.
    ID_T id = id_0;
    
    if constexpr ( debugQ )
    {
        logvalprint("id",id);
    }
    
    std::vector<Key_T> stack;

    // We put all keys from the starting key set to the stack.
    for( const Key_T & key : buckets[id] )
    {
        stack.push_back(key);
    }
    
    const Int threshold = Max(crossing_count,max_n);

    while( !stack.empty() )
    {
        Key_T key = std::move(stack.back());
        stack.pop_back();
        
        PD_T pd = FromKey(key);

        for( Size_T iter = 0; iter < embedding_trials; ++iter )
        {
            if constexpr ( debugQ ) { logvalprint("iter",iter); }
            
            LinkEmbedding_T emb = reapr.Embedding(pd,reapr.RandomRotation());
            
            for( Size_T rot = 0; rot < rotation_trials; ++rot )
            {
                if constexpr ( debugQ ) { logvalprint("rot",rot); }
                
                Size_T projection_iter = 0;
                int projection_flag = 0;
                emb.Rotate( reapr.RandomRotation() );
                projection_flag = emb.RequireIntersections();
                
                while( (projection_flag!=0) && (projection_iter < max_projection_iter) )
                {
                    ++projection_iter;
                    // Rotate is a bit expensive do to an extra allocation and extra copying.
                    // But we land here really very, very, very seldomly.
                    emb.Rotate( reapr.RandomRotation() );
                    projection_flag = emb.RequireIntersections();
                }
                
                if( projection_flag != 0 )
                {
                    eprint(tag() + ": " + emb.MethodName("FindIntersections")+ " returned invalid status flag for " + ToString(max_projection_iter) + " random rotation matrices. Something must be wrong. Returning an invalid diagram. Check your results carefully.");
                    return not_found;
                }
                
                PDC_T pdc( emb );
                
                // We only accept pass-reduced diagrams. But we don't run Reapr.
                pdc.Simplify(reapr, {.embedding_trials = 0});
                
                // TODO: Make sure that pdc does not contain any spurious invalid diagrams.
                
                // We only collect diagrams that can be prime knots with crossing_count crossings.
                if ( (pdc.DiagramCount() > Int(1)) || (pdc[0].CrossingCount() < crossing_count) )
                {
                    // All the key in this bucket belong to diagrams that we don't want.
                    // So, we instruct DeleteBucket to remove their entries from lut.
                    DeleteBucket<true>(id);
                    return not_found;
                }
                
                if ( pdc[0].CrossingCount() > threshold )
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Key exceeds length. Skipping it.");
                    }
                    continue;
                }

                Key_T key = ToKey(pdc[0]);
                
                if constexpr ( debugQ)
                {
                    logvalprint("key",key);
                    
                    if( !BucketsOkayQ() )
                    {
                        eprint(tag() + ": Buckets are corrupted.");
                        return not_found;
                    }
                }
                
                if( lut.contains(key) )
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Key is present in lut.");
                        logvalprint("id before merge",id);
                        logvalprint("lut[key] before merge",lut[key]);
                    }
                    id = MergeBuckets(id,lut[key]);
                    if constexpr ( debugQ )
                    {
                        logvalprint("id after merge",id);
                        logvalprint("lut[key] after merge",lut[key]);
                    }
                }
                else
                {
                    AddKey(key, id);
                    stack.push_back(key);
                }
                
                if constexpr ( debugQ )
                {
                    logvalprint("IdentifiedKeyCount()",IdentifiedKeyCount());
                }
            }
        }
    }
    
    return id;
}

void GenerateIdentified(
    const Int embedding_trials,
    const Int rotation_trials
)
{
    for( ID_T id = 0; id < KnotTypeCount(); ++id )
    {
        (void)Generate(R, id, crossing_count, embedding_trials, rotation_trials);
    }
}

void GenerateUnidentified(
    const Int embedding_trials,
    const Int rotation_trials
)
{
    for( ID_T id = buckets.size(); id --> KnotTypeCount(); )
    {
        (void)Generate(R, id, crossing_count, embedding_trials, rotation_trials);
    }
}
