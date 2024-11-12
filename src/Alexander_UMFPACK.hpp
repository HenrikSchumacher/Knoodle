#pragma once

#include "../submodules/Tensors/UMFPACK.hpp"
// This requires linking agains libumfpack

//#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"
////#include "../submodules/Tensors/src/Sparse/Metis.hpp"
//
#include "../submodules/Tensors/BLAS_Wrappers.hpp"
#include "../submodules/Tensors/LAPACK_Wrappers.hpp"


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
        using Helper_T          = Sparse::MatrixCSR<Complex,Int,LInt>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        using Factorization_T   = UMFPACK<Scal,LInt>;
        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
        
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
        
        template<bool fullQ = false>
        cref<Helper_T> SparseAlexanderHelper( cref<PD_T> pd ) const
        {
            std::string tag ( std::string( "SparseAlexanderHelper_" ) + TypeName<Complex> + (fullQ ? "_Full" : "_Truncated" )
            );
            
            if( !pd.InCacheQ(tag) )
            {
                const Int n = pd.CrossingCount() - 1 + fullQ;
                
                Tensor1<LInt,Int > rp( n + 1 );
                rp[0] = 0;
                Tensor1<Int ,LInt> ci( 3 * n );
                
                // Using complex numbers to assemble two matrices at once.
                
                Tensor1<Complex,LInt> a ( 3 * n );
                
                const auto arc_strands = pd.ArcOverStrands();
                
                const auto & C_arcs = pd.Crossings();
                
                cptr<CrossingState> C_state = pd.CrossingStates().data();
                
                Int row_counter     = 0;
                Int nonzero_counter = 0;
                
                for( Int c = 0; c < C_arcs.Size(); ++c )
                {
                    if( row_counter >= n )
                    {
                        break;
                    }
                    
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
                            // 1 - t
                            ci[nonzero_counter] = i;
                            a [nonzero_counter] = Complex( 1,-1);
                            ++nonzero_counter;
                        }

                        if( fullQ || (j < n) )
                        {
                            // -1
                            ci[nonzero_counter] = j;
                            a [nonzero_counter] = Complex(-1, 0);
                            ++nonzero_counter;
                        }

                        if( fullQ || (k < n) )
                        {
                            // t
                            ci[nonzero_counter] = k;
                            a [nonzero_counter] = Complex( 0, 1);
                            ++nonzero_counter;
                        }
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
                        
                        if( fullQ || (i < n) )
                        {
                            // 1 - t
                            ci[nonzero_counter] = i;
                            a [nonzero_counter] = Complex( 1,-1);
                            ++nonzero_counter;
                        }

                        if( fullQ || (j < n) )
                        {
                            // t
                            ci[nonzero_counter] = j;
                            a [nonzero_counter] = Complex( 0, 1);
                            ++nonzero_counter;
                        }

                        if( fullQ || (k < n) )
                        {
                            
                            // -1
                            ci[nonzero_counter] = k;
                            a [nonzero_counter] = Complex(-1, 0);
                            ++nonzero_counter;
                        }
                    }
                                    
                    ++row_counter;
                    
                    rp[row_counter] = nonzero_counter;
                }

                Helper_T A ( rp.data(), ci.data(), a.data(), n, n, Int(1) );
                A.SortInner();
                A.Compress();
                
                pd.SetCache( tag, std::move(A) );
            }
            
            return pd.template GetCache<Helper_T>( tag );
        }
        
        template<bool fullQ = false, typename ExtScal>
        void WriteSparseAlexanderValues( cref<PD_T> pd, mptr<Scal> vals, const ExtScal t ) const
        {
            cref<Helper_T> A = SparseAlexanderHelper<fullQ>(pd);
            
            const LInt nnz = A.NonzeroCount();
            
            cptr<Real> a = reinterpret_cast<const Real *>(A.Values().data());
            
            const Scal T = static_cast<Scal>(t);
            
            for( LInt i = 0; i < nnz; ++i )
            {
                vals[i] = a[2 * i] + T * a[2 * i + 1];
            }
        }
        
        template<bool fullQ = false>
        Tensor1<Scal,LInt> SparseAlexanderValues( cref<PD_T> pd, const Scal t ) const
        {
            cref<Helper_T> A = SparseAlexanderHelper<fullQ>(pd);
            
            const LInt nnz = A.NonzeroCount();
            
            Tensor1<Scal,LInt> vals( nnz );
            
            cptr<Real> a = reinterpret_cast<const Real *>(A.Values().data());
            
            for( LInt i = 0; i < nnz; ++i )
            {
                vals[i] = a[2 * i] + t * a[2 * i + 1];
            }
            
            return vals;
        }
        
        template<bool fullQ = false>
        SparseMatrix_T SparseAlexanderMatrix( cref<PD_T> pd, const Scal t ) const
        {
            cref<Helper_T> H = SparseAlexanderHelper<fullQ>(pd);
            
            SparseMatrix_T A (
                H.Outer().data(), H.Inner().data(), SparseAlexanderValues(pd,t).data(),
                H.RowCount(), H.ColCount(), Int(1)
            );
            
            return A;
        }
        
        
        template<bool fullQ = false>
        void DenseAlexanderMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
        {
            // Writes the dense Alexander matrix to the provided buffer A.
            // User is responsible for making sure that the buffer is large enough.
            ptic(ClassName()+"::DenseAlexanderMatrix" + (fullQ ? "_Full" : "_Truncated"));
            
            // Assemble dense Alexander matrix, skipping last row and last column if fullQ == false.

            const Int n = pd.CrossingCount() - 1 + fullQ;
            
            const auto arc_strands = pd.ArcOverStrands();
            
            const auto & C_arcs = pd.Crossings();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            const Scal v [3] = { Scal(1) - t, Scal(-1), t };
            
            Int row_counter = 0;
            
            for( Int c = 0; c < C_arcs.Size(); ++c )
            {
                if( row_counter >= n )
                {
                    break;
                }
                
                const CrossingState s = C_state[c];

                mptr<Scal> row = &A[ n * row_counter ];

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
                    
                    if( fullQ || (i < n) )
                    {
                        row[i] += v[0]; // = 1 - t
                    }

                    if( fullQ || (j < n) )
                    {
                        row[j] += v[1]; // -1
                    }

                    if( fullQ || (k < n) )
                    {
                        row[k] += v[2]; // t
                    }
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
                        row[i] += v[0]; // = 1 - t
                    }

                    if( fullQ || (j < n) )
                    {
                        row[j] += v[2]; // t
                    }

                    if( fullQ || (k < n) )
                    {
                        row[k] += v[1]; // -1
                    }
                }
                
                ++row_counter;
            }
            
            ptoc(ClassName()+"::DenseAlexanderMatrix" + (fullQ ? "_Full" : "_Truncated"));
        }

        
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
                Alexander_Sparse( pd, args, arg_count, mantissas, exponents );
            }
            else
            {
                Alexander_Dense ( pd, args, arg_count, mantissas, exponents );
            }
        }
        
        template<typename ExtScal, typename ExtInt>
        void Alexander_Dense(
            cref<PD_T>    pd,
            cptr<ExtScal> args,
            ExtInt        arg_count,
            mptr<ExtScal> mantissas,
            mptr<ExtInt>  exponents
        ) const
        {
            ptic(ClassName()+"::Alexander_Dense");
            
            if( pd.CrossingCount() <= 1 )
            {
                fill_buffer( mantissas, Scal(1), arg_count );
                zerofy_buffer( exponents, arg_count );
            }
            else
            {
                const Int n = pd.CrossingCount() - 1;
                
                for( ExtInt idx = 0; idx < arg_count; ++idx )
                {
                    ProductAccumulator<Scal,Int> det;
                    
                    DenseAlexanderMatrix<false>( pd, static_cast<Scal>(args[idx]), LU_buffer.data() );

                    // Factorize dense Alexander matrix.
                    
                    int info = LAPACK::getrf<Layout::RowMajor>(
                        n, n, LU_buffer.data(), n, LU_ipiv.data()
                    );
                    
                    if( info == 0 )
                    {
                        for( Int i = 0; i < n; ++i )
                        {
                            const Scal U_ii = LU_buffer( (n+1) * i );
                            
                            
                            // We have to change the sign of mantissa depending on whether the permutation is even or odd.
                            
                            // Cf. https://stackoverflow.com/a/50285530/8248900 and https://stackoverflow.com/a/47319635/8248900
                            
                            det *= ( ( i + 1 == LU_ipiv[i] ) ? U_ii : -U_ii );
                        }
                    }
                    else
                    {
                        det *= Scal(0);
                    }

                    
                    mantissas[idx] = static_cast<ExtScal>(det.Mantissa10());
                    exponents[idx] = static_cast<ExtInt >(det.Exponent10());
                }
            }
        
            ptoc(ClassName()+"::Alexander_Dense");
        }
        
        
        template<typename ExtScal, typename ExtInt>
        void Alexander_Sparse(
            cref<PD_T>    pd,
            cptr<ExtScal> args,
            ExtInt        arg_count,
            mptr<ExtScal> mantissas,
            mptr<ExtInt>  exponents
        ) const
        {
            ptic(ClassName()+"::Alexander_Sparse");

            if( pd.CrossingCount() <= 1 )
            {
                fill_buffer( mantissas, ExtScal(1), arg_count );
                zerofy_buffer( exponents, arg_count );
            }
            else
            {
                const auto & A = SparseAlexanderHelper( pd );

                const Int n = pd.CrossingCount() - 1;
                
                Tensor1<Scal,LInt> vals ( A.NonzeroCount() );
                
                UMFPACK<Scal,Int> umfpack ( n, n, A.Outer().data(), A.Inner().data() );

                for( ExtInt idx = 0; idx < arg_count; ++idx )
                {
                    WriteSparseAlexanderValues( pd, vals.data(), args[idx] );
                    
                    umfpack.NumericFactorization( vals.data() );
                    
                    const auto [z,e] = umfpack.Determinant();
                    
                    mantissas[idx] = static_cast<ExtScal>(z);
                    exponents[idx] = static_cast<ExtInt>(std::round(e));
                }
            }

            ptoc(ClassName()+"::LogAlexander_Sparse");
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Alexander_UMFPACK")+ "<" + TypeName<Scal> + "," + TypeName<Int> + "," + TypeName<LInt> + ">";
        }
        
    }; // class Alexander_UMFPACK
    
    
} // namespace KnotTools

