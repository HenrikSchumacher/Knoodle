public:

/*!@brief Struct for storing intersection computed by `Prosector`.*/
class Intersection_T
{
private:
    
    Idx over_edge;
    Idx under_edge_and_handedness;
    
public:
    
    Intersection_T() = default;
    
    Intersection_T(
        const Idx  over_edge_,
        const Idx  under_edge_,
        const bool right_handedQ
    )
    :   over_edge { over_edge_ }
    ,   under_edge_and_handedness {
            (under_edge_ << 1) | Idx{right_handedQ}
        }
    {}
    
    Idx OverEdge() const
    {
        return over_edge;
    }
    
    Idx UnderEdge() const
    {
        return (under_edge_and_handedness >> 1);
    }
    
    bool RightHandedQ() const
    {
        return under_edge_and_handedness & Idx{1};
    }
    
    friend void ToString( const Intersection_T & x )
    {
        return Tools::ToString(std::pair{ x.over_edge, x.under_edge_and_handedness });
    }
    
}; // struct Intersection_T

