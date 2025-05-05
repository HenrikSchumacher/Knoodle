template<typename T = Int>
Tensor1<T,Int> GaussCode()
{
    TOOLS_PTIC(ClassName()+"::GaussCode<" + TypeName<T> + ">" );
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> gauss_code;
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::GaussCode: Trying to compute Gauss code of invalid PlanarDiagram. Returning empty vector.");
        goto Exit;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName() + "::GaussCode: Requested type " + TypeName<T> +  " cannot store Gauss code for this diagram.");
    }
    
    gauss_code = Tensor1<T,Int> ( arc_count );
    
    TraverseWithCrossings(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        [&gauss_code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            T c_pos = static_cast<T>(c_0_pos) + T(1);

            gauss_code[a_pos] = ( ArcOverQ<Tail>(a) ? c_pos : -c_pos );
        },
        []( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
        }
    );
    
Exit:
    
    TOOLS_PTOC(ClassName()+"::GaussCode<" + TypeName<T> + ">" );
    
    return gauss_code;
}



template<typename T = Int>
Tensor1<T,Int> ExtendedGaussCode()
{
    TOOLS_PTIC(ClassName()+"::ExtendedGaussCode<" + TypeName<T> + ">" );
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> gauss_code;
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::ExtendedGaussCode: Trying to compute extended Gauss code of invalid PlanarDiagram. Returning empty vector.");
        
        goto Exit;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName() + "::ExtendedGaussCode: Requested type " + TypeName<T> + " cannot store extended Gauss code for this diagram.");
    }
    
    gauss_code = Tensor1<T,Int>( arc_count );
    
    TraverseWithCrossings(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        [&gauss_code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            // We need 1-based integers to be able to use signs.
            const T c_pos = static_cast<T>(c_0_pos) + T(1);
            
            gauss_code[a_pos] =
                c_0_visitedQ
                ? ( CrossingRightHandedQ(c_0) ? c_pos : -c_pos )
                : ( ArcOverQ<Tail>(a)         ? c_pos : -c_pos );
        },
        []( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
        }
    );
    
Exit:
    
    TOOLS_PTOC(ClassName()+"::ExtendedGaussCode<" + TypeName<T> + ">" );
    
    return gauss_code;
}

// TODO: This one is incomplete.
template<typename T, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromExtendedGaussCode(
    cptr<T>       gauss_code,
    const ExtInt2 arc_count_,
    const ExtInt3 unlink_count_,
    const bool    provably_minimalQ_ = false
)
{
    print("FromExtendedGaussCode");
    
    TOOLS_DUMP(arc_count_);
    
    PlanarDiagram<Int> pd (int_cast<Int>(arc_count_/2),int_cast<Int>(unlink_count_));
    
    pd.provably_minimalQ = provably_minimalQ_;
    
    TOOLS_DUMP(pd.max_arc_count);
    
    static_assert( SignedIntQ<T>, "" );
    
    if( arc_count_ <= 0 )
    {
        return pd;
    }
    
    T   g_0;
    Int c_0;
    Int c_counter = 0;
    
    g_0 = gauss_code[0];
    c_0 = Abs(int_cast<Int>(g_0)) - Int(1);
    if( g_0 == T(0) )
    {
        eprint(ClassName() + "::FromExtendedGaussCode: Input code is invalid as it contains a crossing with label 0 detected. Returnin invalid PlanarDiagram.");
        
        return PlanarDiagram<Int>();
    }
    
    
    for( Int a = pd.max_arc_count; a --> Int(0); )
    {
        TOOLS_DUMP(a);
        
        const T   g_1 = g_0;
        const Int c_1 = c_0;
        
        g_0 = gauss_code[a];
        c_0 = Abs(int_cast<Int>(g_0)) - Int(1);
        if( g_0 == T(0) )
        {
            eprint(ClassName() + "::FromExtendedGaussCode: Input code is invalid as it contains a crossing with label 0 detected. Returnin invalid PlanarDiagram.");
            
            return PlanarDiagram<Int>();
        }
        
        pd.A_cross(a,Head) = c_1;
        pd.A_cross(a,Tail) = c_0;
        // TODO: Handle over/under in ArcState.
        pd.A_state[a] = ArcState::Active;
        
        // Handle a as ingoing arc.
        {
            const bool visitedQ = pd.C_arcs(c_1,In,Left) != Int(-1);
            
            TOOLS_DUMP(g_1)
            TOOLS_DUMP(c_1)
            TOOLS_DUMP(pd.C_state[c_1])
            TOOLS_DUMP(pd.C_arcs(c_1,In,Left ))
            TOOLS_DUMP(pd.C_arcs(c_1,In,Right))
            
            if( !visitedQ )
            {
                pd.C_arcs(c_1,In,Left) = a;
            }
            else
            {
                const bool overQ_1 = (g_1 > T(0));
                
                /*
                 * Situation in case of CrossingRightHandedQ(c) and overQ_1
                 *
                 *          ^     ^
                 *           \   /
                 *            \ /
                 *             / <--- c_1
                 *            ^ ^
                 *           /   \
                 *          /     \
                 *         a
                 */
                
                if( overQ_1 != pd.CrossingRightHandedQ(c_1) )
                {
                    pd.C_arcs(c_1,In,Right) = a;
                }
                else
                {
                    pd.C_arcs(c_1,In,Right) = pd.C_arcs(c_1,In,Left);
                    pd.C_arcs(c_1,In,Left ) = a;
                }
            }
            
            TOOLS_DUMP(pd.C_arcs(c_1,In,Left ))
            TOOLS_DUMP(pd.C_arcs(c_1,In,Right))
        }
        
        // Handle a as outgoing arc.
        {
            const bool visitedQ = pd.C_arcs(c_0,Out,Left) != Int(-1);
            
            TOOLS_DUMP(g_0)
            TOOLS_DUMP(c_0)
            TOOLS_DUMP(pd.C_state[c_0])
            TOOLS_DUMP(pd.C_arcs(c_0,Out,Left ))
            TOOLS_DUMP(pd.C_arcs(c_0,Out,Right))
            
            if( !visitedQ )
            {
                ++c_counter;
                pd.C_state[c_0] = (g_0 > T(0))
                                ? CrossingState::RightHanded
                                : CrossingState::LeftHanded;
                pd.C_arcs(c_0,Out,Left) = a;
            }
            else
            {
                const bool overQ_0 = (g_0 > T(0));
                
                /*
                 * Situation in case of CrossingRightHandedQ(c) and overQ_0
                 *
                 *                 a
                 *          ^     ^
                 *           \   /
                 *            \ /
                 *             / <--- c_0
                 *            ^ ^
                 *           /   \
                 *          /     \
                 */
                
                if( overQ_0 == pd.CrossingRightHandedQ(c_0) )
                {
                    pd.C_arcs(c_0,Out,Right) = a;
                }
                else
                {
                    pd.C_arcs(c_0,Out,Right) = pd.C_arcs(c_0,Out,Left);
                    pd.C_arcs(c_0,Out,Left ) = a;
                }
            }
            
            TOOLS_DUMP(pd.C_arcs(c_0,Out,Left ))
            TOOLS_DUMP(pd.C_arcs(c_0,Out,Right))
        }
        
        
    }
    
    pd.crossing_count = c_counter;
    pd.arc_count      = int_cast<Int>(arc_count_);
    
//    TOOLS_DUMP(pd.C_arcs);
//    TOOLS_DUMP(pd.C_state);
//    TOOLS_DUMP(pd.A_cross);
//    TOOLS_DUMP(pd.A_state);
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(ClassName() + "FromPDCode: Input PD code is invalid because arc_count != 2 * crossing_count. Returning invalid PlanarDiagram.");
    }
    
    return pd;
