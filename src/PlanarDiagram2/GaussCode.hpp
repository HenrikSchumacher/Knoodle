public:

template<typename T = ToSigned<Int>>
Tensor1<T,Int> ExtendedGaussCode()  const
{
    TOOLS_PTIMER(timer,MethodName("ExtendedGaussCode")+"<"+TypeName<T>+">");
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> code;
    
    if( !ValidQ() )
    {
        wprint(MethodName("ExtendedGaussCode")+"<"+TypeName<T>+">: Trying to compute extended Gauss code of invalid planar diagram. Returning empty vector.");
        
        return code;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(MethodName("ExtendedGaussCode")+"<"+TypeName<T>+">: Requested type " + TypeName<T> + " cannot store extended Gauss code for this diagram.");
    }
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(MethodName("ExtendedGaussCode")+"<"+TypeName<T>+">: Not defined for links with multiple components.");
        
        return Tensor1<T,Int>();
    }
    
    code = Tensor1<T,Int>( arc_count );
    
    this->WriteExtendedGaussCode<T>(code.data());

    return code;
}


template<typename T>
void WriteExtendedGaussCode( mptr<T> gauss_code )  const
{
    TOOLS_PTIMER(timer,MethodName("WriteExtendedGaussCode")+"<"+TypeName<T>+">");
    
    static_assert( SignedIntQ<T>, "" );
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(MethodName("WriteExtendedGaussCode")+"<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
    }
    
    this->template Traverse<true>(
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
                ? ( ArcRightHandedQ(a,Tail) ? c_pos : -c_pos )
                : ( ArcRightOverQ(a,Tail)   ? c_pos : -c_pos );
        }
    );
}


template<typename T, typename ExtInt>
static PD_T FromExtendedGaussCode(
    cptr<T>      gauss_code,
    const ExtInt arc_count_,
    const bool   compressQ = true,
    const bool   proven_minimalQ_ = false
)
{
    static_assert( SignedIntQ<T>, "" );
    static_assert( SignedIntQ<ExtInt>, "" );
    
    TOOLS_PTIMER(timer,MethodName("FromExtendedGaussCode")+"<"+TypeName<T>+","+TypeName<ExtInt>+">");
    
    if( arc_count_ <= ExtInt(0) )
    {
        return Unknot(Int(0));
    }
    
    PD_T pd ( int_cast<Int>(arc_count_/2) );
    pd.proven_minimalQ = proven_minimalQ_;
    
    Int crossing_counter = 0;
    
    auto fun = [&gauss_code,&pd,&crossing_counter]
    ( const Int a_prev, const Int a ) -> int
    {
        const T g = gauss_code[a];
        if( g == T(0) )
        {
            eprint(MethodName("FromExtendedGaussCode")+"<"+TypeName<T>+","+TypeName<ExtInt>+">"+": Input code is invalid as it contains a crossing with label 0. Returning invalid planar diagram.");
            return 1;
        }
        
        const Int c = int_cast<Int>(Abs(g)) - Int(1);

        pd.A_cross(a_prev,Head) = c;
        pd.A_cross(a     ,Tail) = c;
        pd.A_state[a] = ArcState_T::Active;
        
        const bool visitedQ = pd.C_arcs(c,In,Left) != Uninitialized;
        
        if( !visitedQ )
        {
            pd.C_state[c] = (g > T(0)) ? CrossingState_T::RightHanded : CrossingState_T::LeftHanded;
            pd.C_arcs(c,Out,Left) = a;
            pd.C_arcs(c,In ,Left) = a_prev;
            ++crossing_counter;
            
            // We cannot assign states, yet because we do not know whether a and a_prev are already stored in the right slot!
        }
        else
        {
            const bool overQ = (g > T(0));
            
            const CrossingState_T c_state = pd.C_state[c];
            
            if( overQ == RightHandedQ(c_state) )
            {
                /*
                 * Situation in case of CrossingRightHandedQ(c) == overQ
                 *
                 *                 a                  a
                 *          ^     ^            ^     ^
                 *           \   /              \   /
                 *            \ /                \ /
                 *             / <--- c  or       \ <--- c
                 *            ^ ^                ^ ^
                 *           /   \              /   \
                 *          /     \            /     \
                 *       a_prev             a_prev
                 */
                
                pd.C_arcs(c,Out,Right) = a;
                pd.C_arcs(c,In ,Right) = pd.C_arcs(c,In ,Left );
                pd.C_arcs(c,In ,Left ) = a_prev;
            }
            else
            {
                /*
                 * Situation in case of CrossingRightHandedQ(c) != overQ
                 *
                 *         a                  a
                 *          ^     ^            ^     ^
                 *           \   /              \   /
                 *            \ /                \ /
                 *             / <--- c  or       \ <--- c
                 *            ^ ^                ^ ^
                 *           /   \              /   \
                 *          /     \            /     \
                 *               a_prev             a_prev
                 */
                
                pd.C_arcs(c,Out,Right) = pd.C_arcs(c,Out,Left );
                pd.C_arcs(c,Out,Left ) = a;
                pd.C_arcs(c,In ,Right) = a_prev;
            }
            
//            // This seems out of order, but pd.C_arcs(c,Out,Right) will typically be the arc stored right after pd.C_arcs(c,In ,Left ).
//            pd.A_state[pd.C_arcs(c,In ,Left )].Set(Head,Left ,c_state);
//            pd.A_state[pd.C_arcs(c,Out,Right)].Set(Tail,Right,c_state);
//            
//            // This seems out of order, but pd.C_arcs(c,Out,Left ) will typically be the arc stored right after pd.C_arcs(c,In ,Right).
//            pd.A_state[pd.C_arcs(c,In ,Right)].Set(Head,Right,c_state);
//            pd.A_state[pd.C_arcs(c,Out,Left )].Set(Tail,Left ,c_state);
        }
        
        return 0;
    };
    
    for( Int a = pd.max_arc_count; a --> Int(1); )
    {
        if( fun( a-Int(1), a ) )
        {
            return InvalidDiagram();
        }
    }
    
    if( fun( int_cast<Int>(arc_count_)-Int(1), Int(0) ) )
    {
        return InvalidDiagram();
    }
    
    pd.crossing_count = crossing_counter;
    pd.arc_count      = pd.CountActiveArcs();
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(MethodName("FromExtendedGaussCode")+"<"+TypeName<T>+","+TypeName<ExtInt>+">"+": Input Gauss code is invalid because arc_count != 2 * crossing_count. Returning invalid PlanarDiagram.");
    }
    
    // TODO: Computing the arc colors is actually redundant and diagrams from Gauss code can only be knots.
    
    // Compression is not really meaningful because the traversal ordering is crucial for the extended Gauss code.
    if( compressQ )
    {
        // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
        // This also applies the coloring to the arcs.
        return pd.template CreateCompressed<true>();
    }
    else
    {
        pd.ComputeArcColors();
        return pd;
    }
}
