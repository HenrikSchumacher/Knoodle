std::pair<Size_T,Tensor2<CodeInt,Size_T>> Generate(
    mref<PlanarDiagram<Int>> pd,
    const Int  collection_threshold,
    const Int  simplification_threshold,
    const Int  branch_count,
    const Int  depth_count
)
{
    TOOLS_PTIMER(timer,MethodName("Generate"));

    CodeSet_T code_set;
    
    if( pd.CrossingCount() <= collection_threshold )
    {
        code_set.insert( pd.template MacLeodCode<CodeInt>() );
    }
    
    Size_T iter_count = Generate_Recursive(
        code_set,
        pd,
        collection_threshold,
        simplification_threshold,
        branch_count,
        Int(0),
        depth_count
    );
    
    Tensor2<CodeInt,Size_T> result ( code_set.size(), collection_threshold, code_filler );
    
    Size_T i = 0;
    
    for( auto & v : code_set )
    {
        v.Write( result.data(i) );
        ++i;
    }
    
    return std::pair(iter_count,result);
}

private:

Size_T Generate_Recursive(
    mref<CodeSet_T> code_set,
    cref<PlanarDiagram<Int>> pd,
    const Int    collection_threshold,
    const Int    simplification_threshold,
    const Int    branch_count,
    const Int    depth,
    const Int    depth_count
)
{
    Size_T iter_counter = 0;
    
    if( depth >= depth_count )
    {
        return iter_counter;
    }
    
    Tensor1<Int,Int> comp_ptr;
    Tensor1<Point_T,Int> x;
    Link_T L;
    
    const bool randomizeQ = (
        permute_randomQ
        ||
        ortho_draw_settings.randomize_virtual_edgesQ
        ||
        (ortho_draw_settings.randomize_bends > 0)
    );
    
    if ( !randomizeQ )
    {
        std::tie(comp_ptr,x) = Embedding(pd).Disband();
        L = Link_T( comp_ptr );
    }
    
    for( Int branch = 0; branch < branch_count; ++branch )
    {
        if ( randomizeQ )
        {
            std::tie(comp_ptr,x) = Embedding(pd).Disband();
            L = Link_T( comp_ptr );
        }

        L.SetTransformationMatrix( RandomRotation() );
        L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
        
        PD_T pd_mixed;
        
        int flag = L.FindIntersections();
        
        if( flag != 0 )
        {
            wprint(MethodName("Generate_Recursive") + ": Link_2D::FindIntersections returned status flag " + ToString(flag) + " != 0. Skipping result.");
            pd_mixed = pd;
        }
        else
        {
            ++iter_counter;
            pd_mixed = PD_T(L);
        }
        
        if ( pd_mixed.CrossingCount() > simplification_threshold )
        {
            pd_mixed.Simplify4();
            if( pd_mixed.CrossingCount() <= collection_threshold )
            {
                code_set.insert( pd_mixed.template MacLeodCode<CodeInt>() );
            }
        }
        else
        {
            PD_T pd_simplified = pd_mixed;
            pd_simplified.Simplify4();
            if( pd_simplified.CrossingCount() <= collection_threshold )
            {
                code_set.insert( pd_simplified.template MacLeodCode<CodeInt>() );
            }
        }
        
        iter_counter += Generate_Recursive(
            code_set,
            pd_mixed,
            collection_threshold,
            simplification_threshold,
            branch_count,
            depth + Int(1),
            depth_count
        );
    }
    
    return iter_counter;
}

public:

