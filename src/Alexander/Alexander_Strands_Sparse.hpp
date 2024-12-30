private:

Multiplier_T Alexander_Strands_Det_Sparse(
    cref<PD_T> pd,
    cref<Scal> arg
) const
{
    if( pd.CrossingCount() <= 1 )
    {
        return Multiplier_T();
    }
    else
    {
        UMFPACK_Ptr umfpack = SparseAlexanderStrandMatrix_UMFPACK(pd);
        
        A.template WriteNonzeroValues<false>( pd, arg, umfpack->Values().data() );
        
        umfpack->NumericFactorization();
        
        const auto [z,e] = umfpack->Determinant();
        
        // Caution: e is the exponent mod 10. We have to convert it to binary representation.
        
        Multiplier_T det = Multiplier_T(z) * Multiplier_T::Power(10,e);
        
        return det;
    }
}

UMFPACK_Ptr SparseAlexanderStrandMatrix_UMFPACK( cref<PD_T> pd ) const
{
    std::string tag ( ClassName() + "::SparseAlexanderStrandMatrix_UMFPACK" );
    
    if( !pd.InCacheQ(tag) )
    {
        const auto & P = A.template Pattern<false>( pd );
        
        UMFPACK_Ptr umfpack = std::make_shared<UMFPACK_T>(
            P.RowCount(), P.ColCount(), P.Outer().data(), P.Inner().data()
        );
        
        pd.SetCache( tag, umfpack );
    }
    
    return pd.template GetCache<UMFPACK_Ptr>( tag );
}

//template<bool fullQ = false>
//cref<Pattern_T> SparseAlexanderStrandMatrix_Pattern( cref<PD_T> pd ) const
//{
//    std::string tag ( ClassName() + "::SparseAlexanderStrandMatrix_Pattern" + "<" + (fullQ ? "_Full" : "_Truncated" ) + ">"
//    );
//    
//    if( !pd.InCacheQ(tag) )
//    {
//        const Int n = pd.CrossingCount() - 1 + fullQ;
//        
//        Tensor1<LInt,Int > rp( n + 1 );
//        rp[0] = 0;
//        Tensor1<Int ,LInt> ci( 3 * n );
//        
//        // Using complex numbers to assemble two matrices at once.
//        
//        Tensor1<Complex,LInt> a ( 3 * n );
//        
//        const auto arc_strands = pd.ArcOverStrands();
//        
//        const auto & C_arcs = pd.Crossings();
//        
//        cptr<CrossingState> C_state = pd.CrossingStates().data();
//        
//        Int row_counter     = 0;
//        Int nonzero_counter = 0;
//        
//        for( Int c = 0; c < C_arcs.Size(); ++c )
//        {
//            if( row_counter >= n )
//            {
//                break;
//            }
//            
//            const CrossingState s = C_state[c];
//            
//            // Convention:
//            // i is incoming over-strand that goes over.
//            // j is incoming over-strand that goes under.
//            // k is outgoing over-strand that goes under.
//            
//            if( RightHandedQ(s) )
//            {
//                pd.AssertCrossing(c);
//                
//                // Reading in order of appearance in C_arcs.
//                const Int k = arc_strands[C_arcs(c,Out,Left )];
//                const Int i = arc_strands[C_arcs(c,In ,Left )];
//                const Int j = arc_strands[C_arcs(c,In ,Right)];
//                
//                // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
//                //
//                //    k -> t O     O
//                //            ^   ^
//                //             \ /
//                //              /
//                //             / \
//                //            /   \
//                //  i -> 1-t O     O j -> -1
//
//
//                if( fullQ || (i < n) )
//                {
//                    // 1 - t
//                    ci[nonzero_counter] = i;
//                    a [nonzero_counter] = Complex( 1,-1);
//                    ++nonzero_counter;
//                }
//
//                if( fullQ || (j < n) )
//                {
//                    // -1
//                    ci[nonzero_counter] = j;
//                    a [nonzero_counter] = Complex(-1, 0);
//                    ++nonzero_counter;
//                }
//
//                if( fullQ || (k < n) )
//                {
//                    // t
//                    ci[nonzero_counter] = k;
//                    a [nonzero_counter] = Complex( 0, 1);
//                    ++nonzero_counter;
//                }
//            }
//            else if( LeftHandedQ(s) )
//            {
//                pd.AssertCrossing(c);
//                
//                const Int k = arc_strands[C_arcs(c,Out,Right)];
//                const Int j = arc_strands[C_arcs(c,In ,Left )];
//                const Int i = arc_strands[C_arcs(c,In ,Right)];
//                
//                // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
//                //           O     O  k -> -1
//                //            ^   ^
//                //             \ /
//                //              \
//                //             / \
//                //            /   \
//                //   j  -> t O     O i -> 1-t
//
//                // Reading in order of appearance in C_arcs.
//                
//                if( fullQ || (i < n) )
//                {
//                    // 1 - t
//                    ci[nonzero_counter] = i;
//                    a [nonzero_counter] = Complex( 1,-1);
//                    ++nonzero_counter;
//                }
//
//                if( fullQ || (j < n) )
//                {
//                    // t
//                    ci[nonzero_counter] = j;
//                    a [nonzero_counter] = Complex( 0, 1);
//                    ++nonzero_counter;
//                }
//
//                if( fullQ || (k < n) )
//                {
//                    
//                    // -1
//                    ci[nonzero_counter] = k;
//                    a [nonzero_counter] = Complex(-1, 0);
//                    ++nonzero_counter;
//                }
//            }
//                            
//            ++row_counter;
//            
//            rp[row_counter] = nonzero_counter;
//        }
//
//        Pattern_T P ( rp.data(), ci.data(), a.data(), n, n, Int(1) );
//        P.SortInner();
//        P.Compress();
//        
//        pd.SetCache( tag, std::move(P) );
//    }
//    
//    return pd.template GetCache<Pattern_T>( tag );
//}
//
//template<bool fullQ = false, typename ExtScal>
//void SparseAlexanderStrandMatrix_Values( cref<PD_T> pd, const ExtScal t, mptr<Scal> vals ) const
//{
//    cref<Pattern_T> P = SparseAlexanderStrandMatrix_Pattern<fullQ>(pd);
//    
//    const LInt nnz = P.NonzeroCount();
//    
//    cptr<Real> a = reinterpret_cast<const Real *>(P.Values().data());
//    
//    const Scal T = static_cast<Scal>(t);
//    
//    for( LInt i = 0; i < nnz; ++i )
//    {
//        vals[i] = a[2 * i] + T * a[2 * i + 1];
//    }
//}
