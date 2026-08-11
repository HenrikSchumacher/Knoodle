public:

/*!@brief **EXPERIMENTAL** Type to represent an intersection time as a double.
 */
class IntersectionTime_Double final
{
private:
    
    double t;
    
public:
    
    IntersectionTime_Double() = default;
    
    IntersectionTime_Double( cref<Polynomial3> numerator, cref<Polynomial3> denominator )
    :   t { ToDouble(numerator) / ToDouble(denominator) }
    {}

    friend double ToDouble( cref<IntersectionTime_Double> T )
    {
        return T.t;
    }
    
    friend std::partial_ordering operator<=>(
        cref<IntersectionTime_Double> S, cref<IntersectionTime_Double> T
    )
    {
        return (S.t <=> T.t);
    }
    
    friend std::string ToString( cref<IntersectionTime_Double> T )
    {
        return ToString(T.t);
    }
    
}; // class IntersectionTime_Double
