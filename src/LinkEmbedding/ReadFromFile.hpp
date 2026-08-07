public:

static LinkEmbedding_T ReadFromFile(
    cref<std::filesystem::path> file, bool Sterbenz_shiftQ = true
)
{
    Tools::InString s (file);

    return FromInString(s, Sterbenz_shiftQ);
}

// TODO: Once we have agreed on a good default behavior for a missing #color attribute, we need to document it.

/*!@brief Read an embedding from `InString` `s`: one `x y z` line per vertex, components
 * separated by blank lines, each component optionally preceded by a
 * `#color <int>` line.
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

    Tensor1<Int,Int> component_color ( int_cast<Int>(comp_count) );

    for( Size_T lc = 0; lc < comp_count; ++lc )
    {
        // The check color_agg[lc] >= Int(0) does not work if `Int` is an unsigned type.
        // const bool declaredQ = (lc < color_agg.size()) && (color_agg[lc] >= Int(0));

        // What is an invalid color is dictated by the class `PlanarDiagram<Int>`.
        const bool declaredQ = (lc < color_agg.size()) && (color_agg[lc] != InvalidColor);
        
        // I don't know whether I like this default behavior.
        // If the #color attribute is present for every link component, then this will simply read in the colors. Fine.
        // If no #color keywords have appeared, then a new color is assigned to each link component,starting from `0` and going up to `comp_count - 1`. That is the old behavior; it is meaningful and it reflects best what the user expects. Plus it this should make this import format compatible with KnotPlot's outputs. (I am not 100% sure about floating-point numbers in scientific form.)
        // But if the #color attribute misses only for _some_ link components and if it is specified for others, then just using the number of the link component as color is dangerous: this color may coincide with some of the user-defined colors. This forces `PlanarDiagramComplex` to interpret these two components as a connected sum. So this behavior might affect the topology of the resulting link in an unintended/unexpected way. For example, the user might expect that the present specified color is used until it is changed.
        // I don't know yet, what exact default behavior we should use here. I think it is important to have a simple rule that meets the users expectations. This must not be a foot gun. This is part of the reason why I had not implemented the color import, yet: I simply was not sure what exactly to do here.
        // Thinking of this, the best action might be to require that the #color attribute appears either for all link components or for none.
        
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
