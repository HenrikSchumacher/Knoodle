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
    std::string tag ( ClassName()+"::SparseAlexanderStrandMatrix_UMFPACK" );
    
    if( !pd.InCacheQ(tag) )
    {
        const auto & P = A.template Pattern<false>( pd );
        
        UMFPACK_Ptr umfpack = std::make_shared<UMFPACK_T>(
            P.RowCount(), P.ColCount(), P.Outer().data(), P.Inner().data()
        );
        
        pd.SetCache( tag, umfpack );
    }
    
    return pd.template GetCache<UMFPACK_Ptr>(tag);
}
