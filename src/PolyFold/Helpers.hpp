template<Size_T t >
static constexpr ct_string<t + Size_T(1)> Tabs = Tabs<t-1> + ct_string<Size_T(2)>("\t");

template<>
static constexpr ct_string<Size_T(1)> Tabs<Size_T(0)> = ct_string<Size_T(1)>("");


template<Size_T t, bool appendQ = true>
static std::string kv( const std::string & key, std::string & value )
{
    return (appendQ ? "," : "") + std::string("\n") + Tabs<t> + "\"" + key + "\"" + " -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true>
static std::string kv( const std::string & key, std::string && value )
{
    return (appendQ ? "," : "") + std::string("\n") + Tabs<t> + "\"" + key + "\"" + " -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true>
static std::string kv( const std::string & key, const char * & value )
{
    return (appendQ ? "," : "") + std::string("\n") + Tabs<t> + "\"" + key + "\"" + "->\"" + value + "\"";
}

template<Size_T t, bool appendQ = true, typename T >
static std::string kv( const std::string & key, const T & value )
{
    return (appendQ ? "," : "") + std::string("\n") + Tabs<t> + "\"" + key + "\"" + " -> " + ToString(value);
}

template<Size_T t, bool appendQ = true>
static std::string kv( const std::string & key, const double & value )
{
    return (appendQ ? "," : "") + std::string("\n") + Tabs<t> + "\"" + key + "\"" + " -> " + ToMathematicaString(value);
}

template<Size_T t, bool appendQ = true>
static std::string kv( const std::string & key, const float & value )
{
    return (appendQ ? "," : "") + std::string("\n") + Tabs<t> + "\"" + key + "\"" + "->" + ToMathematicaString(value);
}
