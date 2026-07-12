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
    max_name_size = 0;
    
    Path_T name_file = KnotInfoNameFile();
    print(tag() + ": Loading KnotInfo's knot names from file \"" + name_file.c_str() + "\"." );
    
    std::ifstream name_stream ( name_file, std::ios::binary );
    if( !name_stream )
    {
        eprint(tag() + ": Could not open " + name_file.string() +". Aborting with incomplete table." );
        return;
    }
    
    Path_T pd_code_file = KnotInfoPDCodeFile();
    print(tag() + ": Loading KnotInfo's pd codes from file \"" + ToString(pd_code_file.c_str()) + "\"." );
    
    {
        Tools::InString s (pd_code_file);
        if( s.EmptyQ() )
        {
            eprint(tag() + ": File  " + pd_code_file.string() + " is empty.");
            return;
        }
        
        Tensor2<Int,Int> pd_code ( crossing_count, Int(4) );
        Name_T name;
        Size_T input_count = 0;
        
//        tic("Reading inputs");
        while( name_stream && !s.EmptyQ() && !s.FailedQ() )
        {
            name_stream >> name;
            max_name_size = std::max(max_name_size,name.size());
            names.push_back(name);
            s.TakeMatrixFunction(pd_code.WriteAccess(),crossing_count,Int(4), "","\n","\n", ""," ","");
            
            if( s.FailedQ() )
            {
                wprint(tag() + ": Reading pd code no. " + ToString(input_count) + " from file \"" + pd_code_file.c_str() + "\" failed. Current position = " + ToString(s.Position()) + "; current character code = " + ToString(static_cast<int>(s.CurrentChar())) + "."
                );
                return;
            }
            
//            logvalprint("buckets.size()",buckets.size());
//            logvalprint("s.FailedQ()",s.FailedQ());
//            logvalprint("s.EmptyQ()",s.EmptyQ());
//            logvalprint("pd_code",pd_code);
            
            PD_T pd = PD_T::template FromPDCode<{.signQ = false,.colorQ = false}>(
                pd_code.data(), crossing_count
            );
            
            if( pd.InvalidQ() )
            {
                eprint(tag() + ": Diagram no. " + ToString(input_count) + " is invalid. Aborting."
                );
                return;
            }
            
            CreateBucket(ToKey(pd));
            ++input_count;
            
            if( s.EmptyQ() ) { break; }
            
            s.SkipChar('\n');
        }
//        toc("Reading inputs");
    }
    
    Tensor1<Generate2Flag,Size_T> flags( KnotTypeCount(), Generate2Flag::NoChange );
    
    // Randomization helps, but Round Robin might be better.;
    
    auto perm = Permutation<ID_T,Sequential>::RandomPermutation(
        ID_T(KnotTypeCount()), ID_T(1), R.RandomEngine()
    );
    
    tic("Run Generate2 on identified diagrams (parallel)");
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
    toc("Run Generate2 on identified diagrams (parallel)");
    
    
    // TODO: We could also apply the chirality transformation to the present codes and classify them accordingly.
    
    tic("Build initial lut");
    for( ID_T id = 0; id < KnotTypeCount(); ++id )
    {
        if( flags[id] == Generate2Flag::Changed )
        {
            for( const Key_T & key : buckets[id] )
            {
                lut[key] = id;
            }
        }
        else if( flags[id] == Generate2Flag::SimplerDiagramDetected )
        {
            DeleteBucket<true>(id);
            failedQ = true;
            break;
        }
        else if( flags[id] == Generate2Flag::Failure )
        {
            failedQ = true;
            break;
        }
    }
    toc("Build initial lut");
    
    if( failedQ )
    {
        eprint(tag() + ": Failed with unrecoverable error.");
        return;
    }
        
    if( !BucketsOkayQ() )
    {
        eprint(tag() + ": Buckets are corrupted.");
        failedQ = true;
        return;
    }
    
    knot_info_loadedQ = true;
}
