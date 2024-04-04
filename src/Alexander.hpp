#pragma once

#include "../submodules/Tensors/Sparse.hpp"
#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"
//#include "../submodules/Tensors/src/Sparse/Metis.hpp"

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
        
        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        
        using Helper_T = Tensor2<Scal,LInt>;
        
        using Factorization_T   = Sparse::CholeskyDecomposition<Scal,Int,LInt>;
        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
        using Aggregator_T      = TripleAggregator<Int,Int,Scal,LInt>;
        
        Alexander()
        :   sparsity_threshold ( 64 )
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
        
        mutable Tensor1<LAPACK::Int,Int> LU_perm;
        
        mutable Tensor1<Real,Int> diagonal;
        
        
//        mutable Factorization_Ptr  herm_alex_fact;
//        
//        mutable Tensor2<Scal,LInt> herm_alex_help;
        
        mutable Tensor1<Scal,LInt> herm_alex_vals;

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
            
            const auto & over_arc_indices = pd.OverArcIndices();
            
            const auto & C_arcs  = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            const Scal v [3] = { Scal(1) - t, Scal(-1), t};
            
            Int counter = 0;
            
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
        
        SparseMatrix_T SparseAlexanderMatrix( cref<PD_T> pd, const int degree ) const
        {
            ptic(ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+")");

            if( (degree != 0) && (degree!= 1) )
            {
                eprint(ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+"): degree "+ToString(degree)+" is not a valid degree. Only 0 and 1 are allowed.");
                
                ptoc(ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+")");
                
                return SparseMatrix_T();
            }
            
            const Int n = pd.CrossingCount()-1;
            
            if( n <= 0 )
            {
                ptoc(ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+")");
                
                return SparseMatrix_T();
            }

            std::vector<Aggregator_T> Agg;

            Agg.emplace_back(3 * n);

            mref<Aggregator_T> agg = Agg[0];

            const auto & over_arc_indices = pd.OverArcIndices();

            const auto & C_arcs  = pd.Crossings();

            cptr<CrossingState> C_state = pd.CrossingStates().data();

//            const Scal v [3] = { Scal(1) - t, Scal(-1), t};
            const Scal v [3] = {
                (degree == 0) ? Scal( 1) : Scal(-1),
                (degree == 0) ? Scal(-1) : Scal( 0),
                (degree == 0) ? Scal( 0) : Scal( 1)
            };


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
<<<<<<< HEAD
            
            agg.Finalize();
            
            SparseMatrix_T A ( Agg, n, n, Int(1), true, false );
            
            ptoc(ClassName()+"::SparseAlexanderMatrix");
            
=======

            agg.Finalize();
            
            

            SparseMatrix_T A ( Agg, n, n, Int(1), true, false );

            ptoc(ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+")");

>>>>>>> ed7bc7aa567dd9bfa9bed4068c9a9366b854928c
            return A;
        }

        
        void RequireSparseHermitianAlexanderMatrix( cref<PD_T> pd  ) const
        {
            ptic(ClassName()+"::RequireSparseHermitianAlexanderMatrix");
            
            std::string tag_help ( std::string( "SparseHermitianAlexanderHelpers_" ) + TypeName<Scal>);
            std::string tag_fact ( std::string( "SparseHermitianAlexanderFactorization_" ) + TypeName<Scal> );

            if( (!pd.InCacheQ(tag_fact)) || (!pd.InCacheQ(tag_help)) )
            {
                const Int n = pd.CrossingCount()-1;
                
                std::vector<Aggregator_T> Agg_0;
                std::vector<Aggregator_T> Agg_1;
                
                Agg_0.emplace_back(3 * n);
                Agg_1.emplace_back(3 * n);
                
                mref<Aggregator_T> agg_0 = Agg_0[0];
                mref<Aggregator_T> agg_1 = Agg_1[0];
                
                const auto & over_arc_indices = pd.OverArcIndices();
                
                const auto & C_arcs = pd.Crossings();
                
                cptr<CrossingState> C_state = pd.CrossingStates().data();
                
                const Scal v_0 [3] = { Scal( 1), Scal(-1), Scal( 0) };
                const Scal v_1 [3] = { Scal(-1), Scal( 0), Scal( 1) };
                
                Int counter = 0;
                
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
                                agg_0.Push( counter, i, v_0[0] );
                                agg_1.Push( counter, i, v_1[0] );
                            }
                            
                            if( j < n )
                            {
                                agg_0.Push( counter, j, v_0[1] );
                                agg_1.Push( counter, j, v_1[1] );
                            }
                            
                            if( k < n )
                            {
                                agg_0.Push( counter, k, v_0[2] );
                                agg_1.Push( counter, k, v_1[2] );
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
                                agg_0.Push(counter, i, v_0[0] );
                                agg_1.Push(counter, i, v_1[0] );
                            }
                            
                            if( j < n )
                            {
                                agg_0.Push(counter, j, v_0[2] );
                                agg_1.Push(counter, j, v_1[2] );
                            }
                            
                            if( k < n )
                            {
                                agg_0.Push(counter, k, v_0[1] );
                                agg_1.Push(counter, k, v_1[1] );
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
                
                agg_0.Finalize();
                agg_1.Finalize();
                
                SparseMatrix_T A_0 ( Agg_0, n, n, Int(1), true, false );
                SparseMatrix_T A_1 ( Agg_1, n, n, Int(1), true, false );
                
                SparseMatrix_T AH_0 = A_0.ConjugateTranspose();
                SparseMatrix_T AH_1 = A_1.ConjugateTranspose();
                
                SparseMatrix_T AHA = AH_0.Dot( A_0 );
                
                
                Sparse::ApproximateMinimumDegree<Int> reorderer;
                
    //                Sparse::Metis<Int> reorderer;

                Permutation<Int> perm = reorderer(
                    AHA.Outer().data(), AHA.Inner().data(), AHA.RowCount(), Int(1)
                );
                
                // Create Cholesky factoriation.
                
                Factorization_Ptr S = std::make_shared<Factorization_T>(
                    AHA.Outer().data(), AHA.Inner().data(), std::move(perm)
                );
                
                S->SymbolicFactorization();
                
                
                pd.SetCache( tag_fact, std::move(S) );
                
                // TODO: We could reorder AHA here to (manginally) speed the numeric factorization.
                
                // TODO: Do we really have to compute 2 sparse transposes and 4 sparse dots?
                Tensor2<Scal,LInt> herm_alex_help ( 4, AHA.NonzeroCount() );
                
                AHA.Values().Write( herm_alex_help.data(0) );
                
                AHA = AH_0.Dot( A_1 );
                AHA.Values().Write( herm_alex_help.data(1) );
                
                AHA = AH_1.Dot( A_0 );
                AHA.Values().Write( herm_alex_help.data(2) );
                
                AHA = AH_1.Dot( A_1 );
                AHA.Values().Write( herm_alex_help.data(3) );
                
                pd.SetCache( tag_help, std::move(herm_alex_help) );
            }

            ptoc(ClassName()+"::RequireSparseHermitianAlexanderMatrix");
        }
        
        void LogAlexanderModuli_Sparse(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            ptic(ClassName()+"::LogAlexanderModuli_Sparse");
            
            if( pd.CrossingCount() <= 1 )
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                RequireSparseHermitianAlexanderMatrix( pd );
                
                std::string tag_help ( std::string("SparseHermitianAlexanderHelpers_") + TypeName<Scal> );
                
                cref<Helper_T> herm_alex_help = std::any_cast<Helper_T &>( pd.GetCache( tag_help )
                );
                
                std::string tag_fact ( std::string("SparseHermitianAlexanderFactorization_") + TypeName<Scal> );
        
                mref<Factorization_Ptr> S = std::any_cast<Factorization_Ptr &>( pd.GetCache( tag_fact ) );
                
                const Int n = pd.CrossingCount() - 1;
                
                const LInt nnz = S->NonzeroCount();
                
                diagonal.RequireSize(n);
                
                herm_alex_vals.RequireSize( nnz );

                for( Int idx = 0; idx < arg_count; ++idx )
                {
                    // TODO: Replace by more accurate LU factorization.
                    
                    const Scal t   = args[idx];
                    const Scal tc  = Conj(t);
                    const Scal tt  = t * tc;

                    
                    for( LInt i = 0; i < nnz; ++i )
                    {
                        herm_alex_vals[i]
                        =          herm_alex_help[0][i]
                            + t  * herm_alex_help[1][i]
                            + tc * herm_alex_help[2][i]
                            + tt * herm_alex_help[3][i];
                    }
                    
                    S->NumericFactorization( herm_alex_vals.data() );
                    
                    const Real log_det = S->FactorLogDeterminant();
                    
                    if( NaNQ(log_det) )
                    {
                        pprint("NaN detected. Aborting.");
                        results[idx] = std::numeric_limits<Real>::lowest();
                    }
                    else
                    {
                        results[idx] = log_det;
                    }
                }
            }

            ptoc(ClassName()+"::LogAlexanderModuli_Sparse");
        }

        void LogAlexanderModuli(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            if( pd.CrossingCount() > sparsity_threshold + 1 )
            {
                LogAlexanderModuli_Sparse( pd, args, arg_count, results );
            }
            else
            {
                LogAlexanderModuli_Dense ( pd, args, arg_count, results );
            }
        }
        
    private:
        
        void LogAlexanderModuli_Dense(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            ptic(ClassName()+"::LogAlexanderModuli_Dense");
            
            if( pd.CrossingCount() <= 1 )
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                const Int n = pd.CrossingCount() - 1;
                
                for( Int idx = 0; idx < arg_count; ++idx )
                {
                    Real log_det = 0;
                                        
                    DenseAlexanderMatrix( pd, args[idx], LU_buffer.data() );
                    
//                    valprint( "dense array", ArrayToString( LU_buffer.data(), {n,n} ) );
                    
                    // Factorize dense Alexander matrix.
                    
                    int info = LAPACK::getrf( n, n, LU_buffer.data(), n, LU_perm.data() );
                    
                    if( info == 0 )
                    {
                        for( Int i = 0; i < n; ++i )
                        {
                            log_det += std::log( Abs( LU_buffer( (n+1) * i ) ) );
                        }
                    }
                    else
                    {
                        log_det = std::numeric_limits<Real>::lowest();
                    }
                    
                    results[idx] = log_det;
                }
            }
        
            ptoc(ClassName()+"::LogAlexanderModuli_Dense");
        }
        
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Alexander")+ "<" + TypeName<Scal> + "," + TypeName<Int> + "," + TypeName<LInt> + ">";
        }
        
    }; // class Alexander
    
    
} // namespace KnotTools
