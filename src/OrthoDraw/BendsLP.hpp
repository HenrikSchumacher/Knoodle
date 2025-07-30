private:

template<typename I, typename ExtInt>
static I Bends_VarCount( mref<PlanarDiagram<ExtInt>> pd )
{
    return I(2) * static_cast<I>(pd.ArcCount());
}

template<typename I, typename ExtInt>
static I Bends_ConCount( mref<PlanarDiagram<ExtInt>> pd )
{
    return static_cast<I>(pd.FaceCount());
}


template<typename ExtInt>
static cref<Tiny::VectorList_AoS<2,ExtInt,ExtInt>> Bends_ArcIndices( mref<PlanarDiagram<ExtInt>> pd )
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
static Sparse::MatrixCSR<S,I,J> Bends_ConstraintMatrix( mref<PlanarDiagram<ExtInt>> pd
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
static Tensor1<S,I> Bends_LowerBoundsOnVariables( mref<PlanarDiagram<ExtInt>> pd )
{
    // All bends must be nonnegative.
    return Tensor1<S,I>( Bends_VarCount<I>(pd), S(0) );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_UpperBoundsOnVariables( mref<PlanarDiagram<ExtInt>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    return Tensor1<S,I>( Bends_VarCount<I>(pd), Scalar::Infty<S> );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_EqualityConstraintVector(
    mref<PlanarDiagram<ExtInt>> pd,
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
static Tensor1<S,I> Bends_ObjectiveVector( mref<PlanarDiagram<ExtInt>> pd )
{
    return Tensor1<S,I>( Bends_VarCount<I>(pd), S(1) );
}



// TODO: This does not fit to the rest in this file. I should move ot somewhere else.

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
    using TurnMatrix_T = Tiny::Matrix<2,2,Turn_T,ExtInt>;
    
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
                
                TurnMatrix_T Bnew = {
                    { bends[C(Out,Left)], bends[C(Out,Right)] },
                    { bends[C(In ,Left)], bends[C(In ,Right)] },
                };
                
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
                
                TurnMatrix_T Bnew = {
                    { bends[C(Out,Left)], bends[C(Out,Right)] },
                    { bends[C(In ,Left)], bends[C(In ,Right)] },
                };

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
                
                TurnMatrix_T Bnew = {
                    { bends[C(Out,Left)], bends[C(Out,Right)] },
                    { bends[C(In ,Left)], bends[C(In ,Right)] },
                };
                
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
                
                TurnMatrix_T Bnew = {
                    { bends[C(Out,Left)], bends[C(Out,Right)] },
                    { bends[C(In ,Left)], bends[C(In ,Right)] },
                };
                
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
