public:

Flag_T GenerateUnidentified(
    mref<Reapr_T> reapr,
    const Int embedding_trials,
    const Int rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("GenerateUnidentified"); };
    
    Flag_T flag = Flag_T::NoChange;
    
    for( Size_T idx = unidentified_buckets.size(); idx --> Size_T(0); )
    {
        const Flag_T loc_flag = GenerateUnidentified(reapr, idx, embedding_trials, rotation_trials);
        
        if( loc_flag == Flag_T::Changed )
        {
            flag = Flag_T::Changed;
            continue;
        }
        else if( loc_flag == Flag_T::NoChange )
        {
            continue;
        }
        if( loc_flag == Flag_T::Identified )
        {
            flag = Flag_T::Changed;
            continue;
        }
        else if( loc_flag == Flag_T::SimplerDiagramDetected )
        {
            flag = Flag_T::Changed;
            continue;
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
Flag_T GenerateUnidentified(
    mref<Reapr_T> reapr,
    const Size_T  idx_0,
    const Size_T  embedding_trials,
    const Size_T  rotation_trials
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("GenerateUnidentified"); };
    TOOLS_PTIMER(timer,tag());
    
    constexpr Size_T max_projection_iter = 10;
    
    if( idx_0 >= unidentified_buckets.size() ) { return Flag_T::NoChange; }
    
    if( unidentified_buckets[idx_0].empty() ) { return Flag_T::NoChange; }
    
    // The current id may change due to merges.
    Size_T idx = idx_0;
    
    Flag_T flag = Flag_T::NoChange;
    
    if constexpr ( debugQ )
    {
        logvalprint("idx",idx);
    }

    stack.clear();
    
    // We put all keys from the starting key set to the stack.
    for( const Key_T & key : unidentified_buckets[idx] )
    {
        stack.push_back(key);
    }
    
//    const Int threshold = Max(crossing_count,max_n);

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
                    return Flag_T::Failure;
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
                
                    DeleteUnidentifiedBucket<true>(idx);
                    return Flag_T::SimplerDiagramDetected;
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
                
                // TODO: We have to hash and to lookup twice here. Maybe it is better to use one std::vector for both bucket lists. (That would require many empty buckets, though.
                
                if( identified_lut.contains(key) )
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Key is present in identified_lut.");
                        logvalprint("idx before merge",idx);
                        logvalprint("identified_lut[key] before merge",identified_lut[key]);
                    }
                    
                    const ID_T id = identified_lut[key];
                    
                    MergeIntoIdentifiedBucket(idx,id);
                    
                    if constexpr ( debugQ )
                    {
                        logvalprint("id after merge",id);
                        logvalprint("identified_lut[key] after merge",identified_lut[key]);
                    }
                    return Flag_T::Identified;
                }
                else if( unidentified_lut.contains(key) )
                {
                    const Size_T idx_new = unidentified_lut[key];
                    
                    if( idx_new == idx )
                    {
                        continue;
                    }

                    if constexpr ( debugQ )
                    {
                        logprint("Key is present in unidentified_lut.");
                        logvalprint("idx before merge",idx);
                        logvalprint("unidentified_lut[key] before merge",idx_new);
                    }
                    
                    idx = MergeIntoUnidentifiedBucket(idx,idx_new);
                    flag = Flag_T::Changed;
                    
                    if constexpr ( debugQ )
                    {
                        logvalprint("idx after merge",idx);
                        logvalprint("unidentified_lut[key] after merge",identified_lut[key]);
                    }
                }
                else
                {
                    if constexpr ( debugQ )
                    {
                        logprint("Inserting new key.");
                    }
                    unidentified_buckets[idx].insert(key);
                    unidentified_lut[key] = idx;
                    stack.push_back(key);
                    flag = Flag_T::Changed;
                }
                
            } // for( Size_T rot = 0; rot < rotation_trials; ++rot )
            
        } // for( Size_T iter = 0; iter < embedding_trials; ++iter )
        
    } // while( !stack.empty() )
    
    return flag;
}
