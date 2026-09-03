//#define TOOLS_NO_RESTRICT

//#define TOOLS_ENABLE_PROFILER
//#define TOOLS_NO_INT128
#define TOOLS_AGGRESSIVE_INLINING


#include "../../Knoodle.hpp"
//#include "../../experimental/LinkEmbedding_Boost.hpp"
//#include "../../experimental/LinkEmbedding3.hpp"
#include "../../experimental/LinkEmbedding4.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using Int   = Int64;
using IReal = Int64;

//using Link1_T = LinkEmbedding<Real,Int>;
//using Link2_T = LinkEmbedding_Boost<Real,Int>;
//using Link3_T = LinkEmbedding3<Real,Int>;
//using Link4_T = LinkEmbedding4<Real,Int,IReal>;
using Link5_T = LinkEmbedding5<Real,Int,IReal>;

using PDC_T   = PlanarDiagramComplex<Int>;
int main()
{
    Profiler::Clear();
    int err;
    
    auto file = HomeDirectory() / "a.txt";
    
    print("");

//    auto L1 = Link1_T::FromFile(file);
//    tic(L1.ClassName());
//    err = L1.RequireIntersections();
//    toc(L1.ClassName());
//    TOOLS_DUMP(err);
//    print("");
    
//    auto L2 = Link2_T::FromFile(file);
//    tic(L2.ClassName());
//    err = L2.RequireIntersections();
//    toc(L2.ClassName());
//    TOOLS_DUMP(err);
//    print("");
    
//    Link3_T L3 = Link3_T::FromFile(file);
//    tic(L3.ClassName());
//    err = L3.RequireIntersections();
//    toc(L3.ClassName());
//    TOOLS_DUMP(err);
//    print("");
//    
//    Link4_T L4 = Link4_T::FromFile(file);
//    tic(L4.ClassName());
//    err = L4.RequireIntersections();
//    toc(L4.ClassName());
//    TOOLS_DUMP(err);
//    print("");
    
    Link5_T L5 = Link5_T::FromFile(file);
    tic(std::string(L5.ClassName()));
    err = L5.RequireIntersections();
    toc(std::string(L5.ClassName()));
    TOOLS_DUMP(err);
    print("");
    
//    TOOLS_DUMP(L1.AllocatedByteCount());
//    TOOLS_DUMP(L2.AllocatedByteCount());
//    TOOLS_DUMP(L3.AllocatedByteCount());
//    TOOLS_DUMP(L4.AllocatedByteCount());
    TOOLS_DUMP(L5.AllocatedByteCount());
    
    print("");
    
    
//    TOOLS_DUMP(L2.EdgeCrossings() == L1.EdgeCrossings());
//    TOOLS_DUMP(L3.EdgeCrossings() == L1.EdgeCrossings());
//    TOOLS_DUMP(L4.EdgeCrossings() == L1.EdgeCrossings());
//    TOOLS_DUMP(L5.EdgeCrossings() == L1.EdgeCrossings());
    
//    print(StringCat(std::string("Test")," ","Bla"));
//    
//    print(Link5_T::Msgr::MethodName("Test"));
//    
//    print(Link5_T::Msgr::Message("Test","Bla bla = ",Tools::ToString(L5.CrossingCount()),". Blub blub. Blib blib"));
    
    
    
    
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
    std::uniform_int_distribution<Int> dist (0,999);
    
    Tensor2<Int,Size_T> a ( reps, 4 );
    
    std::string s_0;
    s_0.reserve(reps);
    
    for( Size_T rep = 0; rep < reps; ++rep )
    {
        a(reps,0) = dist(engine);
        a(reps,1) = dist(engine);
        a(reps,2) = dist(engine);
        a(reps,3) = dist(engine);
    }
    
    tic("s_0");
    for( Size_T rep = 0; rep < reps; ++rep )
    {
        auto s = (Link5_T::ClassName() + "::" + "Test" + ": "
               +   "A = " + Tools::ToString(a(reps,0))
               + ", B = " + Tools::ToString(a(reps,1))
               + ", C = " + Tools::ToString(a(reps,2))
               + ", D = " + Tools::ToString(a(reps,3))
               + ".");
        
        s_0 += s[0];
    }
    toc("s_0");
    
    std::string s_1;
    s_1.reserve(reps);
    tic("s_1");
    for( Size_T rep = 0; rep < reps; ++rep )
    {
        std::string s;
        s.append(Link5_T::ClassName()).append("::").append("Test")
        .append(": ")
        .append(  "A = ").append(Tools::ToString(a(reps,0)))
        .append(", B = ").append(Tools::ToString(a(reps,1)))
        .append(", C = ").append(Tools::ToString(a(reps,2)))
        .append(", D = ").append(Tools::ToString(a(reps,3)));
        s_1 += s[0];
    }
    toc("s_1");
    
    std::string s_2;
    s_2.reserve(reps);
    tic("s_2");
    
    constexpr auto tag = ct_string("Test") + "< " + TypeName<Int> + ">";
    for( Size_T rep = 0; rep < reps; ++rep )
    {
        auto s = Link5_T::Msgr::Message( tag
               , ", A = " , a(reps,0)
               , ", B = " , a(reps,1)
               , ", C = " , a(reps,2)
               , ", D = " , a(reps,3)
               , ".");
        
        s_2 += s[0];
    }
    toc("s_2");
    
    print(std::string_view( &s_0[0], &s_0[12] ));
    print(std::string_view( &s_1[0], &s_1[12] ));
    print(std::string_view( &s_2[0], &s_2[12] ));
    {
        auto s = Link5_T::Msgr::Message( "Test"
            ,   "A = " , a(reps,0)
            , ", B = " , a(reps,1)
            , ", C = " , a(reps,2)
            , ", D = " , a(reps,3)
            , ".");
        print(s);
    }
    
    {
        OutString str;
        str << "Hello world!";
        print(str);
    }
    
    {
        OutString str;
        str << "Hello world!";
        print(std::move(str));
    }
}
