#pragma once

#include "../submodules/Tensors/Sparse.hpp"
//#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"
//#include "../submodules/Tensors/src/Sparse/Metis.hpp"

namespace KnotTools
{
    
    template<typename Scal_, typename Int_, typename LInt_>
    class Seifert
    {
        
    public:
        
        using Scal = Scal_;
        using Real = Scalar::Real<Scal>;
        using Int  = Int_;
        using LInt = LInt_;
        
        ASSERT_SIGNED_INT(Int);
        ASSERT_INT(LInt);
        
        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        
        using Helper_T = Tensor2<Scal,LInt>;
        
        using Factorization_T   = Sparse::CholeskyDecomposition<Scal,Int,LInt>;
        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
        using Aggregator_T      = TripleAggregator<Int,Int,Scal,LInt>;
        
        Seifert()
        :   sparsity_threshold ( 64 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
        
        Seifert( const Int sparsity_threshold_ )
        :   sparsity_threshold (
                std::min(
                    sparsity_threshold_,
                    static_cast<Int>(std::floor(std::sqrt(std::abs(std::numeric_limits<Int>::max()))))
                )
            )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
      
        ~Seifert() = default;
        
    private:
        
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Seifert")+ "<" + TypeName<Scal> + "," + TypeName<Int> + "," + TypeName<LInt> + ">";
        }
        
    }; // class Alexander
    
    
} // namespace KnotTools
