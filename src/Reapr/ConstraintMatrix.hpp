private:

template<Op op>
void ConstraintMatrix_CollectTriples(
    mref<PlanarDiagram_T> pd,
    mref<Aggregator_T> agg,
    const Int row_offset,
    const Int col_offset
)
{
    const Int n = pd.CrossingCount();
    
    cptr<Int>           C_arcs   = pd.Crossings().data();
    cptr<CrossingState> C_states = pd.CrossingStates().data();
    
    for( Int c = 0; c < n; ++c )
    {
        const Int  a_0 = C_arcs[(c << 2)         ];
        const Int  a_1 = C_arcs[(c << 2) | Int(1)];

        const Real s   = static_cast<Real>(ToUnderlying(C_states[c]));
        
        if constexpr ( TransposedQ(op) )
        {
            const Int col = c + col_offset;
            
            agg.Push( a_0 + row_offset, col, -s );
            agg.Push( a_1 + row_offset, col,  s );
        }
        else
        {
            const Int row = c + row_offset;
            
            agg.Push( row, a_0 + col_offset, -s );
            agg.Push( row, a_1 + col_offset,  s );
        }
    }
}

public:

Matrix_T ConstraintMatrix( mref<PlanarDiagram_T> pd )
{
    const I_T n = pd.CrossingCount();
    const I_T m = pd.ArcCount();
    
    Aggregator_T agg( 2 * n );
    
    ConstraintMatrix_CollectTriples<Op::Id>( pd, agg, 0, 0 );
     
    return Matrix_T ( agg, n, m, I_T(1), true, false );
}
