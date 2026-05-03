public:

template<IntQ Int>
std::vector<std::string> SubtableNames( const Int n )
{
    Size_T c = ToSize_T(n);
    
    if( std::cmp_less(n,0) || std::cmp_greater(c,CrossingCount()) )
    {
        std::vector<std::string>();
    }
    
    Subtable & s = subtables[c];
    
    s.RequireTable();
    
    return s.knot_names;
}

//cref<LUT_T> LookupTable()
//{
//    RequireTable();
//    return lut;
//    
//    if( std::cmp_greater_equal(i,0) && std::cmp_less_equal(i,CrossingCount()) )
//    {
//        return subtables[i].lut;
//    }
//    else
//    {
//        std::vector<std::string>();
//    }
//}

template<IntQ T = CodeInt, IntQ Int>
Tensor2<T,Size_T> SubtableCodes( const Int n )
{
    Size_T c = ToSize_T(n);
    
    if( std::cmp_less(n,0) || std::cmp_greater(c,CrossingCount()) )
    {
        return Tensor2<T,Size_T>();
    }
    
    Subtable & s = subtables[c];
    
    s.RequireTable();
    
    Tensor2<T,Size_T> codes ( s.KeyCount(), c );
    
    Size_T j = 0;
    for( const auto & [key,id] : s.lut )
    {
        Klut::KeyToMacLeodCode(key,codes.data(j));
        ++j;
    }
    
    return codes;
}

template<IntQ Int>
Tensor1<ID_T,Size_T> SubtableIDs( const Int n )
{
    Size_T c = ToSize_T(n);
    
    if( std::cmp_less(n,0) || std::cmp_greater(c,CrossingCount()) )
    {
        return Tensor1<ID_T,Size_T>();
    }
    
    Subtable & s = subtables[c];
    
    s.RequireTable();
    
    Tensor1<ID_T,Size_T> ids ( s.KeyCount() );
    
    Size_T j = 0;
    for( const auto & [key,id] : s.lut )
    {
        ids[j] = id;
        ++j;
    }
    
    return ids;
}

template<IntQ Int>
void WriteSubtableToFile( cref<Path_T> file, const Int n )
{
    Size_T c = ToSize_T(n);
    
    if( std::cmp_less(n,0) || std::cmp_greater(c,CrossingCount()) )
    {
        wprint(MethodName("WriteSubtableToFile") + ": There is no subtable for " + ToString(n) +"-crossing diagrams to export. Aborting.");
        return;
    }
    
    Subtable & subtable = subtables[c];
    
    if( std::cmp_not_equal(c, subtable.CrossingCount()) )
    {
        eprint(MethodName("WriteSubtableToFile") + ": No ob crossings of diagram does not match requested number of crossings. Table must be corrupted. Aborting.");
        return;
    }
    
    subtable.RequireTable();
    
    std::ofstream stream ( file );
    
    if( !stream )
    {
        eprint(MethodName("WriteSubtableToFile") + ": Could not open file " + file.string() +". Aborting." );
        return;
    }

    
    Tensor1<CodeInt,Size_T> code ( c );
    
    // The longest name that I found has length 23.
    // Codes how c_count entries with up to 3 digits.
    // We need another byte per entry for the separators `\t`.
    // We need another byte per code for '\n'.
    OutString s ( subtable.lut.size() * (3 * c + 23 + 1) );
    
    for( const auto & [key,id] : subtable.lut )
    {
        Klut::KeyToMacLeodCode(key,code.data());
        s.template PutVectorFun<Format::Vector::TSV>(code.ReadAccess(),c);
        s.PutChar('\t');
        s.PutChars(subtable.knot_names[id]);
        s.PutChar('\n');
    }
    
    stream << s;
}
