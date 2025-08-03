template<typename T>
struct hash
{
    inline Size_T operator()( cref<T> x_0 ) const
    {
        Size_T x = static_cast<Size_T>(x_0);
        x = (x ^ (x >> 30)) * Size_T(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * Size_T(0x94d049bb133111eb);
        x =  x ^ (x >> 31);
        return static_cast<Size_T>(x);
    }
};

template<typename T, typename I>
struct Tensor1Hash
{
    inline Size_T operator()( cref<Tensor1<T,I>> v )  const
    {
        using namespace std;
        
        Size_T seed = 0;
        
        const I n = v.Size();
        
        for( I i = 0; i < n; ++i )
        {
            Tools::hash_combine(seed,v[i]);
        }
        
        return seed;
    }
};

//using CodeInt   = UInt8;
using CodeInt      = ToUnsigned<Int>;
using Code_T       = Tensor1<CodeInt,Int>;
using Hash_T       = Tensor1Hash<CodeInt,Int>;
using CodeSet_T    = std::unordered_set<Code_T,Hash_T>;
//using CodeCounts_T = std::unordered_map<Code_T,Size_T,Hash_T>;

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
    
    Tensor2<CodeInt,Size_T> result (
        code_set.size(), 2 * collection_threshold, PD_T::UninitializedIndex()
    );
    
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
    
    if ( !(permute_randomQ || ortho_draw_settings.randomizeQ) )
    {
        std::tie(comp_ptr,x) = Embedding(pd).Disband();
        L = Link_T( comp_ptr );
    }
    
    for( Int branch = 0; branch < branch_count; ++branch )
    {
        if ( permute_randomQ || ortho_draw_settings.randomizeQ )
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
        output_ptr.Last(), Int(2) * collection_threshold,
        static_cast<CodeInt>(PD_T::Uninitialized)
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
        code_set.size(), 2 * collection_threshold, PD_T::UninitializedIndex()
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
        
        if ( !(permute_randomQ || ortho_draw_settings.randomizeQ) )
        {
            std::tie(comp_ptr,x) = Embedding(pd).Disband();
            L = Link_T( comp_ptr );
        }
        
        for( Int attempt = 0; attempt < attempt_count; ++attempt )
        {
            if ( permute_randomQ || ortho_draw_settings.randomizeQ )
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
//    TOOLS_DUMP(ortho_draw_settings.randomizeQ);
    
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
        output_ptr.Last(), Int(2) * collection_threshold,
        static_cast<CodeInt>(PD_T::Uninitialized)
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



private:

void Generate3_impl(
    CodeSet_T & code_set,
    const Int   output_code_length,
    const Int   permutation_count,
    const Int   rotation_count
)
{
    std::vector<Code_T> stack;
    
    auto conditional_push = [&stack, &code_set, output_code_length]
    ( Code_T && code )
    {
        if ( code.Size() <= output_code_length )
        {
            if( code_set.count(code) == Size_T(0) )
            {
                code_set.insert(code);
                stack.push_back(std::move(code));
            }
        }
    };
    
    // For iteratation over a std::unordered_set can only be done with const iterators. Thus, we have to do copies here. Should be no problem in general.
    for( const Code_T & code : code_set )
    {
        stack.push_back(code);
    }
    
    while( !stack.empty() )
    {
        Code_T code = std::move(stack.back());
        stack.pop_back();
        
        PD_T pd = PD_T::FromMacLeodCode(
            code.data(),
            code.Size(),
            Int(0),true,false
        );
        
        Tensor1<Int,Int> comp_ptr;
        Tensor1<Point_T,Int> x;
        Link_T L;
                
        for( Int perm = 0; perm < permutation_count; ++perm )
        {
            std::tie(comp_ptr,x) = Embedding(pd).Disband();
            L = Link_T(comp_ptr);
            
            for( Int rot = 0; rot < rotation_count; ++rot )
            {
                L.SetTransformationMatrix( RandomRotation() );
                L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
                int flag = L.FindIntersections();
                
                if( flag != 0 )
                {
                    wprint(MethodName("Generate3_impl") + ": Link_2D::FindIntersections returned status flag " + ToString(flag) + " != 0. Skipping result.");
                    continue;
                }
                
                PD_T pd_mixed(L);
                pd_mixed.Simplify4();
                conditional_push( pd_mixed.template MacLeodCode<CodeInt>() );
            }
        }
    }
}


public:

template<typename ExtInt>
std::tuple<Tensor1<Size_T,Size_T>,Tensor2<CodeInt,Size_T>>
GenerateMany3(
    cptr<ExtInt> input_codes,
    const Size_T input_code_count,
    const Int    input_crossing_count,
    const Int    output_crossing_count,
    const Int    permutation_count,
    const Int    rotation_count,
    const Size_T thread_count
)
{
    static_assert(IntQ<ExtInt>,"");
 
    TOOLS_PTIMER(timer,MethodName("GenerateMany3"));
    
//    TOOLS_DUMP(input_code_count);
//    TOOLS_DUMP(input_crossing_count);
//    TOOLS_DUMP(thread_count);
//    TOOLS_DUMP(output_crossing_count);
//    TOOLS_DUMP(permutation_count);
//    TOOLS_DUMP(rotation_count);
    
    const Int input_code_length  = Int(2) * input_crossing_count;
    const Int output_code_length = Int(2) * output_crossing_count;
    
    JobPointers<Size_T> job_ptr ( input_code_count, thread_count );
               
    std::mutex mutex;
    std::vector<std::vector<CodeSet_T>> thread_code_sets ( thread_count );
 
    Tensor1<Size_T,Size_T> output_ptr ( input_code_count + 1 );
    output_ptr[0] = 0;
    
    ParallelDo(
        [
            &thread_code_sets, &output_ptr, &mutex, &job_ptr,
             this, input_codes, input_code_length, output_code_length,
             permutation_count, rotation_count
        ]( const Size_T thread )
        {
            Reapr reapr = *this;
            reapr.Reseed();         // This is crucial!
            
            const Size_T job_begin = job_ptr[thread    ];
            const Size_T job_end   = job_ptr[thread + 1];
            
            std::vector<CodeSet_T> local_code_sets;
            
            for( Size_T job = job_begin; job < job_end; ++job )
            {
                CodeSet_T job_code_set;

                job_code_set.insert(
                    Code_T(&input_codes[input_code_length * job],input_code_length)
                );
                
                Generate3_impl(
                    job_code_set,
                    output_code_length,
                    permutation_count,
                    rotation_count
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
        output_ptr.Last(), output_code_length,
        static_cast<CodeInt>(PD_T::Uninitialized)
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
