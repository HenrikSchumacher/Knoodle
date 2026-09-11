#pragma once

namespace Knoodle
{
    template<typename F_T, typename G_T, typename RandomFunction_T>
    double TestWideInt(
        cref<std::string> test_name, F_T && f, G_T && g, mref<RandomFunction_T> rand,
        Size_T n, Size_T reps,
        bool verboseQ = false
    )
    {
        using F_traits = function_traits<F_T>;
        using G_traits = function_traits<G_T>;
        
        static_assert( F_traits::arity == G_traits::arity );
        
        constexpr Size_T arity = F_traits::arity;
        
        print( "===  TestWideInt, arity = " + ToString(arity) + " : " + test_name + "  ===");
        
        using Dummy_T = char;
        
        using S0 = std::conditional_t< (arity > Size_T{0}),
                typename std::remove_cvref<typename F_traits::template arg<(arity > Size_T{0}) ? Size_T{0} : Size_T{0}>::type>::type, Dummy_T >;
        using S1 = std::conditional_t< (arity > Size_T{1}),
                typename std::remove_cvref<typename F_traits::template arg<(arity > Size_T{1}) ? Size_T{1} : Size_T{0}>::type>::type, Dummy_T >;
        using S2 = std::conditional_t< (arity > Size_T{2}),
                typename std::remove_cvref<typename F_traits::template arg<(arity > Size_T{2}) ? Size_T{2} : Size_T{0}>::type>::type, Dummy_T >;
        using S3 = std::conditional_t< (arity > Size_T{3}),
                typename std::remove_cvref<typename F_traits::template arg<(arity > Size_T{3}) ? Size_T{3} : Size_T{0}>::type>::type, Dummy_T >;
        
        using R = std::remove_cvref<typename F_traits::return_type>::type;
        
        static_assert( SameQ<R,typename std::remove_cvref<typename G_traits::return_type>::type>, "" );
        
        using T0 = std::conditional_t< (arity > Size_T{0}),
                typename std::remove_cvref<typename G_traits::template arg<(arity > Size_T{0}) ? Size_T{0} : Size_T{0}>::type>::type, Dummy_T >;
        using T1 = std::conditional_t< (arity > Size_T{1}),
                typename std::remove_cvref<typename G_traits::template arg<(arity > Size_T{1}) ? Size_T{1} : Size_T{0}>::type>::type, Dummy_T >;
        using T2 = std::conditional_t< (arity > Size_T{2}),
                typename std::remove_cvref<typename G_traits::template arg<(arity > Size_T{2}) ? Size_T{2} : Size_T{0}>::type>::type, Dummy_T >;
        using T3 = std::conditional_t< (arity > Size_T{3}),
                typename std::remove_cvref<typename G_traits::template arg<(arity > Size_T{3}) ? Size_T{3} : Size_T{0}>::type>::type, Dummy_T >;
        
        if( verboseQ )
        {
            valprint("R",PrettyTypeName<R>());
            
            
            if constexpr (arity > 0 ) { valprint("S0",PrettyTypeName<S0>()); }
            if constexpr (arity > 1 ) { valprint("S1",PrettyTypeName<S1>()); }
            if constexpr (arity > 2 ) { valprint("S2",PrettyTypeName<S2>()); }
            if constexpr (arity > 3 ) { valprint("S3",PrettyTypeName<S3>()); }
            
            
            if constexpr (arity > 0 ) { valprint("T0",PrettyTypeName<T0>()); }
            if constexpr (arity > 1 ) { valprint("T1",PrettyTypeName<T1>()); }
            if constexpr (arity > 2 ) { valprint("T2",PrettyTypeName<T2>()); }
            if constexpr (arity > 3 ) { valprint("T3",PrettyTypeName<T3>()); }
        }

        
        Tensor1<S0,Size_T> A_0 ( n );
        Tensor1<S1,Size_T> A_1 ( n );
        Tensor1<S2,Size_T> A_2 ( n );
        Tensor1<S3,Size_T> A_3 ( n );
        Tensor1<R ,Size_T> R_A ( n );
        
        Tensor1<T0,Size_T> B_0 ( n );
        Tensor1<T1,Size_T> B_1 ( n );
        Tensor1<T2,Size_T> B_2 ( n );
        Tensor1<T3,Size_T> B_3 ( n );
        Tensor1<R ,Size_T> R_B ( n );
        
        for( Size_T i = 0; i < n; ++i )
        {
            if constexpr (arity > 0 )
            {
                Randomize(A_0[i],rand); wide_convert( A_0[i], B_0[i] );
            }
            if constexpr (arity > 1 )
            {
                Randomize(A_1[i],rand); wide_convert( A_1[i], B_1[i] );
            }
            if constexpr (arity > 2 )
            {
                Randomize(A_2[i],rand); wide_convert( A_2[i], B_2[i] );
            }
            if constexpr (arity > 3 )
            {
                Randomize(A_3[i],rand); wide_convert( A_3[i], B_3[i] );
            }
            
        }
        
        tic("Computing f");
        {
            for( Size_T rep = 0; rep < reps; ++rep )
            {
                for( Size_T i = 0; i < n; ++i )
                {
                    if constexpr (arity == 1 )
                    {
                        R_A[i] = std::invoke( f, A_0[i] );
                    }
                    if constexpr (arity == 2 )
                    {
                        R_A[i] = std::invoke( f, A_0[i], A_1[i] );
                    }
                    if constexpr (arity == 3 )
                    {
                        R_A[i] = std::invoke( f, A_0[i], A_1[i], A_2[i] );
                    }
                    if constexpr (arity == 4 )
                    {
                        R_A[i] = std::invoke( f, A_0[i], A_1[i], A_2[i], A_3[i] );
                    }
                }
            }
        }
        toc("Computing f");
        
        tic("Computing g");
        {
            for( Size_T rep = 0; rep < reps; ++rep )
            {
                for( Size_T i = 0; i < n; ++i )
                {
                    if constexpr (arity == 1 )
                    {
                        R_B[i] = std::invoke( g, B_0[i] );
                    }
                    if constexpr (arity == 2 )
                    {
                        R_B[i] = std::invoke( g, B_0[i], B_1[i] );
                    }
                    if constexpr (arity == 3 )
                    {
                        R_B[i] = std::invoke( g, B_0[i], B_1[i], B_2[i] );
                    }
                    if constexpr (arity == 4 )
                    {
                        R_B[i] = std::invoke( g, B_0[i], B_1[i], B_2[i], B_3[i] );
                    }
                }
            }
        }
        toc("Computing g");
        
        Size_T success_count = 0;
        Size_T idx = static_cast<Size_T>(-1);
        
        for( Size_T i = 0; i < n; ++i )
        {
            if( R_A[i] == R_B[i] )
            {
                ++success_count;
            }
            else
            {
                if( idx == static_cast<Size_T>(-1) ){ idx = i; }
            }
        }
        
        double percentage = double(100) * double(success_count) / double(n);
        std::cout << "Success rate = " << percentage << "%\n" << std::endl;
        
        if( success_count < n )
        {
            print("Test failed. Returning first failed example:");
            valprint("idx",idx);
            if constexpr (arity > 0 )
            {
                std::cout << TypeName<S0> << " a_0 = " << ToString(A_0[idx]) << std::endl;
            }
            if constexpr (arity > 1 )
            {
                std::cout << TypeName<S1> << " a_1 = " << ToString(A_1[idx]) << std::endl;
            }
            if constexpr (arity > 2 )
            {
                std::cout << TypeName<S2> << " a_2 = " << ToString(A_2[idx]) << std::endl;
            }
            if constexpr (arity > 3 )
            {
                std::cout << TypeName<S3> << " a_3 = " << ToString(A_3[idx]) << std::endl;
            }
            std::cout << TypeName<R> << " r = " << ToString(R_A[idx]) << std::endl;
            print("");
            if constexpr (arity > 0 )
            {
                std::cout << TypeName<T0> << " b_0 = " << ToString(B_0[idx]) << std::endl;
            }
            if constexpr (arity > 1 )
            {
                std::cout << TypeName<T1> << " b_1 = " << ToString(B_1[idx]) << std::endl;
            }
            if constexpr (arity > 2 )
            {
                std::cout << TypeName<T2> << " b_2 = " << ToString(B_2[idx]) << std::endl;
            }
            if constexpr (arity > 3 )
            {
                std::cout << TypeName<T3> << " b_3 = " << ToString(B_3[idx]) << std::endl;
            }
            std::cout << TypeName<R> << " r = " << ToString(R_B[idx]) << std::endl;
            print("");
        }
        
        return percentage;
        
    } // void TestWideInt

} // namespace Knoodle
