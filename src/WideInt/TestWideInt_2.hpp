#pragma once

namespace Knoodle
{
    template<typename F_T, typename G_T, typename RandomFunction_T>
    double TestWideInt_2(
        cref<std::string> test_name, F_T && f, G_T && g, mref<RandomFunction_T> rand,
        Size_T n, Size_T reps,
        bool verboseQ = false
    )
    {
        print( "===  TestWideInt_2 : " + test_name + "  ===");
        using F_traits = function_traits<F_T>;
        using G_traits = function_traits<G_T>;
        
        static_assert( F_traits::arity == 2 );
        static_assert( G_traits::arity == 2 );
        
        using a_S = std::remove_cvref<typename F_traits::template arg<Size_T(0)>::type>::type;
        using b_S = std::remove_cvref<typename F_traits::template arg<Size_T(1)>::type>::type;
        using r_S = std::remove_cvref<typename F_traits::return_type                  >::type;
        
        using a_T = std::remove_cvref<typename G_traits::template arg<Size_T(0)>::type>::type;
        using b_T = std::remove_cvref<typename G_traits::template arg<Size_T(1)>::type>::type;
        using r_T = std::remove_cvref<typename G_traits::return_type                  >::type;
        
        if( verboseQ )
        {
            valprint("a_S",PrettyTypeName<a_S>());
            valprint("b_S",PrettyTypeName<b_S>());
            valprint("r_S",PrettyTypeName<r_S>());
            
            valprint("a_T",PrettyTypeName<a_T>());
            valprint("b_T",PrettyTypeName<b_T>());
            valprint("r_T",PrettyTypeName<r_T>());
        }
        
        Tensor1<a_S,Size_T> A_S ( n );
        Tensor1<b_S,Size_T> B_S ( n );
        Tensor1<r_S,Size_T> R_S ( n );
        
        Tensor1<a_T,Size_T> A_T ( n );
        Tensor1<b_T,Size_T> B_T ( n );
        Tensor1<r_T,Size_T> R_T ( n );
        
        for( Size_T i = 0; i < n; ++i )
        {
            Randomize(A_S[i],rand);
            Randomize(B_S[i],rand);
            
            wide_convert( A_S[i], A_T[i] );
            wide_convert( B_S[i], B_T[i] );
        }
        
        tic("Computing f");
        {
            for( Size_T rep = 0; rep < reps; ++rep )
            {
                for( Size_T i = 0; i < n; ++i )
                {
                    R_S[i] = std::invoke( f, A_S[i], B_S[i] );
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
                    R_T[i] = std::invoke( g, A_T[i], B_T[i] );
                }
            }
        }
        toc("Computing g");
        
        Size_T success_count = 0;
        Size_T idx = static_cast<Size_T>(-1);
        
        for( Size_T i = 0; i < n; ++i )
        {
            if( R_S[i] == R_T[i] )
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
            std::cout << TypeName<a_S> << " a = " << ToString(A_S[idx]) << std::endl;
            std::cout << TypeName<b_S> << " b = " << ToString(B_S[idx]) << std::endl;
            std::cout << TypeName<r_S> << " r = " << ToString(R_S[idx]) << std::endl;
            print("");
            std::cout << TypeName<a_T> << " a = " << ToString(A_T[idx]) << std::endl;
            std::cout << TypeName<b_T> << " b = " << ToString(B_T[idx]) << std::endl;
            std::cout << TypeName<r_T> << " r = " << ToString(R_T[idx]) << std::endl;
            print("");
        }
        
        return percentage;
        
    } // void TestWideInt_2
    
} // namespace Knoodle
