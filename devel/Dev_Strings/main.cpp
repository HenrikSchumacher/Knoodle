//#define TENSORS_BOUND_CHECKS
//#define TOOLS_NO_RESTRICT
//#define TOOLS_NO_INT128

#define TOOLS_ENABLE_PROFILER

#define TOOLS_AGGRESSIVE_INLINING


#include "../../Knoodle.hpp"


using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using Int   = Int64;
using IReal = Int64;

using Link_T = LinkEmbedding<Real,Int>;

int main()
{
    Profiler::Clear();
    
    print("");
    
    {
        //    print("Hello World!");
        
//        OutString s_0 ("Aaah!");
//        OutString s = OutString::FromMisc("Hello World!"," ", 1,",",2,s_0);
        
//        print(s);
    }
    TOOLS_DUMP(Stringy<std::string>);
    TOOLS_DUMP(Stringy<std::string &>);
    TOOLS_DUMP(Stringy<const std::string &>);
    TOOLS_DUMP(Stringy<std::string &&>);
    
    TOOLS_DUMP(Stringy<OutString>);
    TOOLS_DUMP(Stringy<OutString &>);
    TOOLS_DUMP(Stringy<const OutString &>);
    TOOLS_DUMP(Stringy<OutString &&>);
    
    TOOLS_DUMP(Stringy<int>);
    TOOLS_DUMP(Stringy<int &>);
    TOOLS_DUMP(Stringy<const int &>);
    TOOLS_DUMP(Stringy<int &&>);
    
    
    Size_T reps= 1000000;
    auto engine = InitializedRandomEngine<Knoodle::PRNG_T>();
    std::uniform_real_distribution<Real> dist (-1,1);
    Tensor2<Real,Size_T> a ( reps, Size_T(4) );
    
//    std::uniform_int_distribution<Int> dist (-1,1);
//    Tensor2<Int,Size_T> a ( reps, Size_T(4) );
  
    
    for( Size_T i = 0; i < reps; ++i )
    {
        a(i,0) = dist(engine);
        a(i,1) = dist(engine);
        a(i,2) = dist(engine);
        a(i,3) = dist(engine);
    }
    
    std::ofstream file_0 (HomeDirectory() / "test_0.txt");
    file_0 << std::setprecision(17);
    
    tic("s_0");
    file_0 << "{\n";
    for( Size_T i = 0; i < reps - 1; ++i )
    {
        file_0 << " { " << a(i,0) << ", " <<  a(i,1) << ", " << a(i,2) << ", " << a(i,3) << " },\n";
    }
    file_0 << " { " << a(reps-1,0) << ", " <<  a(reps-1,1) << ", " << a(reps-1,2) << ", " << a(reps-1,3) << " }\n";
    file_0 << "}";
    toc("s_0");

    
    std::ofstream file_1 (HomeDirectory() / "test_1.txt");
    file_1 << std::setprecision(17);
    
    tic("s_1");
    for( Size_T i = 0; i < reps; ++i )
    {
        file_1 << a(i,0) << '\t' << a(i,1) << '\t' << a(i,2) << '\t' << a(i,3) << '\n';
    }
    toc("s_1");
    
    std::ofstream file_2 (HomeDirectory() / "test_2.txt");
    tic("s_2");
    file_2 << a;
    toc("s_2");
    
    std::ofstream file_3 (HomeDirectory() / "test_3.txt");
    tic("s_3");
    file_3 << OutString::FromMatrix<Format::Matrix::TSV>(a.WriteAccess(),reps,4);
    toc("s_3");

//
//    {
//        OutString str;
//        str << "Hello world!";
//        print(str);
//    }
//    
//    {
//        OutString str;
//        str << "Hello world!";
//        print(std::move(str));
//    }
    
    logprint(1,",",2,",",3);
    
    print(PrettyTypeName<Link_T>());
    print(Link_T::ClassName());
}
