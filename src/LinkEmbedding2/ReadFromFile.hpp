public:

// TODO: Check and read color.
static LinkEmbedding_T ReadFromFile( cref<std::filesystem::path> file )
{
    Tools::InString s (file);
    
    return FromInString(s);
}

// TODO: Check and read color.
static LinkEmbedding_T FromInString( mref<Tools::InString> s )
{
    Int counter = 0;
    std::vector<Real> v_coords;
    std::vector<Int> component_ptr_agg;
    component_ptr_agg.push_back(Int(0));
    
    while( !s.EmptyQ() && !s.FailedQ() )
    {
        if( s.CurrentChar() == '\n' )
        {
            s.Skip(1);
            component_ptr_agg.push_back(counter);
            continue;
        }
        
        Real x = 0;
        s.Take(x);
        v_coords.push_back(x);
        s.SkipWhiteSpace();
        s.Take(x);
        v_coords.push_back(x);
        s.SkipWhiteSpace();
        s.Take(x);
        v_coords.push_back(x);
        ++counter;
        
        // We have to be careful here, because the last line may easily end with an '\n'.
        if( s.EmptyQ() ) { break; }
        
        s.SkipChar('\n');
    }
    
    if( s.FailedQ() )
    {
        eprint(MethodName("ReadFromFile") + ": Reading file failed. Returning invalid object.");
        return LinkEmbedding_T();
    }
    
    component_ptr_agg.push_back(counter);
    
    LinkEmbedding_T link (
        Tensor1<Int,Int>( &component_ptr_agg[0], int_cast<Int>(component_ptr_agg.size())),
        iota<Int,Int>(component_ptr_agg.size()-Size_T(1))
    );
    
    link.template ReadVertexCoordinates<false>(&v_coords[0]);
    
    return link;
}
