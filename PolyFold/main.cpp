//#define TOOLS_DEBUG

//#define PD_DEBUG

//#define POLYFOLD_SIGNPOSTS
//#define POLYFOLD_NO_QUATERNIONS
//#define POLYFOLD_WITNESSES

#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
#include "../src/PolyFold.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;


using Real  = Real64;   // scalar type used for positions of polygon
using BReal = Real64;   // scalar type used for bounding boxes
using Int   = Int64;    // integer type used, e.g., for indices
using LInt  = Int64;    // integer type for counting objects

using PolyFold_T = PolyFold<Real,Int,LInt,BReal>;

int main( int argc, char** argv )
{
//    Int32 min_32 = std::numeric_limits<Int32>::lowest();
//    Int64 min_64 = std::numeric_limits<Int64>::lowest();
//    TOOLS_DUMP(min_32)
//    TOOLS_DUMP(min_64)
//    
//    TOOLS_DUMP(min_32 > UInt32(0));
//    TOOLS_DUMP(std::cmp_greater(min_32,UInt32(0)));
//    TOOLS_DUMP(min_64 > UInt64(0));
//    TOOLS_DUMP(std::cmp_greater(min_64,UInt64(0)));
    
    
//    std::filesystem::path file ( "/Users/Henrik/a.txt" );
//    
//    TOOLS_DUMP(file)
//    
//    Tensor2<Real,Int> a ( 2, 2 );
//    
//    a[0][0] = 1;
//    a[0][1] = 2;
//    a[1][0] = 3;
//    a[1][1] = 4;
//    
//    TOOLS_DUMP(a[0][0])
//    TOOLS_DUMP(a[0][1])
//    TOOLS_DUMP(a[1][0])
//    TOOLS_DUMP(a[1][1])
//    
//    TOOLS_DUMP(ToString(a))
//    TOOLS_DUMP(a)
//    
//    a.WriteToFile( file );
//    
//    
//    
//    Tensor2<Real,Int> b ( 1, 2 );
//    Tensor2<Real,Int> c ( 3, 2 );
//    Tensor2<Real,Int> d ( 2, 2 );
//    
//    TOOLS_DUMP(b.ReadFromFile( file ));
//    
//    TOOLS_DUMP(b)
//    
//    TOOLS_DUMP(c.ReadFromFile( file ));
//    
//    TOOLS_DUMP(c)
//    
//    TOOLS_DUMP(d.ReadFromFile( file ));
//    
//    TOOLS_DUMP(d)
    
    
    
//    using T_T = CompleteBinaryTree<Int,false,false>;
//    using S_T = CompleteBinaryTree<Int,false,true>;
//    
//    const Int n = 1024 * 1024;
//    
//    const Int reps = 512;
//
//    
//    tic("T.PreOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.PreOrdering();
//    }
//    toc("T.PreOrdering()");
//    
//    tic("T.PostOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.PostOrdering();
//    }
//    toc("T.PostOrdering()");
//    
//    tic("T.BreadthFirstOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.BreadthFirstOrdering();
//    }
//    toc("T.BreadthFirstOrdering()");
//    
//    tic("T.ReverseBreadthFirstOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.ReverseBreadthFirstOrdering();
//    }
//    toc("T.ReverseBreadthFirstOrdering()");
//    
//    print("");
//    
//    tic("S.PreOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        S_T S (n);
//        (void)S.PreOrdering();
//    }
//    toc("S.PreOrdering()");
//    
//    tic("S.PostOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        S_T S (n);
//        (void)S.PostOrdering();
//    }
//    toc("S.PostOrdering()");
//    
//    tic("S.BreadthFirstOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        S_T S (n);
//        (void)S.BreadthFirstOrdering();
//    }
//    toc("S.BreadthFirstOrdering()");
//    
//    tic("S.ReverseBreadthFirstOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        S_T S (n);
//        (void)S.ReverseBreadthFirstOrdering();
//    }
//    toc("S.ReverseBreadthFirstOrdering()");
//    
//    print("");
//    
//    tic("T.PreOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.PreOrdering();
//    }
//    toc("T.PreOrdering()");
//    
//    tic("T.PostOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.PostOrdering();
//    }
//    toc("T.PostOrdering()");
//    
//    tic("T.BreadthFirstOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.BreadthFirstOrdering();
//    }
//    toc("T.BreadthFirstOrdering()");
//    
//    tic("T.ReverseBreadthFirstOrdering()");
//    for( Int rep = 0; rep < reps; ++rep )
//    {
//        T_T T (n);
//        (void)T.ReverseBreadthFirstOrdering();
//    }
//    toc("T.ReverseBreadthFirstOrdering()");
//    
//    print("");
//    
//    
//    
//    T_T T (n);
//    S_T S (n);
//    TOOLS_DUMP( S.PreOrdering()  == T.PreOrdering() )
//    TOOLS_DUMP( S.PostOrdering() == T.PostOrdering() )
//    
//    print("");
    
    
    try
    {
        PolyFold_T polyfold ( argc, argv );
    }
    catch(...)
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
