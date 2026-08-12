public:

static LinkEmbedding_T FromFile(
    cref<std::filesystem::path> file, bool Sterbenz_shiftQ = true
)
{
    Tools::InString s (file);

    return FromInString(s, Sterbenz_shiftQ);
}

/*!@brief Read an embedding from `InString` `s`: one `x y z` line per vertex, components
 * separated by blank lines, each component optionally preceded by a
 * `#color <int>` line.
 *
 * The `#color` attribute has to be given for every link component or for none.
 * A file that tags only some components is rejected as malformed and an invalid
 * object is returned, because an untagged component takes its index as its color
 * and that can silently collide with a color declared for another component.
 *
 * With no `#color` line anywhere, each component takes its own index as color,
 * which is the historical behavior and keeps KnotPlot output readable.
 */
static LinkEmbedding_T FromInString( mref<Tools::InString> s, bool Sterbenz_shiftQ = true )
{
    Int vertex_counter = 0;
    std::vector<Real> v_coords;
    std::vector<Int> component_ptr_agg;
    component_ptr_agg.push_back(Int(0));

    // color_agg[lc] is the color declared for component lc, or < 0 if it had no
    // `#color` header. Kept sparse: only grown when a header is actually seen.
    std::vector<Int> color_agg;

    while( !s.EmptyQ() && !s.FailedQ() )
    {
        if( s.NewlineQ() )
        {
            s.SkipNewline();

            // A blank line ends the current component -- but only if that
            // component actually collected vertices. Pushing unconditionally
            // manufactures empty phantom components from a leading blank line,
            // a trailing one, or two blank lines in a row, none of which the
            // user can see in the file.
            if( vertex_counter > component_ptr_agg.back() )
            {
                component_ptr_agg.push_back(vertex_counter);
            }
            continue;
        }

        if( s.CurrentChar() == '#' )
        {
            // A '#color <int>' line declares the color of the component that
            // follows it.
            
            // `component_ptr_agg` has one entry per component started  so far plus one for the leading `0`. So this is the number of the current component.
            const Size_T lc = component_ptr_agg.size() - Size_T(1);

            if( s.StartsWithQ("#color") )
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
                    eprint(MethodName("FromInString") + ": Malformed '#color' line for link component no. " + ToString(lc) + ". Returning invalid object.");
                    return LinkEmbedding_T();
                }
                
                if( color == InvalidColor )
                {
                    eprint(MethodName("FromInString") + ": Invalid color for link component no. " + ToString(lc) + ". Returning invalid object.");
                    return LinkEmbedding_T();
                }
                
                if( color_agg.size() <= lc )
                {
                    // std::vector::resize only reallocates if lc + Size_T(1) exceeds capacity.
                    // So the if-guard should not be absolutely nececessary.
                    
                    // What is an invalid color is dictated by the class `PlanarDiagram<Int>`.
                    color_agg.resize(lc + Size_T(1),InvalidColor);
                }

                color_agg[lc] = color;
            }

            // Skip whatever is left of the line, plus its newline. May also stop and buffer end without raising the failure flag.
            s.SkipLine();

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
        ++vertex_counter;

        // Skip whatever is left of the line plus its newline character sequence (allowing, e.g., that user put comments on the end of each line with vertex coordinates.
        // Stop also at buffer end without raising the failure flag.
        s.SkipLine();
    }

    if( s.FailedQ() )
    {
        eprint(MethodName("ReadFromFile") + ": Reading file failed. Returning invalid object.");
        return LinkEmbedding_T();
    }

    // Close the final component, unless a trailing blank line already did.
    if( vertex_counter > component_ptr_agg.back() )
    {
        component_ptr_agg.push_back(vertex_counter);
    }

    const Size_T comp_count = component_ptr_agg.size() - Size_T(1);

    // The '#color' attribute has to appear for every link component or for none;
    // anything in between is rejected as malformed.
    //
    // The reason is that a component without the attribute falls back to its own
    // index, and that fallback can silently collide with a color the user
    // declared for a different component: an untagged component 0 next to a
    // '#color 0' component 1 yields the colors `0 0`. Colors are not decoration
    // here -- `PlanarDiagramComplex` reads them as component identity -- so a
    // collision the user never asked for is a foot-gun, and there is no reading
    // of a mixed file that is obviously the intended one. Refusing beats
    // guessing.
    //
    // What counts as an invalid color is dictated by `PlanarDiagram<Int>`. Note
    // a `color_agg[lc] >= Int(0)` test would be meaningless when `Int` is
    // unsigned, hence the comparison against `InvalidColor`.
    Size_T declared_count = 0;

    for( Size_T lc = 0; lc < comp_count; ++lc )
    {
        if( (lc < color_agg.size()) && (color_agg[lc] != InvalidColor) )
        {
            ++declared_count;
        }
    }

    if( (declared_count != Size_T(0)) && (declared_count != comp_count) )
    {
        eprint(MethodName("FromInString") + ": The '#color' attribute is present for " + ToString(declared_count) + " of " + ToString(comp_count) + " link components. It has to be present for all of them or for none, because an untagged component takes its index as color and that can collide with a declared one. Returning invalid object.");
        return LinkEmbedding_T();
    }

    Tensor1<Int,Int> component_color ( int_cast<Int>(comp_count) );

    // Either every component declared a color or none did. When none did, each
    // component takes its own index: that is the historical behavior, it is what
    // a user expects, and it keeps the format readable for KnotPlot output.
    const bool colorsDeclaredQ = (declared_count > Size_T(0));

    for( Size_T lc = 0; lc < comp_count; ++lc )
    {
        component_color[int_cast<Int>(lc)] =
            colorsDeclaredQ ? color_agg[lc] : int_cast<Int>(lc);
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
