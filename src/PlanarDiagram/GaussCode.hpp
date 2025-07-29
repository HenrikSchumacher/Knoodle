public:

template<typename T = ToSigned<Int>>
Tensor1<T,Int> ExtendedGaussCode()  const
{
    TOOLS_PTIMER(timer,ClassName()+"::ExtendedGaussCode<"+TypeName<T>+">");
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> code;
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::ExtendedGaussCode<"+TypeName<T>+">: Trying to compute extended Gauss code of invalid PlanarDiagram. Returning empty vector.");
        
        return code;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::ExtendedGaussCode<"+TypeName<T>+">: Requested type " + TypeName<T> + " cannot store extended Gauss code for this diagram.");
    }
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::ExtendedGaussCode<"+TypeName<T>+">: Not defined for links with multiple components.");
        
        return Tensor1<T,Int>();
    }
    
    code = Tensor1<T,Int>( arc_count );
    
    this->WriteExtendedGaussCode<T>(code.data());

    return code;
}


template<typename T>
void WriteExtendedGaussCode( mptr<T> gauss_code )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::WriteExtendedGaussCode<"+TypeName<T>+">");
    
    static_assert( SignedIntQ<T>, "" );
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::WriteExtendedGaussCode<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
    }
    
    this->template Traverse<true,false,0>(
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
        }
    );
}


template<typename T, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromExtendedGaussCode(
    cptr<T>       gauss_code,
    const ExtInt2 arc_count_,
    const ExtInt3 unlink_count_,
    const bool    compressQ = true,
    const bool    proven_minimalQ_ = false
)
{
    static_assert( SignedIntQ<T>, "" );
    
    if( arc_count_ <= ExtInt2(0) )
    {
        PlanarDiagram<Int> pd ( Int(0) , int_cast<Int>(unlink_count_) );
        pd.proven_minimalQ = true;
        return pd;
    }
    
    PlanarDiagram<Int> pd (
        int_cast<Int>(arc_count_/2),int_cast<Int>(unlink_count_)
    );

    pd.proven_minimalQ = proven_minimalQ_;
    
    Int crossing_counter = 0;
    
    auto fun = [&gauss_code,&pd,&crossing_counter](const Int a_prev,const Int a)
    {
        const T g = gauss_code[a];
        if( g == T(0) )
        {
            eprint(ClassName()+"::FromExtendedGaussCode: Input code is invalid as it contains a crossing with label 0. Returning invalid PlanarDiagram.");
            
            return 1;
        }
        
        const Int c = Abs(int_cast<Int>(g)) - Int(1);

        pd.A_cross(a_prev,Head) = c;
        pd.A_cross(a     ,Tail) = c;
        // TODO: Handle over/under in ArcState.
        pd.A_state[a] = ArcState::Active;
        
        const bool visitedQ = pd.C_arcs(c,In,Left) != Uninitialized;
        
        if( !visitedQ )
        {
            pd.C_state[c] = (g > T(0))
                          ? CrossingState::RightHanded
                          : CrossingState::LeftHanded;
            pd.C_arcs(c,Out,Left) = a;
            pd.C_arcs(c,In ,Left) = a_prev;
            ++crossing_counter;
        }
        else
        {
            const bool overQ = (g > T(0));
            
            if( overQ == pd.CrossingRightHandedQ(c) )
            {
                /*
                 * Situation in case of CrossingRightHandedQ(c) = overQ
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
        }
        
        return 0;
    };
    
    for( Int a = pd.max_arc_count; a --> Int(1); )
    {
        if( fun( a-Int(1), a ) )
        {
            return PlanarDiagram();
        }
    }
    
    if( fun( int_cast<Int>(arc_count_)-Int(1), Int(0) ) )
    {
        return PlanarDiagram();
    }
    
    pd.crossing_count = crossing_counter;
    pd.arc_count      = int_cast<Int>(arc_count_);  
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(ClassName() + "FromPDCode: Input PD code is invalid because arc_count != 2 * crossing_count. Returning invalid PlanarDiagram.");
    }
    
    // Compression is not really meaningful because the traversal ordering is crucial for the extended Gauss code.
    if( compressQ )
    {
        // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
        return pd.CreateCompressed();
    }
    else
    {
        return pd;
    }
}


template<typename T = ToSigned<Int>>
Tensor1<T,Int> ExtendedGaussCodeByLinkTraversal()  const
{
    TOOLS_PTIMER(timer,ClassName()+"::ExtendedGaussCodeByLinkTraversal<" + TypeName<T> + ">" );
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> gauss_code;
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::ExtendedGaussCodeByLinkTraversal: Trying to compute extended Gauss code of invalid PlanarDiagram. Returning empty vector.");
        
        return gauss_code;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::ExtendedGaussCodeByLinkTraversal: Requested type " + TypeName<T> + " cannot store extended Gauss code for this diagram.");
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
    
    return gauss_code;
}
