#define TOOLS_AGGRESSIVE_INLINING

#include "../Knoodle.hpp"

#include "../submodules/Tensors/submodules/Tools/src/ToString_Obsolete.hpp"

using namespace Knoodle;
using namespace Tools;

using Int   = Size_T;
using UInt  = UInt64;
using Real  = Real64;
//using BReal = Real64;
using BReal = Real32;

using T = Int;
//using T = Real;


using Dist_T = std::conditional_t<IntQ<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

int main()
{
    
//    const char prefix_0 [3] = "{\n";
//    const char infix_0  [3] = ",\n";
//    const char suffix_0 [3] = "\n}";
//    const char prefix_1 [4] = "\t{\n";
//    const char infix_1  [3] = ",\n";
//    const char suffix_1 [4] = "\n\t}";
//    const char prefix_2 [5] = "\t\t{ ";
//    const char infix_2  [3] = ", ";
//    const char suffix_2 [3] = " }";
    
//    const char prefix_0 [3] = "{\n";
//    const char infix_0  [3] = ",\n";
//    const char suffix_0 [3] = "\n}";
//    const char prefix_1 [4] = " { ";
//    const char infix_1  [3] = ", ";
//    const char suffix_1 [3] = " }";
//    const char prefix_2 [3] = "{ ";
//    const char infix_2  [3] = ", ";
//    const char suffix_2 [3] = " }";
    
//    std::string s_prefix_0 (prefix_0);
//    std::string s_infix_0  (infix_0);
//    std::string s_suffix_0 (suffix_0);
//    
//    std::string s_prefix_1 (prefix_1);
//    std::string s_infix_1  (infix_1);
//    std::string s_suffix_1 (suffix_1);
//    
//    std::string s_prefix_2 (prefix_2);
//    std::string s_infix_2  (infix_2);
//    std::string s_suffix_2 (suffix_2);
//    
//    TOOLS_DUMP(s_prefix_0);
//    TOOLS_DUMP(s_prefix_0.size());
    
    const T min_value = COND(IntQ<T>, T(0), T(-1.));
    const T max_value = COND(IntQ<T>, T(1000), T(1.));
    Dist_T dist (min_value,max_value);
    
    PRNG_T random_engine = InitializedRandomEngine<PRNG_T>();

    
//    Size_T d_0 = 10;
    Size_T d_0 = 2'000'000;
    Size_T d_1 = 2;
    Size_T d_2 = 2;
    
    std::filesystem::path path ("/Volumes/RamDisk");

    Tensor3<T,Size_T> T3 (d_0,d_1,d_2);

    for( Size_T i = 0; i < d_0; ++i )
    {
        for( Size_T j = 0; j < d_1; ++j )
        {
            for( Size_T k = 0; k < d_2; ++k )
            {
                T3(i,j,k) = dist( random_engine );
            }
        }
    }
    
    Tensor2<T,Size_T> T2 (d_0,d_1);

    for( Size_T i = 0; i < d_0; ++i )
    {
        for( Size_T j = 0; j < d_1; ++j )
        {
            T2(i,j) = dist( random_engine );
        }
    }
    
    Tensor1<T,Size_T> T1 (d_0);

    for( Size_T i = 0; i < d_0; ++i )
    {
        T1(i) = dist( random_engine );
    }
    
    tic("Tensor3_OutString");
    {
        std::ofstream stream ( path / "Tensor3_OutString.txt" );
        stream << T3;
    }
    toc("Tensor3_OutString");
    tic("Tensor3_ArrayToString");
    {
        std::ofstream stream ( path / "Tensor3_ArrayToString.txt" );
        stream << ArrayToString(T3.data(),{T3.Dim(0),T3.Dim(1),T3.Dim(2)});
    }
    toc("Tensor3_ArrayToString");
    
    tic("Tensor2_OutString");
    {
        std::ofstream stream ( path / "Tensor2_OutString.txt" );
        stream << T2;
    }
    toc("Tensor2_OutString");
    tic("Tensor2_ArrayToString");
    {
        std::ofstream stream ( path / "Tensor2_ArrayToString.txt" );
        stream << ArrayToString(T2.data(),{T2.Dim(0),T2.Dim(1)});
    }
    toc("Tensor2_ArrayToString");
    
    tic("Tensor1_OutString");
    {
        std::ofstream stream ( path / "Tensor1_OutString.txt" );
        stream << T1;
    }
    toc("Tensor1_OutString");
    tic("Tensor1_ArrayToString");
    {
        std::ofstream stream ( path / "Tensor1_ArrayToString.txt" );
        stream << ArrayToString(T1.data(),{T1.Dim(0)});
    }
    toc("Tensor1_ArrayToString");
    
    
    tic("Tensor3_OutString");
    {
        std::ofstream stream ( path / "Tensor3_OutString.txt" );
        stream << T3;
    }
    toc("Tensor3_OutString");
    tic("Tensor3_ArrayToString");
    {
        std::ofstream stream ( path / "Tensor3_ArrayToString.txt" );
        stream << ArrayToString(T3.data(),{T3.Dim(0),T3.Dim(1),T3.Dim(2)});
    }
    toc("Tensor3_ArrayToString");
    
    tic("Tensor2_OutString");
    {
        std::ofstream stream ( path / "Tensor2_OutString.txt" );
        stream << T2;
    }
    toc("Tensor2_OutString");
    tic("Tensor2_ArrayToString");
    {
        std::ofstream stream ( path / "Tensor2_ArrayToString.txt" );
        stream << ArrayToString(T2.data(),{T2.Dim(0),T2.Dim(1)});
    }
    toc("Tensor2_ArrayToString");
    
    tic("Tensor1_OutString");
    {
        std::ofstream stream ( path / "Tensor1_OutString.txt" );
        stream << T1;
    }
    toc("Tensor1_OutString");
    tic("Tensor1_ArrayToString");
    {
        std::ofstream stream ( path / "Tensor1_ArrayToString.txt" );
        stream << ArrayToString(T1.data(),{T1.Dim(0)});
    }
    toc("Tensor1_ArrayToString");
    
    
//
//    {
//        
//        std::ofstream stream ( path / "c.txt" );
//        tic("c");
//        Tools::OutString s ( full_size );
//        
//        s.PutMatrix( a.data(), m, n, prefix_0, infix_0, suffix_0, prefix_1, infix_1, suffix_1, false );
//        stream << s.View();
//        toc("c");
//    }
//    
//    
//    Tensor2<T,Size_T> b(m,n);
//    
//    
//    {
//        std::filesystem::path file (path / "d.txt");
//        // Open the stream to 'lock' the file.
//        std::ifstream f(file, std::ios::in | std::ios::binary);
//
//        // Obtain the size of the file.
//        const auto sz = std::filesystem::file_size(file);
//        // Create a buffer.
//        std::string input (sz, '\0');
//        // Read the whole file into the buffer.
//        f.read(input.data(), static_cast<std::streamsize>(sz));
//        
//        tic("Read d");
//        Tools::InString s (input);
//
//        s.TakeMatrixFunction(
//            [&b](const Size_T i, const Size_T j) -> T& { return b(i,j); },
//            m, n, prefix_0, infix_0, suffix_0, prefix_1, infix_1, suffix_1
//        );
//        toc("Read d");
//    }
//    
//    TOOLS_DUMP(b.MinMax());
//    
////    b -= a;
//    for( Size_T i = 0; i < m; ++i )
//    {
//        for( Size_T j = 0; j < n; ++j )
//        {
//            b(i,j) -= a(i,j);
//        }
//    }
//    
//    TOOLS_DUMP(b.MinMax());
    
    
    Tiny::Matrix<3,4,T,Int> A ;
    for( Int i = 0; i < 3; ++i )
    {
        for( Int j = 0; j < 4; ++j )
        {
            A[i][j] = dist(random_engine);
        }
    }
    
    valprint("A[0][0]",ToString(A[0][0]));
    
    print(ToString(A));
    
    Aggregator<T,Int> agg;
    
    agg.Push( T(1) );
    agg.Push( T(2) );
    
    print(ToString(agg.data()[0]));
    print(ToString(agg.data()[1]));
    
    print(std::string_view( OutString::FromVector( agg.data(), agg.Size() ) ));
    
    
    tic("logvalprint");
    for( Size_T i = 0; i < d_0; ++i )
    {
        logvalprint( std::string("T1(") + ToString(i) + ") = ", T1(i));
    }
    toc("logvalprint");
    
    
    tic("logvalprint2");
    {
        std::ofstream stream ( path / "log2.txt" );
        
        std::mutex stream_mutex;
        
//        stream << ArrayToString(T1.data(),{T1.Dim(0)});
        
        for( Size_T i = 0; i < d_0; ++i )
        {
            std::lock_guard guard ( stream_mutex );
            Profiler::log << "T1(";
            Profiler::log << ToString(i);
            Profiler::log << ") = ";
            Profiler::log << ToString(T1(i));
            Profiler::log << "\n";
            Profiler::log << std::endl;
        }
    }
    toc("logvalprint2");
    
}
