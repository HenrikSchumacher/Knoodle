public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<COIN_Real,I,J> BendsMatrix( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_PTIC(ClassName() + "::BendsMatrix");
    
    const I arc_count  = static_cast<I>(pd.ArcCount());
    const I face_count = static_cast<I>(pd.FaceCount());
    
    cptr<Int> A_face   = pd.ArcFaces().data();
    
    TripleAggregator<I,I,COIN_Real,J> agg ( J(4) * arc_count );
    
    // We assemble the matrix transpose because CLP assumes column-major ordering.
    
    for( I a = 0; a < arc_count; ++a )
    {
        const I A_0 = I(2) * a + I(0);
        const I A_1 = I(2) * a + I(1);
        
        const I f_0 = static_cast<I>(A_face[A_0]);
        const I f_1 = static_cast<I>(A_face[A_1]);
                                     
        // f_0 is the face to the _right_ of A_0.
        agg.Push( A_0, f_0, -COIN_Real(1) );
        // f_1 is the face to the _left_  of A_0.
        agg.Push( A_0, f_1,  COIN_Real(1) );
        // f_0 is the face to the _left_  of A_1.
        agg.Push( A_1, f_0,  COIN_Real(1) );
        // f_1 is the face to the _right_ of A_1.
        agg.Push( A_1, f_1, -COIN_Real(1) );
    }
    
    Sparse::MatrixCSR<COIN_Real,I,J> A (
        agg, I(2) * arc_count, face_count, true, false
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
    // No upper bound for bends.
    return Tensor1<COIN_Real,Int>( Int(2) * pd.ArcCount(), +Scalar::Infty<COIN_Real> );
}

template<typename Int>
Tensor1<COIN_Real,Int> BendsRowEqualityVector(
    mref<PlanarDiagram<Int>> pd, const Int ext_face = -1
)
{
    Tensor1<COIN_Real,Int> v ( pd.FaceCount() );
    
    cptr<Int> f_da_ptr = pd.FaceDirectedArcPointers().data();

    Int max_arc_count = 0;
    Int max_face      = 0;
    
    for( Int f = 0; f < pd.FaceCount(); ++f )
    {
        const Int arc_count = f_da_ptr[f+1] - f_da_ptr[f];
        
        if( arc_count > max_arc_count )
        {
            max_arc_count = arc_count;
            max_face      = f;
        }
        
        v[f] = COIN_Real(4) - COIN_Real( f_da_ptr[f+1] - f_da_ptr[f] );
    }
    
    if( (Int(0) <= ext_face) && (ext_face < pd.FaceCount()) )
    {
        v[ext_face] -= COIN_Real(8);
    }
    else
    {
        v[max_face] -= COIN_Real(8);
    }
    
    return v;
}

template<typename Int>
Tensor1<COIN_Real,Int> BendsObjectiveVector( mref<PlanarDiagram<Int>> pd )
{
    return Tensor1<COIN_Real,Int> ( Int(2) * pd.ArcCount(), COIN_Real(1) );
}