template<typename ExtInt>
std::tuple<Tensor1<Size_T,Size_T>,Tensor2<CodeInt,Size_T>>
GenerateMany(
    cptr<ExtInt> input_codes,
    const Size_T input_code_count,
    const Int    input_code_length,
    const Size_T thread_count,
    const Int    collection_threshold,
    const Int    simplification_threshold,
    const Int    branch_count,
    const Int    depth_count
)
{
    static_assert(IntQ<ExtInt>,"");
    
    TOOLS_PTIMER(timer,MethodName("GenerateMany"));
    
    JobPointers<Size_T> job_ptr ( input_code_count, thread_count );
               
    std::mutex mutex;
    std::vector<std::vector<CodeSet_T>> thread_code_sets ( thread_count );
 
    Tensor1<Size_T,Size_T> output_ptr ( input_code_count + 1 );
    output_ptr[0] = 0;
    
    ParallelDo(
        [
            &thread_code_sets, &output_ptr, &mutex, &job_ptr,
             this, input_codes, input_code_length,
             collection_threshold, simplification_threshold, branch_count, depth_count
        ]( const Size_T thread )
        {
            Reapr reapr = *this;
            reapr.Reseed();         // This is crucial!
            
            const Size_T job_begin = job_ptr[thread    ];
            const Size_T job_end   = job_ptr[thread + 1];
            
            std::vector<CodeSet_T> local_code_sets;
            
            for( Size_T job = job_begin; job < job_end; ++job )
            {
                PD_T pd = PD_T::FromMacLeodCode(
                    &input_codes[input_code_length * job],
                    input_code_length,
                    Int(0),true,false
                );
                
                CodeSet_T job_code_set;
                
                if( pd.CrossingCount() <= input_code_length )
                {
                    job_code_set.insert( pd.template MacLeodCode<CodeInt>() );
                }
                
                Generate_Recursive(
                    job_code_set,
                    pd,
                    collection_threshold,
                    simplification_threshold,
                    branch_count,
                    Int(0),
                    depth_count
                );
                
                output_ptr[job+Size_T(1)] = job_code_set.size();
                local_code_sets.push_back( std::move(job_code_set) );
            }
            
            std::lock_guard g ( mutex );
            thread_code_sets[thread] = std::move( local_code_sets );
        },
        thread_count
    );
    
    output_ptr.Accumulate();
    
    Tensor2<CodeInt,Size_T> output_matrix (
        output_ptr.Last(), Int(2) * collection_threshold, code_filler
    );

    ParallelDo(
        [
            &thread_code_sets, &job_ptr, &output_ptr, &output_matrix
        ]( const Size_T thread )
        {
            const Size_T job_begin = job_ptr[thread  ];
            const Size_T job_end   = job_ptr[thread+1];
            
            cref<std::vector<CodeSet_T>> local_code_sets = thread_code_sets[thread];
            
            Size_T k = 0;

            for( Size_T job = job_begin; job < job_end; ++job )
            {
                cref<CodeSet_T> code_set = local_code_sets[k];
                
                Size_T row = output_ptr[job];
                for( auto & code : code_set )
                {
                    code.Write( output_matrix.data(row) );
                    ++row;
                }
                
                ++k;
            }
            
        },
        thread_count
    );
    
    return std::tuple( output_ptr, output_matrix );
}



public:


Tensor2<CodeInt,Size_T> Generate2(
    mref<PlanarDiagram<Int>> pd,
    const Int  collection_threshold,
    const Int  simplification_threshold,
    const Int  attempt_count
)
{
    TOOLS_PTIMER(timer,MethodName("Generate2"));

    CodeSet_T code_set;
    
    Generate2_impl(
        code_set,
        pd,
        collection_threshold,
        simplification_threshold,
        attempt_count
    );
    
    Tensor2<CodeInt,Size_T> result (
        code_set.size(), collection_threshold, code_filler
    );
    
    Size_T i = 0;
    
    for( auto & v : code_set )
    {
        v.Write( result.data(i) );
        ++i;
    }
    
    return result;
}

private:

void Generate2_impl(
    mref<CodeSet_T> code_set,
    cref<PlanarDiagram<Int>> pd_0,
    const Int    collection_threshold,
    const Int    simplification_threshold,
    const Int    attempt_count
)
{
    std::vector<Code_T> stack;
    
    auto conditional_push = [&stack, &code_set,collection_threshold]
    ( cref<PD_T> pd )
    {
        if ( pd.CrossingCount() <= collection_threshold )
        {
            auto code = pd.template MacLeodCode<CodeInt>();
         
            if( code_set.count(code) == Size_T(0) )
            {
                code_set.insert(code);
                stack.push_back( std::move(code));
            }
        }
    };
    
    conditional_push(pd_0);
    
    while( !stack.empty() )
    {
        PD_T pd = PD_T::FromMacLeodCode(
            stack.back().data(),
            stack.back().Size(),
            Int(0),true,false
        );
        
        stack.pop_back();
        
        Tensor1<Int,Int> comp_ptr;
        Tensor1<Point_T,Int> x;
        Link_T L;
        
        const bool randomizeQ = (
            permute_randomQ
            ||
            ortho_draw_settings.randomize_virtual_edgesQ
            ||
            (ortho_draw_settings.randomize_bends > 0)
        );
        
        if ( !randomizeQ )
        {
            std::tie(comp_ptr,x) = Embedding(pd).Disband();
            L = Link_T( comp_ptr );
        }
        
        for( Int attempt = 0; attempt < attempt_count; ++attempt )
        {
            if ( randomizeQ )
            {
                std::tie(comp_ptr,x) = Embedding(pd).Disband();
                L = Link_T( comp_ptr );
            }

            L.SetTransformationMatrix( RandomRotation() );
            L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
            
            int flag = L.FindIntersections();
            
            if( flag != 0 )
            {
                wprint(MethodName("Generate2_impl") + ": Link_2D::FindIntersections returned status flag " + ToString(flag) + " != 0. Skipping result.");
                continue;
            }

            PD_T pd_mixed(L);
            
            if ( pd_mixed.CrossingCount() > simplification_threshold )
            {
                pd_mixed.Simplify4();
            }
            
            conditional_push( pd_mixed );
        }
    }
}

