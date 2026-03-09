public:


// TODO: Check and read color.
static LinkEmbedding_T ReadFromFile(
    cref<std::filesystem::path> file, bool Sterbenz_shiftQ = true
)
{
    std::ifstream stream (file, std::ios::in | std::ios::binary);
    if( !stream )
    {
        eprint(MethodName("ReadFromFile") + ": Opening file " + file.string() + " failed. Returning invalid object.");
        return LinkEmbedding_T();
    }
    
    // Obtain the size of the file.
    const auto file_size = std::filesystem::file_size(file);
    // Create a buffer.
    std::string input (file_size, '\0');
    // Read the whole file into the buffer.
    stream.read(input.data(), static_cast<std::streamsize>(file_size));

    Tools::InString s (input);

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
        
        // We have to be careful here, because the last line may easily and with an '\n'.
        if( s.EmptyQ() )
        {
            break;
        }
        {
            s.SkipChar('\n');
        }
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
    
    if( Sterbenz_shiftQ )
    {
        link.template ReadVertexCoordinates<false,true>(&v_coords[0]);
    }
    else
    {
        link.template ReadVertexCoordinates<false,false>(&v_coords[0]);
    }
    
    return link;
}


// TODO: Check and read color.
static LinkEmbedding_T ReadFromFile2(
    cref<std::filesystem::path> file, bool Sterbenz_shiftQ = true
)
{
    std::ifstream stream ( file );
    
    if( !stream )
    {
        eprint(MethodName("ReadFromFile") + ": Opening file " + file.string() + " failed. Returning invalid object.");
        return LinkEmbedding_T();
    }
    
    Int counter = 0;
    std::vector<Real> v_coordinates;
    std::vector<Int> component_ptr_agg;
    component_ptr_agg.push_back(Int(0));
    
    std::string line;
    std::string token;
    
    bool failedQ = false;
    
    while( std::getline(stream,line) )
    {
        if( line.size() == Size_T(0) )
        {
            component_ptr_agg.push_back(counter);
            continue;
        }
        
        std::stringstream s (line);

        // TODO: Use std::from_chars here
        if( !(s >> token) ) { failedQ = true; break; }
        v_coordinates.push_back(std::stod(token));
        if( !(s >> std::ws) ) { failedQ = true; break; }
        
        if( !(s >> token) ) { failedQ = true; break; }
        v_coordinates.push_back(std::stod(token));
        if( !(s >> std::ws) ) { failedQ = true; break; }
        
        if( !(s >> token) ) { failedQ = true; break; }
        v_coordinates.push_back(std::stod(token));
        
        ++counter;
    }
    
    if( failedQ )
    {
        eprint(MethodName("ReadFromFile") + ": Reading file failed. Returning invalid object.");
        return LinkEmbedding_T();
    }
    
    component_ptr_agg.push_back(counter);
    
    LinkEmbedding_T link (
        Tensor1<Int,Int>( &component_ptr_agg[0], int_cast<Int>(component_ptr_agg.size())),
        iota<Int,Int>(component_ptr_agg.size()-Size_T(1))
    );
    
    if( Sterbenz_shiftQ )
    {
        link.template ReadVertexCoordinates<false,true>(&v_coordinates[0]);
    }
    else
    {
        link.template ReadVertexCoordinates<false,false>(&v_coordinates[0]);
    }
    
    return link;
}
