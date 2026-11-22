#define TOOLS_ENABLE_PROFILER

#define TOOLS_AGGRESSIVE_INLINING

//#define TOOLS_ENABLE_PROFILER

//#define PD_DEBUG
//#define PD_VERBOSE
//#define PD_TIMINGQ
//#define PD_COUNTERS

//#define TOOLS_DEBUG
//#define TENSORS_BOUND_CHECKS
//#define TENSORS_ALLOCATION_LOGS

#ifdef __APPLE__
    #include "../../submodules/Tensors/Accelerate.hpp"
#else
    #include "../../submodules/Tensors/OpenBLAS.hpp"
#endif


#include "../../Knoodle.hpp"
#include "../../experimental/FrontEncodedLookupTable.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

constexpr Size_T key_count  = 10;
constexpr Size_T key_length =  8;

using Value_T = UInt8;

using LUT_T = FrontEncodedLookupTable<key_length,Value_T>;
using FindResult_T = LUT_T::FindResult_T;

static constexpr UInt8 keys [key_count][key_length] = {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,2},
    {0,0,0,0,0,0,1,1},
    {0,0,0,0,1,0,1,1},
    {1,0,0,0,0,0,0,0},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,2},
    {1,0,0,0,0,0,1,1},
    {1,0,0,0,1,0,1,1},
    {9,0,0,0,1,0,1,1}
};

static constexpr Value_T vals [key_count] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

int main()
{
    LUT_T lut;
    
    int info;
    
    info = lut.LoadUnsorted(&keys[0][0],&vals[0],key_count);
    
    TOOLS_DUMP(info);
    TOOLS_DUMP(lut.Buffer().size());
    TOOLS_DUMP(lut.Buffer());
    TOOLS_DUMP(lut.PrefixPointers());
    TOOLS_DUMP(lut.PrefixPositions());
    
    {
        Size_T p = 8;
        TOOLS_DUMP(p);
        FindResult_T result = lut.Find(&keys[p][0]);
        TOOLS_DUMP(result);
    }
    
    for( Size_T p = 0; p  < key_count; ++p )
    {
        TOOLS_DUMP(p);
        FindResult_T result = lut.Find(&keys[p][0]);
        TOOLS_DUMP(result);
    }
    
    return EXIT_SUCCESS;
}
