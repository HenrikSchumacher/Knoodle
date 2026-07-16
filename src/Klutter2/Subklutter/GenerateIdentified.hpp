public:

Flag_T GenerateIdentified(
    mref<Reapr_T> reapr,
    const Int embedding_trials,
    const Int rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("GenerateIdentified"); };
    
    Flag_T flag = Flag_T::NoChange;
    
    // TODO: THIS LOOP IS WRONG!
    for( auto & [id,bucket] : identified_buckets )
    {
        const Flag_T loc_flag = GenerateIdentified(reapr, id, bucket, embedding_trials, rotation_trials);
        
        if( loc_flag == Flag_T::Changed )
        {
            flag = Flag_T::Changed;
            continue;
        }
        else if( loc_flag == Flag_T::NoChange )
        {
            continue;
        }
        else if( loc_flag == Flag_T::SimplerDiagramDetected )
        {
            return Flag_T::Failure;
        }
        else if( loc_flag == Flag_T::Failure )
        {
            return Flag_T::Failure;
        }
    }
    
    return flag;
}


// Start with bucket and generate new diagrams from all members and add them to this bucket. Whenever a newly generate key is found in another bucket, the buckets are merged. The return value is the id of the surviving bucket.

template<bool debugQ = false>
Flag_T GenerateIdentified(
    mref<Reapr_T>  reapr,
    const ID_T     id,
    mref<KeySet_T> bucket,
    const Size_T   embedding_trials,
    const Size_T   rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("GenerateIdentified"); };
    TOOLS_PTIMER(timer,tag());
    
    constexpr Size_T max_projection_iter = 10;
    
//    // TODO: Issue a warning?
//    if( !identified_buckets.contains(id) ) { return Flag_T::NoChange; }
//    
//    // TODO: Better error message?
//    if( identified_buckets[id].empty() ) { return Flag_T::Failure; }
    
    if( bucket.empty() ) { return Flag_T::Failure; }
    
    Flag_T flag = Flag_T::NoChange;
    
    if constexpr ( debugQ )
    {
        logvalprint("id",id);
    }

    stack.clear();
    
//    mref<KeySet_T> bucket = identified_buckets[id];
    
    // We put all keys from the starting key set to the stack.
    for( const Key_T & key : bucket )
    {
        stack.push_back(key);
    }
    
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
                    eprint(tag() + ": " + emb.MethodName("FindIntersections")+ " returned invalid status flag for " + ToString(max_projection_iter) + " random rotation matrices. Something must be wrong. Check your results carefully.");
                    return Flag_T::Failure;
                }
                
                PDC_T pdc( emb );
                
                // We only accept pass-reduced diagrams. But we don't run Reapr.
                pdc.Simplify(reapr, {.embedding_trials = 0});
                
                // We only collect diagrams that can be prime knots with crossing_count crossings.
                if ( (pdc.DiagramCount() > Int(1)) || (pdc[0].CrossingCount() < crossing_count) )
                {
                    // This must never happen.
                    // TODO: Improve information content of error message.
                    eprint(tag() + ": Simpler diagram detected.");
                    return Flag_T::Failure;
                }
                
                if ( pdc[0].CrossingCount() > crossing_count )
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Key exceeds length. Skipping it.");
                    }
                    continue;
                }

                Key_T key = ToKey(pdc[0]);

//                if( !identified_lut.contains(key) )
                if( !bucket.contains(key) )
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Inserting new key.");
                    }
                    bucket.insert(key);
                    identified_lut[key] = id;
                    stack.push_back(key);
                    flag = Flag_T::Changed;
                }
                
            } // for( Size_T rot = 0; rot < rotation_trials; ++rot )
            
        } // for( Size_T iter = 0; iter < embedding_trials; ++iter )
        
    } // while( !stack.empty() )
    
    return flag;
}
