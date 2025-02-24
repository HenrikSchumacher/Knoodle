private:


Multiplier_T Alexander_Strands_Det_Dense(
    cref<PD_T> pd,
    cref<Scal> arg
) const
{
//    tic(ClassName()+"Alexander_Strands_Det_Dense");
    
    if( pd.CrossingCount() <= 1 )
    {
//        toc(ClassName()+"Alexander_Strands_Det_Dense");
        
        return Multiplier_T();
    }
    else
    {
        const Int n = pd.CrossingCount() - 1;
        

        A.template WriteDenseMatrix<false>( pd, static_cast<Scal>(arg), LU_buffer.data() );
        
//        DenseAlexanderStrandMatrix<false>( pd, static_cast<Scal>(arg), LU_buffer.data() );
//        
//        print( ArrayToString( LU_buffer.data(), {n,n} ) );

        // Factorize dense Alexander matrix.
        
        int info = LAPACK::getrf<Layout::RowMajor>(
            n, n, LU_buffer.data(), n, LU_ipiv.data()
        );
        
        Multiplier_T det;
        
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
        
//        TOOLS_DUMP(det);
        
//        toc(ClassName()+"Alexander_Strands_Det_Dense");
        return det;
    }

}

//template<typename ExtScal, typename ExtReal, typename ExtInt>
//void AlexanderModuli_Strand_Dense(
//    cref<PD_T>    pd,
//    cptr<ExtScal> args,
//    ExtInt        arg_count,
//    mptr<ExtReal> moduli
//) const
//{
//    TOOLS_PTIC(ClassName()+"::AlexanderModuli_Strand_Dense");
//    
//    if( pd.CrossingCount() <= 1 )
//    {
//        zerofy_buffer( moduli, arg_count );
//    }
//    else
//    {
////        const ExtReal conversion_factor = Frac<ExtReal>(2,std::log(ExtReal(10)));
//        const ExtReal conversion_factor = 0.8685889638065036;
//        
//        ExtScal m;
//        ExtInt  e;
//        
//        for( ExtInt k = 0; k < arg_count; ++k )
//        {
//            Alexander_Strands_Dense(pd,args[k],m,e);
//            
//            
//            moduli[k] = std::log( Abs(m) ) * e * conversion_factor;
//        }
//    }
//
//    TOOLS_PTOC(ClassName()+"::AlexanderModuli_Strand_Dense");
//}


//template<bool fullQ = false>
//void DenseAlexanderStrandMatrix( cref<PD_T> pd, const Scal t, mptr<Scal> A ) const
//{
//    // Writes the dense Alexander matrix to the provided buffer A.
//    // User is responsible for making sure that the buffer is large enough.
//    TOOLS_PTIC(ClassName()+"::DenseAlexanderStrandMatrix<" + (fullQ ? "Full" : "Truncated") + ">");
//    
//    // Assemble dense Alexander matrix, skipping last row and last column if fullQ == false.
//
//    const Int n = pd.CrossingCount() - 1 + fullQ;
//    
//    const auto arc_strands = pd.ArcOverStrands();
//    
//    const auto & C_arcs = pd.Crossings();
//    
//    cptr<CrossingState> C_state = pd.CrossingStates().data();
//    
//    const Scal v [3] = { Scal(1) - t, Scal(-1), t };
//    
//    Int row_counter = 0;
//    
//    for( Int c = 0; c < C_arcs.Size(); ++c )
//    {
//        if( row_counter >= n )
//        {
//            break;
//        }
//        
//        const CrossingState s = C_state[c];
//
//        mptr<Scal> row = &A[ n * row_counter ];
//
//        zerofy_buffer( row, n );
//        
//        // Convention:
//        // i is incoming over-strand that goes over.
//        // j is incoming over-strand that goes under.
//        // k is outgoing over-strand that goes under.
//        
//        if( RightHandedQ(s) )
//        {
//            pd.AssertCrossing(c);
//            
//            // Reading in order of appearance in C_arcs.
//            const Int k = arc_strands[C_arcs(c,Out,Left )];
//            const Int i = arc_strands[C_arcs(c,In ,Left )];
//            const Int j = arc_strands[C_arcs(c,In ,Right)];
//            
//            // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
//            //
//            //    k -> t O     O
//            //            ^   ^
//            //             \ /
//            //              /
//            //             / \
//            //            /   \
//            //  i -> 1-t O     O j -> -1
//            
//            if( fullQ || (i < n) )
//            {
//                row[i] += v[0]; // = 1 - t
//            }
//
//            if( fullQ || (j < n) )
//            {
//                row[j] += v[1]; // -1
//            }
//
//            if( fullQ || (k < n) )
//            {
//                row[k] += v[2]; // t
//            }
//        }
//        else if( LeftHandedQ(s) )
//        {
//            pd.AssertCrossing(c);
//            
//            // Reading in order of appearance in C_arcs.
//            const Int k = arc_strands[C_arcs(c,Out,Right)];
//            const Int j = arc_strands[C_arcs(c,In ,Left )];
//            const Int i = arc_strands[C_arcs(c,In ,Right)];
//            
//            // cf. Lopez - The Alexander Polynomial, Coloring, and Determinants of Knots
//            //           O     O  k -> -1
//            //            ^   ^
//            //             \ /
//            //              \
//            //             / \
//            //            /   \
//            //   j  -> t O     O i -> 1-t
//            
//            if( fullQ || (i < n) )
//            {
//                row[i] += v[0]; // = 1 - t
//            }
//
//            if( fullQ || (j < n) )
//            {
//                row[j] += v[2]; // t
//            }
//
//            if( fullQ || (k < n) )
//            {
//                row[k] += v[1]; // -1
//            }
//        }
//        
//        ++row_counter;
//    }
//    
//    TOOLS_PTOC(ClassName()+"::DenseAlexanderStrandMatrix<" + (fullQ ? "Full" : "Truncated") + ">");
//}
