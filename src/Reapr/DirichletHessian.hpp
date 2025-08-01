public:

void SetDirichletRegularization( const Real reg ) { dirichlet_reg = reg; }

Real DirichletRegularization() const { return dirichlet_reg; }

private:

template<typename R = Real, typename I = Int, typename J = Int>
void DirichletHessian_CollectTriples(
    cref<PlanarDiagram<Int>> pd,
    mref<TripleAggregator<I,I,R,J>> agg,
    const I row_offset,
    const I col_offset
) const
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    std::string tag = ClassName()+"::DirichletHessian_CollectTriples"
    + "<" + TypeName<I>
    + "," + TypeName<J>
    + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    const Int m = pd.ArcCount();
    
    Size_T max_index = ToSize_T(m) + ToSize_T(Max(row_offset,col_offset));

    if( !std::in_range<I>(max_index) )
    {
        eprint(tag + ": Type " + TypeName<I> + " is too small to store maximum index = " + ToString(max_index) + ". Aborting.");
        return;
    }
    
    Size_T nnz = ToSize_T(2 * agg.Size()) + ToSize_T(3 * pd.ArcCount());
    
    if( !std::in_range<J>(nnz) )
    {
        eprint(tag + ": Type " + TypeName<J> + " is too small to store number of nonzero elements = " + ToString(nnz) + ". Aborting.");
        return;
    }
    
    // Caution: This creates only the triples for essentially the  upper triangle, so the matrix has to be symmetrized when assimilated.
    // ("Essentially", because a few off-diagonal triples lie in the lower triangle.)

    const auto & lc_arcs = pd.LinkComponentArcs();
    const Int lc_count   = lc_arcs.SublistCount();
    cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
//    cptr<Int> lc_arc_idx = lc_arcs.Elements().data();
    
    const Real val_0 = R(2) + static_cast<R>(dirichlet_reg);
    const Real val_1 = R(-1);

    for( Int lc = 0; lc < lc_count; ++lc )
    {
        const I k_begin = static_cast<I>(lc_arc_ptr[lc         ]);
        const I k_end   = static_cast<I>(lc_arc_ptr[lc + Int(1)]);
        
        if( k_end > k_begin)
        {
            const I k_last = k_end - I(1);
            
            for( I k = k_begin; k < k_last; ++k )
            {
                const I k_next = k + I(1);
                agg.Push( k + row_offset, k      + col_offset, val_0 );
                agg.Push( k + row_offset, k_next + col_offset, val_1 );
            }
            // Wrap-around - I really want it to happen last.
            {
                const I k      = k_last;
                const I k_next = k_begin;
                agg.Push( k + row_offset, k      + col_offset, val_0 );
                agg.Push( k + row_offset, k_next + col_offset, val_1 );
            }
        }
    }
} // DirichletHessian_CollectTriples


public:

template<typename R = Real, typename I = Int, typename J = Int>
Sparse::MatrixCSR<R,I,J> DirichletHessian( cref<PlanarDiagram<Int>> pd ) const
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");

    const I m = static_cast<I>(pd.ArcCount());
    
    TripleAggregator<I,I,R,J> agg( I(2) * m );
    
    DirichletHessian_CollectTriples( pd, agg, I(0), I(0) );
    
    // We have to symmetrize because DirichletHessian_CollectTriples computes essentially only the upper triangle.
    return { agg, m, m, I(1), true, true };

} // DirichletHessian
