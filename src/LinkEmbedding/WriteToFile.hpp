public:

bool WriteToFile(
    cref<std::filesystem::path> file,
    const bool colorQ,
    [[maybe_unused]] const int  prec = (SameQ<Real,double> ? 18 : 9)
) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out );

    if( !stream )
    {
        eprint(MethodName("WriteToFile") + ": Could not open file " + file.string() + ". Aborting.");
        return false;
    }
    
    if( component_ptr.Dim(0) <= Int(1) )
    {
        eprint(MethodName("WriteToFile") + ": Diagram is invalid. Aborting.");
        return false;
    };

    const Int comp_count = component_ptr.Dim(0) - 1;

    OutString s;
    
    for( Int lc = 0; lc < component_count; ++lc )
    {
        const Int i_begin = component_ptr[lc    ];
        const Int i_end   = component_ptr[lc + 1];
        
        if( i_end <= i_begin ) { continue; }
        
        if ( colorQ )
        {
            stream << "#color " << ToString(component_color[lc]) << "\n";
        }
        
        const Int m = i_end - i_begin;
        const Int n = 3;

        s.Clear();
        s.PutArray(
            [this,i_begin]( const Int i, const Int j ) { return edge_coords(i_begin + i,Int(0),j); },
            true,
            m, "", "\n", "",
            n, "", " ", ""
        );
        
        stream << s;

        if( (lc + 1) != comp_count )
        {
            stream << "\n\n";
        }
    }
    
    return true;
}
