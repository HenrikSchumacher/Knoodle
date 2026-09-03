#pragma  once

#include "Prosector4.hpp"

namespace Knoodle
{
    /*!@brief **EXPERIMENTAL** Type alias of `LinkEmbedding_Int` with backend that uses wide integers classes `WideInt`, our implementation of wide integers. This is obtimized towards using native integer classes or the compiler extension`__int128` as long as possible. Only starting with 192-bit integers, `WideInt` is used.*/
    template<
        typename   Real  = Real64,
        IntQ       Int   = Int64,
        SignedIntQ IReal = std::conditional_t<
            SameQ<Real,Real64>,
            Int64,
            std::conditional_t<SameQ<Real,Real32>, Int32, Real>
        >
    >
    using LinkEmbedding5 = LinkEmbedding_Int<Real,Prosector5<IReal,Int>,true>;
    
} // namespace Knoodle
