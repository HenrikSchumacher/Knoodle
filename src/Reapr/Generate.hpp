//template<typename T>
//struct hash
//{
//    inline Size_T operator()( cref<T> x_0 ) const
//    {
//        Size_T x = static_cast<Size_T>(x_0);
//        x = (x ^ (x >> 30)) * Size_T(0xbf58476d1ce4e5b9);
//        x = (x ^ (x >> 27)) * Size_T(0x94d049bb133111eb);
//        x =  x ^ (x >> 31);
//        return static_cast<Size_T>(x);
//    }
//};

template<typename T, typename I>
struct Tensor1Hash
{
    inline Size_T operator()( cref<Tensor1<T,I>> v )  const
    {
        using is_avalanching [[maybe_unused]] = std::true_type; // instruct Boost.Unordered to not use post-mixing
        
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
//using CodeSet_T    = std::unordered_set<Code_T,Hash_T>;
using CodeSet_T    = boost::unordered_flat_set<Code_T,Hash_T>;

// 0 can never occurs as entry of a MacLeodCode, so we can use it as filler.
static constexpr CodeInt code_filler = CodeInt(0);

//using CodeCounts_T = std::unordered_map<Code_T,Size_T,Hash_T>;



private:

void Generate_impl(
    CodeSet_T & code_set,
    const Int   output_n,
    const Int   permutation_count,
    const Int   rotation_count
)
{
    std::vector<Code_T> stack;
    
    auto conditional_push = [&stack, &code_set, output_n]
    ( Code_T && code )
    {
        if ( code.Size() <= output_n )
        {
            if( code_set.count(code) == Size_T(0) )
            {
                code_set.insert(code);
                stack.push_back(std::move(code));
            }
        }
    };
    
    // For-iteration over a std::unordered_set can only be done with const iterators. Thus, we have to do copies here. Should be no problem in general.
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
                    wprint(MethodName("Generate_impl") + ": Link_2D::FindIntersections returned status flag " + ToString(flag) + " != 0. Skipping result.");
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

Tensor2<CodeInt,Size_T> Generate(
    mref<PlanarDiagram<Int>> pd,
    const Int  output_n,              // max number of crossings of outputs
    const Int  permutation_count,
    const Int  rotation_count
)
{
    TOOLS_PTIMER(timer,MethodName("Generate"));

    CodeSet_T code_set;
    
    if( pd.CrossingCount() <= output_n )
    {
        code_set.insert( pd.template MacLeodCode<CodeInt>() );
    }
    
    Generate_impl(
        code_set,
        output_n,
        permutation_count,
        rotation_count
    );
    
    Tensor2<CodeInt,Size_T> result ( code_set.size(), output_n, code_filler );
    
    Size_T i = 0;
    
    for( auto & v : code_set )
    {
        v.Write( result.data(i) );
        ++i;
    }
    
    return result;
}

template<typename ExtInt>
std::tuple<Tensor1<Size_T,Size_T>,Tensor2<CodeInt,Size_T>>
GenerateMany(
    cptr<ExtInt> input_codes,
    const Size_T input_count,           // number of codes in input
    const Int    input_n,               // exact number of crossings of inputs
    const Size_T thread_count,
              
    const Int    output_n,              // max number of crossings of outputs
    const Int    permutation_count,
    const Int    rotation_count
)
{
    static_assert(IntQ<ExtInt>,"");
 
    TOOLS_PTIMER(timer,MethodName("GenerateMany"));
    
//    TOOLS_DUMP(input_count);
//    TOOLS_DUMP(input_n);
//    TOOLS_DUMP(thread_count);
//    TOOLS_DUMP(output_n);
//    TOOLS_DUMP(permutation_count);
//    TOOLS_DUMP(rotation_count);
    
    JobPointers<Size_T> job_ptr ( input_count, thread_count );
               
    std::mutex mutex;
    std::vector<std::vector<CodeSet_T>> thread_code_sets ( thread_count );
 
    Tensor1<Size_T,Size_T> output_ptr ( input_count + 1 );
    output_ptr[0] = 0;
    
    ParallelDo(
        [
            &thread_code_sets, &output_ptr, &mutex, &job_ptr,
             this, input_codes, input_n, output_n,
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

                job_code_set.insert( Code_T(&input_codes[input_n * job],input_n) );
                
                Generate_impl(
                    job_code_set,
                    output_n,
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
    
    Tensor2<CodeInt,Size_T> output_matrix ( output_ptr.Last(), output_n, code_filler );

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
