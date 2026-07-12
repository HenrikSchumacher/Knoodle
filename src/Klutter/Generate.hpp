private:

enum class GenerateFlag
{
    NoChange,
    Changed,
    SimplerDiagramDetected,
    Failure
};

// Start with buckets[id_0] and generate new diagrams from all members and add them to this bucket. Whenever a newly generate key is found in another bucket, the buckets are merged. The return value is the id of the surviving bucket.

template<bool debugQ = false>
GenerateFlag Generate(
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
    
    if( id_0 >= buckets.size() ) { return GenerateFlag::NoChange; }
    
    if( buckets[id_0].empty() ) { return GenerateFlag::NoChange; }
    
    // The current id may change due to merges.
    ID_T id = id_0;
    
    GenerateFlag flag = GenerateFlag::NoChange;
    
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
                    return GenerateFlag::Failure;
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
                    
                    // TODO: This is an obstacle for parallelization.
                    DeleteBucket<true>(id);
                    return GenerateFlag::SimplerDiagramDetected;
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
                        return GenerateFlag::Failure;
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
                    // TODO: Obstacle for parallelization.
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
                
                flag = GenerateFlag::Changed;
                
                if constexpr ( debugQ )
                {
                    logvalprint("IdentifiedKeyCount()",IdentifiedKeyCount());
                }
            }
        }
    }
    
    return flag;
}

public:

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
