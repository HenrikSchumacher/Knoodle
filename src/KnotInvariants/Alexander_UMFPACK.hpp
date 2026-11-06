#pragma once

#include "../../submodules/Tensors/UMFPACK.hpp"
// This requires linking against libumfpack

#include "../../submodules/Tensors/src/BLAS_Wrappers.hpp"
#include "../../submodules/Tensors/src/LAPACK_Wrappers.hpp"


namespace Knoodle
{
    
    /*!@brief A class to numerically evaluate the Alexander polynomial using sparse matrix arithmetic. Our Alexander polynomial is normalized so that `A(1/t) = A(t)` and `A(1) = 1`.*/
    template<typename Scal_, IntQ Int_>
    class Alexander_UMFPACK final
    {
    public:
        
        using Scal    = Scal_;
        using Real    = Scalar::Real<Scal>;
        using Int     = Int_;
        using LInt    = Int_;
        
        using Int_UMF = ToSigned<Int>;
        using Complex = Scalar::Complex<Real>;

        using Class_T = Alexander_UMFPACK;
        
        using Exponent_T   = Int64; // Integer for storing exponents.
        
        using Multiplier_T = ProductAccumulator<Scal,Exponent_T>;
        
        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt,Sequential>;
        using Pattern_T         = Sparse::MatrixCSR<Complex,Int,LInt,Sequential>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt,Sequential>;

        using PD_T              = PlanarDiagram<Int>;
        using A_Cross_T         = typename PD_T::A_Cross_T;
        using C_Arcs_T          = typename PD_T::C_Arcs_T;
        
        using UMFPACK_T         = UMFPACK<Scal,Int_UMF>;
        using UMFPACK_Ptr       = std::shared_ptr<UMFPACK_T>;
        
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        
        Alexander_UMFPACK( const Int sparsity_threshold_ )
        :   sparsity_threshold (
                std::min(
                    sparsity_threshold_,
                    static_cast<Int>(std::floor(std::sqrt(std::abs(std::numeric_limits<Int>::max()))))
                )
            )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_ipiv   ( sparsity_threshold )
        {}
        
        // Default constructor
        Alexander_UMFPACK()
        :   sparsity_threshold ( 256 )
//        :   sparsity_threshold ( 1 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_ipiv   ( sparsity_threshold )
        {}
        
    private:
        
        Int sparsity_threshold;
        
        mutable Tensor1<Scal,Int> LU_buffer;
        
        mutable Tensor1<LAPACK::Int,Int> LU_ipiv;
        
        AlexanderStrandMatrix<Scal,Int,LInt> A;

    public:
        
        
        LInt SparsityThreshold() const
        {
            return sparsity_threshold;
        }
        
#include "Alexander_UMFPACK/Alexander_Strands_Dense.hpp"
#include "Alexander_UMFPACK/Alexander_Strands_Sparse.hpp"
#include "Alexander_UMFPACK/Alexander_Strands.hpp"
        
    public:

        template<typename ExtScal, IntQ ExtInt>
        int Alexander(
            cref<PD_T>    pd,
            ExtScal       arg,
            mref<ExtScal> mantissa,
            mref<ExtInt>  exponent,
            bool          multiply_toQ
        ) const
        {
            if( pd.LinkComponentCount() > Int(1) )
            {
                Msgr::eprint("Alexander","Argument pd represents a multiple-component link for with the Alexander polynomial is not defined. Aborting.");
                return 1;
            }
            
            if( pd.CrossingCount() > sparsity_threshold + 1 )
            {
                // Use sparse code path.
                Alexander_Strands<true >( pd, arg, mantissa, exponent, multiply_toQ );
            }
            else
            {
                // Use dense code path.
                Alexander_Strands<false>( pd, arg, mantissa, exponent, multiply_toQ );
            }
            
            return 0;
        }
        
        template<typename ExtScal, IntQ ExtInt>
        int Alexander(
            cref<PD_T>    pd,
            cptr<ExtScal> args,
            ExtInt        arg_count,
            mptr<ExtScal> mantissas,
            mptr<ExtInt>  exponents,
            bool          multiply_toQ
        ) const
        {
            if( pd.LinkComponentCount() > Int(1) )
            {
                Msgr::eprint("Alexander","Argument pd represents a multiple-component link for with the Alexander polynomial is not defined. Aborting.");
                return 1;
            }
            
            if( pd.CrossingCount() > sparsity_threshold + 1 )
            {
                // Use sparse code path.
                Alexander_Strands<true >( pd, args, arg_count, mantissas, exponents, multiply_toQ );
            }
            else
            {
                // Use dense code path.
                Alexander_Strands<false>( pd, args, arg_count, mantissas, exponents, multiply_toQ );
            }
            
            return 0;
        }
        
        
        
    public:
        
        
        using Msgr = Tools::Messenger<Class_T>;
                
        template<typename A>
        static consteval auto MethodName( const A & tag )
        {
            return Msgr::MethodName(tag);
        }
        
        static consteval auto ClassName()
        {
            return ct_string("Alexander_UMFPACK")
                + "<" + TypeName<Scal>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + ">";
        }
        
    }; // class Alexander_UMFPACK
    
    
} // namespace Knoodle

