public: 


// Stores and looks up data for a fixed number of crossings.
class Subtable
{
    friend Klut;
    
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
        
        // A missing file is not an error: the table just stops short of this
        // many crossings (see Klut's constructor). A file that cannot even be
        // checked -- e.g., under a directory without search permission -- is;
        // and the throwing overload of exists would escape this constructor.
        std::error_code k_ec;
        std::error_code v_ec;
        const bool k_existsQ = std::filesystem::exists(k_file, k_ec);
        const bool v_existsQ = std::filesystem::exists(v_file, v_ec);
        
        if( k_ec )
        {
            eprint(ClassName(), "(", crossing_count, "): Could not check for ", k_file.string(), ": ", k_ec.message(), ". Aborting with fail.");
        }
        if( v_ec )
        {
            eprint(ClassName(), "(", crossing_count, "): Could not check for ", v_file.string(), ": ", v_ec.message(), ". Aborting with fail.");
        }
        
        failedQ = !k_existsQ || !v_existsQ;
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
        [[maybe_unused]] auto tag = [](){ return MethodName("Read"); };
        
        TOOLS_PTIMER(timer,tag());
        
        if( failedQ ) { return; }
        if( loadedQ ) { return; }
        
        // TODO: Check that c_count is small enough so that we can use bytes!
        lut.clear();
        knot_names.clear();
        
        std::ifstream v_stream ( v_file, std::ios::in );
        if( !v_stream )
        {
            eprint(tag(), ": Could not open ", v_file.string(), ". Aborting with incomplete table." );
            failedQ = true;
            return;
        }
        
        std::ifstream k_stream ( k_file, std::ios::binary );
        if( !k_stream )
        {
            eprint(tag(), ": Could not open ", k_file.string(), ". Aborting with incomplete table." );
            failedQ = true;
            return;
        }
        
        // v_file is supposed to contain pairs of knot_name and knot_count, separated by whitespace . Here  knot_name is a string identifier for the knot and knot_count is the number of times the knot occurs in the file.
        // k_stream is supposed to contain all short MacLeod codes for all the knots, ordered with respect to knot_name, i.e., all keys of a given knot_name must appear consecutively.
        
        ID_T id = 0;
        Size_T counter = 0;
        
        while( true )
        {
            Name_T knot_name;
            Size_T knot_count = 0;
            
            if( !(v_stream >> knot_name) )
            {
                // Only a clean end of file ends the table.
                if( v_stream.eof() ) { break; }
            
                eprint(tag(), ": Could not read entry ", knot_names.size() + Size_T(1), " of ", v_file.string(), ". Aborting with incomplete table." );
                Fail();
                return;
            }
            
            // A count that does not parse must not silently end the table.
            if( !(v_stream >> knot_count) || (knot_count == Size_T(0)) )
            {
                eprint(tag(), ": ", v_file.string(), " gives no valid key count for ", knot_name, ". Aborting with incomplete table." );
                Fail();
                return;
            }
            
            knot_names.push_back( std::move(knot_name) );
            
            for( Size_T i = 0; i < knot_count; ++i )
            {
                Key_T key = {};
                // Read c_count bytes from the stream.
                k_stream.read( reinterpret_cast<char *>(&key[0]),
                              static_cast<std::streamsize>(c_count) );

                // Check the read itself, not the stream before it: a short read
                // at the end of the file would insert a zero-padded key.
                if( k_stream.gcount() != static_cast<std::streamsize>(c_count) )
                {
                    eprint(tag(), ": ", k_file.string(), " ends after ", counter, " keys, but ", v_file.string(), " lists more (", knot_names.back(), " has ", knot_count, "). Aborting with incomplete table." );
                    Fail();
                    return;
                }
                
                // A key with two entries leaves one of them unreachable.
                const auto [it, insertedQ] = lut.insert( std::pair{std::move(key), id} );
                
                if( !insertedQ )
                {
                    eprint(tag(), ": Key ", counter, " of ", k_file.string(), " (for ", knot_names.back(), ") repeats a key of ", knot_names[it->second], ". Aborting with incomplete table." );
                    Fail();
                    return;
                }
                
                ++counter;
                
            } // for( Size_T i = 0; i < knot_count; ++i )
            
            ++id;
        }

        // Every key must belong to an entry of the values file.
        if( k_stream.peek() != std::ifstream::traits_type::eof() )
        {
            eprint(tag(), ": ", k_file.string(), " holds more than the ", counter, " keys listed in ", v_file.string(), ". Aborting with incomplete table." );
            Fail();
            return;
        }
        
        loadedQ = !failedQ;
    }
    
    // Fail without leaving a partial table behind: SubtableNames, SubtableIDs,
    // and SubtableCodes export whatever is loaded.
    void Fail()
    {
        lut.clear();
        knot_names.clear();
        failedQ = true;
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
    
public:
    
    static std::string MethodName( const std::string & tag )
    {
        return ClassName() + "::" + tag;
    }
    
    static std::string ClassName()
    {
        return std::string("Klut::Subtable");
    }
    
}; // class Subtable
