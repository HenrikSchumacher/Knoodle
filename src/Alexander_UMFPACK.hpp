#pragma once

#include "../submodules/Tensors/UMFPACK.hpp"
// This requires linking agains libumfpack

//#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"
////#include "../submodules/Tensors/src/Sparse/Metis.hpp"
//
#include "../submodules/Tensors/src/BLAS_Wrappers.hpp"
#include "../submodules/Tensors/src/LAPACK_Wrappers.hpp"


namespace KnotTools
{
    
    template<typename Scal_, typename Int_>
    class Alexander_UMFPACK
    {
        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Scal    = Scal_;
        using Real    = Scalar::Real<Scal>;
        using Int     = Int_;
        using LInt    = Int_;
        
        using Complex = Scalar::Complex<Real>;

        
        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
        using Pattern_T         = Sparse::MatrixCSR<Complex,Int,LInt>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        using Factorization_T   = UMFPACK<Scal,LInt>;
        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
        
        using UMFPACK_T         = UMFPACK<Scal,Int>;
        using UMFPACK_Ptr       = std::shared_ptr<UMFPACK_T>;
        
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        
        Alexander_UMFPACK()
        :   sparsity_threshold ( 256 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_ipiv   ( sparsity_threshold )
        {}
        
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
      
        ~Alexander_UMFPACK() = default;
        
    private:
        
        Int sparsity_threshold;
        
        mutable Tensor1<Scal,Int> LU_buffer;
        
        mutable Tensor1<LAPACK::Int,Int> LU_ipiv;

    public:
        
        
        LInt SparsityThreshold() const
        {
            return sparsity_threshold;
        }
        
#include "Alexander/Alexander_Strands_Dense.hpp"
#include "Alexander/Alexander_Strands_Sparse.hpp"
        
//#include "Alexander/Alexander_Faces_Dense.hpp"
//#include "Alexander/Alexander_Faces_Sparse.hpp"

        
        template<typename ExtScal, typename ExtInt>
        void Alexander(
            cref<PD_T>    pd,
            cptr<ExtScal> args,
            ExtInt        arg_count,
            mptr<ExtScal> mantissas,
            mptr<ExtInt>  exponents
        ) const
        {
            if( pd.CrossingCount() > sparsity_threshold + 1 )
            {
                Alexander_Strands_Sparse( pd, args, arg_count, mantissas, exponents );
            }
            else
            {
                Alexander_Strands_Dense ( pd, args, arg_count, mantissas, exponents );
            }
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Alexander_UMFPACK")+ "<" + TypeName<Scal> + "," + TypeName<Int> + "," + TypeName<LInt> + ">";
        }
        
    }; // class Alexander_UMFPACK
    
    
} // namespace KnotTools

