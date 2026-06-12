
using Code_T  = std::array<CodeInt,code_length>;
//        using Code_T  = Tensor1<CodeInt,Size_T>;

static_assert(max_crossing_count <= 64, "");
static_assert((max_crossing_count * sizeof(CodeInt)) % sizeof(Size_T) == 0, "");

struct CodeHash
{
    // instruct Boost.Unordered to not use post-mixing
    using is_avalanching [[maybe_unused]] = std::true_type;
    
    inline Size_T operator()( cref<Code_T> v )  const
    {
        using namespace std;
        
        constexpr Size_T n = code_length * sizeof(CodeInt) / sizeof(Size_T);
        
        cptr<Size_T> w = reinterpret_cast< const Size_T *>(&v[0]);
        
        Size_T seed = 0;
        
        for( Size_T i = 0; i < n; ++i )
        {
            Tools::hash_combine(seed,w[i]);
        }
        
        return seed;
    }
};

struct CodeLess
{
    inline bool operator()( cref<Code_T> v, cref<Code_T> w )  const
    {
        using namespace std;
        
        constexpr Size_T n = code_length * sizeof(CodeInt) / sizeof(Size_T);
        
        cptr<Size_T> v_ = reinterpret_cast< const Size_T *>(&v[0]);
        cptr<Size_T> w_ = reinterpret_cast< const Size_T *>(&w[0]);
        
        for( Size_T i = n; i --> Size_T(0); )
        {
            if( v_[i] < w_[i] )
            {
                return true;
            }
            else if ( v_[i] > w_[i]  )
            {
                return false;
            }
        }
        
        return false;
    }
};

// These are the containers I tried.
//        using CodeSet_T = std::set<Code_T,CodeLess>;
//        using CodeSet_T = boost::container::flat_set<Code_T,CodeLess>;
//        using CodeSet_T = boost::container::set<Code_T,CodeLess>;

//        using CodeSet_T = std::unordered_set<Code_T,CodeHash>;
//        using CodeSet_T = boost::unordered_set<Code_T,CodeHash>;
//        using CodeSet_T = boost::unordered_flat_set<Code_T,CodeHash>;

using CodeSet_T = SetContainer<Code_T,CodeHash>;

Size_T CodeSize() const
{
    return Size_T(crossing_count);
}

static Code_T Code( cref<PD_T> pd )
{
    Code_T code = { Scalar::Max<CodeInt> };
    
    pd.template WriteMacLeodCode<CodeInt>( &code[0] );
    
    return code;
}
