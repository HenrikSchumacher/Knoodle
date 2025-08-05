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
