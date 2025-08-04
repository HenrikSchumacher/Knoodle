private:

template<typename I, typename ExtInt>
static I Bends_VarCount( cref<PlanarDiagram<ExtInt>> pd )
{
    return I(2) * static_cast<I>(pd.ArcCount());
}

template<typename I, typename ExtInt>
static I Bends_ConCount( cref<PlanarDiagram<ExtInt>> pd )
{
    return static_cast<I>(pd.FaceCount());
}


template<typename ExtInt>
static cref<Tiny::VectorList_AoS<2,ExtInt,ExtInt>> Bends_ArcIndices( cref<PlanarDiagram<ExtInt>> pd )
{
    std::string tag (MethodName("Bends_ArcIndices"));
    
    using PD_loc_T = PlanarDiagram<ExtInt>;
    
    if(!pd.InCacheQ(tag))
    {
        const ExtInt a_count = pd.Arcs().Dim(0);
        
        Tiny::VectorList_AoS<2,ExtInt,ExtInt> A_idx ( a_count );
        
        ExtInt a_counter = 0;

        for( ExtInt a = 0; a < a_count; ++a )
        {
            if( pd.ArcActiveQ(a) )
            {
                A_idx(a,0) = PD_loc_T::template ToDarc<Tail>(a_counter);
                A_idx(a,1) = PD_loc_T::template ToDarc<Head>(a_counter);
                ++a_counter;
            }
            else
            {
                A_idx(a,0) = PD_loc_T::Uninitialized;
                A_idx(a,1) = PD_loc_T::Uninitialized;
            }
        }
        
        pd.SetCache(tag,std::move(A_idx));
    }
    
    return pd.template GetCache<Tiny::VectorList_AoS<2,ExtInt,ExtInt>>(tag);
}

public:

template<typename S, typename I, typename J, typename ExtInt>
static Sparse::MatrixCSR<S,I,J> Bends_ConstraintMatrix(
    cref<PlanarDiagram<ExtInt>> pd
)
{
    TOOLS_PTIMER(timer,MethodName("Bends_ConstraintMatrix"));
    
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    
    TripleAggregator<I,I,S,J> agg ( J(4) * static_cast<J>(pd.ArcCount()) );

    auto & A_idx = Bends_ArcIndices(pd);
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    const ExtInt a_count = pd.Arcs().Dim(0);
    
    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        // right face of a
        const I f_0  = static_cast<I>( dA_F[pd.template ToDarc<Tail>(a)] );
        // left  face of a
        const I f_1  = static_cast<I>( dA_F[pd.template ToDarc<Head>(a)] );
        
        const I di_0 = static_cast<I>( A_idx(a,0) );
        const I di_1 = static_cast<I>( A_idx(a,1) );
        
        agg.Push( di_0, f_0,  S(1) );
        agg.Push( di_0, f_1, -S(1) );
        agg.Push( di_1, f_0, -S(1) );
        agg.Push( di_1, f_1,  S(1) );
    }
    
    Sparse::MatrixCSR<S,I,J> A (
        agg, Bends_VarCount<I>(pd), Bends_ConCount<I>(pd), true, false
    );
    
    return A;
}



template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_LowerBoundsOnVariables( cref<PlanarDiagram<ExtInt>> pd )
{
    // All bends must be nonnegative.
    return Tensor1<S,I>( Bends_VarCount<I>(pd), S(0) );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_UpperBoundsOnVariables( cref<PlanarDiagram<ExtInt>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    return Tensor1<S,I>( Bends_VarCount<I>(pd), Scalar::Infty<S> );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_EqualityConstraintVector(
    cref<PlanarDiagram<ExtInt>> pd,
    const ExtInt ext_region = PlanarDiagram<ExtInt>::Uninitialized
)
{
    Tensor1<S,I> v ( Bends_ConCount<I>(pd) );
    mptr<S> v_ptr = v.data();
    
    auto & F_dA = pd.FaceDarcs();
    
    ExtInt max_f_size = 0;
    ExtInt max_f      = 0;
    
    const ExtInt f_count = F_dA.SublistCount();
    
    for( ExtInt f = 0; f < f_count; ++f )
    {
        const ExtInt f_size = F_dA.SublistSize(f);
        
        if( f_size > max_f_size )
        {
            max_f_size = f_size;
            max_f      = f;
        }
        
        v_ptr[f] = S(4) - S(f_size);
    }
    
    if(
        (ext_region != PlanarDiagram<ExtInt>::Uninitialized)
        &&
        InIntervalQ(ext_region,ExtInt(0),F_dA.SublistCount())
    )
    {
        v_ptr[ext_region] -= S(8);
    }
    else
    {
        v_ptr[max_f] -= S(8);
    }
    
    return v;
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_ObjectiveVector( cref<PlanarDiagram<ExtInt>> pd )
{
    return Tensor1<S,I>( Bends_VarCount<I>(pd), S(1) );
}
