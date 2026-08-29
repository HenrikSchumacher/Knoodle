#pragma once


#include "Prosector_Boost.hpp"

namespace Knoodle
{
    /*!@brief **EXPERIMENTAL** Type alias of `LinkEmbedding_Int` with backend that uses wide integers classes `boost::multiprecision::int128_t` and `boost::multiprecision::int256_t`. This is only available if the preprocessor macro `KNOODLE_USE_BOOST_MULTIPRECISION` is defined.
     *
     * This is not a very efficient implementation, in particular when scalar type `IReal = std::int32_t` is used. But it should lead to correct results. We use it for test purposed. */
    template<
        typename   Real  = Real64,
        IntQ       Int   = Int64,
        SignedIntQ IReal = std::conditional_t<SameQ<Real,Real64>, Int64,
                                std::conditional_t<SameQ<Real,Real32>, Int32, Real>
                           >
    >
    using LinkEmbedding_Boost = LinkEmbedding_Int<Real, Prosector_Boost<IReal,Int>>;
    
    template<
        typename   Real  = Real64,
        IntQ       Int   = Int64,
        SignedIntQ IReal = std::conditional_t<SameQ<Real,Real64>, Int64,
                                std::conditional_t<SameQ<Real,Real32>, Int32, Real>
                           >
    >
    using LinkEmbedding2 = LinkEmbedding_Boost<Real,Int,IReal>;
    
} // namespace Knoodle
