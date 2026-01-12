#pragma once

namespace Knoodle
{
    // TODO: Some care has to be taken in cases where not all crossings or arcs are active.
    
    template<typename Scal_, typename Int_, typename LInt_>
    class AlexanderStrandMatrix final
    {
    public:
        
        using Scal    = Scal_;
        using Real    = Scalar::Real<Scal>;
        using Int     = Int_;
        using LInt    = LInt_;
        
        using Complex = Scalar::Complex<Real>;

        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
        using Pattern_T         = Sparse::MatrixCSR<Complex,Int,LInt>;
        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        using PD_T              = PlanarDiagram<Int>;
        using A_Cross_T         = typename PD_T::A_Cross_T;
        using C_Arc_T           = typename PD_T::C_Arc_T;
        
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;

        // Default constructor
        AlexanderStrandMatrix() = default;
        
    public:
        
        template<bool fullQ = false>
        cref<Pattern_T> Pattern( cref<PD_T> pd ) const
        {
            std::string tag ( ClassName()+"::Pattern<" + (fullQ ? "Full" : "Truncated" ) + ">"
            );
            
            if( !pd.InCacheQ(tag) )
            {
                const Int n = pd.CrossingCount() - Int(1) + fullQ;
                
                // Using complex numbers to assemble two matrices at once.
                // We must use additive assembly; otherwise, Reidemeister-I loops may break things.
                TripleAggregator<Int,Int,Complex,LInt> agg (LInt(3) * n);
                
                const auto arc_strands = pd.ArcOverStrands();
                
                const auto & C_arcs = pd.Crossings();
                
                cptr<CrossingState_T> C_state = pd.CrossingStates().data();
                
                Int row_counter     = 0;
                
                const Int c_count = C_arcs.Dim(0);
                
                for( Int c = 0; c < c_count; ++c )
                {
                    if( row_counter >= n ) { break; } // Needed to break early, when we strike out a row.
                    
                    const CrossingState_T s = C_state[c];

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
                        
                        /* cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                         *           O     O  k -> -1
                         *            ^   ^
                         *             \ /
                         *              \
                         *             / \
                         *            /   \
                         *   j  -> t O     O i -> 1-t
                         */

                        if( fullQ || (i < n) )
                        {
                            agg.Push(row_counter,i,Complex( 1,-1)); // 1 - t
                        }
                        if( fullQ || (j < n) )
                        {
                            agg.Push(row_counter,j,Complex(-1, 0)); // -1
                        }
                        if( fullQ || (k < n) )
                        {
                            agg.Push(row_counter,k,Complex( 0, 1)); // t
                        }
                        
                        // Beware: We may increment row_counter only if crossing c is active.
                        ++row_counter;
                    }
                    else if( LeftHandedQ(s) )
                    {
                        pd.AssertCrossing(c);
                        
                        // Reading in order of appearance in C_arcs.
                        
                        const Int k = arc_strands[C_arcs(c,Out,Right)];
                        const Int j = arc_strands[C_arcs(c,In ,Left )];
                        const Int i = arc_strands[C_arcs(c,In ,Right)];
                        
                        /* cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                         *           O     O  k -> -1
                         *            ^   ^
                         *             \ /
                         *              \
                         *             / \
                         *            /   \
                         *   j  -> t O     O i -> 1-t
                         */

                        if( fullQ || (i < n) )
                        {
                            agg.Push(row_counter,i,Complex( 1,-1)); // 1 - t
                        }
                        if( fullQ || (j < n) )
                        {
                            agg.Push(row_counter,j,Complex( 0, 1)); // t
                        }
                        if( fullQ || (k < n) )
                        {
                            agg.Push(row_counter,k,Complex(-1, 0)); // -1
                        }
                        
                        // Beware: We may increment row_counter only if crossing c is active.
                        ++row_counter;
                    }
                    
                    // We do nothing if crossing c is not active.
                }
                
                Pattern_T A ( agg, n, n, Int(1), true, false, false );
                
                pd.SetCache( tag, std::move(A) );
            }
            
