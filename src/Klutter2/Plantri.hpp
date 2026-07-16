public:

Path_T PlantriPDCodeFile() const
{
    return working_directory / ("PlantriPDCodes_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".txt");
}

//template<IntQ ExtInt>
void LoadPlantriPDCodes()
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
        wprint(tag() + ": Plantri data has already been loaded. Aborting.");
        return;
    }
    
    if( failedQ )
    {
        eprint(tag() + ": Flag failedQ is set to true. Aborting.");
        return;
    }
    
    Tensor3<Int,Int> input;
    Size_T input_count = 0;
    
    {
        Path_T pd_code_file = PlantriPDCodeFile();
        print(tag() + ": Loading plantri's pd codes from file \"" + pd_code_file.c_str() + "\"." );
        
        Tools::InString s (pd_code_file);

        if( s.FailedQ() )
        {
            eprint(tag() + ": Failed opening file  " + pd_code_file.string() + " .");
            failedQ = true;
            return;
        }

        if( s.EmptyQ() )
        {
            eprint(tag() + ": File  " + pd_code_file.string() + " is empty.");
            failedQ = true;
            return;
        }
        
        // s.LineCount() upper bound on (number of inputs) * crossing_count;
        input = Tensor3<Int,Int>( s.LineCount() / crossing_count, crossing_count, Int(4) );

        Tensor2<Int,Int> pd_code ( crossing_count, Int(4) );
        
        tic("Reading inputs");
        while( !s.EmptyQ() && !s.FailedQ() )
        {
            s.TakeMatrixFunction(pd_code.WriteAccess(),crossing_count,Int(4), "","\n","\n", ""," ","");
            if( s.FailedQ() )
            {
                wprint(tag() + ": Reading pd code no. " + ToString(input_count) + " from file \"" + pd_code_file.c_str() + "\" failed. Current position = " + ToString(s.Position()) + "; current character code = " + ToString(static_cast<int>(s.CurrentChar())) + "."
                );
                return;
            }
            pd_code.Write(input.data(input_count));
            ++input_count;
            
            if( s.EmptyQ() ) { break; }
            
            s.SkipChar('\n');
        }
        toc("Reading inputs");
        
        if( s.FailedQ() )
        {
            eprint(tag() + ": Reading inputs failed.");
            failedQ = true;
            return;
        }
    }
    
    using SurvivorSet_T = SetContainer<std::pair<Key_T,Invariant_T>>;
    
    std::vector<SurvivorSet_T> thread_survivors (thread_count);
    
    // The plantri codes are sorted in a way that many simple diagrams come first and many difficult come last.
    // For the sake of load balancing, we randomly permute the inputs during reading.
    
    auto perm = Permutation<Size_T,Sequential>::RandomPermutation(
        input_count, Size_T(1), reaprs.front().RandomEngine()
    );
    
    tic("Coarse sieving");
    ParallelDo(
        [&thread_survivors, &tag, &perm, &input, input_count, this]( Size_T thread )
        {
            TimeInterval thread_timer;
            thread_timer.Tic();
            
            Reapr_T reapr = reaprs[thread];
            Classifier classifier;
            SurvivorSet_T survivors;
            
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
                
                if( pd_0.LinkComponentCount() > Int(1) ) { continue; }
                
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
                    if( (pdc.DiagramCount() > Int(1)) || (pdc[0].CrossingCount() != crossing_count) )
                    {
                        continue;
                    }
                    
                    for( bool mirrorQ : {false,true} )
                    {
                        for( bool reverseQ : {false,true} )
                        {
                            // We collect the chirality transforms of the _original_ diagram, not the simplified one.
            
                            PD_T pd_1 = pdc[0].CachelessCopy();
                            pd_1.ChiralityTransform(mirrorQ,reverseQ);
            
                            const Invariant_T invariant = classifier(pd_1);

                            survivors.insert({ToKey(pd_1),invariant});
                        }
                    }
                }
                
            } // for( Size_T job = job_begin; job < job_end; ++job )
            
            thread_survivors[thread] = std::move(survivors);
            
            thread_timer.Toc();
            logprint(tag() + ": thread " + ToString(thread) + " time = " + ToString(thread_timer.Duration()) + "; job_count = " + ToString(job_end - job_begin)+ "." );
        },
        thread_count
    );
    toc("Coarse sieving");
    
    tic("Filling subklutters");
    for( Size_T thread = 0; thread < thread_count; ++thread )
    {
        for( auto & [key,invariant] : thread_survivors[thread] )
        {
            if( I_S.contains(invariant) )
            {
                subklutters[I_S[invariant]].InsertUnidentifiedKey(key);
            }
        }
    }
    thread_survivors = std::vector<SurvivorSet_T>();
    toc("Filling subklutters");
    
    plantri_loadedQ = true;
    
} // LoadPlantriPDCodes
