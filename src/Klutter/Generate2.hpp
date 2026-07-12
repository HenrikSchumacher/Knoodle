private:

// In principle, Generate2 does the same as Generate, but it does so before lut is created and it works entirely within a bucket. This way it can be run in parallel.

enum class Generate2Flag
{
    NoChange,
    Changed,
    SimplerDiagramDetected,
    Failure
};

Generate2Flag Generate2(
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
    
    Generate2Flag flag = Generate2Flag::NoChange;

    if( buckets.empty() ) { return flag; }
    
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
                    return Generate2Flag::Failure;
                }
                
                PDC_T pdc( emb );
                
                // We only accept pass-reduced diagrams. But we don't run Reapr.
                pdc.Simplify(reapr, {.embedding_trials = 0});
                
                // TODO: Make sure that pdc does not contain any spurious invalid diagrams.
                
                // We only collect diagrams that can be prime knots with crossing_count crossings.
                if ( (pdc.DiagramCount() > Int(1)) || (pdc[0].CrossingCount() < crossing_count) )
                {
                    // All the keys in this bucket belong to diagrams that we don't want.
                    return Generate2Flag::SimplerDiagramDetected;
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
                    flag = Generate2Flag::Changed;
                    bucket.insert(key);
                    stack.push_back(key);
                }
            }
        }
    }
    
    return flag;
}
