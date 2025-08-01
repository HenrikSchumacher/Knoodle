#pragma once

#include "../submodules/Tensors/Sparse.hpp"
#include "../submodules/Tensors/src/Sparse/Metis.hpp"

namespace Knoodle
{
    
    template<typename Scal_, typename Int_, typename LInt_>
    class Alexander_Metis_LeftLooking final
    {
//        static_assert(SignedIntQ<Int_>,"");
        static_assert(IntQ<LInt_>,"");
        
    public:
        
        using Scal = Scal_;
        using Real = Scalar::Real<Scal>;
        using Int  = Int_;
        using LInt = LInt_;
        
        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
        using Factorization_T   = Sparse::CholeskyDecomposition<Scal,Int,LInt>;
        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
        using Aggregator_T      = TripleAggregator<Int,Int,Scal,LInt>;
        
        Alexander_Metis_LeftLooking( const Int sparsity_threshold_ )
        :   sparsity_threshold (
                std::min(
                    sparsity_threshold_,
                    static_cast<Int>(std::floor(std::sqrt(std::abs(std::numeric_limits<Int>::max()))))
                )
            )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
      
        
        // Default constructor
        Alexander_Metis_LeftLooking()
        :   sparsity_threshold ( 256 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
        
        // Destructor
        ~Alexander_Metis_LeftLooking() = default;
        // Copy constructor
        Alexander_Metis_LeftLooking( const Alexander_Metis_LeftLooking & other ) = default;
        // Copy assignment operator
        Alexander_Metis_LeftLooking & operator=( const Alexander_Metis_LeftLooking & other ) = default;
        // Move constructor
        Alexander_Metis_LeftLooking( Alexander_Metis_LeftLooking && other ) = default;
        // Move assignment operator
        Alexander_Metis_LeftLooking & operator=( Alexander_Metis_LeftLooking && other ) = default;
        
    private:
        
        Int sparsity_threshold;
        
        mutable Tensor1<Scal,Int> LU_buffer;
        
        mutable Tensor1<LAPACK::Int,Int> LU_perm;
        
        mutable Tensor1<Real,Int> diagonal;

    public:
        
        
        LInt SparsityThreshold() const
        {
            return sparsity_threshold;
        }
    
        void DenseAlexanderMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
        {
            // Writes the dense Alexander matrix to the provided buffer A.
            // User is responsible for making sure that the buffer is large enough.
            TOOLS_PTIMER(timer,MethodName("DenseAlexanderMatrix"));
            
            // Assemble dense Alexander matrix, skipping last row and last column.

            
            const Int n = pd.CrossingCount() - 1;
            
            
//            SparseAlexanderMatrix( pd, t ).WriteDense( A, n );
//
//            valprint( "sparse matrix", ArrayToString( A, {n,n} ) );
            
            // TODO: Replace this by pd.CrossingOverStrands() and measure!
            const auto arc_strands = pd.ArcOverStrand();
            
            const auto & C_arcs  = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            const Scal v [3] = { Scal(1) - t, Scal(-1), t};
            
            Int counter = 0;
            
            const Int c_count = C_arcs.Dim(0);
            
            for( Int c = 0; c < c_count; ++c )
            {
                if( counter >= n ) { break; }
                
                if( LeftHandedQ(C_state[c]) )
                {
                    Int C [2][2];
                    copy_buffer<4>( C_arcs.data(c), &C[0][0] );
                
                    const Int i = arc_strands[C[1][0]];
                    const Int j = arc_strands[C[1][1]];
                    const Int k = arc_strands[C[0][0]];
                    
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
                }
                else if( RightHandedQ(C_state[c]) )
                {
                    Int C [2][2];
                    copy_buffer<4>( C_arcs.data(c), &C[0][0] );
                
                    const Int i = arc_strands[C[1][1]];
                    const Int j = arc_strands[C[1][0]];
                    const Int k = arc_strands[C[0][1]];
                    
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
                }
            }
            
//            valprint( "dense matrix", ArrayToString( A, {n,n} ) );
        }
        
        SparseMatrix_T SparseAlexanderMatrix( cref<PD_T> pd, const Scal t ) const
        {
            TOOLS_PTIMER(timer,MethodName("SparseAlexanderMatrix"));
            
            // TODO: Insert shortcut for crossing_count <= 1.
            
            const Int n = pd.CrossingCount()-1;
            
            std::vector<Aggregator_T> Agg;
            
            Agg.emplace_back(3 * n);
            
            mref<Aggregator_T> agg = Agg[0];
            
            // TODO: Replace this by pd.CrossingOverStrands() and measure!
            const auto arc_strands = pd.ArcOverStrands();
            
            const auto & C_arcs  = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            const Scal v [3] = { Scal(1) - t, Scal(-1), t};
            
            Int counter = 0;
            
            const Int c_count = C_arcs.Dim(0);
            
            for( Int c = 0; c < c_count; ++c )
            {
                if( counter >= n ) { break; }
                
                pd.AssertCrossing(c);
                
                if( LeftHandedQ( C_state[c] ) )
                {
                    Int C [2][2];
                    copy_buffer<4>( C_arcs.data(c), &C[0][0] );
                
                    const Int i = arc_strands[C[1][0]];
                    const Int j = arc_strands[C[1][1]];
                    const Int k = arc_strands[C[0][0]];
                    
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
                }
                else if( RightHandedQ(C_state[c]) )
                {
                    Int C [2][2];
                    copy_buffer<4>( C_arcs.data(c), &C[0][0] );
                
                    const Int i = arc_strands[C[1][1]];
                    const Int j = arc_strands[C[1][0]];
                    const Int k = arc_strands[C[0][1]];
                    
                    
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
                }
            }
            
//            agg.Finalize();
            
            SparseMatrix_T A ( Agg, n, n, Int(1), true, false );
            
            return A;
        }
        
        Factorization_Ptr AlexanderFactorization( cref<PD_T> pd, const Scal t ) const
        {
            std::string tag ( "AlexanderFactorization" );
            tag += TypeName<Scal>;
            
            TOOLS_PTIMER(timer,MethodName("AlexanderFactorization"));
            
            auto A = SparseAlexanderMatrix( pd, t ) ;
            
            auto AT = A.ConjugateTranspose();
            
            auto B = AT.Dot(A);

            Factorization_Ptr S;
            
            if( !pd.InCacheQ(tag) )
            {
                // Finding fill-in reducing reordering.
                
                Sparse::Metis<Int> metis;

                Permutation<Int> perm = metis(
                    B.Outer().data(), B.Inner().data(), B.RowCount(), Int(1)
                );
                
//                Permutation<Int> perm ( B.RowCount(), Int(1) );
                
                // Create Cholesky factoriation.
                
                S = std::make_shared<Factorization_T>(
                    B.Outer().data(), B.Inner().data(), std::move(perm)
                );
                
                S->SymbolicFactorization();
                
                S->NumericFactorization_LeftLooking( B.Values().data(), Scal(0) );
                
                pd.SetCache( tag, S );
            }
            else
            {
                // Use old symbolic factorization; just redo numeric factorization.
                
                S = pd.template GetCache<Factorization_Ptr>(tag);
                
                S->NumericFactorization_LeftLooking( B.Values().data(), Scal(0) );
            }
            
            return S;
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
            TOOLS_PTIMER(timer,MethodName("LogAlexanderModuli_Dense"));
            
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
                    
                    int info = LAPACK::getrf<Layout::RowMajor>(
                        n, n, LU_buffer.data(), n, LU_perm.data()
                    );
                    
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
        }
        
        
        void LogAlexanderModuli_Sparse(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            TOOLS_PTIMER(timer,MethodName("LogAlexanderModuli_Sparse"));
            
            if( pd.CrossingCount() <= 1 )
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                const Int n = pd.CrossingCount() - 1;
                
                diagonal.template RequireSize<false>(n);
                
                for( Int idx = 0; idx < arg_count; ++idx )
                {
                    // TODO: Replace by more accurate LU factorization.
                    
                    const auto & S = AlexanderFactorization( pd, args[idx] );
                    
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
        }
        
//    public:
//        
//        static std::string ClassName()
//        {
//            return std::string("Alexander_Metis_LeftLooking")+ "<" + TypeName<Scal> + "," + TypeName<Int> + "," + TypeName<LInt> + ">";
//        }
        
    private:
        
        static constexpr ct_string class_name = ct_string("Alexander_Metis_LeftLooking")
            + "<" + TypeName<Scal>
            + "," + TypeName<Int>
            + "," + TypeName<LInt>
            + ">";
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*!
         * @brief Returns the name of the class, including template parameters.
         *
         *  Used for logging, profiling, and error handling.
         */
        
        static constexpr ct_string<class_name.byte_count()> ClassName()
        {
            return class_name;
        }
        
    }; // class Alexander
    
    
} // namespace Knoodle
