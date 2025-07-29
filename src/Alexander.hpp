#pragma once

#include "../submodules/Tensors/Sparse.hpp"
#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"
//#include "../submodules/Tensors/src/Sparse/Metis.hpp"

#include "../submodules/Tensors/src/BLAS_Wrappers.hpp"
#include "../submodules/Tensors/src/LAPACK_Wrappers.hpp"

namespace Knoodle
{
    // TODO: Check against calling on trivial diagram.
    // TODO: Check against calling on multiple compenent diagram.
    
    template<typename Scal_, typename Int_, typename LInt_>
    class Alexander final
    {
        static_assert(IntQ<Int_>,"");
        static_assert(IntQ<LInt_>,"");
        
    public:
        
        using Scal = Scal_;
        using Real = Scalar::Real<Scal>;
        using Int  = Int_;
        using LInt = LInt_;
        
        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        
        using Helper_T          = Tensor2<Scal,LInt>;
        
        using Factorization_T   = Sparse::CholeskyDecomposition<Scal,Int,LInt>;
        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
        using Aggregator_T      = TripleAggregator<Int,Int,Scal,LInt>;
        
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;

        
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
      
        // Default constructor
        Alexander()
        :   sparsity_threshold ( 256 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
        
    private:
        
        Int sparsity_threshold;
        
        mutable Tensor1<Scal,Int> LU_buffer;
        
        mutable Tensor1<LAPACK::Int,Int> LU_perm;
        
        mutable Tensor1<Scal,LInt> herm_alex_vals;

    public:
        
        
        LInt SparsityThreshold() const
        {
            return sparsity_threshold;
        }
    
        // This is the reference implementation.
        // All other routines have to copy code from here.
        template<bool fullQ = false>
        SparseMatrix_T SparseAlexanderMatrix( cref<PD_T> pd, const int degree ) const
        {
            TOOLS_PTIMER(timer,ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+")");

            if( (degree != 0) && (degree!= 1) )
            {
                eprint(ClassName()+"::SparseAlexanderMatrix("+ToString(degree)+"): degree "+ToString(degree)+" is not a valid degree. Only 0 and 1 are allowed.");
                
                return SparseMatrix_T();
            }
            
            const Int n = pd.CrossingCount() - 1 + fullQ;
            
            if( n <= Int(0) )
            {
                return SparseMatrix_T();
            }

            std::vector<Aggregator_T> Agg;

            Agg.emplace_back(Int(3) * n);

            mref<Aggregator_T> agg = Agg[0];

            const auto arc_strands = pd.ArcOverStrands();

            const auto & C_arcs = pd.Crossings();

            cptr<CrossingState> C_state = pd.CrossingStates().data();

            // const Scal v [3] = { Scal(1) - t, -1, t };
            const Scal v [3] = {
                (degree == 0) ? Scal( 1) : Scal(-1), // 1 - t
                (degree == 0) ? Scal(-1) : Scal( 0), // - 1
                (degree == 0) ? Scal( 0) : Scal( 1)  // t
            };

            Int counter = 0;
            
            const Int c_count = C_arcs.Dim(0);
            
            for( Int c = 0; c < c_count; ++c )
            {
                if( counter >= n ) { break; }
                
                const CrossingState s = C_state[c];

                // Convention:
                // i is incoming over-strand that goes over.
                // j is incoming over-strand that goes under.
                // k is outgoing over-strand that goes under.
                
                if( RightHandedQ(s) )
                {
                    pd.AssertCrossing(c);
                    
                    // Reading in order of appearance in C_arcs.
                    const Int k = arc_strands[C_arcs(c,Out,Left )];
                    const Int i = arc_strands[C_arcs(c,In ,Left )];
                    const Int j = arc_strands[C_arcs(c,In ,Right)];
                    
                    // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                    //
                    //    k -> t O     O
                    //            ^   ^
                    //             \ /
                    //              /
                    //             / \
                    //            /   \
                    //  i -> 1-t O     O j -> -1
                    
                    if( fullQ || (i < n) )
                    {
                        agg.Push(counter, i, v[0] ); // 1 - t
                    }

                    if( fullQ || (j < n) )
                    {
                        agg.Push(counter, j, v[1] ); // -1
                    }

                    if( fullQ || (k < n) )
                    {
                        agg.Push(counter, k, v[2] ); // t
                    }

                    ++counter;
                }
                else if( LeftHandedQ(s) )
                {
                    pd.AssertCrossing(c);
                    
                    // Reading in order of appearance in C_arcs.
                    const Int k = arc_strands[C_arcs(c,Out,Right)];
                    const Int j = arc_strands[C_arcs(c,In ,Left )];
                    const Int i = arc_strands[C_arcs(c,In ,Right)];
                    
                    // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                    //           O     O  k -> -1
                    //            ^   ^
                    //             \ /
                    //              \
                    //             / \
                    //            /   \
                    //   j  -> t O     O i -> 1-t
                    
                    if( fullQ || (i < n) )
                    {
                        agg.Push( counter, i, v[0] ); // 1 - t
                    }

                    if( fullQ || (j < n) )
                    {
                        agg.Push( counter, j, v[2] ); // t
                    }

                    if( fullQ || (k < n) )
                    {
                        agg.Push( counter, k, v[1] ); // -1
                    }
                    
                    ++counter;
                }
            }

//            agg.Finalize();

            SparseMatrix_T A ( Agg, n, n, Int(1), true, false );

            return A;
        }
        
        void DenseAlexanderMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
        {
            // Writes the dense Alexander matrix to the provided buffer A.
            // User is responsible for making sure that the buffer is large enough.
            TOOLS_PTIMER(timer,MethodName("DenseAlexanderMatrix"));
            
            // Assemble dense Alexander matrix, skipping last row and last column.

            const Int n = pd.CrossingCount() - 1;
            
            const auto arc_strands = pd.ArcOverStrands();
            
            const auto & C_arcs = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
             const Scal v [3] = { Scal(1) - t, Scal(-1), t };
            
            Int counter = 0;
            
            const Int c_count = C_arcs.Dim(0);
            
            for( Int c = 0; c < c_count; ++c )
            {
                if( counter >= n ) { break; }
                
                const CrossingState s = C_state[c];

                mptr<Scal> row = &A[ n * counter ];

                zerofy_buffer( row, n );
                
                // Convention:
                // i is incoming over-strand that goes over.
                // j is incoming over-strand that goes under.
                // k is outgoing over-strand that goes under.
                
                if( RightHandedQ(s) )
                {
                    pd.AssertCrossing(c);
                    
                    // Reading in order of appearance in C_arcs.
                    const Int k = arc_strands[C_arcs(c,Out,Left )];
                    const Int i = arc_strands[C_arcs(c,In ,Left )];
                    const Int j = arc_strands[C_arcs(c,In ,Right)];
                    
                    // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                    //
                    //    k -> t O     O
                    //            ^   ^
                    //             \ /
                    //              /
                    //             / \
                    //            /   \
                    //  i -> 1-t O     O j -> -1
                    
                    if( i < n )
                    {
                        row[i] += v[0]; // = 1 - t
                    }

                    if( j < n )
                    {
                        row[j] += v[1]; // -1
                    }

                    if( k < n )
                    {
                        row[k] += v[2]; // t
                    }

                    ++counter;
                }
                else if( LeftHandedQ(s) )
                {
                    pd.AssertCrossing(c);
                    
                    // Reading in order of appearance in C_arcs.
                    const Int k = arc_strands[C_arcs(c,Out,Right)];
                    const Int j = arc_strands[C_arcs(c,In ,Left )];
                    const Int i = arc_strands[C_arcs(c,In ,Right)];
                    
                    // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                    //           O     O  k -> -1
                    //            ^   ^
                    //             \ /
                    //              \
                    //             / \
                    //            /   \
                    //   j  -> t O     O i -> 1-t
                    
                    if( i < n )
                    {
                        row[i] += v[0]; // = 1 - t
                    }

                    if( j < n )
                    {
                        row[j] += v[2]; // t
                    }

                    if( k < n )
                    {
                        row[k] += v[1]; // -1
                    }
                    
                    ++counter;
                }
            }
        }

        
        void RequireSparseHermitianAlexanderMatrix( cref<PD_T> pd ) const
        {
            TOOLS_PTIMER(timer,MethodName("RequireSparseHermitianAlexanderMatrix"));
            
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
                
                const auto arc_strands = pd.ArcOverStrands();
                
                const auto & C_arcs = pd.Crossings();
                
                cptr<CrossingState> C_state = pd.CrossingStates().data();
                
                Int counter = 0;
                
                const Int c_count = C_arcs.Dim(0);
                
                for( Int c = 0; c < c_count; ++c )
                {
                    if( counter >= n ) { break; }
                    
                    const CrossingState s = C_state[c];
                    
                    // Convention:
                    // i is incoming over-strand that goes over.
                    // j is incoming over-strand that goes under.
                    // k is outgoing over-strand that goes under.
                    
                    if( RightHandedQ(s) )
                    {
                        pd.AssertCrossing(c);
                        
                        // Reading in order of appearance in C_arcs.
                        const Int k = arc_strands[C_arcs(c,Out,Left )];
                        const Int i = arc_strands[C_arcs(c,In ,Left )];
                        const Int j = arc_strands[C_arcs(c,In ,Right)];
                        
                        // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                        //
                        //    k -> t O     O
                        //            ^   ^
                        //             \ /
                        //              /
                        //             / \
                        //            /   \
                        //  i -> 1-t O     O j -> -1


                        if( i < n )
                        {
                            // 1 - t
                            agg_0.Push( counter, i,  1 );
                            agg_1.Push( counter, i, -1 );
                        }

                        if( j < n )
                        {
                            // -1
                            agg_0.Push( counter, j, -1 );
                            agg_1.Push( counter, j,  0 );
                        }

                        if( k < n )
                        {
                            // t
                            agg_0.Push( counter, k,  0 );
                            agg_1.Push( counter, k,  1 );
                        }
                        
                        ++counter;
                    }
                    else if( LeftHandedQ(s) )
                    {
                        pd.AssertCrossing(c);
                        
                        const Int k = arc_strands[C_arcs(c,Out,Right)];
                        const Int j = arc_strands[C_arcs(c,In ,Left )];
                        const Int i = arc_strands[C_arcs(c,In ,Right)];
                        
                        // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                        //           O     O  k -> -1
                        //            ^   ^
                        //             \ /
                        //              \
                        //             / \
                        //            /   \
                        //   j  -> t O     O i -> 1-t
        
                        // Reading in order of appearance in C_arcs.
                        
                        if( i < n )
                        {
                            agg_0.Push( counter, i,  1 ); // 1 - t
                            agg_1.Push( counter, i, -1 ); // 1 - t
                        }

                        if( j < n )
                        {
                            agg_0.Push( counter, j,  0 ); // t
                            agg_1.Push( counter, j,  1 );
                        }

                        if( k < n )
                        {
                            agg_0.Push( counter, k, -1 ); // -1
                            agg_1.Push( counter, k,  0 );
                        }
                        
                        ++counter;
                    }
                }
                
//                agg_0.Finalize();
//                agg_1.Finalize();
                
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
                
                // Create Cholesky factorization.
                
                Factorization_Ptr S = std::make_shared<Factorization_T>(
                    AHA.Outer().data(), AHA.Inner().data(), std::move(perm)
                );
                
                S->SymbolicFactorization();
                
                pd.SetCache( tag_fact, std::move(S) );
                
                // TODO: Do we really have to compute 2 sparse transposes and 4 sparse dots?
                Tensor2<Scal,LInt> herm_alex_help ( 4, AHA.NonzeroCount() );
                
                AHA.Values().Write( herm_alex_help.data(0) );
                AHA = SparseMatrix_T(); // Release memory.
                
                AHA = AH_0.Dot( A_1 );
                AHA.Values().Write( herm_alex_help.data(1) );
                AHA = SparseMatrix_T(); // Release memory.
                
                AHA = AH_1.Dot( A_0 );
                AHA.Values().Write( herm_alex_help.data(2) );
                AHA = SparseMatrix_T(); // Release memory.
                
                AHA = AH_1.Dot( A_1 );
                AHA.Values().Write( herm_alex_help.data(3) );
                
                pd.SetCache( tag_help, std::move(herm_alex_help) );
            }
        }
        
