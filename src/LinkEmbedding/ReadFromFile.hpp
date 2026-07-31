public:

static LinkEmbedding_T ReadFromFile(
    cref<std::filesystem::path> file, bool Sterbenz_shiftQ = true
)
{
    Tools::InString s (file);

    return FromInString(s, Sterbenz_shiftQ);
}

/*!@brief Read an embedding from `s`: one `x y z` line per vertex, components
 * separated by blank lines, each component optionally preceded by a
 * `#color <int>` line.
 *
 * The `#color` header is what `WriteToFile( ..., colorQ = true )` emits. Reading
 * it back here is what makes that writer's output round-trip; before, any file
 * written with `colorQ = true` was rejected by this parser, because the leading
 * '#' is not the start of a `Real`. Lines beginning with '#' that are not
 * `#color` are treated as comments and skipped, so hand-annotated files stay
 * readable. A component with no `#color` header keeps the old default: its own
 * index, i.e. the `iota` colors this routine used to hand out unconditionally.
 */
static LinkEmbedding_T FromInString( mref<Tools::InString> s, bool Sterbenz_shiftQ = true )
{
    Int counter = 0;
    std::vector<Real> v_coords;
    std::vector<Int> component_ptr_agg;
    component_ptr_agg.push_back(Int(0));

    // color_agg[lc] is the color declared for component lc, or < 0 if it had no
    // `#color` header. Kept sparse: only grown when a header is actually seen.
    std::vector<Int> color_agg;

    while( !s.EmptyQ() && !s.FailedQ() )
    {
        if( s.CurrentChar() == '\n' )
        {
            s.Skip(1);
            component_ptr_agg.push_back(counter);
            continue;
        }

        if( s.CurrentChar() == '#' )
        {
            // A '#color <int>' line declares the color of the component that
            // follows it; component_ptr_agg has one entry per component started
            // so far, so its last index is the component about to be read.
            const std::string_view line = s.View();

            if( line.starts_with("#color") )
            {
                s.Skip(Size_T(6));

                // Only spaces/tabs, deliberately not SkipWhiteSpace(): that also
                // eats newlines, which would silently swallow the component
                // separator if a '#color' line ever lacked its value.
                while( !s.EmptyQ()
                      && ((s.CurrentChar() == ' ') || (s.CurrentChar() == '\t')) )
                {
                    s.Skip(Size_T(1));
                }

                Int color = 0;
                s.Take(color);

                if( s.FailedQ() )
                {
                    eprint(MethodName("FromInString") + ": Malformed '#color' line. Returning invalid object.");
                    return LinkEmbedding_T();
                }

                const Size_T lc = component_ptr_agg.size() - Size_T(1);

                if( color_agg.size() <= lc ) { color_agg.resize(lc + Size_T(1),Int(-1)); }

                color_agg[lc] = color;
            }

            // Skip whatever is left of the line, plus its newline.
            while( !s.EmptyQ() && (s.CurrentChar() != '\n') ) { s.Skip(Size_T(1)); }
            if( !s.EmptyQ() ) { s.Skip(Size_T(1)); }

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

    const Size_T comp_count = component_ptr_agg.size() - Size_T(1);

    Tensor1<Int,Int> component_color ( int_cast<Int>(comp_count) );

    for( Size_T lc = 0; lc < comp_count; ++lc )
    {
        const bool declaredQ = (lc < color_agg.size()) && (color_agg[lc] >= Int(0));

        component_color[int_cast<Int>(lc)] =
            declaredQ ? color_agg[lc] : int_cast<Int>(lc);
    }

    LinkEmbedding_T link (
        Tensor1<Int,Int>( &component_ptr_agg[0], int_cast<Int>(component_ptr_agg.size())),
        std::move(component_color)
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

//// TODO: Check and read color.
//static LinkEmbedding_T ReadFromFile2(
//    cref<std::filesystem::path> file, bool Sterbenz_shiftQ = true
//)
//{
//    std::ifstream stream ( file );
//    
//    if( !stream )
//    {
//        eprint(MethodName("ReadFromFile") + ": Opening file " + file.string() + " failed. Returning invalid object.");
//        return LinkEmbedding_T();
//    }
//    
//    Int counter = 0;
//    std::vector<Real> v_coordinates;
//    std::vector<Int> component_ptr_agg;
//    component_ptr_agg.push_back(Int(0));
//    
//    std::string line;
//    std::string token;
//    
//    bool failedQ = false;
//    
//    while( std::getline(stream,line) )
//    {
//        if( line.size() == Size_T(0) )
//        {
//            component_ptr_agg.push_back(counter);
//            continue;
//        }
//        
//        std::stringstream s (line);
//
//        // TODO: Use std::from_chars here
//        if( !(s >> token) ) { failedQ = true; break; }
//        v_coordinates.push_back(std::stod(token));
//        if( !(s >> std::ws) ) { failedQ = true; break; }
//        
//        if( !(s >> token) ) { failedQ = true; break; }
//        v_coordinates.push_back(std::stod(token));
//        if( !(s >> std::ws) ) { failedQ = true; break; }
//        
//        if( !(s >> token) ) { failedQ = true; break; }
//        v_coordinates.push_back(std::stod(token));
//        
//        ++counter;
//    }
//    
//    if( failedQ )
//    {
//        eprint(MethodName("ReadFromFile") + ": Reading file failed. Returning invalid object.");
//        return LinkEmbedding_T();
//    }
//    
//    component_ptr_agg.push_back(counter);
//    
//    LinkEmbedding_T link (
//        Tensor1<Int,Int>( &component_ptr_agg[0], int_cast<Int>(component_ptr_agg.size())),
//        iota<Int,Int>(component_ptr_agg.size()-Size_T(1))
//    );
//    
//    if( Sterbenz_shiftQ )
//    {
//        link.template ReadVertexCoordinates<false,true>(&v_coordinates[0]);
//    }
//    else
//    {
//        link.template ReadVertexCoordinates<false,false>(&v_coordinates[0]);
//    }
//    
//    return link;
//}
