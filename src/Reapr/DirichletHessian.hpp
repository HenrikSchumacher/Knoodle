public:

void SetDirichletRegularization( const Real reg ) { dirichlet_reg = reg; }

Real DirichletRegularization() const { return dirichlet_reg; }

private:

template<typename I, typename J, typename Int>
void DirichletHessian_CollectTriples(
    mref<PlanarDiagram<Int>> pd,
    mref<TripleAggregator<I,I,Real,J>> agg,
    const I row_offset,
    const I col_offset
) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIC(ClassName()+"::DirichletHessian_CollectTriples"
       + "<" + TypeName<I>
       + "," + TypeName<J>
       + "," + TypeName<Int>
       + ">"
    );
    
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
        
        I a   = static_cast<I>(comp_arc_idx[k_begin]);
        I ap1 = static_cast<I>(next_arc[a  ]);
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            const I row = static_cast<I>(a + row_offset);
            
            agg.Push( row, a   + col_offset, val_0 );
            agg.Push( row, ap1 + col_offset, val_1 );

            a   = ap1;
            ap1 = next_arc[a];
        }
    }

    TOOLS_PTOC(ClassName()+"::DirichletHessian_CollectTriples"
       + "<" + TypeName<I>
       + "," + TypeName<J>
       + "," + TypeName<Int>
       + ">"
    );
    
} // DirichletHessian_CollectTriples


public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<Real,I,J> DirichletHessian( mref<PlanarDiagram<Int>> pd ) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");

    const I m = static_cast<I>(pd.ArcCount());
    
    TripleAggregator<I,I,Real,J> agg( I(2) * m );
    
    DirichletHessian_CollectTriples( pd, agg, I(0), I(0) );
    
    // We have to symmetrize because DirichletHessian_CollectTriples computes essentially only the upper triangle.
    return { agg, m, m, I(1), true, true };

} // DirichletHessian
