public:

template<typename ExtScal, typename ExtInt>
void Alexander_Strands_Sparse(
    cref<PD_T>    pd,
    cref<ExtScal> T,
    mref<ExtScal> mantissa,
    mref<ExtInt>  exponent
) const
{
    if( pd.CrossingCount() <= 1 )
    {
        mantissa = 1;
        exponent = 0;
    }
    else
    {
        UMFPACK_Ptr umfpack = SparseAlexanderStrandMatrix_UMFPACK(pd);
        
        SparseAlexanderStrandMatrix_Values( pd, T, umfpack->Values().data() );
        
        umfpack->NumericFactorization();
        
        const auto [z,e] = umfpack->Determinant();
        
        mantissa = static_cast<ExtScal>(z);
        exponent = static_cast<ExtInt>(std::round(e));
    }
}

template<typename ExtScal, typename ExtInt>
void Alexander_Strands_Sparse(
    cref<PD_T>    pd,
    cptr<ExtScal> args,
    ExtInt        arg_count,
    mptr<ExtScal> mantissas,
    mptr<ExtInt>  exponents
) const
{
    ptic(ClassName()+"::Alexander_Strands_Sparse");

    if( pd.CrossingCount() <= 1 )
    {
        fill_buffer( mantissas, ExtScal(1), arg_count );
        zerofy_buffer( exponents, arg_count );
    }
    else
    {
        for( ExtInt k = 0; k < arg_count; ++k )
        {
            Alexander_Strands_Sparse( pd, args[k], mantissas[k], exponents[k] );
        }
    }

    ptoc(ClassName()+"::Alexander_Strands_Sparse");
}

UMFPACK_Ptr SparseAlexanderStrandMatrix_UMFPACK( cref<PD_T> pd ) const
{
    std::string tag ( ClassName() + "::SparseAlexanderStrandMatrix_UMFPACK" );
    
    if( !pd.InCacheQ(tag) )
    {
        const Int m = pd.CrossingCount();

        const auto & A = SparseAlexanderStrandMatrix_Pattern( pd );

        UMFPACK_Ptr umfpack = std::make_shared<UMFPACK_T>(
            m, m, A.Outer().data(), A.Inner().data()
        );
        
        pd.SetCache( tag, umfpack );
    }
    
    return pd.template GetCache<UMFPACK_Ptr>( tag );
}

template<bool fullQ = false>
cref<Pattern_T> SparseAlexanderStrandMatrix_Pattern( cref<PD_T> pd ) const
{
    std::string tag ( ClassName() + "::SparseAlexanderStrandMatrix_Pattern" + "<" + (fullQ ? "_Full" : "_Truncated" ) + ">"
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

        Pattern_T A ( rp.data(), ci.data(), a.data(), n, n, Int(1) );
        A.SortInner();
        A.Compress();
        
        pd.SetCache( tag, std::move(A) );
    }
    
    return pd.template GetCache<Pattern_T>( tag );
}

template<bool fullQ = false, typename ExtScal>
void SparseAlexanderStrandMatrix_Values( cref<PD_T> pd, const ExtScal t, mptr<Scal> vals ) const
{
    cref<Pattern_T> A = SparseAlexanderStrandMatrix_Pattern<fullQ>(pd);
    
    const LInt nnz = A.NonzeroCount();
    
    cptr<Real> a = reinterpret_cast<const Real *>(A.Values().data());
    
    const Scal T = static_cast<Scal>(t);
    
    for( LInt i = 0; i < nnz; ++i )
    {
        vals[i] = a[2 * i] + T * a[2 * i + 1];
    }
}

//template<bool fullQ = false>
//Tensor1<Scal,LInt> SparseAlexanderStrandMatrix_Values( cref<PD_T> pd, const Scal t ) const
//{
//    Tensor1<Scal,LInt> vals( nnz );
//    
//    SparseAlexanderStrandMatrix_Values( pd, t, vals.data() );
//
//    return vals;
//}


// This needs to copy the sparsity pattern. So, only meant for haveing look, not for production.
template<bool fullQ = false>
SparseMatrix_T SparseAlexanderStrandMatrix( cref<PD_T> pd, const Scal t ) const
{
    cref<Pattern_T> H = SparseAlexanderStrandMatrix_Pattern<fullQ>(pd);
    
    SparseMatrix_T A (
        H.Outer().data(), H.Inner().data(),
        H.RowCount(), H.ColCount(), Int(1)
    );
    
    SparseAlexanderStrandMatrix_Values( pd, t, A.Values().data() );
    
    return A;
}
