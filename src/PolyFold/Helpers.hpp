template<Size_T t, bool appendQ = true>
void kv( const std::string & key, std::string & value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, std::string && value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, const char * & value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true, typename T >
void kv( const std::string & key, const T & value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ToString(value);
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, const double & value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ToMathematicaString(value);
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, const float & value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ToMathematicaString(value);
}


