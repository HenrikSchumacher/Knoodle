//#define TOOLS_NO_RESTRICT

//#define TOOLS_ENABLE_PROFILER
//#define TOOLS_NO_INT128
#define TOOLS_AGGRESSIVE_INLINING
#define KNOODLE_USE_BOOST_MULTIPRECISION


#include "../../Knoodle.hpp"
//#include "../../experimental/LinkEmbedding_Boost.hpp"
#include "../../experimental/LinkEmbedding5.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using Int   = Int64;
using IReal = Int64;

//using Link2_T         = LinkEmbedding_Boost<Real,Int>;
using Link1_T         = LinkEmbedding<Real,Int>;
using Link4_T         = LinkEmbedding4<Real,Int,IReal>;
using Link5_T         = LinkEmbedding5<Real,Int,IReal>;

//using Prosector_T    = Link_T::Prosector_T;
//using Vector3_T      = Prosector_T::Vector3_T;
//using Intersection_T = Prosector_T::Intersection;
//using Flag_T         = Prosector_T::Flag_T;

using PDC_T          = PlanarDiagramComplex<Int>;


int main()
{
    Profiler::Clear();
    int err;

    Link1_T L1 = Link1_T::FromFile( HomeDirectory() / "a.txt");
    tic("Link1");
    err = L1.RequireIntersections();
    toc("Link1");
    TOOLS_DUMP(err);
    
    std::ofstream a_L1 ( HomeDirectory() / "a_L1.txt");
    a_L1 << L1.EdgeIntersections();
    
    Link4_T L4 = Link4_T::FromFile( HomeDirectory() / "a.txt");
    tic("Link4");
    err = L4.RequireIntersections();
    toc("Link4");
    TOOLS_DUMP(err);
    
    std::ofstream a_L4 ( HomeDirectory() / "a_L4.txt");
    a_L4 << L4.EdgeIntersections();
    
    Link5_T L5 = Link5_T::FromFile( HomeDirectory() / "a.txt");
    tic("Link5");
    err = L5.RequireIntersections();
    toc("Link5");
    TOOLS_DUMP(L5.EdgeCount());
    TOOLS_DUMP(err);
    
    std::ofstream a_L5 ( HomeDirectory() / "a_L5.txt");
    a_L5 << L5.EdgeIntersections();
    
    print(L1.AllocatedByteCountDetails());
    print(L5.AllocatedByteCountDetails());
    
//    TOOLS_DUMP(L1.AllocatedByteCount());
//    TOOLS_DUMP(L4.AllocatedByteCount());
//    TOOLS_DUMP(L5.AllocatedByteCount());
}