        void LogAlexanderModuli_Sparse(
            cref<PD_T> pd, cptr<Scal> args, Int arg_count, mptr<Real> results
        ) const
        {
            TOOLS_PTIMER(timer,MethodName("LogAlexanderModuli_Sparse"));
            
            if( pd.CrossingCount() <= Int(1) )
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                RequireSparseHermitianAlexanderMatrix( pd );
                
                std::string tag_help ( std::string("SparseHermitianAlexanderHelpers_") + TypeName<Scal> );
                
                cref<Helper_T> herm_alex_help = pd.template GetCache<Helper_T>(tag_help);
                
                std::string tag_fact ( std::string("SparseHermitianAlexanderFactorization_") + TypeName<Scal> );
        
                mref<Factorization_Ptr> S = pd.template GetCache<Factorization_Ptr>(tag_fact);
                
                const LInt nnz = S->NonzeroCount();
                
                herm_alex_vals.template RequireSize<false>( nnz );

                for( Int idx = 0; idx < arg_count; ++idx )
                {
                    // TODO: Replace by more accurate LU factorization.
                    
                    const Scal t   = args[idx];
                    const Scal tc  = Conj(t);
                    const Scal ttc = t * tc;

                    for( LInt i = 0; i < nnz; ++i )
                    {
                        herm_alex_vals[i]
                        =           herm_alex_help[0][i]
                            + t   * herm_alex_help[1][i]
                            + tc  * herm_alex_help[2][i]
                            + ttc * herm_alex_help[3][i];
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
            
            if( pd.CrossingCount() <= Int(1) )
            {
                zerofy_buffer( results, arg_count );
            }
            else
            {
                const Int n = pd.CrossingCount() - Int(1);
                
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
        
        
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("Alexander")
                + "<" + TypeName<Scal>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + ">";
        }
        
    }; // class Alexander
    
    
} // namespace Knoodle
