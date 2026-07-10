public:

static PDC_T ReadFromFile( cref<std::filesystem::path> file )
{
    Tools::InString s (file);
    
    return FromInString(s);
}

// This requires 7-number pd codes (i.e., crossing sign and colors for the two strands at each crossing need to be given, too).
// This might look inflexible, but it is too easy to shoot oneself into the foot if we allow also shorter codes here.
static PDC_T FromInString( mref<Tools::InString> s )
{
    PDC_T pdc;
    
    // Currently, we have only two parsing modes here. But we might need more if we want to use abbreviations for certain subdiagrams or if we want allow also MacLeod codes for knot summands.
    enum struct ParsingMode_T : UInt8
    {
        None       = 0,
        Subdiagram = 1
    };
    
    Int crossing_counter = 0;
    std::vector<Int> pd_buffer;
    UInt8 proven_minimalQ = 0;
    Int last_color_deactivated = PD_T::Uninitialized;
    Int x = 0;
    Size_T input_diagram_counter = 0;
    ParsingMode_T mode = ParsingMode_T::None;
    bool failedQ = false;
    
    
    auto clear = [&crossing_counter,&proven_minimalQ,&last_color_deactivated,&pd_buffer,&input_diagram_counter,&mode]()
    {
        // Clean the slate.
        crossing_counter = 0;
        pd_buffer.clear();
        proven_minimalQ = 0;
        last_color_deactivated = PD_T::InvalidColor;
        ++input_diagram_counter;
        mode = ParsingMode_T::None;
    };
    
    auto push_diagram = [&crossing_counter,&proven_minimalQ,&pd_buffer,&pdc,&input_diagram_counter,&mode,clear]()
    {
        // Only push if a subdiagram has already been started.
        // For example, if the first subdiagram stored in a file is an unlink, then this prevents pushing two unlinks.
        if( mode != ParsingMode_T::Subdiagram ) { return; }
        
        PD_T pd = PD_T::template FromPDCode<{.signQ = true, .colorQ = true}>(
            &pd_buffer[0], crossing_counter, static_cast<bool>(proven_minimalQ)
        );
        
        if( pd.ValidQ() )
        {
            pdc.Push(std::move(pd));
        }
        else
        {
            wprint(MethodName("FromInString") + ": Input diagram " + ToString(input_diagram_counter) + " unknot with invalid color and thus discarded.");
        }
        
        clear(); // Needed to track input_diagram_counter and to set default for last_color_deactivated.
    };

    s.SkipWhiteSpace();
    // `k` and `l` indicate the start of a new knot or link when many links are stored in the same string. We merely need to skip the first occurrence.
    if( (s.CurrentChar() == 'k') || (s.CurrentChar() == 'l') ) { s.Skip(1); }
    s.SkipWhiteSpace();

    while( !s.EmptyQ() && !s.FailedQ() )
    {
        const auto current_char = s.CurrentChar();
        
        if( current_char == 'u' )
        {
            push_diagram();
            
            if (mode != ParsingMode_T::None )
            {
                eprint(MethodName("FromInString") + ": Found character 'u' on input string while in not being in mode ParsingMode_T::None.");
                failedQ = true;
                break;
            }
            
            s.Skip(1);
            s.SkipWhiteSpace();
            // s.Take may fail an not write to last_color_deactivated. So it is important to set `PD_T::InvalidColord` as default value.
            s.Take(last_color_deactivated);
            s.SkipWhiteSpace();
            if( last_color_deactivated == PD_T::InvalidColor )
            {
                wprint(MethodName("FromInString") + ": Input diagram " + ToString(input_diagram_counter) + " diagram is invalid and thus discarded.");
            }
            else
            {
                pdc.Push(PD_T::Unknot(last_color_deactivated));
            }
            
            clear(); // Needed to track `input_diagram_counter` and to reset `last_color_deactivated` and `mode`.
            

            // We should now have `mode == ParsingMode_T::None`;
            
            continue;
        }
        else if( current_char == 's' )
        {
            push_diagram();
            
            if (mode != ParsingMode_T::None )
            {
                eprint(MethodName("FromInString") + ": Found character 's' on input string while `mode != ParsingMode_T::None`.");
                failedQ = true;
                break;
            }
            
            s.Skip(1);
            s.SkipWhiteSpace();
            s.Take(proven_minimalQ);
            s.SkipWhiteSpace();
            mode = ParsingMode_T::Subdiagram;
            
            continue;
        }
        if( current_char == 'k' )
        {
            // Indicates that a new knot or link is to be started. We stop parsing.
            // Mind that we do not skip this character because it may be important for reading time: several `k` or `l` in a row may indicate unlinks.
            break;
        }
        if( current_char == 'l' )
        {
            // Indicates that a new knot or link is to be started. We stop parsing.
            // Mind that we do not skip this character because it may be important for reading time: several `k` or `l` in a row may indicate unlinks.
            break;
        }
        else if (mode != ParsingMode_T::Subdiagram )
        {
            // We read pd code only in `Subdiagram` mode.
            
            eprint(MethodName("FromInString") + ": Not in mode `ParsingMode_T::Subdiagram` when attempting to read pd codes.");
            failedQ = true;
            break;
        }
        
        // We have to be careful here, because the last line may easily end with an '\n'.
        for( int i = 0; i < 6; ++i )
        {
            s.Take(x);
            pd_buffer.push_back(x);
            s.SkipWhiteSpace();
        }
        s.Take(x);
        pd_buffer.push_back(x);
        
        ++crossing_counter;
        
        if( s.EmptyQ() ) { break; }
        
        s.SkipWhiteSpace();
        
        // We are still in `mode == ParsingMode_T::Subdiagram`;
    }
    
    if( failedQ || s.FailedQ() )
    {
        eprint(MethodName("Read") + ": Reading failed. Returning invalid object.");
        return PDC_T();
    }
    
    // Push the last diagram we started to parse, if applicable.
    push_diagram();
    
    return pdc;
}
