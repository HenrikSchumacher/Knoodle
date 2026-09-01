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
 * object is returned.
 *
 * Lines with `#color` attribute must be preceded by a blank line.
 *
 * With no `#color` line anywhere, each component takes its own index as color.
 
 */
static LinkEmbedding_T FromInString( mref<Tools::InString> s, bool Sterbenz_shiftQ = true )
{
    std::vector<std::array<Real,3>> v_coords;
    std::vector<Size_T> component_ptr_agg;
    component_ptr_agg.push_back(Size_T{0});
    std::vector<Int> color_agg;
    
    auto lc = [&component_ptr_agg](){ return component_ptr_agg.size() - Size_T{1}; };
    
    bool color_declaredQ    = false; // Whether some #color attribute has beend found before.
    bool comp_wo_colorQ     = false; // Whether there is some component without #color.
    bool blank_may_followQ  = false; // Whether a blank line is allowed to come next.
    bool color_may_followQ  = true;  // Whether #color is allowed to come next.
    bool comp_needs_colorQ  = true;  // Whether component still needs a color assignment.
    bool coords_may_followQ = true;  // Whether we are ready to read coordinates.
    
    while( !s.EmptyQ() && !s.FailedQ() )
    {
        if( s.NewlineQ() )
        {
            if( !blank_may_followQ )
            {
                eprint(MethodName("FromInString") + ": Malformed blank line for link component no. " + ToString(lc()) + ". Returning invalid object.");
                return LinkEmbedding_T();
            }
            
            s.SkipNewline();
            component_ptr_agg.push_back(v_coords.size());
            
            blank_may_followQ  = false;
            color_may_followQ  = !comp_wo_colorQ;
            comp_needs_colorQ  = !color_declaredQ;
            coords_may_followQ = !color_declaredQ;
            
            continue;
        }

        if( s.CurrentChar() == '#' )
        {
            if( s.StartsWithQ("#color") )
            {
                // A '#color <int>' line declares the color of the component that
                // follows it.
                
                if( !color_may_followQ )
                {
                    if( comp_wo_colorQ )
                    {
                        eprint(MethodName("FromInString") + " at link component no. " + ToString(lc()) + ": Current component has a '#color' attribute but is preceded by components without one. That is illegal: either all components need to have a '#color' attribute or none. Returning invalid object.");
                    }
                    else
                    {
                        eprint(MethodName("FromInString") + ": At link component no. " + ToString(lc()) + ": '#color' attribute appears at illegal position. Returning invalid object.");
                    }
                    return LinkEmbedding_T();
                }
                
                s.Skip(Size_T{6}); // Skip the chars of string `"#color"`.
                // Deliberately skip only spaces and tabs, not using SkipWhiteSpace(): that
                // also eats newlines, which would silently swallow the component separator
                // if a '#color' line ever lacked its value.
                while( !s.EmptyQ()
                      && ((s.CurrentChar() ==' ') || (s.CurrentChar() == '\t')) )
                {
                    s.Skip(Size_T{1});
                }

                Int color = InvalidColor;
                s.Take(color);
 
                if( s.FailedQ() )
                {
                    eprint(MethodName("FromInString") + " at link component no. " + ToString(lc()) + ": Malformed '#color' line. Returning invalid object.");
                    return LinkEmbedding_T();
                }
                
                if( color == InvalidColor )
                {
                    eprint(MethodName("FromInString") + " at link component no. " + ToString(lc()) + ": Color is invalid. Returning invalid object.");
                    return LinkEmbedding_T();
                }

                color_agg.push_back(color);
                color_declaredQ    = true;
                
                blank_may_followQ  = false;
                color_may_followQ  = false;
                comp_needs_colorQ  = false;
                coords_may_followQ = true;
            }

            // Any other comment line does not change the state.

            // Skip whatever is left of the line, plus its newline. May also stop and buffer end without raising the failure flag.
            s.SkipLine();
            continue;
        }
        
        if( !coords_may_followQ )
        {
            eprint(MethodName("FromInString") + " at link component no. " + ToString(lc()) + ": Missing '#color' attribute. Returning invalid object.");
            return LinkEmbedding_T();
        }
        
        if( comp_needs_colorQ  )
        {
            if( color_declaredQ )
            {
                eprint(MethodName("FromInString") + " at link component no. " + ToString(lc()) + ": `comp_needs_colorQ` is `true` and `color_declaredQ` is `true`. (This should be impossible.)");
                return LinkEmbedding_T();
            }
            
            color_agg.push_back(static_cast<Int>(lc()));
            comp_wo_colorQ    = true;
            comp_needs_colorQ = false;
        }
        
        std::array<Real,3> x = {};
        s.Take(x[0]);
        s.SkipWhiteSpace(); // We are quite forgiving here.
        s.Take(x[1]);
        s.SkipWhiteSpace(); // We are quite forgiving here.
        s.Take(x[2]);
        v_coords.push_back(x);

        // Skip whatever is left of the line plus its newline character sequence (allowing, e.g., that user puts comments on the end of each line with vertex coordinates.
        // Stop also at buffer end without raising the failure flag.
        s.SkipLine();       // We are quite forgiving here.

        blank_may_followQ  = true;
        color_may_followQ  = false;
        
        assert(!comp_needs_colorQ);
        assert(coords_may_followQ);
    }

    if( s.FailedQ() )
    {
        eprint(MethodName("FromInString") + ": Reading file failed. Returning invalid object.");
        return LinkEmbedding_T();
    }

    if( blank_may_followQ )
    {
        // Close the final component, unless a trailing blank line already did.
        component_ptr_agg.push_back(v_coords.size());
    }
    else
    {
        // We have trailing blank line or a trailing `#color` statement here.
        // We are happy to forgive the trailing blank line.
        // Since that did not raise an error, we must have `color_declaredQ == true`.
        // Then we must have `coords_may_followQ == true`.
        
        if( coords_may_followQ )
        {
            eprint(MethodName("FromInString") + " at link component no. " + ToString(lc()) + ": Trailing '#color' attribute. Returning invalid object.");
            return LinkEmbedding_T();
        }
    }

    // This is really more like an assert than a real check.
    if( component_ptr_agg.size() != color_agg.size() + 1 )
    {
        eprint(MethodName("FromInString") + ": component_ptr_agg.size() != color_agg.size() + 1. Something during parsing must have gone wrong. Returning invalid object.");
        TOOLS_DUMP(component_ptr_agg.size());
        TOOLS_DUMP(color_agg.size());
        return LinkEmbedding_T();
    }
    
    LinkEmbedding_T link (
        Tensor1<Int,Int>( &component_ptr_agg[0], int_cast<Int>(component_ptr_agg.size())),
        Tensor1<Int,Int>( &color_agg[0]  , int_cast<Int>(color_agg.size()))
    );
    
    if( Sterbenz_shiftQ )
    {
        link.template ReadVertexCoordinates<false,true>(&v_coords[0][0]);
    }
    else
    {
        link.template ReadVertexCoordinates<false,false>(&v_coords[0][0]);
    }
    
    return link;
}
