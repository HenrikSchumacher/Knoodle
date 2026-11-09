public: 


// Stores and looks up data for a fixed number of crossings.
class Subtable
{
public:
    
    friend Klut;
    
    using Class_T = Subtable;
    
    // For a weird reason. the routine `Read` will complain if this is place at the end of the class.
    
    using Msgr = Tools::Messenger<Class_T>;
           
    template<typename A>
    static consteval auto MethodName( const A & tag )
    {
        return Msgr::MethodName(tag);
    }

    static consteval auto ClassName()
    {
        return ct_string("Klut::Subtable");
    }
    
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
            Msgr::eprint( "Subtable()", "Requested number of crossings ", crossing_count, "is too big. Aborting with fail.");
            failedQ = true;
            return;
        }

        if( !std::filesystem::exists(k_file) )
        {
            failedQ = true;
            Msgr::eprint( "Subtable()", "Could not find file ", k_file, ".");
        }
        
        if( !std::filesystem::exists(v_file) )
        {
            failedQ = true;
            Msgr::eprint( "Subtable()", "Could not find file ", v_file, ".");
        }
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
        
private:
    
    void Read()
    {
        TOOLS_PTIMER(timer,MethodName("Read"));
        
        if( failedQ ) { return; }
        if( loadedQ ) { return; }
        
        // TODO: Check that c_count is small enough so that we can use bytes!
        lut.clear();
        knot_names.clear();
        
        std::ifstream v_stream ( v_file, std::ios::in );
        if( !v_stream )
        {
            Msgr::eprint("Read", "Could not open ", v_file, ". Aborting with incomplete table." );
            failedQ = true;
            return;
        }
        
        std::ifstream k_stream ( k_file, std::ios::binary );
        if( !k_stream )
        {
            Msgr::eprint("Read", "Could not open ", k_file, ". Aborting with incomplete table." );
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
            
            if( v_stream.eof() ) { break; }
            
            v_stream >> knot_count;
            
            knot_names.push_back( std::move(knot_name) );
            
            for( Size_T i = 0; i < knot_count; ++i )
            {
                if( !k_stream )
                {
                    Msgr::eprint("Read", "Problem while reading ", counter, "-th key from ", k_file, ". Aborting with incomplete table." );
                    
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
    
public:
    
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
                   
}; // class Subtable
