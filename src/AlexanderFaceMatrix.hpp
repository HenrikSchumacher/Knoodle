#pragma once

namespace Knoodle
{
    // TODO: Some care has to be taken in cases where not all crossings or arcs are active.
    
    template<typename Scal_, typename Int_, typename LInt_>
    class AlexanderFaceMatrix final
    {
        static_assert(SignedIntQ<Int_>,"");
        
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
                
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        

        // Default constructor
        AlexanderFaceMatrix() = default;
        // Destructor
        ~AlexanderFaceMatrix() = default;
        // Copy constructor
        AlexanderFaceMatrix( const AlexanderFaceMatrix & other ) = default;
        // Copy assignment operator
        AlexanderFaceMatrix & operator=( const AlexanderFaceMatrix & other ) = default;
        // Move constructor
        AlexanderFaceMatrix( AlexanderFaceMatrix && other ) = default;
        // Move assignment operator
        AlexanderFaceMatrix & operator=( AlexanderFaceMatrix && other ) = default;
    
    public:
        
        template<bool fullQ = false>
        cref<Pattern_T> Pattern( cref<PD_T> pd ) const
        {
            std::string tag ( ClassName()+"::Pattern" + "<" + (fullQ ? "_Full" : "_Truncated" ) + ">"
            );
            
            if( !pd.InCacheQ(tag) )
            {
                const Int m = pd.CrossingCount();
                
                Tensor1<LInt,Int > rp( m + 1 );
                rp[0] = 0;
                Tensor1<Int ,LInt> ci( 4 * m );
                
                // Using complex numbers to assemble two matrices at once.
                
                Tensor1<Complex,LInt> a ( 4 * m );
                
                const auto & C_arcs  = pd.Crossings();
                const auto & A_faces = pd.ArcFaces();
                
                cptr<CrossingState> C_state = pd.CrossingStates().data();
                
                Int row_counter     = 0;
                Int nonzero_counter = 0;
                
                const Int c_count = C_arcs.Dim(0);
                
                for( Int c = 0; c < c_count; ++c )
                {
                    if( row_counter >= m )
                    {
                        break;
                    }
                    
                    /* N, E, S, W are the regions to the North, East, South, West
                     * of crossing c.
                     *
                     *        O         O
                     *         ^   N   ^ a_out
                     *          \     /
                     *           \   /
                     *            \ /
                     *        W    X c  E
                     *            / \
                     *           /   \
                     *          /     \
                     *    a_in /   S   \
                     *        O         O
                     */
                    
                    // We take these to arcs because C_arcs(c,Out,Right) and C_arcs(c,In ,Left) lie adjacent in memory.
                    const Int a_out = C_arcs(c,Out,Right);
                    const Int a_in  = C_arcs(c,In ,Left);
                    
                    // With a high probability (certainly after recompression of the diagram), a_in and a_out are adjacent in memory.
                    // Then these are consecutive reads.

                    // Caution: In A_faces the _right_ face comes first!
                    const Int S = A_faces(a_in ,0);
                    const Int W = A_faces(a_in ,1);
                    const Int E = A_faces(a_out,0);
                    const Int N = A_faces(a_out,1);
                    
                    const CrossingState s = C_state[c];
                    
                    if( RightHandedQ(s) )
                    {
                        pd.AssertCrossing(c);

                        /* Alexander - Topological Invariants of Knots and Links
                         *
                         *      O         O
                         *       ^ N->-1 ^
                         *        \     /
                         *         \   /
                         *          \ /
                         *   W->T  * / c  E->1
                         *          / \
                         *         / * \
                         *        /     \
                         *       / S->-T \
                         *      O         O
                         */
                        
                        if( fullQ || (N < m) )
                        {
                            ci[nonzero_counter] = N;
                            a [nonzero_counter] = Complex(-1, 0);
                            ++nonzero_counter;
                        }
                        
                        if( fullQ || (W < m) )
                        {
                            ci[nonzero_counter] = W;
                            a [nonzero_counter] = Complex( 0, 1);
                            ++nonzero_counter;
                        }

                        if( fullQ || (S < m) )
                        {
                            ci[nonzero_counter] = S;
                            a [nonzero_counter] = Complex( 0,-1);
                            ++nonzero_counter;
                        }

                        if( fullQ || (E < m) )
                        {
                            ci[nonzero_counter] = E;
                            a [nonzero_counter] = Complex( 1, 0);
                            ++nonzero_counter;
                        }
                    }
                    else if( LeftHandedQ(s) )
                    {
                        pd.AssertCrossing(c);
                        
                        /* Alexander - Topological Invariants of Knots and Links
                         *
                         *      O         O
                         *       ^ N->T  ^
                         *        \     /
                         *         \ * /
                         *          \ /
                         *  W->-T  * \ c  E->-1
                         *          / \
                         *         /   \
                         *        /     \
                         *       / S->1  \
                         *      O         O
                         */
                        
                        if( fullQ || (N < m) )
                        {
                            ci[nonzero_counter] = N;
                            a [nonzero_counter] = Complex( 0, 1);
                            ++nonzero_counter;
                        }
                        
                        if( fullQ || (W < m) )
                        {
                            ci[nonzero_counter] = W;
                            a [nonzero_counter] = Complex( 0,-1);
                            ++nonzero_counter;
                        }

                        if( fullQ || (S < m) )
                        {
                            ci[nonzero_counter] = S;
                            a [nonzero_counter] = Complex( 1, 0);
                            ++nonzero_counter;
                        }

                        if( fullQ || (E < m) )
                        {
                            ci[nonzero_counter] = E;
                            a [nonzero_counter] = Complex(-1, 0);
                            ++nonzero_counter;
                        }
                    }
                                    
                    ++row_counter;
                    
                    rp[row_counter] = nonzero_counter;
                }

                // Caution: We do not use move-constructors here because the Tensor1 objects `ci` and `a` might be a bit too long. Only `rp` knows how long they really ought to be.
                Pattern_T A (
                    rp.data(), ci.data(), a.data(), m, m, Int(1)
                );
                
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
        Tensor1<Scal,LInt> SparseValues( cref<PD_T> pd, const Scal t ) const
        {
            Tensor1<Scal,LInt> vals( NonzeroCount<fullQ>(pd) );
        
            WriteNonzeroValues( pd, vals.data(), t );
        
            return vals;
        }

        template<bool fullQ = false>
        SparseMatrix_T SparseMatrix( cref<PD_T> pd, const Scal t ) const
        {
            cref<Pattern_T> H = Pattern<fullQ>(pd);
            
            SparseMatrix_T A (
                H.Outer().data(), H.Inner().data(),
                H.RowCount(), H.ColCount(), Int(1)
            );
            
            WriteNonzeroValues( pd, t, A.Values().data() );
            
            return A;
        }
        
        template<bool fullQ = false>
        void WriteDenseMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
        {
            // Writes the dense Alexander matrix to the provided buffer A.
            // User is responsible for making sure that the buffer is large enough.
            TOOLS_PTIC(ClassName()+"::WriteDenseMatrix<" + (fullQ ? "Full" : "Truncated") + ">");
            
            // Assemble dense Alexander matrix, skipping last row and last column if fullQ == false.

            const Int m = pd.CrossingCount();
            const Int n = fullQ ? pd.FaceCount() : m;
            
            const auto & C_arcs  = pd.Crossings();
            const auto & A_faces = pd.ArcFaces();
            
            cptr<CrossingState> C_state = pd.CrossingStates().data();
            
            Int row_counter = 0;
            
            const Scal T = scalar_cast<Scal>(t);
            
            const Int c_count = C_arcs.Dim(0);
            
            for( Int c = 0; c < c_count; ++c )
            {
                if( row_counter >= m ) { break; }
                
                const CrossingState s = C_state[c];

                mptr<Scal> row = &A[ n * row_counter ];

                // TODO: We can optimize this away if we know that all values but the nonzero values are already correct.
                zerofy_buffer( row, n );

                /* N, E, S, W are the regions to the North, East, South, West
                 * of crossing c.
                 *
                 *        O         O
                 *         ^   N   ^ a_out
                 *          \     /
                 *           \   /
                 *            \ /
                 *        W    X c  E
                 *            / \
                 *           /   \
                 *          /     \
                 *    a_in /   S   \
                 *        O         O
                 */
                
                // We take these to arcs because C_arcs(c,Out,Right) and C_arcs(c,In ,Left) lie adjacent in memory.
                const Int a_out = C_arcs(c,Out,Right);
                const Int a_in  = C_arcs(c,In ,Left);
                
                // With a high probability (certainly after recompression of the diagram), a_in and a_out are adjacent in memory.
                // Then these are consecutive reads.
                
                // Caution: In A_faces the _right_ face comes first!
                const Int S = A_faces(a_in ,0);
                const Int W = A_faces(a_in ,1);
                const Int E = A_faces(a_out,0);
                const Int N = A_faces(a_out,1);
                
                
                if( RightHandedQ(s) )
                {
                    pd.AssertCrossing(c);

                    /* Alexander - Topological Invariants of Knots and Links
                     *
                     *      O         O
                     *       ^ N->-1 ^
                     *        \     /
                     *         \   /
                     *          \ /
                     *   W->T  * / c  E->1
                     *          / \
                     *         / * \
                     *        /     \
                     *       / S->-T \
                     *      O         O
                     */
                    
                    if( fullQ || (N < m) )
                    {
                        row[N] += -1;
                    }
                    
                    if( fullQ || (W < m) )
                    {
                        row[W] +=  T;
                    }

                    if( fullQ || (S < m) )
                    {
                        row[S] += -T;
                    }

                    if( fullQ || (E < m) )
                    {
                        row[E] +=  1;
                    }
                }
                else if( LeftHandedQ(s) )
                {
                    pd.AssertCrossing(c);
                    
                    /* Alexander - Topological Invariants of Knots and Links
                     *
                     *      O         O
                     *       ^ N->T  ^
                     *        \     /
                     *         \ * /
                     *          \ /
                     *  W->-T  * \ c  E->-1
                     *          / \
                     *         /   \
                     *        /     \
                     *       / S->1  \
                     *      O         O
                     */
                    
                    if( fullQ || (N < m) )
                    {
                        row[N] += T;
                    }
                    
                    if( fullQ || (W < m) )
                    {
                        row[W] += -T;
                    }

                    if( fullQ || (S < m) )
                    {
                        row[S] +=  1;
                    }

                    if( fullQ || (E < m) )
                    {
                        row[E] += -1;
                    }
                }
                
                ++row_counter;
            }
            
            TOOLS_PTOC(ClassName()+"::WriteDenseMatrix<" + (fullQ ? "Full" : "Truncated") + ">");

        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("AlexanderFaceMatrix")
                + "<" + TypeName<Scal>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + ">";
        }
        
    }; // class AlexanderFaceMatrix
    
    
} // namespace Knoodle
