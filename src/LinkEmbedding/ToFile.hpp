public:

/*!@brief Write the vertex coordinates of the link to file `file`. The vertex coordinates of each link component are write in `x y z` lines in the order they appear in the link component. Link components are separated by blank lines.
 *
 * @param file The file to write to.
 *
 * @param colorQ Whether the colors of the link components shall be exported in the format `#color <int>` at the start of each component. In any case, the output should be compatible with KnotPlot.
 */
bool WriteToFile( cref<std::filesystem::path> file, const bool colorQ = true ) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out );

    if( !stream )
    {
        Msgr::eprint("WriteToFile", ": Could not open file ", file.string(), ". Aborting.");
        return false;
    }
    
    if( component_ptr.Dim(0) <= Int{1} )
    {
        Msgr::eprint("WriteToFile", ": Diagram is invalid. Aborting.");
        return false;
    };

    OutString s;
    WriteToOutString(s,colorQ);
    stream << s;
    
    if( !stream )
    {
        Msgr::eprint("WriteToFile", ": Failed to write to file. Aborting.");
        return false;
    }
    
    return true;
}


/*!@brief Write the vertex coordinates of the link to `OutString` `s`. The vertex coordinates of each link component are written in `x y z` lines in the order they appear in the link component. Link components are separated by blank lines.
 *
 * @param s Output stream.
 *
 * @param colorQ Whether the colors of the link components shall be exported in the format `#color <int>` at the start of each component. In any case, the output should be compatible with KnotPlot. CAUTION: If this is a multiple-component link and if several components have the same color, then not writing the colors leads to a loss/change of some important topological information. We provide this option only to allow export for downstream application that cannot handle the `#color` statement and that do not treat it as comment.
 *
 */

bool WriteToOutString( mref<OutString> s, const bool colorQ = true ) const
{
    for( Int lc = 0; lc < component_count; ++lc )
    {
        const Int i_begin = component_ptr[lc         ];
        const Int i_end   = component_ptr[lc + Int{1}];
        
        if( i_end <= i_begin ) { continue; }
        
        if ( colorQ )
        {
            // Note: Lines with `#color` attribute must be preceded by a blank line (comment lines are ignored), unless it is the first line with no preceding newline (in which case it preceeds the first link component). Otherwise KnotPlot won't interpret this as the start of a new link component,
            s.PutChars("#color ");
            s.Put(component_color[lc]);
            s.PutChar('\n');
        }
        
        const Int m = i_end - i_begin;
        const Int n = 3;

        s.PutArray(
            [this,i_begin]( const Int i, const Int j )
            {
                return edge_coords(i_begin + i, Int{0}, j);
            },
            true,
            m, "", "\n", "",
            n, "", " ", ""
        );
        
        if( (lc + Int{1}) != component_count )
        {
            s.PutChars("\n\n");
        }
    }
    
    return true;
}


/*!@brief Export to `OutString`, using default options.*/
friend OutString & operator<<( OutString & s, const LinkEmbedding_T & L )
{
    (void)L.WriteToOutString(s);
    return s;
}

/*!@brief Export to `std::basic_ostream`, using default options.*/
template<typename C, typename T>
friend std::basic_ostream<C,T> & operator<<(
    std::basic_ostream<C,T> & stream, const LinkEmbedding_T & L
)
{
    OutString s;
    s << L;
    return stream << s;
}
