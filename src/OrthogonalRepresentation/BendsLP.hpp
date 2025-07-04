public:

// TODO: Try some of the suggestions from https://stackoverflow.com/a/63254942/8248900:
// TODO: Coin-OR's Lemon library
// TODO: Coin-OR's Network Simplex

/*! @brief Computes a vector `bends` whose entries are signed integers. For each arc `a` the entry `bends[a]` is the number of 90-degree bends for that arc. Positive numbers mean bends to the left; positive numbers mean bend to the right. The l^1 norm of `bends` subject to the constraints that for each face `f` the sum of bends along bounday arcs of `d` and of the number of corners of `f` have to sum to 4, which corresponds to winding number 1. These constraints are linear, so that this is a linearly constraint and l^1 optimization problem. It is reformulated as linear programming problem and solved with a simplex-based method to obtain a basis solution `bend`. By the structure of this problem, all entries of `bends` are guaranteed to be integers.
 *  This approach is similar to Tamassia, On embedding a graph in the grid with the minimum number of bends. Siam J. Comput. 16 (1987) http://dx.doi.org/10.1137/0216030. The main difference is that we can assume that each vertex in the graph underlying the planar diagram has valence 4. Moreover, we do not use the elaborate formulation as min cost flow problem, which would allow us to use a potentially faster network solver. Instead, we use a CLP, a generic solver for linear problems.
 *
 * @param pd Planar diagram whose bends we want to compute.
 *
 * @param ext_region Specify which face shall be treated as exterior region. If `ext_region < 0` or `ext_region > pd.FaceCount()`, then a face with maximum number of arcs is chosen.
 */

template<typename ExtInt, typename ExtInt2>
Tensor1<Turn_T,Int> ComputeBends(
    mref<PlanarDiagram<ExtInt>> pd,
    const ExtInt2 ext_region = -1,
    bool dualQ = false
)
{
    TOOLS_MAKE_FP_STRICT();

    TOOLS_PTIMER( timer, ClassName()+"::ComputeBends");
    
    ExtInt ext_region_ = int_cast<ExtInt>(ext_region);
            
    {
        Size_T max_idx = Size_T(2) * static_cast<Size_T>(pd.Arcs().Dim(0));
        Size_T nnz     = Size_T(4) * static_cast<Size_T>(pd.ArcCount());
        
        if( std::cmp_greater( max_idx, std::numeric_limits<COIN_Int>::max() ) )
        {
            eprint(ClassName()+"::ComputeBends: Too many arcs to fit into type " + TypeName<COIN_Int> + ".");
            
            return Tensor1<Turn_T,Int>();
        }
        
        if( std::cmp_greater( nnz, std::numeric_limits<COIN_LInt>::max() ) )
        {
            eprint(ClassName()+"::ComputeBends: System matrix has more nonzeroes than can be counted by type `CoinBigIndex` ( a.k.a. " + TypeName<COIN_LInt> + "  ).");
            
            return Tensor1<Turn_T,Int>();
        }
    }
    
    using R = COIN_Real;
    using I = COIN_Int;
    using J = COIN_LInt;
    
    auto con_eq = Bends_EqualityConstraintVector<R,I>(pd,ext_region_);
    
    ClpWrapper<double,Int> clp(
        Bends_ObjectiveVector<R,I>(pd),
        Bends_LowerBoundsOnVariables<R,I>(pd),
        Bends_UpperBoundsOnVariables<R,I>(pd),
        Bends_ConstraintMatrix<R,I,J>(pd),
        con_eq,
        con_eq,
        { .dualQ = settings.use_dual_simplexQ }
    );
    
    Tensor1<Turn_T,Int> bends ( pd.Arcs().Dim(0) );
    
    auto s = clp.template IntegralPrimalSolution<Turn_T>();

    for( ExtInt a = 0; a < pd.Arcs().Dim(0); ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const ExtInt head = pd.ToDarc(a,Head);
            const ExtInt tail = pd.ToDarc(a,Tail);
            bends[a] = s[head] - s[tail];
        }
        else
        {
            bends[a] = Turn_T(0);
        }
    }

    return bends;
}


public:

