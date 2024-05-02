#pragma once

#include "submodules/Tensors/Tensors.hpp"
#include "submodules/Tensors/submodules/Tools/Oriented2D.hpp"

namespace KnotTools 
{
    using namespace Tensors;
    using namespace Tools;
    
    enum class CrossingState : int
    {
        Unitialized = -2,
        RightHanded =  1,
        LeftHanded  = -1,
        Untied      =  0
    };
    
    enum class ArcState : int
    {
        Active    =  1,
        Inactive  =  0
    };
    
} // namespace KnotTools

#include "src/Link.hpp"
#include "src/AABBTree.hpp"

//#include "src/Crossing.hpp"
//#include "src/Arc.hpp"


#include "src/Intersection.hpp"
#include "src/Link_2D.hpp"

#include "src/PlanarDiagram.hpp"

//#include "src/Alexander.hpp"
//
//#include "src/Seifert.hpp"
