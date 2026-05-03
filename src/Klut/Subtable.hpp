public: 


// Stores and looks up data for a fixed number of crossings.
class Subtable
{
private:
    
    Size_T c_count = 0;
    Path_T k_file;
    Path_T v_file;
    
    LUT_T lut;
    std::vector<Name_T> knot_names;
    
    bool loadedQ = false;
    bool failedQ = false;
    
public:
    
    Subtable() = default;
    
    Subtable(
        const Size_T crossing_count,
        cref<Path_T> key_file,
        cref<Path_T> value_file
    )
    :   c_count { crossing_count }
    ,   k_file  { key_file       }
    ,   v_file  { value_file     }
    {
        if( crossing_count > max_crossing_count )
        {
            eprint(ClassName() + "(" + ToString(crossing_count) + "): Requested number of crossings is too big. Aborting with fail.");
            failedQ = true;
            return;
        }
//        failedQ = (!ifstream(k_file).good()) || (!ifstream(v_file).good());
        failedQ = (!std::filesystem::exists(k_file)) || (!std::filesystem::exists(v_file));
    }
    
    
    bool FailedQ()
    {
        return failedQ;
    }
    
    bool LoadedQ()
    {
        return loadedQ;
    }
    
    void RequireTable()
    {
        if( !loadedQ && !failedQ ) { Read(); }
    }
        
    void Read()
    {
        [[maybe_unused]] auto tag = [this](){ return MethodName("Read"); };
        
        TOOLS_PTIMER(timer,tag());
        
        if( loadedQ || failedQ ) { return; }
        
        // TODO: Check that c_count is small enough so that we can use bytes!
        lut.clear();
        knot_names.clear();
        
        std::ifstream v_stream ( v_file, std::ios::in );
        if( !v_stream )
        {
            eprint(tag() + ": Could not open " + v_file.string() +". Aborting with incomplete table." );
            failedQ = true;
            return;
        }
        
        std::ifstream k_stream ( k_file, std::ios::binary );
        if( !k_stream )
        {
            eprint(tag() + ": Could not open " + k_file.string() +". Aborting with incomplete table." );
            failedQ = true;
            return;
        }
        
        // v_file is supposed to contain pairs of knot_name and knot_count, separated by whitespace . Here  knot_name is a string identifier for the knot and knot_count is the number of times the knot occurs in the file.
        // k_stream is supposed to contain all short MacLeod codes for all the knots, ordered with respect to knot_name, i.e., all keys of a given knot_name must appear consecutively.
        
        ID_T id = 0;
        Size_T counter = 0;
        
        while( v_stream )
        {
            Name_T knot_name;
            Size_T knot_count;
            
            v_stream >> knot_name;
            
            if( v_stream.eof() ) { return; }
            
            v_stream >> knot_count;
            
//            if( v_stream.eof() ) { return; }
//
//            TOOLS_LOGDUMP(knot_name);
//            TOOLS_LOGDUMP(knot_count);
            
            knot_names.push_back( std::move(knot_name) );
            
            for( Size_T i = 0; i < knot_count; ++i )
            {
                if( !k_stream )
                {
                    eprint(tag() + ": Problem while reading " + ToString(counter) + "-th key from " + k_file.string() +". Aborting with incomplete table." );
                    
                    TOOLS_LOGDUMP(k_stream.good());
                    TOOLS_LOGDUMP(k_stream.fail());
                    TOOLS_LOGDUMP(k_stream.bad());
                    TOOLS_LOGDUMP(k_stream.eof());
                    failedQ = true;
                    return;
                }
                
                Key_T key = {};
                // Read c_count bytes from the stream.
                k_stream.read( reinterpret_cast<char *>(&key[0]),
                              static_cast<std::streamsize>(c_count) );

                lut.insert( std::pair{std::move(key), id} );
                
                ++counter;
                
            } // for( Size_T i = 0; i < knot_count; ++i )
            
            ++id;
        }
        
        loadedQ = !failedQ;
    }
    
    Size_T CrossingCount() const
    {
        return c_count;
    }
    
    Size_T KeyCount()
    {
        RequireTable();
        return lut.size();
    }
    
    Size_T KnotCount()
    {
        RequireTable();
        return knot_names.size();
    }
    
    ID_T FindID( cref<Key_T> key )
    {
        RequireTable();
        if( failedQ ) { return error; }
        
        auto it = lut.find(key);

        if( it == lut.end() ) { return not_found; }
        
        return it->second;
    }
    
    std::string FindName( cref<Key_T> key )
    {
        RequireTable();
        if( failedQ ) { return "Error"; }
        
        Size_T id = static_cast<Size_T>(FindID(key));
        
        if( id == error     ) { return "Error"; };
        if( id == not_found ) { return "NotFound"; };
        
        return knot_names[id];
    }
    
    std::vector<std::string> Names()
    {
        RequireTable();
        return knot_names;
    }
    
    cref<LUT_T> LookupTable()
    {
        RequireTable();
        return lut;
    }
    
    Tensor2<CodeInt,ID_T> Codes()
    {
        RequireTable();
        Tensor2<CodeInt,ID_T> codes ( KeyCount(),  c_count );
        
        ID_T i = 0;
        for( const auto & [key,id] : lut )
        {
            Klut::KeyToMacLeodCode(key,codes.data(i));
            ++i;
        }
        
        return codes;
    }
    
    Tensor1<ID_T,ID_T> IDs()
    {
        RequireTable();
        
        Tensor1<ID_T,ID_T> ids ( KeyCount() );
        
        ID_T i = 0;
        for( const auto & [key,id] : lut )
        {
            ids[i] = id;
            ++i;
        }
        
        return ids;
    }
    
    void ExportToFile( cref<Path_T> file )
    {
        RequireTable();
        
        std::ofstream stream ( file );
        
        if( !stream )
        {
            eprint(MethodName("ExportToFile") + ": Could not open file " + file.string() +". Aborting." );
            return;
        }
        
        Tensor1<CodeInt,Size_T> code ( c_count );
        
        // The longest name that I found has length 23.
        // Codes how c_count entries with up to 3 digits.
        // We need another byte per entry for the separators `\t`.
        // We need another byte per code for '\n'.
        OutString s ( lut.size() * (3 * c_count + 23 + 1) );
        
        for( const auto & [key,id] : lut )
        {
            Klut::KeyToMacLeodCode(key,code.data());
            s.template PutVectorFun<Format::Vector::TSV>(code.ReadAccess(),c_count);
            s.PutChar('\t');
            s.PutChars(knot_names[id]);
            s.PutChar('\n');
        }
        
        stream << s;
    }
    
    
public:
    
    std::string MethodName( const std::string & tag ) const
    {
        return ClassName() + "::" + tag;
    }
    
    std::string ClassName() const
    {
        return std::string("Klut::Subtable");
    }
    
}; // class Subtable