template<typename S, typename I, typename J, typename ExtInt>
static Sparse::MatrixCSR<S,I,J> Bends_ConstraintMatrix( mref<PlanarDiagram<ExtInt>> pd
)
{
    TOOLS_PTIC(ClassName()+"::Bends_ConstraintMatrix");
    
    cptr<ExtInt> dA_F = pd.ArcFaces().data();
    
    TripleAggregator<I,I,S,J> agg ( J(4) * static_cast<J>(pd.ArcCount()) );
    
    // CAUTION:
    // We assemble the matrix transpose because CLP assumes column-major ordering!
    
    for( ExtInt a = 0; a < pd.Arcs().Dim(0); ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; };
        
        const I da_0 = static_cast<I>( pd.template ToDarc<Tail>(a) );
        const I da_1 = static_cast<I>( pd.template ToDarc<Head>(a) );
        
        const I f_0  = static_cast<I>(dA_F[da_0]); // right face of a
        const I f_1  = static_cast<I>(dA_F[da_1]); // left  face of a
                                     
        agg.Push( da_0, f_0,  S(1) );
        agg.Push( da_0, f_1, -S(1) );
        agg.Push( da_1, f_0, -S(1) );
        agg.Push( da_1, f_1,  S(1) );
    }
    
    const I var_count        = I(2) * static_cast<I>(pd.Arcs().Dim(0));
    const I constraint_count = static_cast<I>(pd.FaceCount());
    
    Sparse::MatrixCSR<S,I,J> A ( agg, var_count, constraint_count, true, false );

    TOOLS_PTOC(ClassName()+"::Bends_ConstraintMatrix");
    
    return A;
}



