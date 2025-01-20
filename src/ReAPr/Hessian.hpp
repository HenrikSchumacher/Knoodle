public:

void SetLaplaceRegularization( const Real reg )
{
    laplace_reg = reg;
}

Real LaplaceRegularization() const
{
    return laplace_reg;
}

private:


void HessianMatrix_Triples(
    mref<PlanarDiagram_T> pd,
    mref<Aggregator_T> agg,
    const Int row_offset,
    const Int col_offset
)
{
    // Caution: This creates only the triples for the upper triangle.
    
    const Int comp_count   = pd.LinkComponentCount();
    cptr<Int> comp_arc_ptr = pd.LinkComponentArcPointers().data();
    cptr<Int> comp_arc_idx = pd.LinkComponentArcIndices().data();
    cptr<Int> next_arc     = pd.ArcNextArc().data();
    
    const Real val_0 = 2 + (2 + laplace_reg) * (2 + laplace_reg);
    const Real val_1 = -4 - 2 * laplace_reg;
    const Real val_2 = 1;

    for( Int comp = 0; comp < comp_count; ++comp )
    {
        const Int k_begin   = comp_arc_ptr[comp    ];
        const Int k_end     = comp_arc_ptr[comp + 1];
        
        Int a   = comp_arc_idx[k_begin];
        Int ap1 = next_arc[a  ];
        Int ap2 = next_arc[ap1];
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int row = a + row_offset;
            
            agg.Push( row, a   + col_offset, val_0 );
            agg.Push( row, ap1 + col_offset, val_1 );
            agg.Push( row, ap2 + col_offset, val_2 );

            a   = ap1;
            ap1 = ap2;
            ap2 = next_arc[ap1];
        }
    }
}

public:

cref<Matrix_T> HessianMatrix( mref<PlanarDiagram_T> pd )
{
    const std::string tag     = ClassName()+"::HessianMatrix";
    const std::string reg_tag = ClassName()+"::LaplaceRegularization";
    
    if(
       (!pd.InCacheQ(tag))
       ||
       (!pd.InCacheQ(reg_tag))
       ||
       (pd.template GetCache<Real>(reg_tag) != laplace_reg)
    )
    {
        pd.ClearCache(tag);
        pd.ClearCache(reg_tag);
        
        const I_T m = pd.ArcCount();
        
        Aggregator_T agg( 3 * m );
        
        HessianMatrix_Triples( pd, agg, 0, 0 );
        
//        agg.Finalize();
                        
        Matrix_T A ( agg, m, m, I_T(1), true, true );
        
        pd.SetCache(tag,std::move(A));
        pd.SetCache(reg_tag,laplace_reg);
    }
    
    return pd.template GetCache<Matrix_T>(tag);
}
