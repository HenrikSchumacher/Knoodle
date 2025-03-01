#pragma  once

namespace KnotTools
{

    template<typename Real_ = double, typename Int_ = Int32, typename SInt_ = Int8>
    struct Intersection
    {
        using Real = Real_;
        using Int  = Int_;
        using SInt = SInt_;
        
        Int  edges [2] = {-2}; // First edge goes over, second edge goes under.
        Real times [2] = {-2};
        
        // +1 means right-handed, -1 means left-handed, 0 means degenerate.
        SInt handedness;
        
        Intersection(
            const Int over_edge_,
            const Int under_edge_,
            const Real over_edge_time_,
            const Real under_edge_time_,
            const SInt handedness_
        )
        :   edges       { over_edge_,      under_edge_      }
        ,   times       { over_edge_time_, under_edge_time_ }
        ,   handedness  { handedness_                       }
        {}

    };

}
