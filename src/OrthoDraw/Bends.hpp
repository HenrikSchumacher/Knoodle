template<typename ExtInt>
void RedistributeBends(
    cref<PlanarDiagram<ExtInt>> pd,
    mref<Tensor1<Turn_T,Int>> bends,
    Int iter = Int(0)
)
{
    TOOLS_PTIMER(timer,ClassName()+"::RedistributeBends");
    
    constexpr Turn_T one = Turn_T(1);
    
    using CrossingMatrix_T = Tiny::Matrix<2,2,ExtInt,ExtInt>;
    using TurnMatrix_T     = Tiny::Matrix<2,2,Turn_T,ExtInt>;
    
//    print("RedistributeBends");
    auto & C_A_loc = pd.Crossings();
    
    Int counter = 0;
    
    const ExtInt c_count = C_A_loc.Dim(0);
    
    for( ExtInt c = 0; c < c_count; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }

        CrossingMatrix_T C ( C_A_loc.data(c) );
        
        // We better do not touch Reidemeiter I loops as we would count the bends in a redundant way.
        if( (C(Out,Left ) == C(In,Left )) || (C(Out,Right) == C(In,Right)) )
        {
            continue;
        }
    
        TurnMatrix_T B = {
            { bends[C(Out,Left)], bends[C(Out,Right)] },
            { bends[C(In ,Left)], bends[C(In ,Right)] },
        };
        
        const Turn_T total_bends_before = B.AbsTotal();
        
        Turn_T in_score    = B(In ,Left ) + B(In ,Right) ;
        Turn_T out_score   = B(Out,Left ) + B(Out,Right);
        
        if( in_score >= out_score + Turn_T(3) )
        {
            const Turn_T total_bends_after =
              Abs(B(Out,Left )+one) + Abs(B(Out,Right)+one)
            + Abs(B(In ,Left )-one) + Abs(B(In ,Right)-one);

            if( total_bends_after == total_bends_before)
            {
                ++bends[C(Out,Left )]; ++bends[C(Out,Right)];
                --bends[C(In ,Left )]; --bends[C(In ,Right)];
                
//                TurnMatrix_T Bnew = {
//                    { bends[C(Out,Left)], bends[C(Out,Right)] },
//                    { bends[C(In ,Left)], bends[C(In ,Right)] },
//                };
                
                ++counter;
                continue;
            }
        }
        
        if( in_score + Turn_T(3) <= out_score )
        {
            const Turn_T total_bends_after =
              Abs(B(Out,Left )-one) + Abs(B(Out,Right)-one)
            + Abs(B(In ,Left )+one) + Abs(B(In ,Right)+one);
            
            if( total_bends_after == total_bends_before)
            {
                --bends[C(Out,Left )]; --bends[C(Out,Right)];
                ++bends[C(In ,Left )]; ++bends[C(In ,Right)];
                
//                TurnMatrix_T Bnew = {
//                    { bends[C(Out,Left)], bends[C(Out,Right)] },
//                    { bends[C(In ,Left)], bends[C(In ,Right)] },
//                };

                ++counter;
                continue;
            }
        }
        
        Turn_T left_score  = B(Out,Left ) - B(In ,Left ) ;
        Turn_T right_score = B(In ,Right) - B(Out,Right);
        
        if( left_score >= right_score + Turn_T(3) )
        {
            const Turn_T total_bends_after =
              Abs(B(Out,Left )-one) + Abs(B(Out,Right)-one)
            + Abs(B(In ,Left )+one) + Abs(B(In ,Right)+one);
            
            if( total_bends_after == total_bends_before)
            {
                --bends[C(Out,Left )]; --bends[C(Out,Right)];
                ++bends[C(In ,Left )]; ++bends[C(In ,Right)];
                
//                TurnMatrix_T Bnew = {
//                    { bends[C(Out,Left)], bends[C(Out,Right)] },
//                    { bends[C(In ,Left)], bends[C(In ,Right)] },
//                };
                
                ++counter;
                continue;
            }
        }
        
        if( left_score + Turn_T(3) <= right_score  )
        {
            const Turn_T total_bends_after =
              Abs(B(Out,Left )+one) + Abs(B(Out,Right)+one)
            + Abs(B(In ,Left )-one) + Abs(B(In ,Right)-one);
            
            if( total_bends_after == total_bends_before)
            {
                ++bends[C(Out,Left )]; ++bends[C(Out,Right)];
                --bends[C(In ,Left )]; --bends[C(In ,Right)];
                
//                TurnMatrix_T Bnew = {
//                    { bends[C(Out,Left)], bends[C(Out,Right)] },
//                    { bends[C(In ,Left)], bends[C(In ,Right)] },
//                };
                
                ++counter;
                continue;
            }
        }
    }
    
    if( (iter < 6) && (counter > Int(0)) )
    {
        RedistributeBends(pd,bends,iter+Int(1));
    }
