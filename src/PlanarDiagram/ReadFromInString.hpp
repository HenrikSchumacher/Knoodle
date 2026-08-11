public:

/*!@brief Reconstruct a diagram from the text written by
 * `InternalStateString` / `WriteToOutString`.
 *
 * Deliberately tolerant: it looks up each field by name and ignores
 * everything else, so a `PrintInfo` dump lifted out of a log file -- which
 * carries surrounding log lines and the extra `CacheKeys()` field -- parses
 * without editing. Fields may appear in any order.
 *
 * On any missing or malformed field an error is printed and an invalid
 * diagram is returned, so a failed read cannot be mistaken for an empty one.
 */

static PD_T ReadFromInString( mref<Tools::InString> s )
{
    return FromInternalStateString( s.View() );
}

static PD_T ReadFromFile( cref<std::filesystem::path> file )
{
    Tools::InString s (file);

    return ReadFromInString(s);
}

static PD_T FromInternalStateString( std::string_view text )
{
    auto fail = [&]( const std::string & msg ) -> PD_T
    {
        eprint(ClassName() + "::FromInternalStateString: " + msg
               + ". Returning invalid diagram.");
        return InvalidDiagram();
    };

    // Locate "<key> =", anchored so that `arc_count` does not match inside
    // `max_arc_count`. Returns the offset just past the '='.
    auto find_field = [&text]( std::string_view key ) -> std::size_t
    {
        for( std::size_t p = text.find(key); p != std::string_view::npos;
             p = text.find(key, p + 1) )
        {
            const bool left_ok = (p == 0)
                || (text[p-1] == '\n') || (text[p-1] == ' ')
                || (text[p-1] == '\t') || (text[p-1] == '\r');

            std::size_t q = p + key.size();
            while( (q < text.size()) && ((text[q] == ' ') || (text[q] == '\t')) )
            {
                ++q;
            }

            if( left_ok && (q < text.size()) && (text[q] == '=') )
            {
                return q + 1;
            }
        }
        return std::string_view::npos;
    };

    // Every value is either a scalar or a brace-nested list. In both cases we
    // just want the tokens in order; the nesting is implied by the field's
    // known shape, so we collect tokens until the braces balance again (or,
    // for a scalar, until end of line).
    auto tokens_of = [&text]( std::size_t pos, std::vector<std::string> & out )
    {
        out.clear();

        std::size_t i = pos;
        while( (i < text.size()) && ((text[i] == ' ') || (text[i] == '\t')) )
        {
            ++i;
        }

        const bool listQ = (i < text.size()) && (text[i] == '{');
        Int depth = 0;
        std::string tok;

        auto flush = [&out,&tok]()
        {
            if( !tok.empty() ) { out.push_back(tok); tok.clear(); }
        };

        for( ; i < text.size(); ++i )
        {
            const char c = text[i];

            if( c == '{' ) { flush(); ++depth; continue; }
            if( c == '}' )
            {
                flush(); --depth;
                if( depth <= 0 ) { return; }
                continue;
            }
            if( (c == ',') || (c == ' ') || (c == '\t') || (c == '\r') )
            {
                flush(); continue;
            }
            if( c == '\n' )
            {
                flush();
                if( !listQ ) { return; }
                continue;
            }
            tok += c;
        }
        flush();
    };

    std::vector<std::string> tok;

    auto scalar = [&]( std::string_view key, Int & out ) -> bool
    {
        const std::size_t p = find_field(key);
        if( p == std::string_view::npos ) { return false; }
        tokens_of(p, tok);
        if( tok.size() != Size_T(1) ) { return false; }
        try { out = static_cast<Int>(std::stoll(tok[0])); }
        catch(...) { return false; }
        return true;
    };

    Int max_c = 0, c_count = 0, max_a = 0, a_count = 0;
    Int last_color = Uninitialized;
    Int minimalQ = 0;

    if( !scalar("max_crossing_count", max_c) )
    {
        return fail("field `max_crossing_count` missing or malformed");
    }
    // The remaining counts are re-derived by the constructor; we read them
    // only to sanity-check the arrays below.
    (void)scalar("crossing_count", c_count);
    if( !scalar("max_arc_count", max_a) )
    {
        return fail("field `max_arc_count` missing or malformed");
    }
    (void)scalar("arc_count", a_count);
    (void)scalar("last_color_deactivated", last_color);
    (void)scalar("proven_minimalQ", minimalQ);

    if( max_a != Int(2) * max_c )
    {
        return fail("max_arc_count = " + ToString(max_a)
                    + " is not twice max_crossing_count = " + ToString(max_c));
    }

    auto read_ints = [&]( std::string_view key, Size_T n,
                          std::vector<Int> & out ) -> bool
    {
        const std::size_t p = find_field(key);
        if( p == std::string_view::npos ) { return false; }
        tokens_of(p, tok);
        if( tok.size() != n ) { return false; }
        out.resize(n);
        for( Size_T i = 0; i < n; ++i )
        {
            try { out[i] = static_cast<Int>(std::stoll(tok[i])); }
            catch(...) { return false; }
        }
        return true;
    };

    const Size_T n_c = static_cast<Size_T>(max_c);
    const Size_T n_a = static_cast<Size_T>(max_a);

    std::vector<Int> crossings, arcs, colors;

    if( !read_ints("C_arcs", Size_T(4) * n_c, crossings) )
    {
        return fail("field `C_arcs` missing, malformed, or not of length 4 * "
                    + ToString(max_c));
    }
    if( !read_ints("A_cross", Size_T(2) * n_a, arcs) )
    {
        return fail("field `A_cross` missing, malformed, or not of length 2 * "
                    + ToString(max_a));
    }
    if( !read_ints("A_color", n_a, colors) )
    {
        return fail("field `A_color` missing, malformed, or not of length "
                    + ToString(max_a));
    }

    // The two state fields are written symbolically by `ToString`.
    std::vector<CrossingState_T> c_states ( n_c );
    {
        const std::size_t p = find_field("C_state");
        if( p == std::string_view::npos ) { return fail("field `C_state` missing"); }
        tokens_of(p, tok);
        if( tok.size() != n_c )
        {
            return fail("field `C_state` is not of length " + ToString(max_c));
        }
        for( Size_T i = 0; i < n_c; ++i )
        {
            if     ( tok[i] == "RightHanded" ) { c_states[i] = CrossingState_T::RightHanded; }
            else if( tok[i] == "LeftHanded"  ) { c_states[i] = CrossingState_T::LeftHanded;  }
            else if( tok[i] == "Inactive"    ) { c_states[i] = CrossingState_T::Inactive;    }
            else { return fail("unknown crossing state `" + tok[i] + "`"); }
        }
    }

    std::vector<ArcState_T> a_states ( n_a );
    {
        const std::size_t p = find_field("A_state");
        if( p == std::string_view::npos ) { return fail("field `A_state` missing"); }
        tokens_of(p, tok);
        if( tok.size() != n_a )
        {
            return fail("field `A_state` is not of length " + ToString(max_a));
        }
        for( Size_T i = 0; i < n_a; ++i )
        {
            if     ( tok[i] == "Active"   ) { a_states[i] = ArcState_T::Active;   }
            else if( tok[i] == "Inactive" ) { a_states[i] = ArcState_T::Inactive; }
            else { return fail("unknown arc state `" + tok[i] + "`"); }
        }
    }

    PD_T pd (
        max_c,
        crossings.data(), c_states.data(),
        arcs.data(),      a_states.data(),
        colors.data(),    last_color,
        static_cast<bool>(minimalQ),
        false   // compressQ: keep the labels we were given
    );

    if( (c_count > Int(0)) && (pd.CrossingCount() != c_count) )
    {
        wprint(ClassName() + "::FromInternalStateString: recomputed "
               "crossing_count = " + ToString(pd.CrossingCount())
               + " disagrees with the stored " + ToString(c_count) + ".");
    }
    if( (a_count > Int(0)) && (pd.ArcCount() != a_count) )
    {
        wprint(ClassName() + "::FromInternalStateString: recomputed "
               "arc_count = " + ToString(pd.ArcCount())
               + " disagrees with the stored " + ToString(a_count) + ".");
    }

    return pd;
}
