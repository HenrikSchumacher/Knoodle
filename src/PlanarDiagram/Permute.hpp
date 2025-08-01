public:

template<typename PRNGT_T>
PlanarDiagram PermuteRandom( mref<PRNGT_T> random_engine ) const
{
    TOOLS_PTIMER(timer,MethodName("PermuteRandom"));
                 
    auto c_perm = Permutation<Int>::RandomPermutation(
        max_crossing_count, Int(1), random_engine
    );
    
    auto a_perm = Permutation<Int>::RandomPermutation(
        max_arc_count, Int(1), random_engine
    );
    
    return Permute(c_perm,a_perm);
}

template<typename ExtInt>
PlanarDiagram Permute(
    mref<Permutation<ExtInt>> c_perm, mref<Permutation<ExtInt>> a_perm
)  const
{
    TOOLS_PTIMER(timer,ClassName()+"::Permute<"+TypeName<ExtInt>+">");
    
    if( std::cmp_not_equal(max_crossing_count,c_perm.Size()) )
    {
        eprint(ClassName()+"::Permute<"+TypeName<ExtInt>+">: Size " + Tools::ToString(c_perm.Size()) + " does not match number of elements " + Tools::ToString(max_crossing_count) + " in Crossings(). Aborting.");
        return PlanarDiagram();
    }
    
    if( std::cmp_not_equal(max_arc_count,a_perm.Size()) )
    {
        eprint(ClassName()+"::Permute<"+TypeName<ExtInt>+">: Size " + Tools::ToString(a_perm.Size()) + " does not match number of elements " + Tools::ToString(max_arc_count) + " in Arcs(). Aborting.");
        return  PlanarDiagram();
    }
    
    cptr<Int> c_p = c_perm.GetPermutation().data();
    cptr<Int> a_p = a_perm.GetPermutation().data();
    
    PlanarDiagram pd ( max_crossing_count, this->UnlinkCount() );
    
    pd.proven_minimalQ = this->proven_minimalQ;
    pd.crossing_count  = this->crossing_count;
    pd.arc_count       = this->arc_count;
    
    for( Int s = 0; s < max_crossing_count; ++s )
    {
        const Int t = c_p[s];

        pd.C_state[t] = this->C_state[s];
        
        if( !this->CrossingActiveQ(s) )
        {
            fill_buffer<4>(pd.C_arcs.data(t),Uninitialized);
            continue;
        }

        Tiny::Matrix<2,2,Int,Int> C_s(this->C_arcs.data(s));
        Tiny::Matrix<2,2,Int,Int> C_t;
        
        C_t(Out,Left ) = a_p[C_s(Out,Left )];
        C_t(Out,Right) = a_p[C_s(Out,Right)];
        C_t(In ,Left ) = a_p[C_s(In ,Left )];
        C_t(In ,Right) = a_p[C_s(In ,Right)];

        C_t.Write( pd.C_arcs.data(t) );
    }
    
    for( Int s = 0; s < max_arc_count; ++s )
    {
        const Int t = a_p[s];
        
        pd.A_state[t] = this->A_state[s];
        
        if( !this->ArcActiveQ(s) )
        {
            fill_buffer<2>(pd.A_cross.data(t),Uninitialized);
            continue;
        }
        
        Tiny::Vector<2,Int,Int> A_s(this->A_cross.data(s));
        Tiny::Vector<2,Int,Int> A_t;
        
        A_t(Tail) = c_p[A_s(Tail)];
        A_t(Head) = c_p[A_s(Head)];
        
        A_t.Write(pd.A_cross.data(t) );
    }
    
    return pd;
}
