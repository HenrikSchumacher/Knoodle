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
        
        using Matrix_T     = Sparse::MatrixCSR<Scal,Int,LInt>;
        using PD_T         = PlanarDiagram<Int>;
        using Aggregator_T = TripleAggregator<Int,Int,Scal,LInt>;
        
    
        Alexander()  = default;
        
        ~Alexander() = default;
        

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
        
        
        
        Scal KnotDeterminant( cref<PD_T> pd ) const
        {
            ptic("KnotDeterminant");

            auto & M = Matrix(pd,-1);
            
            auto A = M.Transpose().Dot(M);
            
            auto perm = Metis<Int>()( A.Outer().data(), A.Inner().data(), A.RowCount(), Int(1) );
            
            Sparse::CholeskyDecomposition<Scal,Int,LInt> S (
                A.Outer().data(), A.Inner().data(), std::move(perm)
            );

            S.SymbolicFactorization();
            
            S.NumericFactorization( A.Values().data() );

            const auto & U = S.GetU();
            
            Scal det = 1;

            for( Int i = 0; i < U.RowCount(); ++i )
            {
                det *= U.Value(U.Outer(i));
            }

            ptoc("KnotDeterminant");

            return det;
        }
        
        Scal Log2KnotDeterminant( cref<PD_T> pd ) const
        {
            ptic("Log2KnotDeterminant");

            auto & M = Matrix(pd,-1);
            
            auto A = M.Transpose().Dot(M);

            auto perm = Metis<Int>()( A.Outer().data(), A.Inner().data(), A.RowCount(), Int(1) );
            
            Sparse::CholeskyDecomposition<Scal,Int,LInt> S (
                A.Outer().data(), A.Inner().data(), std::move(perm)
            );

            S.SymbolicFactorization();
            
            S.NumericFactorization( A.Values().data() );
            
            const auto & U = S.GetU();

            Scal log2_det = 0;

            for( Int i = 0; i < U.RowCount(); ++i )
            {
                log2_det += std::log2( U.Value(U.Outer(i)) );
            }

            ptoc("Log2KnotDeterminant");

            return log2_det;
        }
        
        Permutation<Int> MetisPermutation( mref<PD_T> pd ) const
        {
            std::string tag ( "MetisPermutation" );
            
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
        
        
        
        static std::string ClassName()
        {
            return std::string("Alexander")+"<"+
                TypeName<Scal>+","+TypeName<Int>+","+TypeName<LInt>
            +">";
        }
        
    }; // class Alexander
    
    
} // namespace KnotTools
