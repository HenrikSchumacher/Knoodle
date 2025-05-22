public:

void SetDirichletRegularization( const Real reg ) { dirichlet_reg = reg; }

Real DirichletRegularization() const { return dirichlet_reg; }

private:


void DirichletHessian_CollectTriples(
    mref<PlanarDiagram_T> pd,
    mref<Aggregator_T> agg,
    const Int row_offset,
    const Int col_offset
) const
{
    TOOLS_PTIC(ClassName() + "::DirichletHessian_CollectTriples");
    
    // Caution: This creates only the triples for the essentially upper triangle.
    // ("Essentially", because few off-diagonal triples lie in the lower triangle.)

    const Int comp_count   = pd.LinkComponentCount();
    cptr<Int> comp_arc_ptr = pd.LinkComponentArcPointers().data();
    cptr<Int> comp_arc_idx = pd.LinkComponentArcIndices().data();
    cptr<Int> next_arc     = pd.ArcNextArc().data();
    
    const Real val_0 = Real(2) + bending_reg;
    const Real val_1 = Real(-1);

    for( Int comp = 0; comp < comp_count; ++comp )
    {
        const Int k_begin   = comp_arc_ptr[comp    ];
        const Int k_end     = comp_arc_ptr[comp + 1];
        
        Int a   = comp_arc_idx[k_begin];
        Int ap1 = next_arc[a  ];
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int row = a + row_offset;
            
            agg.Push( row, a   + col_offset, val_0 );
            agg.Push( row, ap1 + col_offset, val_1 );

            a   = ap1;
            ap1 = next_arc[a];
        }
    }

    TOOLS_PTOC(ClassName() + "::DirichletHessian_CollectTriples");
    
} // DirichletHessian_CollectTriples


public:

Matrix_T DirichletHessian( mref<PlanarDiagram_T> pd ) const
{
    const I_T m = pd.ArcCount();
    
    Aggregator_T agg( 2 * m );
    
    DirichletHessian_CollectTriples( pd, agg, 0, 0 );
    
    // We have to symmetrize because DirichletHessian_CollectTriples computes essentially only the upper triangle.
    return Matrix_T ( agg, m, m, I_T(1), true, true );

} // DirichletHessian