            return pd.template GetCache<Pattern_T>(tag);
        }

        template<bool fullQ>
        LInt NonzeroCount( cref<PD_T> pd )
        {
            return Pattern<fullQ>(pd).NonzeroCount();
        }
        
        template<bool fullQ = false, typename ExtScal>
        void WriteNonzeroValues( cref<PD_T> pd, const ExtScal t, mptr<Scal> vals ) const
        {
            cref<Pattern_T> A = Pattern<fullQ>(pd);
            
            const LInt nnz = A.NonzeroCount();
            
            cptr<Real> a = reinterpret_cast<const Real *>(A.Values().data());
            
            const Scal T = static_cast<Scal>(t);
            
            for( LInt i = 0; i < nnz; ++i )
            {
                vals[i] = a[2 * i] + T * a[2 * i + 1];
            }
        }

        template<bool fullQ = false>
        Tensor1<Scal,LInt> NonzeroValues( cref<PD_T> pd, const Scal t ) const
        {
            Tensor1<Scal,LInt> vals( NonzeroCount<fullQ>(pd) );
        
            WriteNonzeroValues( pd, vals.data(), t );
        
            return vals;
        }


        // This needs to copy the sparsity pattern. So, only meant for having a look, not for production.
        template<bool fullQ = false>
        SparseMatrix_T SparseMatrix( cref<PD_T> pd, const Scal t ) const
        {
            cref<Pattern_T> H = Pattern<fullQ>(pd);
            
            SparseMatrix_T A (
                H.Outer().data(), H.Inner().data(),
                H.RowCount(), H.ColCount(), Int(1)
            );
            
            WriteValues( pd, t, A.Values().data() );
            
            return A;
        }
        
        
        template<bool fullQ = false>
        void WriteDenseMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
        {
            // Writes the dense Alexander matrix to the provided buffer A.
            // User is responsible for making sure that the buffer is large enough.
            TOOLS_PTIMER(timer,ClassName()+"::WriteDenseMatrix<" + (fullQ ? "Full" : "Truncated") + ">");
            
            // Assemble dense Alexander matrix, skipping last row and last column if fullQ == false.

            const Int n = pd.CrossingCount() - 1 + fullQ;
            
            const auto arc_strands = pd.ArcOverStrands();
            
            const auto & C_arcs = pd.Crossings();
            
            cptr<CrossingState_T> C_state = pd.CrossingStates().data();
            
            const Scal v [3] = { Scal(1) - t, Scal(-1), t };
            
            Int row_counter = 0;
            
            const Int c_count = C_arcs.Dim(0);
            
            for( Int c = 0; c < c_count; ++c )
            {
                if( row_counter >= n ) { break; }
                
                const CrossingState_T s = C_state[c];

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
                    
                    /* cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                    //
                     *    k -> t O     O
                     *            ^   ^
                     *             \ /
                     *              /
                     *             / \
                     *            /   \
                     *  i -> 1-t O     O j -> -1
                     */
                    
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
                    
                    ++row_counter;
                }
                else if( LeftHandedQ(s) )
                {
                    pd.AssertCrossing(c);
                    
                    // Reading in order of appearance in C_arcs.
                    const Int k = arc_strands[C_arcs(c,Out,Right)];
                    const Int j = arc_strands[C_arcs(c,In ,Left )];
                    const Int i = arc_strands[C_arcs(c,In ,Right)];
                    
                    /* cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
                     *           O     O  k -> -1
                     *            ^   ^
                     *             \ /
                     *              \
                     *             / \
                     *            /   \
                     *   j  -> t O     O i -> 1-t
                     */
                    
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
                    
                    ++row_counter;
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
            return std::string("AlexanderStrandMatrix")
                + "<" + TypeName<Scal>
                + "," + TypeName<Int>
                + "," + TypeName<LInt> + ">";
        }
        
    }; // class AlexanderStrandMatrix
    
    
} // namespace Knoodle


