#pragma once

//#ifdef KNOODLE_USE_BOOST_UNORDERED
//    #include <boost/unordered/unordered_flat_map.hpp>
//    #include <boost/unordered/unordered_flat_set.hpp>
//#else
//    #include <unordered_map>
//    #include <unordered_set>
//#endif

namespace Knoodle
{
    template<IntQ Int>
    struct IndexPair_T
    {
        Int i;
        Int j;
        
        friend bool operator==( cref<IndexPair_T> a, cref<IndexPair_T> b )
        {
            return (a.i == b.i) && (a.j == b.j);
        }
    };
    
    template<IntQ Int>
    struct IndexPairHash_T
    {
        using is_avalanching [[maybe_unused]] = std::true_type; // instruct Boost.Unordered to not use post-mixing
        
        inline std::size_t operator()(const IndexPair_T<Int> & p) const
        {
            std::size_t seed = 0;
            hash_combine(seed, p.i);
            hash_combine(seed, p.j);
            return seed;
        }
    };
    
    template<IntQ Int, typename Scal>
    using MatrixTripleContainer_T
            = AssociativeContainer<IndexPair_T<Int>, Scal, IndexPairHash_T<Int>>;
    
} // namespace Knoodle
