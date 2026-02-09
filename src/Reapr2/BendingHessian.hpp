private:

template<typename R = Real, typename I = Int, typename J = Int>
void BendingHessian_CollectTriples(
    cref<PD_T> pd,
    mref<TripleAggregator<I,I,R,J>> agg,
    const I row_offset,
    const I col_offset
) const
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    [[maybe_unused]] auto tag = [](){ return MethodName("BendingHessian_CollectTriples")
        + "<" + TypeName<R>
        + "," + TypeName<I>
        + "," + TypeName<J>
        + ">";};
    
    TOOLS_PTIMER(timer,tag());
    
    const Int m = pd.ArcCount();
    
    Size_T max_index = ToSize_T(m) + ToSize_T(Max(row_offset,col_offset));
    
    if( !std::in_range<I>(max_index) )
    {
        eprint(tag() + ": Type " + TypeName<I> + " is too small to store maximum index = " + ToString(max_index) + ". Aborting.");
        return;
    }
    
    Size_T nnz = ToSize_T(2 * agg.Size()) + ToSize_T(5 * pd.ArcCount());
    
    if( !std::in_range<J>(nnz) )
    {
        eprint(tag() + ": Type " + TypeName<J> + " is too small to store number of nonzero elements = " + ToString(nnz) + ". Aborting.");
        return;
    }
    
    // Caution: This creates only the triples for the upper triangle.
    
    const auto & lc_arcs = pd.LinkComponentArcs();
    const Int lc_count   = lc_arcs.SublistCount();
    cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
    
    const R reg   = static_cast<R>(settings.bending_reg);
    const R val_0 = R(2) + (R(2) + reg) * (R(2) + reg);
    const R val_1 = -R(4) - R(2) * reg;
    const R val_2 = R(1);

    for( Int lc = 0; lc < lc_count; ++lc )
    {
        const I k_begin = static_cast<I>(lc_arc_ptr[lc         ]);
        const I k_end   = static_cast<I>(lc_arc_ptr[lc + Int(1)]);
        
        if( k_end > k_begin + I(1))
        {
            const I k_pen  = k_end - I(2);
            const I k_last = k_end - I(1);
            
            for( Int k = k_begin; k < k_pen; ++k )
            {
                const I kp1 = k + I(1);
                const I kp2 = k + I(2);
    
                agg.Push( k + row_offset, k   + col_offset, val_0 );
                agg.Push( k + row_offset, kp1 + col_offset, val_1 );
                agg.Push( k + row_offset, kp2 + col_offset, val_2 );
            }
            
            {
                const I k   = k_pen;
                const I kp1 = k_last;
                const I kp2 = k_begin;
    
                agg.Push( k + row_offset, k   + col_offset, val_0 );
                agg.Push( k + row_offset, kp1 + col_offset, val_1 );
                agg.Push( k + row_offset, kp2 + col_offset, val_2 );
            }
            
            {
                const I k   = k_last;
                const I kp1 = k_begin;
                const I kp2 = k_begin + I(1);
    
                agg.Push( k + row_offset, k   + col_offset, val_0 );
                agg.Push( k + row_offset, kp1 + col_offset, val_1 );
                agg.Push( k + row_offset, kp2 + col_offset, val_2 );
            }
        }
        else
        {
            // Write the identity matrix.
            for( Int k = k_begin; k < k_end; ++k )
            {
                agg.Push( k + row_offset, k + col_offset, Real(1) );
            }
        }

    }
    
} // BendingHessian_CollectTriples

public:

template<typename R = Real, typename I = Int, typename J = Int>
Sparse::MatrixCSR<R,I,J> BendingHessian( cref<PD_T> pd ) const
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    const I m = static_cast<I>(pd.ArcCount());
    
    TripleAggregator<I,I,R,J> agg( J(3) * static_cast<J>(m) );
    
    BendingHessian_CollectTriples( pd, agg, I(0), I(0) );
    
    return { agg, m, m, I(1), true, true };
    
} // BendingHessian