public:

template<typename ExtInt>
std::tuple<Tensor1<Size_T,Size_T>,Tensor2<CodeInt,Size_T>>
GenerateMany2(
    cptr<ExtInt> input_codes,
    const Size_T input_code_count,
    const Int    input_code_length,
    const Size_T thread_count,
    const Int    collection_threshold,
    const Int    simplification_threshold,
    const Int    attempt_count
)
{
    static_assert(IntQ<ExtInt>,"");
    
    TOOLS_PTIMER(timer,MethodName("GenerateMany2"));
    
//    TOOLS_DUMP(input_code_count);
//    TOOLS_DUMP(input_code_length);
//    TOOLS_DUMP(thread_count);
//    TOOLS_DUMP(collection_threshold);
//    TOOLS_DUMP(simplification_threshold);
//    TOOLS_DUMP(attempt_count);
//    TOOLS_DUMP(permute_randomQ);
//    TOOLS_DUMP(ortho_draw_settings.randomize_bends);
//    TOOLS_DUMP(ortho_draw_settings.randomize_virtual_edgesQ);
    
    JobPointers<Size_T> job_ptr ( input_code_count, thread_count );
               
    std::mutex mutex;
    std::vector<std::vector<CodeSet_T>> thread_code_sets ( thread_count );
 
    Tensor1<Size_T,Size_T> output_ptr ( input_code_count + 1 );
    output_ptr[0] = 0;
    
    ParallelDo(
        [
            &thread_code_sets, &output_ptr, &mutex, &job_ptr,
             this, input_codes, input_code_length,
             collection_threshold,simplification_threshold, attempt_count
        ]( const Size_T thread )
        {
            Reapr reapr = *this;
            reapr.Reseed();         // This is crucial!
            
            const Size_T job_begin = job_ptr[thread    ];
            const Size_T job_end   = job_ptr[thread + 1];
            
            std::vector<CodeSet_T> local_code_sets;
            
            for( Size_T job = job_begin; job < job_end; ++job )
            {
                PD_T pd = PD_T::FromMacLeodCode(
                    &input_codes[input_code_length * job],
                    input_code_length,
                    Int(0),true,false
                );
                
                CodeSet_T job_code_set;
                
                Generate2_impl(
                    job_code_set,
                    pd,
                    collection_threshold,
                    simplification_threshold,
                    attempt_count
                );
                
                output_ptr[job+Size_T(1)] = job_code_set.size();
                local_code_sets.push_back( std::move(job_code_set) );
            }
            
            std::lock_guard g ( mutex );
            thread_code_sets[thread] = std::move( local_code_sets );
        },
        thread_count
    );
    
    output_ptr.Accumulate();
    
    Tensor2<CodeInt,Size_T> output_matrix (
        output_ptr.Last(), Int(2) * collection_threshold, code_filler
    );

    ParallelDo(
        [
            &thread_code_sets, &job_ptr, &output_ptr, &output_matrix
        ]( const Size_T thread )
        {
            const Size_T job_begin = job_ptr[thread  ];
            const Size_T job_end   = job_ptr[thread+1];
            
            cref<std::vector<CodeSet_T>> local_code_sets = thread_code_sets[thread];
            
            Size_T k = 0;

            for( Size_T job = job_begin; job < job_end; ++job )
            {
                cref<CodeSet_T> code_set = local_code_sets[k];
                
                Size_T row = output_ptr[job];
                for( auto & code : code_set )
                {
                    code.Write( output_matrix.data(row) );
                    ++row;
                }
                
                ++k;
            }
            
        },
        thread_count
    );
    
    return std::tuple( output_ptr, output_matrix );
}

