#pragma once

#include "submodules/Tensors/Tensors.hpp"
#include "submodules/Tensors/submodules/Tools/Oriented2D.hpp"

namespace KnotTools
{
    using namespace Tensors;
    using namespace Tools;
    
    using Tools::ToString;
    
    enum class CrossingState : int
    {
        Unitialized = -2,
        RightHanded =  1,
        LeftHanded  = -1,
        Untied      =  0
    };
    
    std::string ToString( const CrossingState & s )
    {
        switch( s )
        {
            case CrossingState::Unitialized : return "Unitialized";
                
            case CrossingState::RightHanded : return "RightHanded";
                
            case CrossingState::LeftHanded  : return "LeftHanded";
                
            case CrossingState::Untied      : return "Untied";
        }
    }
    
    enum class ArcState : int
    {
        Active    =  1,
        Inactive  =  0
    };
    
    std::string ToString( const ArcState & s )
    {
        switch( s )
        {
            case ArcState::Active   : return "Active";
                
            case ArcState::Inactive : return "Inactive";
        }
    }
    
} // namespace KnotTools

#include "src/Link.hpp"

#include "src/AABBTree.hpp"


#include "src/Intersection.hpp"

#include "src/Link_2D.hpp"

#include "src/PlanarDiagram.hpp"

//#include "src/Alexander.hpp"
//
//#include "src/Seifert.hpp"

#include "src/Link_3D.hpp"

#include "src/Collision.hpp"

#include "src/LinearHomotopy_3D.hpp"
