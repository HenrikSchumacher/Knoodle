public:


template<typename S, typename I, typename J>
Sparse::MatrixCSR<S,I,J> Lengths_ConstraintMatrix()
{
    TOOLS_PTIC(ClassName()+"::Lengths_ConstraintMatrix");
    
    TripleAggregator<I,I,S,J> agg ( J(2) * static_cast<J>(TRE_count) );
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    // Among the first 2 * FaceCount() columns, the even column 2 * f corresponds to constraints for horizontal edge  lengths around face f and the odd columns 2 *f +1  corresponds to constraints for vertical edge lengths around f.
    
    const I var_count = static_cast<I>(Dh.EdgeCount() + Dv.EdgeCount());
    const I constraint_count = I(2) * int_cast<I>(TRF_count);
    
    TOOLS_DUMP(Dh.EdgeCount());
    TOOLS_DUMP(Dh.Edges());
    TOOLS_DUMP(Dv.EdgeCount());
    TOOLS_DUMP(Dv.Edges());
    
    TOOLS_DUMP(E_Hs);
    TOOLS_DUMP(E_Vs);
    
    const I Hs_offset = 0;
    const I Vs_offset = Dh.EdgeCount();
    print("A");
    for( Int e = 0; e < TRE_count; ++e )
    {
//        TOOLS_DUMP(e);
        if( !EdgeActiveQ(e) ) { continue; }
        
        const I f_0 = static_cast<I>(TRE_TRF(e,0)); // right face
        const I f_1 = static_cast<I>(TRE_TRF(e,1)); // left  face

        Dir_T dir = E_dir[e];
        
        if( dir == East )
        {
            const I i = static_cast<I>(Vs_offset + ??? E_Hs[e]);
            TOOLS_DUMP(i);
            agg.Push( i, I(2) * f_0 + I(0), S(-1) );
            agg.Push( i, I(2) * f_1 + I(0), S( 1) );
        }
        else if( dir == North )
        {
            const I i = static_cast<I>(Hs_offset + ??? E_Vs[e]);
            TOOLS_DUMP(i);
            agg.Push( i, I(2) * f_0 + I(1), S(-1) );
            agg.Push( i, I(2) * f_1 + I(1), S( 1) );
        }
        else if( dir == West )
        {
            const I i = static_cast<I>(Vs_offset + ??? E_Hs[e]);
            TOOLS_DUMP(i);
            agg.Push( i, I(2) * f_0 + I(0), S( 1) );
            agg.Push( i, I(2) * f_1 + I(0), S(-1) );
        }
        else if( dir == South )
        {
            const I i = static_cast<I>(Hs_offset + ??? E_Vs[e]);
            TOOLS_DUMP(i);
            agg.Push( i, I(2) * f_0 + I(1), S( 1) );
            agg.Push( i, I(2) * f_1 + I(1), S(-1) );
        }
    }

    TOOLS_DUMP(var_count);
    TOOLS_DUMP(constraint_count);
    auto ii = agg.Get_0();
    auto jj = agg.Get_1();
    auto aa = agg.Get_2();
    
    ii.template Resize<true>(agg.Size());
    jj.template Resize<true>(agg.Size());
    aa.template Resize<true>(agg.Size());
    
    TOOLS_DUMP(ii.MinMax());
    TOOLS_DUMP(jj.MinMax());
    TOOLS_DUMP(aa.MinMax());
    
    print("B");
    Sparse::MatrixCSR<S,I,J> A ( agg, var_count, constraint_count, true, false );
    print("C");
    TOOLS_PTOC(ClassName()+"::Lengths_ConstraintMatrix");
    
    return A;
}

template<typename S, typename I>
Tensor1<S,I> Lengths_LowerBoundsOnVariables()
{
    const I var_count = static_cast<I>(Dh.EdgeCount() + Dv.EdgeCount());

    // Edge lengths must be at least 1.
    return Tensor1<S,I>( var_count, S(1) );
}

template<typename S, typename I>
Tensor1<S,I> Lengths_UpperBoundsOnVariables()
{
    TOOLS_MAKE_FP_STRICT();
    
    const I var_count = static_cast<I>(Dh.EdgeCount() + Dv.EdgeCount());
    
    // Edge lengths are not bounded from above.
    return Tensor1<S,I>( var_count, Scalar::Infty<S> );
}

template<typename S, typename I>
Tensor1<S,I> Lengths_LowerBoundsOnConstraints()
{
    const I constraint_count = I(2) * static_cast<I>(TRF_count);
    
    // We have only equality constraints.
    return Tensor1<S,I>( constraint_count, S(0) );
}
template<typename S, typename I>
Tensor1<S,I> Lengths_UpperBoundsOnConstraints()
{
    const I constraint_count = I(2) * static_cast<I>(TRF_count);
    
    // We have only equality constraints.
    return Tensor1<S,I>( constraint_count, S(0) );
}

template<typename S, typename I>
Tensor1<S,I> Lengths_ObjectiveVector()
{
    TOOLS_DUMP(Dh.EdgeCount());
    TOOLS_DUMP(Dh_edge_costs.Size());
    
    TOOLS_DUMP(Dv.EdgeCount());
    TOOLS_DUMP(Dv_edge_costs.Size());
    
    
    const I var_count = static_cast<I>(Dh.EdgeCount() + Dv.EdgeCount());
    
    Tensor1<S,I> c ( var_count );
    
    const I Hs_offset = 0;
    const I Vs_offset = Dh.EdgeCount();

    Dh_edge_costs.Write(c.data() + Hs_offset);
    Dv_edge_costs.Write(c.data() + Vs_offset);
    
    return c;
}

