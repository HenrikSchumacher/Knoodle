public:

void SetBendingRegularization( const Real reg ) { bending_reg = reg; }

Real BendingRegularization() const { return bending_reg; }

private:


template<typename I, typename J, typename Int>
void BendingHessian_CollectTriples(
    mref<PlanarDiagram<Int>> pd,
    mref<TripleAggregator<I,I,Real,J>> agg,
    const I row_offset,
    const I col_offset
) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIMER(timer,ClassName()+"::BendingHessian_CollectTriples"
       + "<" + TypeName<I>
       + "," + TypeName<J>
       + "," + TypeName<Int>
       + ">"
    );
    
    // Caution: This creates only the triples for the upper triangle.
    
    const Int comp_count   = pd.LinkComponentCount();
    cptr<Int> comp_arc_ptr = pd.LinkComponentArcPointers().data();
    cptr<Int> comp_arc_idx = pd.LinkComponentArcIndices().data();
    cptr<Int> next_arc     = pd.ArcNextArc().data();
    
    const Real val_0 = 2 + (2 + bending_reg) * (2 + bending_reg);
    const Real val_1 = -4 - 2 * bending_reg;
    const Real val_2 = 1;

    for( Int comp = 0; comp < comp_count; ++comp )
    {
        const Int k_begin   = comp_arc_ptr[comp    ];
        const Int k_end     = comp_arc_ptr[comp + 1];
        
        I a   = static_cast<I>(comp_arc_idx[k_begin]);
        I ap1 = static_cast<I>(next_arc[a  ]);
        I ap2 = static_cast<I>(next_arc[ap1]);
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            const I row = a + row_offset;
            
            agg.Push( row, a   + col_offset, val_0 );
            agg.Push( row, ap1 + col_offset, val_1 );
            agg.Push( row, ap2 + col_offset, val_2 );

            a   = ap1;
            ap1 = ap2;
            ap2 = next_arc[ap1];
        }
    }
    
} // BendingHessian_CollectTriples

public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<Real,I,J> BendingHessian( mref<PlanarDiagram<Int>> pd ) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    const I m = static_cast<I>(pd.ArcCount());
    
    TripleAggregator<I,I,Real,J> agg( J(3) * static_cast<J>(m) );
    
    BendingHessian_CollectTriples( pd, agg, I(0), I(0) );
    
    return { agg, m, m, I(1), true, true };
    
} // BendingHessian
