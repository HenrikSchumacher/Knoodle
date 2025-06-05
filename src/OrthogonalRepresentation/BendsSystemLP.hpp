public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<COIN_Real,I,J> BendsMatrix( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_PTIC(ClassName() + "::BendsMatrix");
    
    const I arc_count_  = static_cast<I>(pd.ArcCount());
    const I face_count_ = static_cast<I>(pd.FaceCount());
    
    cptr<Int> dA_F = pd.ArcFaces().data();
    
    TripleAggregator<I,I,COIN_Real,J> agg ( J(4) * arc_count );
    
    // We assemble the matrix transpose because CLP assumes column-major ordering.
    
    for( I a = 0; a < arc_count_; ++a )
    {
        const I da_0 = pd.ToDiArc(a,Tail);
        const I da_1 = pd.ToDiArc(a,Head);
        
        const I f_0 = static_cast<I>(dA_F[da_0]); // right face of a
        const I f_1 = static_cast<I>(dA_F[da_1]); // left  face of a
                                     
        agg.Push( da_0, f_0,  COIN_Real(1) );
        agg.Push( da_0, f_1, -COIN_Real(1) );
        agg.Push( da_1, f_0, -COIN_Real(1) );
        agg.Push( da_1, f_1,  COIN_Real(1) );
    }
    
    Sparse::MatrixCSR<COIN_Real,I,J> A (
        agg, I(2) * arc_count_, face_count_, true, false
    );

    TOOLS_PTOC(ClassName() + "::BendsMatrix");
    
    return A;
}



template<typename Int>
Tensor1<COIN_Real,Int> BendsColLowerBounds( mref<PlanarDiagram<Int>> pd )
{
    // All bends must be nonnegative.
    return Tensor1<COIN_Real,Int>( Int(2) * pd.ArcCount(), COIN_Real(0) );
}

template<typename Int>
Tensor1<COIN_Real,Int> BendsColUpperBounds( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    return Tensor1<COIN_Real,Int>( Int(2) * pd.ArcCount(), Scalar::Infty<COIN_Real> );
//    return Tensor1<COIN_Real,Int>( Int(2) * pd.ArcCount(), COIN_Real(3) );
}

template<typename Int>
Tensor1<COIN_Real,Int> BendsRowEqualityVector(
    mref<PlanarDiagram<Int>> pd, const Int ext_f = -1
)
{
    const Int face_count_ = pd.FaceCount();
    
    Tensor1<COIN_Real,Int> v ( face_count_ );
    
    cptr<Int> F_dA_ptr = pd.FaceDirectedArcPointers().data();

    Int max_f_size = 0;
    Int max_f      = 0;
    
    for( Int f = 0; f < pd.FaceCount(); ++f )
    {
        const Int f_size = F_dA_ptr[f+1] - F_dA_ptr[f];
        
        if( f_size > max_f_size )
        {
            max_f_size = f_size;
            max_f      = f;
        }
        
        v[f] = COIN_Real(4) - COIN_Real(f_size);
    }
    
    if( (Int(0) <= ext_f) && (ext_f < face_count_) )
    {
        v[ext_f] -= COIN_Real(8);
    }
    else
    {
        v[max_f] -= COIN_Real(8);
    }
    
    return v;
}

template<typename Int>
Tensor1<COIN_Real,Int> BendsObjectiveVector( mref<PlanarDiagram<Int>> pd )
{
    return Tensor1<COIN_Real,Int> ( Int(2) * pd.ArcCount(), COIN_Real(1) );
}
