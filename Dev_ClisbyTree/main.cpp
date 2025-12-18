#include "../Knoodle.hpp"
#include "../src/ClisbyTree.hpp"
#include "../src/PolyFold.hpp"
#include "../src/NaivePolygonFolder.hpp"

using namespace Knoodle;

using Real = double;
using Int  = Int64;
using LInt = Int64;
//using NaivePolygonFolder_T = NaivePolygonFolder<3,Real,Int,Int>;

using Clisby_T = ClisbyTree<3,Real,Int,LInt>;

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    const Int  n    = 64;
    const Real diam = 1;
    
    Clisby_T T (n,diam);
    
    auto result = T.Fold({1,16},1.2,false,true,true);
    
    TOOLS_DUMP(result);
    
    return 0;
}
