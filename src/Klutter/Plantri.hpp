public:

Path_T PlantriPDCodeFile() const
{
    return working_directory / ("PlantriPDCodes_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".txt");
}

template<IntQ ExtInt>
void LoadPlantriPDCodes(
    ExtInt        embedding_trials_,
    ExtInt        rotation_trials_
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("LoadPlantriPDCodes"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( !knot_info_loadedQ )
    {
        eprint(tag() + ": KnotInfo data not loaded, yet. Please load that first. Aborting.");
        return;
    }
    
    if( plantri_loadedQ )
    {
        wprint(tag() + ": Platri data has already been loaded. Aborting.");
        return;
    }
    
    Tensor3<Int,Int> input;
    Size_T input_count = 0;
    
    {
        Path_T file = PlantriPDCodeFile();
        
        Tools::InString s (file);
        if( s.EmptyQ() )
        {
            eprint(tag() + ": File  " + file.string() + " is empty.");
            return;
        }
        
//        // DEBUGGING
//        TOOLS_DUMP(s.LineCount());
        
        // s.LineCount() upper bound on (number of inputs) * crossing_count;
        input = Tensor3<Int,Int>( s.LineCount() / crossing_count, crossing_count, Int(4) );

        Tensor2<Int,Int> pd_code ( crossing_count, Int(4) );
        
//        tic("Reading inputs");
        while( !s.EmptyQ() && !s.FailedQ() )
        {
            s.TakeMatrixFunction(pd_code.WriteAccess(),crossing_count,Int(4), "","\n","\n", ""," ","");
            if( s.FailedQ() )
            {
                wprint(tag() + ": Reading pd code no. " + ToString(input_count) + " failed.");
                break;
            }
            pd_code.Write(input.data(input_count));
            ++input_count;
            
            if( s.EmptyQ() ) { break; }
            
            s.SkipChar('\n');
        }
//        toc("Reading inputs");
    }
    
//    // DEBUGGING
//    TOOLS_DUMP(input_count);
//    TOOLS_DUMP(input.Dimension(0));
    
    std::vector<KeySet_T> thread_survivors (thread_count);
    
    // The plantri codes are sorted in a way that many simple diagrams come first and many difficult come last.
    // For the sake of load balancing, we randomly permute the inputs during reading.
    
    auto perm = Permutation<Size_T,Sequential>::RandomPermutation(
        input_count, ID_T(1), R.RandomEngine()
    );
    
//    valprint("elements to sieve", (Size_T(1) << crossing_count) * input_count );
    
    tic("Coarse sieving");
    ParallelDo(
        [&thread_survivors, &tag, &perm, &input, input_count, this]( Size_T thread )
        {
            TimeInterval thread_timer;
            thread_timer.Tic();
            
            Reapr_T reapr (R.Settings());
            KeySet_T survivors;
            
            Size_T job_begin = JobPointer(input_count, thread_count, thread    );
            Size_T job_end   = JobPointer(input_count, thread_count, thread + 1);
            
            const UInt64 i_max = (UInt64(1) << crossing_count );
            typename PD_T::CrossingStateContainer_T C_state (crossing_count);
            
            for( Size_T job = job_begin; job < job_end; ++job )
            {
                Size_T p = perm.GetPermutation()[job];
                
                PD_T pd_0 = PD_T::template FromPDCode<{.signQ = false,.colorQ = false}>(
                    input.data(p), crossing_count, false, false
                );
                
                if( pd_0.LinkComponentCount() > Int(1) )
                {
        //            wprint(tag() + ": Found diagram with more than one link component.");
                    continue;
                }
                
                for( UInt64 i = 0; i < i_max; ++i )
                {
                    for( Int j = 0; j < crossing_count; ++j )
                    {
                        C_state [j] = BooleanToCrossingState(get_bit(i,j));
                    }

                    PD_T pd (
                        crossing_count,
                        pd_0.Crossings().data(),
                        C_state.data(),
                        pd_0.Arcs().data(),
                        pd_0.ArcStates().data()
                    );
                    
                    PDC_T pdc( pd.CachelessCopy() );
                    
                    // We only accept pass-reduced diagrams. But we don't run Reapr.
                    pdc.Simplify(reapr, {.embedding_trials = 0});
                    
                    // TODO: Make sure that pdc does not contain any spurious invalid diagrams.

                    // We only collect diagrams that can be prime knots with crossing_count crossings.
                    if ( (pdc.DiagramCount() > Int(1)) || (pdc[0].CrossingCount() < crossing_count) )
                    {
                        continue;
                    }
                    
                    // TODO: If we knew that lut contains with every key also its chiral transformed siblings, then we could use lut to cull already here.
                    
                    survivors.insert(ToKey(pd));
                }
                
            } // for( Size_T job = job_begin; job < job_end; ++job )
            
            thread_survivors[thread] = std::move(survivors);
            
            thread_timer.Toc();
            logprint(tag() + ": thread " + ToString(thread) + " time = " + ToString(thread_timer.Duration()) + "; job_count = " + ToString(job_end - job_begin)+ "." );
        },
        thread_count
    );
    
    KeySet_T survivors = std::move(thread_survivors[0]);
    
    for( Size_T thread = 1; thread < thread_count; ++thread )
    {
        survivors.merge(thread_survivors[thread]);
    }
    thread_survivors = std::vector<KeySet_T>();
    
    toc("Coarse sieving");
    
    TOOLS_DUMP(survivors.size());
    
    buckets.reserve(KnotTypeCount() + Size_T(4) * survivors.size());

    const Size_T embedding_trials = ToSize_T(embedding_trials_);
    const Size_T rotation_trials  = ToSize_T(rotation_trials_);
        
    tic("Fine sieving");
    for( const Key_T key_0 : survivors )
    {
        PD_T pd_0 = FromKey(key_0);
        
        for( bool mirrorQ : {false,true} )
        {
            for( bool reverseQ : {false,true} )
            {
                // We collect the chirality transforms of the _original_ diagram, not the simplified one.

                PD_T pd_1 = pd_0.CachelessCopy();
                pd_1.ChiralityTransform(mirrorQ,reverseQ);

                const Key_T key_1 = ToKey(pd_1);

                if constexpr ( debugQ ) { logvalprint("key",key_1); }

                if( !lut.contains(key_1) )
                {
                    const ID_T id = CreateBucket(key_1);
                    Generate(R,id,crossing_count,embedding_trials,rotation_trials);
                }
                else
                {
                    if constexpr ( debugQ ) { logprint("Key found in lut."); }
                }
            }
        }
    }
    toc("Fine sieving");
    
    if( !BucketsOkayQ() )
    {
        eprint(tag() + ": Buckets are corrupted.");
        failedQ = true;
        return;
    }
    
    plantri_loadedQ = true;
    
} // LoadPlantriPDCodes
