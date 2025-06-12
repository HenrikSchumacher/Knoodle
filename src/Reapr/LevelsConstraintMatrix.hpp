private:

template<Op op, typename I, typename J, typename Int>
void LevelsConstraintMatrix_CollectTriples(
    mref<PlanarDiagram<Int>> pd,
    mref<TripleAggregator<I,I,Real,J>> agg,
    const I row_offset,
    const I col_offset
)
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIC( ClassName()+"::LevelsConstraintMatrix_CollectTriples"
        + "<" + ToString(op)
        + "," + TypeName<I>
        + "," + TypeName<J>
        + "," + TypeName<Int>
        + ">"
    );
    
    const Int n = pd.CrossingCount();
    
    cptr<Int>           C_arcs   = pd.Crossings().data();
    cptr<CrossingState> C_states = pd.CrossingStates().data();
    
    for( Int c = 0; c < n; ++c )
    {
        const I a_0 = int_cast<I>(C_arcs[(c << 2)         ]);
        const I a_1 = int_cast<I>(C_arcs[(c << 2) | Int(1)]);

        const Real s = static_cast<Real>(ToUnderlying(C_states[c]));
        
        if constexpr ( TransposedQ(op) )
        {
            const I col = int_cast<I>(c + col_offset);
            
            agg.Push( a_0 + row_offset, col, -s );
            agg.Push( a_1 + row_offset, col,  s );
        }
        else
        {
            const I row = int_cast<I>(c + row_offset);
            
            agg.Push( row, a_0 + col_offset, -s );
            agg.Push( row, a_1 + col_offset,  s );
        }
    }
    
    TOOLS_PTOC( ClassName()+"::LevelsConstraintMatrix_CollectTriples"
        + "<" + ToString(op)
        + "," + TypeName<I>
        + "," + TypeName<J>
        + "," + TypeName<Int>
        + ">"
    );
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
