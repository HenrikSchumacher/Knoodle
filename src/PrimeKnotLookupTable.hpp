#pragma once

namespace Knoodle
{
    class PrimeKnotLookupTable
    {
        //        static_assert(UnsignedIntQ<KeyEntry_T>,"");
    public:
        
        using E_T     = char;
        using Key_T   = std::string;
        using Value_T = std::string;
        
        // Will work only up to 13-crossing knots!
        using ID_T    = UInt16; // TODO: Is this big/small enough?
        
        using LUT_T   = std::unordered_map<Key_T,ID_T>;
        
        using Path_T  = std::filesystem::path;
        
        static constexpr Size_T max_c_count = 13;
        static constexpr ID_T   not_found   = std::numeric_limits<ID_T>::max();
        
        // Stores and looks up data for a fixed number of crossings.
        class Subtable
        {
        private:
            
            Size_T c_count = 0;
            Path_T k_file;
            Path_T v_file;
            
            LUT_T lut;
            std::vector<Value_T> knot_names;
            
        public:
            
            Subtable() = default;
            
            Subtable(
                const Size_T c_count,
                cref<Path_T> key_file,
                cref<Path_T> value_file
            )
            :   c_count { c_count    }
            ,   k_file  { key_file   }
            ,   v_file  { value_file }
            {
                TOOLS_PTIMER(timer,ClassName()+"(" + ToString(c_count) + ")");
                
                // TODO: Check that c_count is small enough so that we can use bytes!
                lut.clear();
                knot_names.clear();
                
                std::ifstream v_stream ( v_file, std::ios::in     );
                if( !v_stream )
                {
                    eprint(MethodName("ReadTable") + ": Could not open " + v_file.string() +". Aborting with incomplete table." );
                    
                    return;
                }
                
                std::ifstream k_stream ( k_file, std::ios::binary );
                if( !k_stream )
                {
                    eprint(MethodName("ReadTable") + ": Could not open " + k_file.string() +". Aborting with incomplete table." );
                    
                    return;
                }
                
                // v_file is supposed to contain pairs of knot_name and knot_count, separated by whitespace . Here  knot_name is a string identifier for the knot and knot_count is the number of times the knot occurs in the file.
                // k_stream is supposed to contain all short MacLeod codes for all the knots, ordered with respect to knot_name, i.e., all keys of a given knot_name must appear consecutively.
                
                ID_T id = 0;
                Size_T counter = 0;
                
                while( v_stream )
                {
                    Value_T knot_name;
                    Size_T  knot_count;
                    
                    v_stream >> knot_name;
                    
                    if( v_stream.eof() ) { return; }
                    
                    v_stream >> knot_count;
                    
                    //                if( v_stream.eof() ) { return; }
                    
                    TOOLS_LOGDUMP(knot_name);
                    TOOLS_LOGDUMP(knot_count);
                    
                    knot_names.push_back( std::move(knot_name) );
                    
                    for( Size_T i = 0; i < knot_count; ++i )
                    {
                        if( !k_stream )
                        {
                            eprint(MethodName("ReadTable") + ": Problem while reading " + ToString(counter) + "-th key from " + k_file.string() +". Aborting with incomplete table." );
                            
                            TOOLS_LOGDUMP(k_stream.good());
                            TOOLS_LOGDUMP(k_stream.fail());
                            TOOLS_LOGDUMP(k_stream.bad());
                            TOOLS_LOGDUMP(k_stream.eof());
                            return;
                        }
                        
                        Key_T key ( c_count, '\0' );
                        
                        // Read c_count bytes from the stream.
                        k_stream.read( &key[0], c_count );
                        
                        lut.insert( std::pair{std::move(key), id} );
                        
                        ++counter;
                        
                    } // for( Size_T i = 0; i < knot_count; ++i )
                    
                    ++id;
                }
            }
            
            Size_T CrossingCount() const
            {
                return c_count;
            }
            
            Size_T KeyCount() const
            {
                return lut.size();
            }
            
            Size_T KnotCount() const
            {
                return knot_names.size();
            }
            
            ID_T LookupID( cref<Key_T> key ) const
            {
                auto it = lut.find(key);
                
//                if( key.size() != c_count )
//                {
//                    wprint(MethodName("LookupID") + ": key \"" + ToString(key) + "\" is longer than crossing count = " + ToString(c_count) + " of this lookup table. Returning not_found.");
//                    
//                    return not_found;
//                }
                
                if( it == lut.end() )
                {
                    wprint(MethodName("LookupID") + ": key \"" + ToString(key) + "\" could not be found. Returning not_found.");
                    
                    return not_found;
                }
                
                return it->second;
            }
            
            std::string LookupName( cref<Key_T> key ) const
            {
                Size_T id = static_cast<Size_T>(LookupID(key));
                
                return (id != not_found) ? knot_names[id] : std::string("NotFound");
            }
            
            
            
//            template<typename Int>
//            std::string LookupName( cref<PlanarDiagram<Int>> pd ) const
//            {
//                if( !std::cmp_equal(pd.CrossingCount(),c_count) )
//                {
//                    wprint(MethodName("LookupName") + ": input's crossing count = " + ToString(pd.CrossingCount()) + " does not coincide with crossing count = " + ToString(c_count) + " of this lookup table. Returning \"NotFound\".");
//
//                    return "NotFound";
//                }
//
//                std::string key (c_count, '\0');
//
//                pd.template WriteShortMacLeodCode<UInt8>(
//                    reinterpret_cast<UInt8 *>(&key[0])
//                );
//
//                return LookupName(key);
//            }
            
            
        public:
            
