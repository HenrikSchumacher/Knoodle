public:

struct Intersection
{
    Idx              edges [2] = {Idx(-2)}; // First edge goes over, second edge goes under.
    IntersectionTime times [2];
    
    // +1 means right-handed, -1 means left-handed, 0 means degenerate.
    Sign_T handedness {0};
    
    Flag_T flag { Flag_T::Uninitialized };
    
    Intersection() = default;
    
    template<SignedIntQ ExtInt>
    Intersection(
        const Idx              over_edge_,
        const Idx              under_edge_,
        cref<IntersectionTime> over_edge_time_,
        cref<IntersectionTime> under_edge_time_,
        const ExtInt           handedness_, // Flipping sign might promote to int.
        const Flag_T           flag_
    )
    :   edges       { over_edge_,      under_edge_      }
    ,   times       { over_edge_time_, under_edge_time_ }
    ,   handedness  { static_cast<Sign_T>(handedness_)  }
    ,   flag        { flag_                             }
    {}

    
    static Intersection InvalidIntersection( const Flag_T flag_)
    {
        Intersection intersection;
        
        intersection.flag = flag_;
        
        return intersection;
    }
    
}; // struct Intersection

