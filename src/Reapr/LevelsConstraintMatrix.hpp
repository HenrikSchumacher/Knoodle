private:

template<Op op, typename I, typename J, typename Int>
void LevelsConstraintMatrix_CollectTriples(
    mref<PlanarDiagram<Int>> pd,
    mref<TripleAggregator<I,I,Real,J>> agg,
    const I row_offset,
    const I col_offset
) const
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    std::string tag = ClassName()+"::LevelsConstraintMatrix_CollectTriples"
    + "<" + ToString(op)
    + "," + TypeName<I>
    + "," + TypeName<J>
    + "," + TypeName<Int>
    + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    const Int n = pd.CrossingCount();
    const Int m = pd.ArcCount();
    
    Size_T max_index = ToSize_T(Max(m,n)) + ToSize_T(Max(row_offset,col_offset));
    
    if( !std::in_range<I>(max_index) )
    {
        eprint(tag + ": Type " + TypeName<I> + " is too small to store maximum index = " + ToString(max_index) + ". Aborting.");
        return;
    }
    
    Size_T nnz = ToSize_T(2 * agg.Size()) + ToSize_T(m);
    
    if( !std::in_range<J>(nnz) )
    {
        eprint(tag + ": Type " + TypeName<J> + " is too small to store number of nonzero elements = " + ToString(nnz) + ". Aborting.");
        return;
    }
    
    cptr<Int>           C_arcs   = pd.Crossings().data();
    cptr<CrossingState> C_states = pd.CrossingStates().data();
    cptr<Int>           A_pos    = pd.ArcPositions().data();
    
    const Int C_count = pd.MaxCrossingCount();
    
    I c_pos = 0;
    
    for( Int c = 0; c < C_count; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }
        
        const Int a_0 = C_arcs[Int(4) * c         ];
        const Int a_1 = C_arcs[Int(4) * c + Int(1)];

        const I a_0_pos = static_cast<I>(A_pos[a_0]);
        const I a_1_pos = static_cast<I>(A_pos[a_1]);
                                  
        const Real s = static_cast<Real>(ToUnderlying(C_states[c]));
        
        //  Case: right-handed.
        //
        //      a_0     a_1
        //        ^     ^
        //         \   /
        //          \ /
        //           /
        //          / \
        //         /   \
        //        /     \
        //
        // If s == 1 (right-handed), then the levels `x` have to satisfy the following inequalities:
        //   x[a_1] >= x[a_0] + 1
        // - x[a_0] + x[a_1] >= 1
        //   x[a_0] - x[a_1] <= -1

        if constexpr ( TransposedQ(op) )
        {
            const I col = int_cast<I>(c_pos + col_offset);
            
            agg.Push( a_0_pos + row_offset, col,  s );
            agg.Push( a_1_pos + row_offset, col, -s );
        }
        else
        {
            const I row = int_cast<I>(c_pos + row_offset);
            
            agg.Push( row, a_0_pos + col_offset,  s );
            agg.Push( row, a_1_pos + col_offset, -s );
        }
        
        ++c_pos;
    }
}

public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<Real,I,J> LevelsConstraintMatrix( mref<PlanarDiagram<Int>> pd )
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    const I n = int_cast<I>(pd.CrossingCount());
    const I m = int_cast<I>(pd.ArcCount());
    
    TripleAggregator<I,I,Real,J> agg( J(2) * n );
    
    LevelsConstraintMatrix_CollectTriples<Op::Id>( pd, agg, I(0), I(0) );
    
    return { agg, n, m, I(1), true, false };
}
