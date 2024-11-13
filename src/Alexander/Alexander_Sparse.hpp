public:

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
        const Int n = pd.CrossingCount() - 1;

        const auto & A = SparseAlexanderHelper( pd );

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

    ptoc(ClassName()+"::Alexander_Sparse");
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
