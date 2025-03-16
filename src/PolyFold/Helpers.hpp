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


template<Size_T t, bool appendQ = true>
void PrintCallCounts( cref<typename Clisby_T::CallCounters_T> call_counters)
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"Call Counts\" -> <|";
        kv<t+1,0>("Transformation Loads", call_counters.load_transform);
        kv<t+1>("Matrix-Matrix Multiplications", call_counters.mm);
        kv<t+1>("Matrix-Vector Multiplications", call_counters.mv);
        kv<t+1>("Ball Overlap Checks", call_counters.overlap);
    log << "\n" + ct_tabs<t> + "|>";
}

template<Size_T t, bool appendQ = true>
void PrintClisbyFlagCounts( cref<FoldFlagCounts_T> counts )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"Clisby Flag Counts\" -> <|";
        kv<t+1,0>("Accepted", counts[0]);
        kv<t+1>("Rejected by Input Check", counts[1]);
        kv<t+1>("Rejected by First Pivot Check", counts[2]);
        kv<t+1>("Rejected by Second Pivot Check", counts[3]);
        kv<t+1>("Rejected by Tree", counts[4]);
    log << "\n" + ct_tabs<t> + "|>";
}

template<Size_T t, bool appendQ = true>
void PrintIntersectionFlagCounts( cref<std::string> key, cref<IntersectionFlagCounts_T> counts )
{
    using F_T = Link_T::Intersector_T::F_T;
    
    auto get = [&counts]( F_T flag )
    {
        return counts[ToUnderlying(flag)];
    };
    
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> <|";
        kv<t+1,0>("Empty Intersection", get(F_T::Empty));
        kv<t+1>("Transversal Intersection", get(F_T::Transversal) );
        kv<t+1>("Intersections on Corner of First Edge", get(F_T::AtCorner0) );
        kv<t+1>("Intersections on Corner of Second Edge", get(F_T::AtCorner1) );
        kv<t+1>("Intersections on Corners of Both Edges", get(F_T::CornerCorner) );
        kv<t+1>("Interval-like Intersections", get(F_T::Interval) );
        kv<t+1>("Spatial Intersections", get(F_T::Spatial) );
    log << "\n" + ct_tabs<t> + "|>";
    
}



