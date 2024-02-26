#pragma once

#include "../submodules/Tensors/Sparse.hpp"
#include "../submodules/Tensors/src/Sparse/Metis.hpp"

namespace KnotTools
{
    
    template<typename Scal_, typename Int_, typename LInt_>
    class Alexander
    {
        
    public:
        
        using Scal = Scal_;
        using Real = Scalar::Real<Scal>;
        using Int  = Int_;
        using LInt = LInt_;
        
        ASSERT_SIGNED_INT(Int);
        ASSERT_INT(LInt);
        
        using SparseMatrix_T  = Sparse::MatrixCSR<Scal,Int,LInt>;
        using Factorization_T = Sparse::CholeskyDecomposition<Scal,Int,LInt>;
        using PD_T            = PlanarDiagram<Int>;
        using Aggregator_T    = TripleAggregator<Int,Int,Scal,LInt>;
        
        Alexander()
        :   sparsity_threshold ( 1024 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
        
        Alexander( const Int sparsity_threshold_ )
        :   sparsity_threshold ( 
                std::min(
                    sparsity_threshold_,
                    static_cast<Int>(std::floor(std::sqrt(std::abs(std::numeric_limits<Int>::max()))))
                )
            )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
      
        
        ~Alexander() = default;
        
    private:
        
        Int sparsity_threshold;
        
        mutable Tensor1<Scal,Int> LU_buffer;
        
        mutable Tensor1<int,Int>  LU_perm;

    public:
        
        
        LInt SparsityThreshold() const
        {
            return sparsity_threshold;
        }
    
        void DenseAlexanderMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
        {
            // Writes the dense Alexander matrix to the provided buffer A.
            // User is responsible for making sure that the buffer is large enough.
            ptic(ClassName()+"::DenseAlexanderMatrix");
            
            // Assemble dense Alexander matrix, skipping last row and last column.

            
            const Int n = pd.CrossingCount() - 1;
            
            
//            SparseAlexanderMatrix( pd, t ).WriteDense( A, n );
//
//            valprint( "sparse matrix", ArrayToString( A, {n,n} ) );

            
            Int counter = 0;

            
            const auto & over_arc_indices = pd.OverArcIndices();
            
            const auto & C_arcs  = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            const Scal v [3] = { Scal(1) - t, Scal(-1), t};
            
            for( Int c = 0; c < C_arcs.Size(); ++c )
            {
                if( counter >= n )
                {
                    break;
                }
                
                switch( C_state[c] )
                {
                    case CrossingState::Negative:
                    {
                        const Tiny::Matrix<2,2,Int,Int> C ( C_arcs.data(c) );
                    
                        const Int i = over_arc_indices[C[1][0]];
                        const Int j = over_arc_indices[C[1][1]];
                        const Int k = over_arc_indices[C[0][0]];
                        
                        mptr<Scal> row = &A[ n * counter ];
                        
                        zerofy_buffer( row, n );
                        
                        if( i < n )
                        {
                            row[i] += v[0];
                        }
                        
                        if( j < n )
                        {
                            row[j] += v[1];
                        }
                        
                        if( k < n )
                        {
                            row[k] += v[2];
                        }
                        
                        ++counter;
                        
                        break;
                    }
                    case CrossingState::Positive:
                    {
                        const Tiny::Matrix<2,2,Int,Int> C ( C_arcs.data(c) );
                    
                        const Int i = over_arc_indices[C[1][1]];
                        const Int j = over_arc_indices[C[1][0]];
                        const Int k = over_arc_indices[C[0][1]];
                        
                        mptr<Scal> row = &A[ n * counter ];
                        
                        zerofy_buffer( row, n );
                        
                        if( i < n )
                        {
                            row[i] += v[0];
                        }
                        
                        if( j < n )
                        {
                            row[j] += v[2];
                        }
                        
                        if( k < n )
                        {
                            row[k] += v[1];
                        }
                        
                        ++counter;
                        
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            
//            valprint( "dense matrix", ArrayToString( A, {n,n} ) );
            
            ptoc(ClassName()+"::DenseAlexanderMatrix");
        }
        
        SparseMatrix_T SparseAlexanderMatrix( cref<PD_T> pd, const Scal t ) const
        {
            ptic(ClassName()+"::SparseAlexanderMatrix");
            
            // TODO: Insert shortcut for crossing_count <= 1.
            
            const Int n = pd.CrossingCount()-1;
            
            std::vector<Aggregator_T> Agg;
            
            Agg.emplace_back(3 * n);
            
            mref<Aggregator_T> agg = Agg[0];
            
            const Tensor1<Int,Int> over_arc_indices = pd.OverArcIndices();
            
            const auto & C_arcs  = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            Int counter = 0;

            const Scal v [3] = { Scal(1) - t, Scal(-1), t};
            
            for( Int c = 0; c < C_arcs.Size(); ++c )
            {
                if( counter >= n )
                {
                    break;
                }
                
                switch( C_state[c] )
                {
                    case CrossingState::Negative:
                    {
                        const Tiny::Matrix<2,2,Int,Int> C ( C_arcs.data(c) );
                    
                        const Int i = over_arc_indices[C[1][0]];
                        const Int j = over_arc_indices[C[1][1]];
                        const Int k = over_arc_indices[C[0][0]];
                        
                        if( i < n )
                        {
                            agg.Push( counter, i, v[0] );
                        }
                        
                        if( j < n )
                        {
                            agg.Push( counter, j, v[1] );
                        }
                        
                        if( k < n )
                        {
                            agg.Push( counter, k, v[2] );
                        }
                        
                        ++counter;
                        
                        break;
                    }
                    case CrossingState::Positive:
                    {
                        const Tiny::Matrix<2,2,Int,Int> C ( C_arcs.data(c) );
                    
                        const Int i = over_arc_indices[C[1][1]];
                        const Int j = over_arc_indices[C[1][0]];
                        const Int k = over_arc_indices[C[0][1]];
                        
                        
                        if( i < n )
                        {
                            agg.Push(counter, i, v[0] );
                        }
                        
                        if( j < n )
                        {
                            agg.Push(counter, j, v[2] );
                        }
                        
                        if( k < n )
                        {
                            agg.Push(counter, k, v[1] );
                        }
                        
                        ++counter;
                        
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            
            agg.Finalize();
            
            SparseMatrix_T A ( Agg, n, n, Int(1), true, false );
            
            ptoc(ClassName()+"::SparseAlexanderMatrix");
            
            return A;
        }
        
        std::shared_ptr<Factorization_T> AlexanderFactorization( cref<PD_T> pd, const Scal t ) const
        {
            std::string tag ( "AlexanderFactorization" );
            
            ptic(ClassName()+"::AlexanderFactorization");
            
            std::shared_ptr<Factorization_T> S;
            
            auto A = SparseAlexanderMatrix( pd, t ) ;
            
            auto AT = A.ConjugateTranspose();
            
            auto B = AT.Dot(A);
            
            if( !pd.InCacheQ(tag) )
            {
                // Finding fill-in reducing reordering.
                
                Metis<Int> metis;

                Permutation<Int> perm = metis(
                    B.Outer().data(), B.Inner().data(), B.RowCount(), Int(1)
                );
                
                // Create Cholesky factoriation.
                
                S = std::make_shared<Factorization_T>(
                    B.Outer().data(), B.Inner().data(), std::move(perm)
                );
                
                S->SymbolicFactorization();
                
                S->NumericFactorization( B.Values().data(), Scal(0) );
                
                pd.SetCache( tag, S );
            }
            else
            {
                // Use old symbolic factorization; just redo numeric factorization.
                
                S = std::any_cast<std::shared_ptr<Factorization_T>>( pd.GetCache( tag ) );
                
                S->NumericFactorization( B.Values().data(), Scal(0) );
            }
            
            ptoc(ClassName()+"::AlexanderFactorization");
            
            return S;
        }

        void Log2AlexanderModuli(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            if( pd.CrossingCount() > sparsity_threshold + 1 )
            {
                Log2AlexanderModuli_Sparse( pd, args, arg_count, results );
            }
            else
            {
                Log2AlexanderModuli_Dense ( pd, args, arg_count, results );
            }
        }
        
    private:
        
        void Log2AlexanderModuli_Dense(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            ptic(ClassName()+"::Log2AlexanderModuli_Dense");
            
            if( pd.CrossingCount() <= 1)
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                const Int n = pd.CrossingCount() - 1;
                
                for( Int idx = 0; idx < arg_count; ++idx )
                {
                    Real log2_det = 0;
                                        
                    DenseAlexanderMatrix(pd, args[idx], LU_buffer.data() );
                    
//                    valprint( "dense array", ArrayToString( LU_buffer.data(), {n,n} ) );
                    
                    // Factorize dense Alexander matrix.
                    
                    int info = LAPACK::getrf( n, n, LU_buffer.data(), n, LU_perm.data() );
                    
                    if( info == 0 )
                    {
                        for( Int i = 0; i < n; ++i )
                        {
                            log2_det += std::log2( Abs( LU_buffer( (n+1) * i ) ) );
                        }
                    }
                    else
                    {
                        if constexpr ( std::numeric_limits<Real>::has_infinity )
                        {
                            log2_det = -std::numeric_limits<Real>::infinity();
                        }
                        else
                        {
                            log2_det = std::numeric_limits<Real>::quiet_NaN;
                        }
                    }
                    
                    results[idx] = log2_det;
                }
            }
        
            ptoc(ClassName()+"::Log2AlexanderModuli_Dense");
        }
        
        
    
        void Log2AlexanderModuli_Sparse(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            ptic(ClassName()+"::Log2AlexanderModuli_Sparse");
            
            if( pd.CrossingCount() <= 1)
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                const Int n = pd.CrossingCount() - 1;
                
                for( Int idx = 0; idx < arg_count; ++idx )
                {
                    Real log2_det = 0;
                    
                    // TODO: Replace by more accurate LU factorization.
                    
                    std::shared_ptr<Factorization_T> S = AlexanderFactorization( pd, args[idx] );
                    
                    const auto & U = S->GetU();
                    
                    for( Int i = 0; i < n; ++i )
                    {
                        log2_det += std::log2( Abs(U.Value(U.Outer(i))) );
                    }
                    
                    results[idx] = log2_det;
                }
            }

            ptoc(ClassName()+"::Log2AlexanderModuli_Sparse");
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Alexander")+ "<" + TypeName<Scal> + "," + TypeName<Int> + "," + TypeName<LInt> + ">";
        }
        
    }; // class Alexander
    
    
} // namespace KnotTools
