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
        using Int  = Int_;
        using LInt = LInt_;
        
        ASSERT_SIGNED_INT(Int);
        ASSERT_INT(LInt);
        
        using Matrix_T        = Sparse::MatrixCSR<Scal,Int,LInt>;
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
        
        Scal GetArg( cref<PD_T> pd ) const
        {
            std::string tag ( "AlexanderArgument" );
            
            if( !pd.InCacheQ(tag) )
            {
                pd.SetCache( tag, std::make_any<Scal>( -1 ) );

                pd.ClearCache( std::string("AlexanderMatrix") );
            }
            
            auto bla = pd.GetCache( tag );
            
            return std::any_cast<Scal &>( bla );
        }
        
        void SetArg( cref<PD_T> pd, const Scal t ) const
        {
            if( t != GetArg( pd ) )
            {
                pd.SetCache( std::string ( "AlexanderArgument" ), std::make_any<Scal>( t ) );
                pd.ClearCache( std::string("AlexanderMatrix") );
            }
        }
        
        
        cref<Matrix_T> Matrix( cref<PD_T> pd, const Scal t_ ) const
        {
            SetArg( pd, t_ );
            
            return Matrix( pd );
        }
        
        cref<Matrix_T> Matrix( cref<PD_T> pd ) const
        {
            std::string tag ( "AlexanderMatrix" );
            
            ptic(ClassName()+"::Matrix");
            
            if( !pd.InCacheQ(tag) )
            {
                const Scal t = GetArg( pd );
                
                const Int crossing_count = pd.CrossingCount();
                
                
                std::vector<Aggregator_T> Agg;
                
                Agg.emplace_back(3 * crossing_count);
                
                mref<Aggregator_T> agg = Agg[0];
                
                const Tensor1<Int,Int> over_arc_indices = pd.OverArcIndices();
                
                const auto & C_arcs  = pd.Crossings();
                
                cptr<CrossingState> C_state = pd.CrossingStates().data();
                
                Int counter = 0;
                
                const Int size = crossing_count-1;
                
                const Scal v [3] = { Scalar::One<Scal> - t, -Scalar::One<Scal>, t};
                
                for( Int c = 0; c < C_arcs.Size(); ++c )
                {
                    if( counter >= size )
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
                            
                            if( i < size )
                            {
                                agg.Push( counter, i, v[0] );
                            }
                            
                            if( j < size )
                            {
                                agg.Push( counter, j, v[1] );
                            }
                            
                            if( k < size )
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
                            
                            
                            if( i < size )
                            {
                                agg.Push(counter, i, v[0] );
                            }
                            
                            if( j < size )
                            {
                                agg.Push(counter, j, v[2] );
                            }
                            
                            if( k < size )
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
                
                pd.SetCache( tag,
                    std::make_any<Matrix_T>( Agg, size, size, Int(1), true, false )
                );
            }
            
            ptoc(ClassName()+"::Matrix");
            
            return std::any_cast<Matrix_T &>( pd.GetCache(tag) );
        }
        
        cref<Matrix_T> AlexanderATA( cref<PD_T> pd ) const
        {
            std::string tag ( "AlexanderATA" );
            
            ptic(ClassName()+"::AlexanderATA");
            
            if( !pd.InCacheQ(tag) )
            {
                auto & A = Matrix(pd);
                
                auto AT = A.Transpose();
                
                pd.SetCache( tag,
                    std::make_any<Matrix_T>( AT.Dot(A) )
                );
            }
            
            ptoc(ClassName()+"::AlexanderATA");
            
            return std::any_cast<Matrix_T &>( pd.GetCache(tag) );
        }
        
        std::unique_ptr<Factorization_T> AlexanderFactorization( cref<PD_T> pd ) const
        {   
            auto & B = AlexanderATA(pd);
            
            Permutation<Int> perm = MetisReordering( pd );

//            Permutation<Int> perm ( B.RowCount(), 1 );
            
            std::unique_ptr<Factorization_T> S = std::make_unique<Factorization_T>(
                B.Outer().data(), B.Inner().data(), std::move(perm)
            );
            
            S->SymbolicFactorization();
            
            S->NumericFactorization( B.Values().data(), Scal(0) );
            
            return std::move(S);
        }
        
        
        cref<Permutation<Int>> AlexanderReordering( cref<PD_T> pd ) const
        {
            return AlexanderFactorization(pd)->GetPermutation();
        }
        
        Scal KnotDeterminant( cref<PD_T> pd ) const
        {
            
            ptic(ClassName()+"::KnotDeterminant");
            
            if( pd.CrossingCount() <= 1)
            {
                ptoc(ClassName()+"::KnotDeterminant");
                return 1;
            }
            
            Scal det = 1;
            
            const Int n = pd.CrossingCount() - 1;
            
            if( n > sparsity_threshold )
            {
                // Using sparse Cholesky factorization.
                
                // TODO: Replace by more accurate LU factorization.

                std::unique_ptr<Factorization_T> S = std::move( AlexanderFactorization(pd) );
                
                const auto & U = S->GetU();
                
                for( Int i = 0; i < n; ++i )
                {
                    det *= U.Value(U.Outer(i));
                }
            }
            else
            {
                // Using dense LU factorization.
                
                Matrix(pd).WriteDense( LU_buffer.data(), n ) ;
                                
                int info = LAPACK::getrf( n, n, LU_buffer.data(), n, LU_perm.data() );
               
                if( info == 0 )
                {
                    for( Int i = 0; i < n; ++i )
                    {
                        det *= LU_buffer( (n+1) * i );
                    }
                }
                else
                {
                    det = 0;
                }
            }

            ptoc(ClassName()+"::KnotDeterminant");

            return Abs(det);
        }
        
        Scal Log2KnotDeterminant( cref<PD_T> pd, Int sparsity_threshold = 1024 ) const
        {
            
            ptic(ClassName()+"::Log2KnotDeterminant");

            if( pd.CrossingCount() <= 1)
            {
                ptoc(ClassName()+"::Log2KnotDeterminant");
                return 0;
            }
            
            Scal log2_det = 0;
            
            const Int n = pd.CrossingCount() - 1;
            
            if( n > LU_buffer.Dimension(0) )
            {
                // Using sparse Cholesky factorization.
                
                // TODO: Replace by more accurate LU factorization.
                
                std::unique_ptr<Factorization_T> S = std::move( AlexanderFactorization(pd) );
                
                const auto & U = S->GetU();
                
                for( Int i = 0; i < n; ++i )
                {
                    log2_det += std::log2( U.Value(U.Outer(i)) );
                }
            }
            else
            {
                // Using dense LU factorization.
                
                Matrix(pd).WriteDense( LU_buffer.data(), n ) ;
                                
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
                    if constexpr ( std::numeric_limits<Scal>::has_infinity )
                    {
                        log2_det = -std::numeric_limits<Scal>::infinity();
                    }
                    else
                    {
                        log2_det = std::numeric_limits<Scal>::quiet_NaN;
                    }
                }
            }

            ptoc(ClassName()+"::Log2KnotDeterminant");

            return log2_det;
        }
        
        
        mref<Permutation<Int>> MetisReordering( cref<PD_T> pd ) const
        {
            std::string tag ( "MetisReordering" );

            std::string m_tag ( "AlexanderMatrix" );


            ptic(ClassName()+"::"+tag);

            if( !pd.InCacheQ(tag) )
            {
                auto & M = Matrix( pd );

                auto A = M.Transpose().Dot(M);

                Metis<Int> metis;

                Permutation<Int> perm = metis(
                    A.Outer().data(), A.Inner().data(), A.RowCount(), Int(1)
                );

                pd.SetCache( tag, std::move(perm) );
            }

            ptoc(ClassName()+"::"+tag);

            return std::any_cast<Permutation<Int> &>( pd.GetCache(tag) );
            
        }
        
        LInt SparsityThreshold() const
        {
            return sparsity_threshold;
        }
        
        
        static std::string ClassName()
        {
            return std::string("Alexander")+"<"+
                TypeName<Scal>+","+TypeName<Int>+","+TypeName<LInt>
            +">";
        }
        
    }; // class Alexander
    
    
} // namespace KnotTools
