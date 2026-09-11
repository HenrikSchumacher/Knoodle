//#define TOOLS_NO_RESTRICT

//#define TOOLS_ENABLE_PROFILER
//#define TOOLS_NO_INT128
#define TOOLS_AGGRESSIVE_INLINING


#include "../../Knoodle.hpp"
//#include "../../experimental/LinkEmbedding_Boost.hpp"
//#include "../../experimental/LinkEmbedding3.hpp"
#include "../../experimental/LinkEmbedding5.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using Int   = Int64;
using IReal = Int64;

using Link1_T = LinkEmbedding<Real,Int>;
//using Link2_T = LinkEmbedding_Boost<Real,Int>;
//using Link3_T = LinkEmbedding3<Real,Int>;
using Link4_T = LinkEmbedding4<Real,Int,IReal>;
//using Link5_T = LinkEmbedding5<Real,Int,IReal>;

using PDC_T   = PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;

int main()
{
    Profiler::Clear();
    int err;
    
    auto file = HomeDirectory() / "a.txt";
    
    print("");

    auto L1 = Link1_T::FromFile(file);
    tic(L1.ClassName());
    err = L1.RequireIntersections();
    toc(L1.ClassName());
    TOOLS_DUMP(err);
    print("");
    
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
    
    Link4_T L4 = Link4_T::FromFile(file);
    tic(L4.ClassName());
    err = L4.RequireIntersections();
    toc(L4.ClassName());
    TOOLS_DUMP(err);
    print("");
    

    std::ostringstream ostrstream;
    
    
//    ofstream << L4;
//    ostrstream << L4;
    
    PDC_T pdc (L4);
    print(pdc[0]);
    {
        std::ofstream ofstream ( HomeDirectory() /"b.txt" );
        ofstream << pdc[0];
        ofstream << pdc[0];
    }
    
    PD_T pd_0;
    PD_T pd_1;
    
    {
        InString in ( HomeDirectory() /"b.txt" );
        print(in.View());
        in >> pd_0;
        in >> pd_1;
        
    }
    print("pd_0 = ", pd_0);
    print("pd_1 = ", pd_1);
    
//    Link5_T L5 = Link5_T::FromFile(file);
//    tic(std::string(L5.ClassName()));
//    err = L5.RequireIntersections();
//    toc(std::string(L5.ClassName()));
//    TOOLS_DUMP(err);
//    print("");
    
//    TOOLS_DUMP(L1.AllocatedByteCount());
//    TOOLS_DUMP(L2.AllocatedByteCount());
//    TOOLS_DUMP(L3.AllocatedByteCount());
    TOOLS_DUMP(L4.AllocatedByteCount());
//    TOOLS_DUMP(L5.AllocatedByteCount());
    
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
    
    
    
    
//    TOOLS_DUMP(Stringy<std::string>);
//    TOOLS_DUMP(Stringy<std::string &>);
//    TOOLS_DUMP(Stringy<const std::string &>);
//    TOOLS_DUMP(Stringy<std::string &&>);
//    
//    TOOLS_DUMP(Stringy<OutString>);
//    TOOLS_DUMP(Stringy<OutString &>);
//    TOOLS_DUMP(Stringy<const OutString &>);
//    TOOLS_DUMP(Stringy<OutString &&>);
//    
//    TOOLS_DUMP(Stringy<int>);
//    TOOLS_DUMP(Stringy<int &>);
//    TOOLS_DUMP(Stringy<const int &>);
//    TOOLS_DUMP(Stringy<int &&>);
    
    
}
