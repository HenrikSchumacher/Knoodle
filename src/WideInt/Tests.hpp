#pragma once

#include "TestHelpers.hpp"
#include "TestWideInt.hpp"

//#include "TestWideInt_2.hpp"
//#include "TestWideInt_3.hpp"
//#include "TestWideInt_4.hpp"

namespace Knoodle
{
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_NegativeQ( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("NegativeQ",
            []( cref<T> a ) -> bool { return a.NegativeQ(); },
            []( cref<S> a ) -> bool { return (a < 0); },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_Sign( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("Sign",
            []( cref<T> a ) -> Int8 { return Sign<Int8>(a); },
            []( cref<S> a ) -> Int8 { return Sign<Int8>(a); },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_Negate( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("operator- (unary)",
            []( cref<T> a ) -> S { T b =a; b.Negate(); S r; wide_convert(b,r); return r; },
            []( cref<S> a ) -> S { return -a; },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_less_small( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using R = std::conditional_t<signQ,ToSigned<Limb_T>,Limb_T>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        // We also need to test in equality cases, thus I allow only a limited range of numbers.
        
        std::uniform_int_distribution<R> dist ( signQ ? R(-2): R(0), signQ ? R(2): R(4) );
        
        auto rand = [&engine,&dist](){ return static_cast<Limb_T>(dist(engine)); };
        
        using S  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using T  = CheckInt<limb_count,Limb_T,signQ>;
        
        return TestWideInt("operator< (small)",
            []( cref<S> a, cref<S> b ) -> bool { return a < b; },
            []( cref<T> a, cref<T> b ) -> bool { return a < b; },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_greater_small( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using R = std::conditional_t<signQ,ToSigned<Limb_T>,Limb_T>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        // We also need to test in equality cases, thus I allow only a limited range of numbers.
        
        std::uniform_int_distribution<R> dist ( signQ ? R(-2): R(0), signQ ? R(2): R(4) );
        
        auto rand = [&engine,&dist](){ return static_cast<Limb_T>(dist(engine)); };
        
        using S  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using T  = CheckInt<limb_count,Limb_T,signQ>;
        
        return TestWideInt("operator> (small)",
            []( cref<S> a, cref<S> b ) -> bool { return a > b; },
            []( cref<T> a, cref<T> b ) -> bool { return a > b; },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_less( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("operator<",
            []( cref<T> a, cref<T> b ) -> bool { return (a < b); },
            []( cref<S> a, cref<S> b ) -> bool { return (a < b); },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_greater( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("opertor>",
            []( cref<T> a, cref<T> b ) -> bool { return (a > b); },
            []( cref<S> a, cref<S> b ) -> bool { return (a > b); },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_Add( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T = WideInt<limb_count,Limb_T,Comp_T,signQ>;
        using S = CheckInt<limb_count,Limb_T,signQ>;

        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };
        
        return TestWideInt("operator+",
            []( cref<T> a, cref<T> b ) -> S { S r; wide_convert(a + b,r); return r; },
            []( cref<S> a, cref<S> b ) -> S { return (a + b); },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_Subtract( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T = WideInt<limb_count,Limb_T,Comp_T,signQ>;
        using S = CheckInt<limb_count,Limb_T,signQ>;

        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };
        
        return TestWideInt("operator-",
            []( cref<T> a, cref<T> b ) -> S { S r; wide_convert(a - b,r); return r; },
            []( cref<S> a, cref<S> b ) -> S { return (a - b); },
            rand, n, reps, verboseQ
        );
    }
    
    

    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_long_mul( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<2 * limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("long_mul",
            []( cref<T> a, cref<T> b ) -> S { S r; wide_convert(long_mul(a,b),r); return r; },
            []( cref<S> a, cref<S> b ) -> S { return a * b; },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_long_fma( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<2 * limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("long_fma",
            []( cref<T> a, cref<T> b, cref<T> c ) -> S
            {
                auto r_0 = long_fma(a,b,c);
                S r;
//                wide_convert(r_0,r);
            
//                S a_wint;
//                S b_wint;
//                wide_convert(a,a_wint);
//                wide_convert(b,b_wint);
//                TOOLS_DUMP(ToDouble(a));
//                TOOLS_DUMP(static_cast<double>(a_wint));
//                TOOLS_DUMP(a.NegativeQ());
//                TOOLS_DUMP(a_wint < 0);
//                TOOLS_DUMP(ToDouble(b));
//                TOOLS_DUMP(static_cast<double>(b_wint));
//                TOOLS_DUMP(b.NegativeQ());
//                TOOLS_DUMP(b_wint < 0);
//                TOOLS_DUMP(static_cast<double>(r));
                return r;
            },
            []( cref<S> a, cref<S> b, cref<S> c ) -> S
            {
                S r_wint = a * b + c;
//                TOOLS_DUMP(static_cast<double>(a));
//                TOOLS_DUMP(static_cast<double>(b));
//                TOOLS_DUMP(static_cast<double>(r_wint));
                return r_wint;
            },
            rand, n, reps, verboseQ
        );
    }
    
    template<Size_T limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    double Test_long_det( Size_T n, Size_T reps, bool verboseQ = false )
    {
        using T  = WideInt <limb_count,Limb_T,Comp_T,signQ>;
        using S  = CheckInt<2 * limb_count,Limb_T,signQ>;
        
        PRNG_T engine = InitializedRandomEngine<PRNG_T>();
        
        std::uniform_int_distribution<Limb_T> dist (
            std::numeric_limits<Limb_T>::lowest(), std::numeric_limits<Limb_T>::max()
        );
        
        auto rand = [&engine,&dist](){ return dist(engine); };

        return TestWideInt("long_det",
            []( cref<T> a, cref<T> b, cref<T> c, cref<T> d ) -> S
            {
                S r; wide_convert(long_det(a,b,c,d),r); return r;
            },
            []( cref<S> a, cref<S> b, cref<S> c, cref<S> d ) -> S
            {
                return a * d - b * c;
            },
            rand, n, reps, verboseQ
        );
    }

} // namespace Knoodle
