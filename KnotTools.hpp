#pragma once

#include "submodules/Tensors/Tensors.hpp"
#include "submodules/Tensors/submodules/Tools/Oriented2D.hpp"

namespace KnotTools
{
    using namespace Tensors;
    using namespace Tools;
    
    using Tools::ToString;
    
    enum class CrossingState : Int8
    {
        // Important! Active values are the only odd ones.
        RightHanded          =  1,
        RightHandedUnchanged =  2,
        LeftHanded           = -1,
        LeftHandedUnchanged  = -2,
        Inactive             =  0
    };
    
    inline std::string ToString( const CrossingState & s )
    {
        switch( s )
        {
            case CrossingState::Inactive             : return "Inactive";
                
            case CrossingState::RightHanded          : return "RightHanded";
                
            case CrossingState::RightHandedUnchanged : return "RightHandedUnchanged";
                
            case CrossingState::LeftHanded           : return "LeftHanded";
                
            case CrossingState::LeftHandedUnchanged  : return "LeftHandedUnchanged";
        }
    }

    inline constexpr bool ActiveQ( const CrossingState & s )
    {
        return ToUnderlying(s);
    }
    
    inline constexpr bool ChangedQ( const CrossingState & s )
    {
        return ToUnderlying(s) % 2;
    }
    
    inline constexpr bool UnchangedQ( const CrossingState & s )
    {
        return !ChangedQ(s);
    }
    
    inline constexpr bool RightHandedQ( const CrossingState & s )
    {
        return ToUnderlying(s) > 0;
    }
    
    inline constexpr bool LeftHandedQ( const CrossingState & s )
    {
        return ToUnderlying(s) < 0;
    }
    
    inline constexpr bool OppositeHandednessQ(
        const CrossingState & s_0, const CrossingState & s_1
    )
    {
        // TODO: Careful, this evaluates to true if both are `Inactive`.
        
        return ( Sign(ToUnderlying(s_0)) == -Sign(ToUnderlying(s_1)) );
    }
    
    inline constexpr bool SameHandednessQ(
        const CrossingState & s_0, const CrossingState & s_1
    )
    {
        // TODO: Careful, this evaluates to true if both are `Inactive`.
        
        return ( Sign(ToUnderlying(s_0)) == Sign(ToUnderlying(s_1)) );
    }
    
    
    
    enum class ArcState : Int8
    {
        Unchanged =  2,
        Active    =  1,
        Inactive  =  0
    };
    
    inline constexpr bool ChangedQ( const ArcState & s )
    {
        return ToUnderlying(s) % 2;
    }
    
    inline constexpr bool UnchangedQ( const ArcState & s )
    {
        return !ChangedQ(s);
    }
    
    inline constexpr bool ActiveQ( const ArcState & s )
    {
        return ToUnderlying(s);
    }
    
    inline std::string ToString( const ArcState & s )
    {
        switch( s )
        {
            case ArcState::Active    : return "Active";
                
            case ArcState::Inactive  : return "Inactive";
                
            case ArcState::Unchanged : return "Unchanged";
        }
    }
    
} // namespace KnotTools

#include "src/Link.hpp"

#include "src/AABBTree.hpp"


#include "src/Intersection.hpp"

#include "src/Link_2D.hpp"

#include "src/PlanarDiagram.hpp"
#include "src/ArcSimplifier.hpp"
#include "src/StrandSimplifier.hpp"

//#include "src/Alexander.hpp"
//
//#include "src/Seifert.hpp"

#include "src/Link_3D.hpp"

#include "src/Collision.hpp"

#include "src/LinearHomotopy_3D.hpp"
