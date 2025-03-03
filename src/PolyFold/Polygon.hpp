cref<PolygonContainer_T> GetPolygon() const
{
    return x;
}

static std::string PolygonString( cref<PolygonContainer_T> X )
{
    return ToStringTSV(X);
}

template<Size_T t0>
void PolygonSnapshot( const Int i )
{
    constexpr Size_T t1 = t0 + 1;
    
    std::string file_name = "Polygon_" + StringWithLeadingZeroes(i,9) + ".tsv";
    
    std::ofstream s ( path / file_name );
    
    s << PolygonString(x);

    print_ctr = 0;
    
    PRNG_State_T state = State( prng );
    
    log << ",\n" + ct_tabs<t0> + "\"Snapshot\" -> <|";
        kv<t1,0>("Multiplier", state.multiplier);
        kv<t1>("Increment"   , state.increment );
        kv<t1>("State"       , state.state     );
        kv<t1>("File"        , file_name       );
    log << "\n" + ct_tabs<t0> + "|>";
    
    log << std::flush;
}
