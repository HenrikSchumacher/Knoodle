public:

template<typename T = ToSigned<Int>>
Tensor1<T,Int> ExtendedGaussCode2( const Int a_0 = 0 )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::ExtendedGaussCode2<"+TypeName<T>+">");
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor1<T,Int> code;
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::ExtendedGaussCode2<"+TypeName<T>+">: Trying to compute extended Gauss code of invalid PlanarDiagram. Returning empty vector.");
        
        return code;
    }
    
    if( std::cmp_greater( crossing_count + Int(1), std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::ExtendedGaussCode<"+TypeName<T>+">: Requested type " + TypeName<T> + " cannot store extended Gauss code for this diagram.");
    }
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::ExtendedGaussCode2<"+TypeName<T>+">: Not defined for links with multiple components.");
        
        return Tensor1<T,Int>();
    }
    
    code = Tensor1<T,Int>( arc_count );
    
    this->WriteExtendedGaussCode2<T>(a_0,code.data());

    return code;
}

template<typename T>
void WriteExtendedGaussCode2( const Int a_0, mptr<T> code )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::WriteExtendedGaussCode2<"+TypeName<T>+">");
    
    static_assert( SignedIntQ<T>, "" );
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::WriteExtendedGaussCode2<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
    }
    
    if( !ArcActiveQ(a_0) )
    {
        wprint(ClassName()+"::WriteExtendedGaussCode2<"+TypeName<T>+">: Arc "+ToString(a_0)+" is not active. Aborting.");
    }
    
    C_scratch.Fill(Uninitialized);
    
    mptr<Int>  C_pos = C_scratch.data();
    mptr<bool> A_visitedQ = reinterpret_cast<bool *>(A_scratch.data());
    fill_buffer(A_visitedQ,false,max_arc_count);
    
    Int c_counter = 0;
    Int a_counter = 0;
    
    TraverseComponent(
        a_0,
        [&code,&c_counter,&a_counter,A_visitedQ,C_pos,this]
        ( const Int a )
        {
            const Int c_0 = this->A_cross(a,Tail);
            
            Int c_0_pos = C_pos[c_0];
            
            bool c_0_visitedQ = ( c_0_pos != Uninitialized );
            
            if( !c_0_visitedQ )
            {
                c_0_pos = C_pos[c_0] = c_counter;
                ++c_counter;
            }

            // We need 1-based integers to be able to use signs.
            const T c_pos = static_cast<T>(c_0_pos) + T(1);
            
            code[a_counter] =
                c_0_visitedQ
                ? ( CrossingRightHandedQ(c_0) ? c_pos : -c_pos )
                : ( ArcOverQ(a,Tail)          ? c_pos : -c_pos );
            
            A_visitedQ[a] = true;
            ++a_counter;
        }
    );
}