template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_LowerBoundsOnVariables( mref<PlanarDiagram<ExtInt>> pd )
{
    const I var_count = I(2) *static_cast<I>(pd.Arcs().Dim(0));
    
    // All bends must be nonnegative.
    return Tensor1<S,I>( var_count, S(0) );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_UpperBoundsOnVariables( mref<PlanarDiagram<ExtInt>> pd )
{
    TOOLS_MAKE_FP_STRICT();
    
    const I var_count = I(2) * static_cast<I>(pd.Arcs().Dim(0));
    
    return Tensor1<S,I>( var_count, Scalar::Infty<S> );
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_EqualityConstraintVector(
    mref<PlanarDiagram<ExtInt>> pd, const ExtInt ext_region = -1
)
{
    const I constraint_count = int_cast<I>(pd.FaceCount());
    
    Tensor1<S,I> v ( constraint_count );
    mptr<S> v_ptr = v.data();
    
//    cptr<ExtInt> F_dA_ptr = pd.FaceDarcs().Pointers().data();

    auto & F_dA = pd.FaceDarcs();
    
    ExtInt max_f_size = 0;
    ExtInt max_f      = 0;
    
    for( ExtInt f = 0; f < F_dA.SublistCount(); ++f )
    {
//        const ExtInt f_size = F_dA_ptr[f+1] - F_dA_ptr[f];
        
        const ExtInt f_size = F_dA.SublistSize(f);
        
        if( f_size > max_f_size )
        {
            max_f_size = f_size;
            max_f      = f;
        }
        
        v_ptr[f] = S(4) - S(f_size);
    }
    
    if( (ExtInt(0) <= ext_region) && (ext_region < F_dA.SublistCount()) )
    {
        v_ptr[ext_region] -= S(8);
    }
    else
    {
        v_ptr[ext_region] -= S(8);
    }
    
    return v;
}

template<typename S, typename I, typename ExtInt>
static Tensor1<S,I> Bends_ObjectiveVector( mref<PlanarDiagram<ExtInt>> pd )
{
    const I var_count = I(2) * static_cast<I>(pd.Arcs().Dim(0));
    
    return Tensor1<S,I>( var_count, S(1) );
}


//template<typename ExtInt>
//void RedistributeBends1(
//    cref<PlanarDiagram<ExtInt>> pd,
//    mref<Tensor1<Turn_T,Int>> bends
//)
//{
//    TOOLS_PTIMER(timer,ClassName()+"::RedistributeBends1");
//    
////    print(ClassName()+"::RedistributeBends1");
//    auto & C_A = pd.Crossings();
//    
//    Int counter = 0;
//    
//    for( ExtInt c = 0; c < C_A.Dim(0); ++c )
//    {
//        if( !pd.CrossingActiveQ(c) ) { continue; }
//
//        Tiny::Matrix<2,2,ExtInt,ExtInt> C ( C_A.data(c) );
//        
//        Tiny::Matrix<2,2,Turn_T,ExtInt> B = {
//            { bends[C(Out,Left)], bends[C(Out,Right)] },
//            { bends[C(In ,Left)], bends[C(In ,Right)] },
//        };
//        
//        // Checking whether there are two neighboring arcs without any bends.
//        
//        if      ( (B(Out,Left ) == Turn_T(0)) && (B(Out,Right) == Turn_T(0)) )
//        {
//            Turn_T score = B(In ,Left ) + B(In ,Right) ;
//            
//            if( score >= Turn_T(4) )
//            {
//                print("Redistributing bends at crossing " + ToString(c) + " (case A.1)");
//                
//                // Before:             X C(Out,Right)
//                //                     ^
//                //                     |
//                //                     |
//                //      C(Out,Left )   |c
//                // ?         X<--------X<--------+ C(In ,Right)
//                // |                   ^         ^
//                // |                   |         |
//                // |                   |         |
//                // v     C(In ,Left )  |         |
//                // X------------------>+         |
//                //                               |
//                //                               |
//                //                               |
//                //                               |
//                //                     ?-------->+
//                
//                // After:
//                //
//                // ?         X<--------+         X C(Out,Right)
//                // |     C(Out,Left )  |         ^
//                // |                   |         |
//                // |                   |         |
//                // v    C(In ,Left )   |c        |
//                // X------------------>X-------->+
//                //                     ^
//                //                     |
//                //                     |
//                //                     |
//                //           ?-------->+ C(In ,Right)
//                
//                bends[C(Out,Left )] += Turn_T(1);
//                bends[C(Out,Right)] += Turn_T(1);
//                bends[C(In ,Left )] -= Turn_T(1);
//                bends[C(In ,Right)] -= Turn_T(1);
//                ++counter;
//            }
//            else if ( score <= Turn_T(-4) )
//            {
//                print("Redistributing bends at crossing " + ToString(c) + " (case A.2)");
//                
//                bends[C(Out,Left )] -= Turn_T(1);
//                bends[C(Out,Right)] -= Turn_T(1);
//                bends[C(In ,Left )] += Turn_T(1);
//                bends[C(In ,Right)] += Turn_T(1);
//                ++counter;
//            }
//        }
//        else if ( (B(Out,Left ) == Turn_T(0)) && (B(In ,Left ) == Turn_T(0)) )
//        {
//
//            Turn_T score = B(In ,Right) - B(Out,Right) ;
//            
//            if( score >= Turn_T(4) )
//            {
//                // Before:             X C(Out,Left )
//                //                     ^
//                //                     |
//                //                     |
//                //      C(In ,Left )   |c         C(Out,Right)
//                // ?         X-------->X-------->+
//                // |                   ^         |
//                // |                   |         |
//                // |                   |         |
//                // |                   |         |
//                // X------------------>+         |
//                //           C(In ,Right)        |
//                //                               |
//                //                               |
//                //                               v
//                //                     ?<--------+
//                
//                // After:
//                //             C(In ,Left )
//                // ?         ?---------+         ?
//                // |                   |         ^
//                // |                   |         |
//                // |                   |         |
//                // |                   vc        | C(Out,Left )
//                // X------------------>X-------->+
//                //       C(In ,Right)  |
//                //                     |
//                //                     |
//                //                     v
//                //           ?<--------+ C(Out,Right)
//                
//                print("Redistributing bends at crossing " + ToString(c) + " (case B.1)");
//                
//                bends[C(Out,Left )] += Turn_T(1);
//                bends[C(Out,Right)] += Turn_T(1);
//                bends[C(In ,Left )] -= Turn_T(1);
//                bends[C(In ,Right)] -= Turn_T(1);
//                ++counter;
//            }
//            else if ( score <= Turn_T(-4) )
//            {
//                print("Redistributing bends at crossing " + ToString(c) + " (case B.2)");
//                bends[C(Out,Left )] -= Turn_T(1);
//                bends[C(Out,Right)] -= Turn_T(1);
//                bends[C(In ,Left )] += Turn_T(1);
//                bends[C(In ,Right)] += Turn_T(1);
//                ++counter;
//            }
//        }
//        else if ( (B(In ,Left ) == Turn_T(0)) && (B(In ,Right) == Turn_T(0)) )
//        {
//            Turn_T score = - B(Out,Left ) - B(Out,Right);
//            
//            if( score >= Turn_T(4) )
//            {
//                // Before:             X C(In ,Left )
//                //                     |
//                //                     |
//                //                     |
//                //      C(In ,Right)   vc
//                // ?         X-------->X-------->+ C(Out,Left )
//                // ^                   |         |
//                // |                   |         |
//                // |                   |         |
//                // |                   v         |
//                // X-------------------+         |
//                //      C(Out,Rigjt)             |
//                //                               |
//                //                               |
//                //                               v
//                //                     ?<--------+
//                
//                // After:
//                //           C(In ,Right)
//                // ?         X---------+         X
//                // ^                   |         |
//                // |                   |         |
//                // |                   |         |
//                // |     C(Out,Right)  vc        v
//                // X<------------------X<--------+ C(In ,Left )
//                //                     |
//                //                     |
//                //                     |
//                //                     v C(Out,Left )
//                //           ?<--------+
//                
//                print("Redistributing bends at crossing " + ToString(c) + " (case C.1)");
//                
//                bends[C(Out,Left )] += Turn_T(1);
//                bends[C(Out,Right)] += Turn_T(1);
//                bends[C(In ,Left )] -= Turn_T(1);
//                bends[C(In ,Right)] -= Turn_T(1);
//                ++counter;
//            }
//            else if ( score <= Turn_T(-4) )
//            {
//                print("Redistributing bends at crossing " + ToString(c) + " (case C.2)");
//                
//                bends[C(Out,Left )] -= Turn_T(1);
//                bends[C(Out,Right)] -= Turn_T(1);
//                bends[C(In ,Left )] += Turn_T(1);
//                bends[C(In ,Right)] += Turn_T(1);
//                ++counter;
//            }
//        }
//        else if ( (B(In ,Right) == Turn_T(0)) && (B(Out,Right) == Turn_T(0)) )
//        {
//            Turn_T score = B(In ,Left ) - B(Out,Left );
//            
//            if( score >= Turn_T(4) )
//            {
//                // Before:             X C(In ,Right)
//                //                     |
//                //                     |
//                //                     |
//                //      C(Out,Right)   vc         C(In ,Left )
//                // ?         X<--------X<--------+
//                // ^                   |         ^
//                // |                   |         |
//                // |                   |         |
//                // |                   v         |
//                // X<------------------+         |
//                //           C(Out,Left )        |
//                //                               |
//                //                               |
//                //                               |
//                //                     ?-------->+
//                
//                // After:
//                //             C(Out,Right)
//                // ?         ?<--------+         ?
//                // ^                   |         |
//                // |                   |         |
//                // |                   |         |
//                // |                   |c        v C(In ,Right)
//                // X<------------------X<--------+
//                //       C(Out,Left )  |
//                //                     |
//                //                     |
//                //                     v
//                //           ?<--------+ C(Out,Left )
//                
//                print("Redistributing bends at crossing " + ToString(c) + " (case D.1)");
//                
//                bends[C(Out,Left )] += Turn_T(1);
//                bends[C(Out,Right)] += Turn_T(1);
//                bends[C(In ,Left )] -= Turn_T(1);
//                bends[C(In ,Right)] -= Turn_T(1);
//                ++counter;
//            }
//            else if ( score <= Turn_T(-4) )
//            {
//                print("Redistributing bends at crossing " + ToString(c) + " (case D.2)");
//                bends[C(Out,Left )] -= Turn_T(1);
//                bends[C(Out,Right)] -= Turn_T(1);
//                bends[C(In ,Left )] += Turn_T(1);
//                bends[C(In ,Right)] += Turn_T(1);
//                ++counter;
//            }
//        }
//    }
//    
//    if( counter > Int(0) )
//    {
//        RedistributeBends1(pd,bends);
//    }
//}


template<typename ExtInt>
void RedistributeBends(
    cref<PlanarDiagram<ExtInt>> pd,
    mref<Tensor1<Turn_T,Int>> bends,
    Int iter = Int(0)
)
{
    TOOLS_PTIMER(timer,ClassName()+"::RedistributeBends");
    
    constexpr Turn_T one = Turn_T(1);
    
//    print("RedistributeBends");
    auto & C_A = pd.Crossings();
    
    Int counter = 0;
    
    for( ExtInt c = 0; c < C_A.Dim(0); ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }

        Tiny::Matrix<2,2,ExtInt,ExtInt> C ( C_A.data(c) );
        
        // We better do not touch Reidemeiter I loops as we would count the bends in a redundant way.
        if( (C(Out,Left ) == C(In,Left )) || (C(Out,Right) == C(In,Right)) )
        {
            continue;
        }
    
        Tiny::Matrix<2,2,Turn_T,ExtInt> B = {
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
                
                Tiny::Matrix<2,2,Turn_T,ExtInt> Bnew = {
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
                
                Tiny::Matrix<2,2,Turn_T,ExtInt> Bnew = {
                    { bends[C(Out,Left)], bends[C(Out,Right)] },
                    { bends[C(In ,Left)], bends[C(In ,Right)] },
                };

                ++counter;
                continue;
            }
        }
        
        Turn_T left_score  =   B(Out,Left ) - B(In ,Left ) ;
        Turn_T right_score =   B(In ,Right) - B(Out,Right);
        
        if( left_score >= right_score + Turn_T(3) )
        {
            const Turn_T total_bends_after =
              Abs(B(Out,Left )-one) + Abs(B(Out,Right)-one)
            + Abs(B(In ,Left )+one) + Abs(B(In ,Right)+one);
            
            if( total_bends_after == total_bends_before)
            {
                --bends[C(Out,Left )]; --bends[C(Out,Right)];
                ++bends[C(In ,Left )]; ++bends[C(In ,Right)];
                
                Tiny::Matrix<2,2,Turn_T,ExtInt> Bnew = {
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
                
                Tiny::Matrix<2,2,Turn_T,ExtInt> Bnew = {
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
