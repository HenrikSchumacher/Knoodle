#include "../Knoodle.hpp"

using namespace Knoodle;

using Int  = Size_T;
using UInt = UInt64;
using Real = Real64;

using T = Real;

struct V
{
    std::array<T,8> a;
    
    V( T b_0, T b_1, T b_2, T b_3, T b_4, T b_5, T b_6, T b_7 )
    :   a {{ b_0, b_1, b_2, b_3, b_4, b_5, b_6, b_7 }}
    {}
    
    V()
    :   a {{}}
    {}
    
    T operator[]( std::size_t i ) const
    {
        return a[i];
    }
    
    T & operator[]( std::size_t i )
    {
        return a[i];
    }
};

using Rand_T = PRNG_T;
//using Rand_T = std::mt19937_64;


//template<typename f_T>
//void F( mptr<T> a, Int m, mref<std::array<Rand_T,8>> R, f_T & f )
//{
//    for( Int i = 0; i < 8 * m; i += 8 )
//    {
//        a[i + 0] = f(i,0);
//        a[i + 1] = f(i,1);
//        a[i + 2] = f(i,2);
//        a[i + 3] = f(i,3);
//        a[i + 4] = f(i,4);
//        a[i + 5] = f(i,5);
//        a[i + 6] = f(i,6);
//        a[i + 7] = f(i,7);
//    }
//}

//template<typename f_T>
//void F( mptr<T> a, Int m, mref<std::array<Rand_T,8>> R, f_T & f )
//{
//    T * begin = static_cast<T *>(__builtin_assume_aligned(a        , alignof(decltype(*a))));
//    T * end   = static_cast<T *>(__builtin_assume_aligned(&a[8 * m], alignof(decltype(*a))));
//    for( T * p = begin; p != end; p += 8 )
//    {
//        p[0] = f(i,0);
//        p[1] = f(i,1);
//        p[2] = f(i,2);
//        p[3] = f(i,3);
//        p[4] = f(i,4);
//        p[5] = f(i,5);
//        p[6] = f(i,6);
//        p[7] = f(i,7);
//    }
//}

template<typename f_T>
void G( mref<Tensor1<V,Int>> a, Int m, mref<std::array<Rand_T,8>> R, f_T & f )
{
    for( Int i = 0; i < m; ++i )
    {
        a[i][0] = f(i,0);
        a[i][1] = f(i,1);
        a[i][2] = f(i,2);
        a[i][3] = f(i,3);
        a[i][4] = f(i,4);
        a[i][5] = f(i,5);
        a[i][6] = f(i,6);
        a[i][7] = f(i,7);
    }
}

int main()
{
    const Int m = 10'000'000;

//    Rand_T r = InitializedRandomEngine<Rand_T>();
    
    std::array<Rand_T,8> R =  {
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>(),
        InitializedRandomEngine<Rand_T>()
    };
    
//    std::uniform_int_distribution<T> f (T(0),T(1000));
    std::uniform_real_distribution<T> dist (T(0),T(1000));
    
    auto f = [&dist,&R]( const Int i, const Int j )
    {
        (void)i;
        return dist(R[j]);
    };
    
    
    Int reps = 10;
    
    tic("push_back to x");
    std::vector<V> x;
    x.reserve(m);
    for( Int rep = 0; rep < reps; ++rep )
    {
        x.clear();
        for( Int i = 0; i < m; ++i )
        {
            x.push_back(V(f(i,0),f(i,1),f(i,2),f(i,3),f(i,4),f(i,5),f(i,6),f(i,7)));
        }
    }
    toc("push_back to x");

    print("");
    
    tic("emplace_back to y");
    std::vector<V> y;
    y.reserve(m);
    
    for( Int rep = 0; rep < reps; ++rep )
    {
        y.clear();
        for( Int i = 0; i < m; ++i )
        {
            y.emplace_back(f(i,0),f(i,1),f(i,2),f(i,3),f(i,4),f(i,5),f(i,6),f(i,7));
        }
    }
    toc("emplace_back to y");

    print("");
    
    tic("moving to z");
    std::vector<V> z;
    z.reserve(m);
    
    for( Int rep = 0; rep < reps; ++rep )
    {
        z.clear();
        for( Int i = 0; i < m; ++i )
        {
            V v (f(i,0),f(i,1),f(i,2),f(i,3),f(i,4),f(i,5),f(i,6),f(i,7));
            
            z.push_back(std::move(v));
        }
    }
    toc("moving to z");
    
    print("");

    tic("writing to x");
    for( Int rep = 0; rep < reps; ++rep )
    {
//        x.clear();
        TOOLS_DUMP(x[0][0]);
        for( size_t i = 0;  i < x.size(); ++i )
        {
            for( Int j = 0; j < 8; ++j )
            {
                x[i][j] = f(i,j);
            }
        }
    }
    toc("writing to x");
    
    print("");
    
    tic("writing to a");
    Tensor2<T,Int,64> a ( m, 8 );
    for( Int rep = 0; rep < reps; ++rep )
    {
        TOOLS_DUMP(a(0,0));
//        F(a.data(),m,R,f);
        for( Int i = 0; i != m; ++i )
        {
            mptr<T> a_ptr = a.data(i);
            a_ptr[0] = f(i,0);
            a_ptr[1] = f(i,1);
            a_ptr[2] = f(i,2);
            a_ptr[3] = f(i,3);
            a_ptr[4] = f(i,4);
            a_ptr[5] = f(i,5);
            a_ptr[6] = f(i,6);
            a_ptr[7] = f(i,7);
        }
    }
    toc("writing to a");
    
    tic("writing to a as matrix");
    for( Int rep = 0; rep < reps; ++rep )
    {
        TOOLS_DUMP(a(0,0));
        for( Int i = 0; i < m; ++i )
        {
            for( Int j = 0; j < 8; ++j )
            {
                a(i,j) = f(i,j);
            }
        }
    }
    toc("writing to a as matrix");
 }
