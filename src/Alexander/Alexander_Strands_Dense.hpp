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
