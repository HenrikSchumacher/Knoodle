public:

/*!@brief Struct for storing intersection computed by `Prosector`.*/
struct Intersection
{
    Idx              edges [2] = {Idx(-2)}; /**< First edge goes over, second edge goes under. */
    IntersectionTime times [2]; /**< The times of intersection relative to the two line segments (left end point is at time 0, right end point is at time 1). The values here are rational functions with integer coeffients. */

    Sign_T handedness {0}; /**< The handedness of the resulting crossing: +1 means right-handed, -1 means left-handed, 0 means degenerate. */
    
    Flag_T flag { Flag_T::Uninitialized }; /**< The flag that was raised when computing this intersection. */
    
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

    /*!@brief Return an invalid instance of `Intersection` with `flag` set to `flag_` */
    static Intersection InvalidIntersection( const Flag_T flag_)
    {
        Intersection intersection;
        intersection.flag = flag_;
        return intersection;
    }
    
}; // struct Intersection

