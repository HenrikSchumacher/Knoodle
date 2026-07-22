#define TOOLS_NO_RESTRICT

#include "../Knoodle.hpp"
#include "../src/Prosector.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Int32;
using IReal = Int32;
//using Real = Int64;
using Int  = Size_T;

using Prosector_T    = Prosector<IReal,Int>;
using Vector3_T      = Prosector_T::Vector3_T;
using Intersection_T = Prosector_T::Intersection;
using Flag_T         = Prosector_T::Flag_T;

int main()
{
    Prosector_T S;
    
    print(S.ClassName());
    TOOLS_DUMP(TypeName<Prosector_T::Int>);
    TOOLS_DUMP(TypeName<Prosector_T::LInt>);
    TOOLS_DUMP(TypeName<Prosector_T::LLInt>);
    print("");
    
    Int i = 0;
    Int j = 1;
    Int k = 2;
    
    Vector3_T x_0 {0, 1, 1};
    Vector3_T x_1 {12, 10, 4};
    Vector3_T y_0 {0, -4, -1};
    Vector3_T y_1 {8, 12, 1};
    Vector3_T z_0 {0, 12, 3};
    Vector3_T z_1 {8, -4, 5};
    
    TOOLS_DUMP(x_0);
    TOOLS_DUMP(x_1);
    TOOLS_DUMP(y_0);
    TOOLS_DUMP(y_1);
    TOOLS_DUMP(z_0);
    TOOLS_DUMP(z_1);
    
    print("");
    
    Vector3_T u = x_1 - x_0;
    Vector3_T v = y_1 - y_0;
    Vector3_T p = y_1 - x_0;
    Vector3_T q = x_1 - y_0;
    
    TOOLS_DUMP(u);
    TOOLS_DUMP(v);
    TOOLS_DUMP(p);
    TOOLS_DUMP(q);
    
    print("\nIntersecting lines x and y.");
    S.LoadSegments(i, x_0, x_1, j, y_0, y_1);
    
    TOOLS_DUMP(S.IntersectionType());
    TOOLS_DUMP(S.Flag());
    Intersection_T xy_inter = S.ComputeIntersection();
    
    TOOLS_DUMP(xy_inter.flag);
    TOOLS_DUMP(xy_inter.edges[0]);
    TOOLS_DUMP(xy_inter.edges[1]);
    TOOLS_DUMP(xy_inter.times[0]);
    TOOLS_DUMP(xy_inter.times[1]);
    TOOLS_DUMP(xy_inter.times[0].ToDouble());
    TOOLS_DUMP(xy_inter.times[1].ToDouble());
    TOOLS_DUMP(xy_inter.handedness);
    
    print("\nIntersecting lines x and z.");
    S.LoadSegments(i, x_0, x_1, k, z_0, z_1);
    TOOLS_DUMP(S.IntersectionType());
    TOOLS_DUMP(S.Flag());
    Intersection_T xz_inter = S.ComputeIntersection();
    
    TOOLS_DUMP(xz_inter.flag);
    TOOLS_DUMP(xz_inter.edges[0]);
    TOOLS_DUMP(xz_inter.edges[1]);
    TOOLS_DUMP(xz_inter.times[0]);
    TOOLS_DUMP(xz_inter.times[1]);
    TOOLS_DUMP(xz_inter.times[0].ToDouble());
    TOOLS_DUMP(xz_inter.times[1].ToDouble());
    TOOLS_DUMP(xz_inter.handedness);
    
    print("");
    
    TOOLS_DUMP(xy_inter.times[0] < xz_inter.times[0]);
    TOOLS_DUMP(xy_inter.times[0] > xz_inter.times[0]);
    
//    TOOLS_DUMP(Prosector_T::Det_Perturbed(u,v));
//    TOOLS_DUMP(Prosector_T::Det_Perturbed(u,p));
//    TOOLS_DUMP(Prosector_T::Det_Perturbed(u,q));
//    TOOLS_DUMP(Prosector_T::Det_Perturbed(v,p));
//    TOOLS_DUMP(Prosector_T::Det_Perturbed(v,q));
}
