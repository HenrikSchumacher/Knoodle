private:

Multiplier_T Alexander_Strands_Det_Sparse(
    cref<PD_T> pd,
    cref<Scal> arg
) const
{
    if( pd.CrossingCount() <= 1 ) { return Multiplier_T(); }

    UMFPACK_Ptr umfpack = SparseAlexanderStrandMatrix_UMFPACK(pd);
    
    A.template WriteNonzeroValues<false>( pd, arg, umfpack->Values().data() );
    
    umfpack->NumericFactorization();
    
    const auto [z,e] = umfpack->Determinant();
    
    // Caution: e is the exponent to basis 10. We have to convert it to binary representation.
    
    // e might be a double, so it seems we would lose precision by casting to Multiplier_T::Int. But I am quite sure that the values returned by UMFPACK are always integers.
    
    Multiplier_T det = Multiplier_T(z) * Multiplier_T::Power(10, static_cast<Multiplier_T::Int>(e));
    
    return det;
}

UMFPACK_Ptr SparseAlexanderStrandMatrix_UMFPACK( cref<PD_T> pd ) const
{
    std::string tag ( MethodName("SparseAlexanderStrandMatrix_UMFPACK") );
    
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
