// CAUTION: This is highly EXPERIMENTAL. Not ready for production.

//#define PD_DEBUG

//#define POLYFOLD_SIGNPOSTS
//#define POLYFOLD_NO_QUATERNIONS


#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
#include "../src/PolyFold.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

using Real  = Real64;   // scalar type used for positions of polygon
using BReal = Real64;   // scalar type used for bounding boxes
using Int   = Int64;    // integer type used, e.g., for indices in PlanarDiagram etc.
using LInt  = Int64;    // integer type for counting objects

constexpr Int AmbDim = 3;

using ClisbyTree_T = ClisbyTree<AmbDim,Real,Int,LInt,
    ClisbyTree_TArgs{ .clang_matrixQ = 1, .quaternionsQ = 0}
>;

using Subtree_T = ClisbyTree_T::ModifiedClisbyTree;

using Flag_T = ClisbyTree_T::FoldFlag_T;

constexpr bool reflectionsQ = false;


int main( int argc, char** argv )
{
    const Int n       = 100000;
    const Int burn_in = 1000000;
    const Int skip    = 10000;
    
    const Size_T thread_count = 8;
    
    ClisbyTree_T T (n, Real(1));
    
    Subtree_T S (T);
    
    std::vector<ClisbyTree_T::ModifiedClisbyTree> thread_trees;
    
    for( Int thread = 0; thread < thread_count; ++thread )
    {
        thread_trees.emplace_back(T);
    }
    
    ThreadTensor2<ClisbyTree_T::FoldFlag_T,Size_T> thread_flags( thread_count, 1 );
    
    
    tic("Burn-in");
    {
        auto counters = T.template FoldRandom<reflectionsQ,true>(burn_in);
        
        valprint("acceptance rate", Real(100)*Frac<Real>(counters[0],counters.Total()));
    }
    toc("Burn-in");
    
    tic("ClisbyTree");
    {
        auto counters = T.template FoldRandom<reflectionsQ,true>(skip);
        
        valprint("acceptance rate", Real(100)*Frac<Real>(counters[0],counters.Total()));
    }
    toc("ClisbyTree");
    
//    tic("ModifiedClisbyTree");
//    {
//        Int accept_count = 0;
//        while( accept_count < skip )
//        {
//            Flag_T flag = S.template FoldRandom<false,true>();
//            
//            if( flag == 0 )
//            {
//                T.template LoadModifications<false>(S);
//                ++accept_count;
//            }
//        }
//    }
//    toc("ModifiedClisbyTree");
    
    tic("ModifiedClisbyTree Parallel");
    {
        Int iter = 0;
        Int accept_count = 0;
        
        while( accept_count < skip )
        {
            ++iter;
            ParallelDo(
                [&thread_trees,&thread_flags]( const Size_T thread)
                {
                    thread_flags(thread,0)
                    = thread_trees[thread].template FoldRandom<reflectionsQ,true>();
                },
                thread_count
            );
            
            for( Size_T thread = 0; thread < thread_count; ++thread )
            {
                if( thread_flags(thread,0) == 0 )
                {
                    T.LoadModifications<reflectionsQ>(thread_trees[thread]);
                    ++accept_count;
                    break;
                }
            }
        }
        dump(iter);
    }
    toc("ModifiedClisbyTree Parallel");
    
    
    tic("ModifiedClisbyTree");
    {
        Int accept_count = 0;
        while( accept_count < skip )
        {
            Flag_T flag = S.template FoldRandom<reflectionsQ,true>();
            
            if( flag == 0 )
            {
                T.template LoadModifications<reflectionsQ>(S);
                ++accept_count;
            }
        }
    }
    toc("ModifiedClisbyTree");
    

    tic("ClisbyTree");
    {
        auto counters = T.template FoldRandom<reflectionsQ,true>(skip);
        
        valprint("acceptance rate", Real(100)*Frac<Real>(counters[0],counters.Total()));
    }
    toc("ClisbyTree");
    
    
    tic("ModifiedClisbyTree");
    {
        Int accept_count = 0;
        while( accept_count < skip )
        {
            Flag_T flag = S.template FoldRandom<reflectionsQ,true>();
            
            if( flag == 0 )
            {
                T.template LoadModifications<reflectionsQ>(S);
                ++accept_count;
            }
        }
    }
    toc("ModifiedClisbyTree");
    
    
    tic("ClisbyTree");
    {
        auto counters = T.template FoldRandom<reflectionsQ,true>(skip);
        
        valprint("acceptance rate", Real(100)*Frac<Real>(counters[0],counters.Total()));
    }
    toc("ClisbyTree");
    

    tic("ModifiedClisbyTree");
    {
        Int accept_count = 0;
        while( accept_count < skip )
        {
            Flag_T flag = S.template FoldRandom<reflectionsQ,true>();
            
            if( flag == 0 )
            {
                T.template LoadModifications<reflectionsQ>(S);
                ++accept_count;
            }
        }
    }
    toc("ModifiedClisbyTree");
    
    
    
    dump(T.VertexCoordinates(0));
    
    return 0;
}
