#include "../Knoodle.hpp"
#include "../src/ClisbyTree.hpp"
#include "../src/PolyFold.hpp"
#include "../src/NaivePolygonFolder.hpp"

using namespace Knoodle;

using Real = double;
using Int  = Int64;
using LInt = Int64;
using NaivePolygonFolder_T = NaivePolygonFolder<3,Real,Int,Int>;

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    const Int  n    = 64;
    const Real diam = 1;
    
    NaivePolygonFolder_T F (n,diam);
    
    auto flags = F.FoldRandom(10000,true);
    
    TOOLS_DUMP(flags);
    
//    TOOLS_DUMP(F.VertexCoordinates());
    
    return 0;
}