            std::string MethodName( const std::string & tag ) const
            {
                return ClassName() + "::" + tag;
            }
            
            std::string ClassName() const
            {
                return std::string("PrimeKnotLookupTable::Subtable(") + ToString(c_count) +")";
            }
            
        }; // class Subtable
        
        
    private:
        
        std::vector<Subtable> subtables {1};
        
    public:
        
        PrimeKnotLookupTable() = default;
        
        PrimeKnotLookupTable(
            cref<Path_T> path,
            Size_T crossing_count   // build up to this number of crossings.
        )
        {
            // TODO: Check size of max_crossing_count!
            
            for( Size_T c = 1; c <= std::min(max_c_count,crossing_count); ++c )
            {
                if( c >= Size_T(3) )
                {
                    std::string s = StringWithLeadingZeroes(c,Size_T(2));
                    Path_T k_file = path / (std::string("KLUT_Keys_") + s + ".bin");
                    Path_T v_file = path / (std::string("KLUT_Values_") + s + ".tsv");
                    subtables.push_back( Subtable(c, k_file, v_file) );
                }
                else
                {
                    subtables.push_back( Subtable() );
                }
            }
        }
        
        Size_T CrossingCount() const
        {
            return subtables.size() - Size_T(1);
        }
        
        std::pair<ID_T,ID_T> LookupID( cref<Key_T> key ) const
        {
            const ID_T c  = static_cast<ID_T>(key.size());
            
            if( key.size() >= subtables.size() )
            {
//                wprint(MethodName("LookupID") + ": key length = " + ToString(c) + " is greater than maximum crossing number = " + ToString(subtables.size() - Size_T(1)) + " of table.");
                return std::pair{c,not_found};
            }
            
            const ID_T id = subtables[key.size()].LookupID(key);
            
            return std::pair{c,id};
        }
        
        std::string LookupName( cref<Key_T> key ) const
        {
            if( key.size() >= subtables.size() )
            {
//                wprint(MethodName("LookupID") + ": key length = " + ToString(key.size()) + " is greater than maximum crossing number = " + ToString(subtables.size() - Size_T(1)) + " of table.");
                return "NotFound";
            }

            return subtables[key.size()].LookupName(key);
        }
        
        
        template<typename T, typename Int>
        static Key_T KeyFromShortMacLeodCode( cptr<T> short_mac_leod, Int n )
        {
            static_assert(IntQ<T>,"");
            static_assert(IntQ<Int>,"");
            const Size_T n_ = ToSize_T(n);
            Key_T key (n_,'\0');
            copy_buffer(short_mac_leod,&key[0],n_);
            return key;
        }
        
        template<typename T, typename Int>
        static Key_T KeyFromShortMacLeodCode( cref<Tensor1<T,Int>> short_mac_leod )
        {
            return KeyFromShortMacLeodCode(
               &short_mac_leod[0], short_mac_leod.Size()
            );
        }
        
        template<typename T>
        static Key_T KeyFromShortMacLeodCode( cref<std::vector<T>> short_mac_leod )
        {
            return KeyFromShortMacLeodCode(
               &short_mac_leod[0], short_mac_leod.size()
            );
        }
         
        
        template<typename T, typename Int>
        std::pair<ID_T,ID_T> LookupID( cptr<T> short_mac_leod, Int n ) const
        {
            return LookupID( KeyFromShortMacLeodCode(short_mac_leod,n) );
        }

        template<typename T, typename Int>
        std::string LookupName( cptr<T> short_mac_leod, Int n ) const
        {
            return LookupName( KeyFromShortMacLeodCode(short_mac_leod,n) );
        }
        
        
        template<typename T, typename Int>
        std::pair<ID_T,ID_T> LookupID( cref<Tensor1<T,Int>> short_mac_leod ) const
        {
            return LookupID( KeyFromShortMacLeodCode(short_mac_leod) );
        }

        template<typename T, typename Int>
        std::string LookupName( cref<Tensor1<T,Int>> short_mac_leod ) const
        {
            return LookupName( KeyFromShortMacLeodCode(short_mac_leod) );
        }
        
        
        template<typename T, typename Int>
        std::pair<ID_T,ID_T> LookupID( cref<std::vector<T,Int>> short_mac_leod ) const
        {
            return LookupID( KeyFromShortMacLeodCode(short_mac_leod) );
        }

        template<typename T, typename Int>
        std::string LookupName( cref<std::vector<T,Int>> short_mac_leod ) const
        {
            return LookupName( KeyFromShortMacLeodCode(short_mac_leod) );
        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return std::string("PrimeKnotLookupTable");
        }
        
    }; // class PrimeKnotLookupTable
    
} // namespace Knoodle