//    else
//    {
//        if( counter > Int(0) )
//        {
//            print("!!! ABORTED !!!");
//        }
//    }
}

template<typename ExtInt>
void RandomizeBends(
    cref<PlanarDiagram<ExtInt>> pd,
    mref<Tensor1<Turn_T,Int>> bends,
    int iter_count
)
{
    TOOLS_PTIMER(timer,ClassName()+"::RandomizeBends");

    PRNG_T random_engine = InitializedRandomEngine<PRNG_T>();
    
    RandomizeBends_impl(pd,bends,iter_count,random_engine);
}

private:

template<typename ExtInt>
void RandomizeBends_impl(
    cref<PlanarDiagram<ExtInt>> pd,
    mref<Tensor1<Turn_T,Int>> bends,
    int iter_count,
    mref<PRNG_T> random_engine
)
{
    using CrossingMatrix_T = Tiny::Matrix<2,2,ExtInt,ExtInt>;
    
    std::uniform_int_distribution<Int8> dice( -1, 1 );
    
    auto & C_A_loc = pd.Crossings();

    const ExtInt c_count = C_A_loc.Dim(0);
    
    for( ExtInt c = 0; c < c_count; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }
        
        CrossingMatrix_T C ( C_A_loc.data(c) );
        
        for( int iter = 0; iter < iter_count; ++iter )
        {
            Int8 x = dice(random_engine);
            
            if( x == Int8(0) )
            {
                continue;
            }

            if( x == Int8(1) )
            {
                --bends[C(Out,Left )]; --bends[C(Out,Right)];
                ++bends[C(In ,Left )]; ++bends[C(In ,Right)];
            }
            else if( x == Int8(-2) )
            {
                ++bends[C(Out,Left )]; ++bends[C(Out,Right)];
                --bends[C(In ,Left )]; --bends[C(In ,Right)];
            }
        }
    }
}


public:

template<typename S, typename I, typename J, typename ExtInt>
static Sparse::MatrixCSR<S,I,J> Bends_ConstraintMatrix(
    cref<PlanarDiagram<ExtInt>> pd, cref<Tiny::VectorList_AoS<2,ExtInt,ExtInt>> A_idx
)
{
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    
    TripleAggregator<I,I,S,J> agg ( J(4) * static_cast<J>(pd.ArcCount()) );
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    const ExtInt a_count = pd.Arcs().Dim(0);
    
    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        // right face of a
        const I f_0  = static_cast<I>( dA_F[pd.ToDarc(a,Tail)] );
        // left  face of a
        const I f_1  = static_cast<I>( dA_F[pd.ToDarc(a,Head)] );
        
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
static Tiny::VectorList_AoS<2,ExtInt,ExtInt> Bends_ArcIndices( cref<PlanarDiagram<ExtInt>> pd )
{
    using PD_loc_T = PlanarDiagram<ExtInt>;
    
    const ExtInt a_count = pd.Arcs().Dim(0);
    
    Tiny::VectorList_AoS<2,ExtInt,ExtInt> A_idx ( a_count );
    
    ExtInt a_counter = 0;

    for( ExtInt a = 0; a < a_count; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            A_idx(a,0) = PD_loc_T::ToDarc(a_counter,Tail);
            A_idx(a,1) = PD_loc_T::ToDarc(a_counter,Head);
            ++a_counter;
        }
        else
        {
            A_idx(a,0) = PD_loc_T::Uninitialized;
            A_idx(a,1) = PD_loc_T::Uninitialized;
        }
    }
    
    return A_idx;
}
