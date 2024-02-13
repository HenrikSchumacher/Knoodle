#pragma once

#include "submodules/Tensors/Tensors.hpp"
#include "submodules/Tensors/submodules/Tools/Oriented2D.hpp"

namespace KnotTools 
{
    using namespace Tensors;
    using namespace Tools;
    
    enum class Crossing_State : signed char
    {
        Unitialized = -2,
        Positive    =  1,
        Negative    = -1,
        Untied      =  0
    };
    
    enum class Arc_State : signed char
    {
        Active    =  1,
        Inactive  =  0
    };
    
} // namespace KnotTools

#include "src/Link.hpp"
#include "src/AABBTree.hpp"

//#include "src/Crossing.hpp"
//#include "src/Arc.hpp"


#include "src/Link_2D.hpp"

#include "src/PlanarDiagram.hpp"