//    return pd.CreateCompressed();
}





//template<typename T = Int>
//T MergeExperimentalGaussCode( Int c, bool side, bool overQ ) const
//{
//    return (static_cast<T>(c) << 2) | (T(side) << 1) | T(overQ);
//}
//
//template<typename T = Int>
//std::tuple<Int,bool,bool>SplitExperimentalGaussCode( T code ) const
//{
//    const Int c = static_cast<Int>(code);
//
//    return std::tuple<Int,bool,bool>( (c >> 2), (c >> 1) & Int(1), c & Int(1) );
//}
//
//template<typename T = Int>
//Tensor1<T,Int> ExperimentalGaussCode()
//{
//    TOOLS_PTIC(ClassName()+"::ExperimentalGaussCode<" + TypeName<T> + ">" );
//
//    static_assert( IntQ<T>, "" );
//
//    if(
//       std::cmp_greater(
//            Int(4) * Ramp(crossing_count - Int(1)) + Int(3),
//            std::numeric_limits<T>::max()
//        )
//    )
//    {
//        eprint(ClassName() + "::ExperimentalGaussCode: Requested type T cannot store oriented Gauss code for this diagram.");
//
//        TOOLS_PTOC(ClassName()+"::ExperimentalGaussCode<" + TypeName<T> + ">" );
//
//        return Tensor1<T,Int>();
//    }
//
//    Tensor1<T,Int> gauss_code ( arc_count );
//
//    TraverseWithCrossings(
//        [&gauss_code,this](
//            const Int a,   const Int a_pos,
//            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
//            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
//        )
//        {
//            const T c_pos = static_cast<T>(c_0_pos);
//
//            const bool side = (C_arcs(c_0,Tail,Right) == a);
//
//            const bool overQ = (side == CrossingRightHandedQ(c_0) );
//
//            gauss_code[a] = MergeExperimentalGaussCode(c_pos,side,overQ);
//        }
//    );
//
//    TOOLS_PTOC(ClassName()+"::ExperimentalGaussCode<" + TypeName<T> + ">" );
//
//    return gauss_code;
//}






template<typename T = Int>
Tensor1<T,Int> ExtendedGaussCode2()
{
    TOOLS_PTIC(ClassName()+"::ExtendedGaussCode2<" + TypeName<T> + ">" );
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> gauss_code;
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::ExtendedGaussCode2: Trying to compute extended Gauss code of invalid PlanarDiagram. Returning empty vector.");
        
        goto Exit;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName() + "::ExtendedGaussCode2: Requested type " + TypeName<T> + " cannot store extended Gauss code for this diagram.");
    }
    
    gauss_code = Tensor1<T,Int>( arc_count );
    
    TraverseLinkComponentsWithCrossings(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        [&gauss_code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            // We need 1-based integers to be able to use signs.
            const T c_pos = static_cast<T>(c_0_pos) + T(1);
            
            gauss_code[a_pos] =
                c_0_visitedQ
                ? ( CrossingRightHandedQ(c_0) ? c_pos : -c_pos )
                : ( ArcOverQ<Tail>(a)         ? c_pos : -c_pos );
        },
        []( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
        }
    );
    
Exit:
    
    TOOLS_PTOC(ClassName()+"::ExtendedGaussCode2<" + TypeName<T> + ">" );
    
    return gauss_code;
}
