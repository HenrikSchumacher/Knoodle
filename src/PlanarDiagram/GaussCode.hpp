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
    
    Traverse(
        []( const Int lc, const Int lc_begin ){},
        [&gauss_code,this](
            const Int a,   const Int a_idx,
            const Int c_0, const Int c_0_label, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_label, const bool c_1_visitedQ
        )
        {
            T c_label = static_cast<T>(c_0_label) + T(1);

            gauss_code[a_idx] = ( ArcOverQ<Tail>(a) ? c_label : -c_label );
        },
        []( const Int lc, const Int lc_begin, const Int lc_end ){}
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
    
    Traverse(
        []( const Int lc, const Int lc_begin ){},
        [&gauss_code,this](
            const Int a,   const Int a_label,
            const Int c_0, const Int c_0_label, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_label, const bool c_1_visitedQ
        )
        {
            // We need 1-based integers to be able to use signs.
            const T c_label = static_cast<T>(c_0_label) + T(1);
            
            gauss_code[a_label] =
                c_0_visitedQ
                ? ( CrossingRightHandedQ(c_0) ? c_label : -c_label )
                : ( ArcOverQ<Tail>(a)         ? c_label : -c_label );
        },
        []( const Int lc, const Int lc_begin, const Int lc_end ){}
    );
    
Exit:
    
    TOOLS_PTOC(ClassName()+"::ExtendedGaussCode<" + TypeName<T> + ">" );
    
    return gauss_code;
}

// TODO: This one is incomplete.
//template<typename T, typename ExtInt2, typename ExtInt3>
//static PlanarDiagram<Int> FromExtendedGaussCode(
//    cptr<T>       gauss_code,
//    const ExtInt2 crossing_count_,
//    const ExtInt3 unlink_count_,
//    const bool    provably_minimalQ_ = false
//)
//{
//    PlanarDiagram<Int> pd (int_cast<Int>(crossing_count_),int_cast<Int>(unlink_count_));
//    
//    pd.provably_minimalQ = provably_minimalQ_;
//    
//    
//    static_assert( SignedIntQ<T>, "" );
//    
//    if( crossing_count_ <= 0 )
//    {
//        return pd;
//    }
//    
//    Int c = Abs(int_cast<Int>(gauss_code[0])) - Int(1);
//    
//    for( Int a = pd.max_arc_count; a --> Int(0); )
//    {
//        
//        const T code = gauss_code[a];
//        
//        if( code == T(0) )
//        {
//            eprint(ClassName() + "::FromExtendedGaussCode: Input code is invalid as it contains a crossing with label 0 detected. Returnin invalid PlanarDiagram.");
//            
//            return PlanarDiagram<Int>();
//        }
//        const Int c_next = c;
//        c = Abs(int_cast<Int>(code)) - Int(1);
//        
//        pd.A_cross(a,Head) = c_next;
//        pd.A_cross(a,Tail) = c;
//        pd.A_state[a] = ArcState::Active;
//        
//        // Handle a as outgoing arc.
//        {
//            const bool visitedQ = pd.A_cross(c,Out,Left) != Int(-1);
//            
//            if( !visitedQ )
//            {
//                pd.C_state[c] = (code > T(0) ? CrossingState::RightHanded : CrossingState::LeftHanded );
//                pd.C_arcs(c,Out,Left) = a;
//            }
//            else
//            {
//                
//                const bool overQ = code > T(0);
//                
//                /*
//                 * Situation in case of RightHandedQ(c) and overQ
//                 *
//                 *        a_0
//                 *          ^     ^
//                 *           \   /
//                 *            \ /
//                 *             / <--- c
//                 *            ^ ^
//                 *           /   \
//                 *          /     \
//                 */
//                
//                if( overQ == RightHandedQ(c) )
//                {
//                    pd.C_arcs(c,Out,Right) = a;
//                }
//                else
//                {
//                    pd.C_arcs(c,Out,Right) = pd.C_arcs(c,Out,Left);
//                    pd.C_arcs(c,Out,Left)  = a;
//                }
//            }
//        }
//        
//        // Handle a as ingoing arc.
//        {
//            const bool visitedQ = pd.A_cross(c_next,In,Left) != Int(-1);
//            
//            if( !visitedQ )
//            {
//                pd.C_state[c_next] = code > T(0)
//                                   ? CrossingState::RightHanded
//                                   : CrossingState::LeftHanded;
//                pd.C_arcs(c_next,Out,Left) = a;
//            }
//            else
//            {
//                
//                const bool overQ = code > T(0);
//                
//                /*
//                 * Situation in case of RightHandedQ(c) and !overQ
//                 *
//                 *          ^     ^
//                 *           \   /
//                 *            \ /
//                 *             / <--- c_next
//                 *            ^ ^
//                 *           /   \
//                 *          /     \
//                 *        a_0
//                 */
//                
//                if( overQ != RightHandedQ(c) )
//                {
//                    pd.C_arcs(c_next,In,Right) = a;
//                }
//                else
//                {
//                    pd.C_arcs(c_next,In,Right) = pd.C_arcs(c_next,In,Left);
//                    pd.C_arcs(c_next,In,Left)  = a;
//                }
//            }
//        }
//    }
//    
//    pd.crossing_count = pd.CountActiveCrossings();
//    pd.arc_count      = pd.CountActiveArcs();
//    
//    if( pd.arc_count != Int(2) * pd.crossing_count )
//    {
//        eprint(ClassName() + "FromPDCode: Input PD code is invalid because crossing_count != 2 * arc_count. Returning invalid PlanarDiagram.");
//    }
//    
//    return pd.CreateCompressed();
//}



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
//    Traverse(
//        [&gauss_code,this](
//            const Int a,   const Int a_idx,
//            const Int c_0, const Int c_0_label, const bool c_0_visitedQ,
//            const Int c_1, const Int c_1_label, const bool c_1_visitedQ
//        )
//        {
//            const T c_label = static_cast<T>(c_0_label);
//
//            const bool side = (C_arcs(c_0,Tail,Right) == a);
//
//            const bool overQ = (side == CrossingRightHandedQ(c_0) );
//
//            gauss_code[a] = MergeExperimentalGaussCode(c_label,side,overQ);
//        }
//    );
//
//    TOOLS_PTOC(ClassName()+"::ExperimentalGaussCode<" + TypeName<T> + ">" );
//
//    return gauss_code;
//}
