#pragma once

//#include <boost/unordered/unordered_flat_map.hpp>
//#include <ankerl/unordered_dense.h>

namespace Knoodle
{
    class PrimeKnotLookupTable
    {
    public:
        
        using Value_T = std::string;
        
        // Will work only up to 13-crossing knots!
        using ID_T   = UInt16; // TODO: Is this big/small enough?
        
        static constexpr Size_T max_c_count = 13;
        static constexpr ID_T   not_found   = std::numeric_limits<ID_T>::max();
        
//        using Key_T  = std::string;
//        using Hash_T = std::hash<Key_T>;
        
        using Key_T  = std::array<UInt64,2>; // keys have length of 16 bytes
        using Hash_T = Tools::array_hash;
        
//        using LUT_T  = std::map<Key_T,ID_T>;
//        using LUT_T  = std::unordered_map<Key_T,ID_T,Hash_T>;
//        using LUT_T  = ankerl::unordered_dense::map<Key_T,ID_T,Hash_T>;
//        using LUT_T =  boost::unordered_flat_map<Key_T,ID_T,Hash_T>;
        
        using LUT_T =  AssociativeContainer<Key_T,ID_T,Hash_T>;

        using Path_T  = std::filesystem::path;
        
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
                    
//                    TOOLS_LOGDUMP(knot_name);
//                    TOOLS_LOGDUMP(knot_count);
                    
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
                        
//                        Key_T key ( c_count, '\0' );
//                        // Read c_count bytes from the stream.
//                        k_stream.read( &key[0], c_count );
                        
                        Key_T key = {};
                        // Read c_count bytes from the stream.
                        k_stream.read( reinterpret_cast<char *>(&key[0]), c_count );

                        // TODO:
                        
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
//                pd.template WriteMacLeodCode<UInt8>(
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
            
            Size_T c_count = std::min(max_c_count,crossing_count);
            
            subtables = std::vector<Subtable>( c_count + Size_T(1) );
            
            for( Size_T c = 1; c <= c_count; ++c )
            {
                if( c >= Size_T(3) )
                {
                    std::string s = StringWithLeadingZeroes(c,Size_T(2));
                    Path_T k_file = path / (std::string("Klut_Keys_") + s + ".bin");
                    Path_T v_file = path / (std::string("Klut_Values_") + s + ".tsv");
                    subtables[c] = Subtable(c, k_file, v_file);
                }
            }
        }
        
        Size_T CrossingCount() const
        {
            return subtables.size() - Size_T(1);
        }
        
        template<typename Int>
        std::pair<ID_T,ID_T> LookupID( cref<Key_T> key, Int n ) const
        {
            const ID_T c  = static_cast<ID_T>(n);
            
            if( std::cmp_greater_equal(n,subtables.size()) )
            {
//                wprint(MethodName("LookupID") + ": key length = " + ToString(c) + " is greater than maximum crossing number = " + ToString(subtables.size() - Size_T(1)) + " of table.");
                return std::pair{c,not_found};
            }
            
            const ID_T id = subtables[static_cast<Size_T>(n)].LookupID(key);
            
            return std::pair{c,id};
        }
        
        template<typename Int>
        std::string LookupName( cref<Key_T> key, Int n  ) const
        {
            if( std::cmp_greater_equal(n,subtables.size()) )
            {
//                wprint(MethodName("LookupID") + ": key length = " + ToString(key.size()) + " is greater than maximum crossing number = " + ToString(subtables.size() - Size_T(1)) + " of table.");
                return "NotFound";
            }

            return subtables[static_cast<Size_T>(n)].LookupName(key);
        }
        
        
        template<typename T, typename Int>
        static Key_T KeyFromMacLeodCode( cptr<T> s_mac_leod, Int n )
        {
            static_assert(IntQ<T>,"");
            static_assert(IntQ<Int>,"");
            
//            const Size_T n_ = ToSize_T(n);
//            Key_T key (n_,'\0');
//            copy_buffer(s_mac_leod,&key[0],n_);
            
            Key_T key = {};
            copy_buffer(s_mac_leod,reinterpret_cast<char *>(&key[0]),n);
            
            return key;
        }
        
        template<typename T, typename Int>
        static Key_T KeyFromMacLeodCode( cref<Tensor1<T,Int>> s_mac_leod )
        {
            return KeyFromMacLeodCode( &s_mac_leod[0], s_mac_leod.Size() );
        }
        
        template<typename T>
        static Key_T KeyFromMacLeodCode( cref<std::vector<T>> s_mac_leod )
        {
            return KeyFromMacLeodCode( &s_mac_leod[0], s_mac_leod.size() );
        }


        
       
       template<typename T, typename Int>
       std::pair<ID_T,ID_T> LookupID( cptr<T> s_mac_leod, Int n ) const
       {
           return LookupID( KeyFromMacLeodCode(s_mac_leod,n), n );
       }

       template<typename T, typename Int>
       std::string LookupName( cptr<T> s_mac_leod, Int n ) const
       {
           return LookupName( KeyFromMacLeodCode(s_mac_leod,n), n );
       }
        
        
        template<typename T, typename Int>
        std::pair<ID_T,ID_T> LookupID( cref<Tensor1<T,Int>> s_mac_leod ) const
        {
            return LookupID( &s_mac_leod[0], s_mac_leod.Size() );
        }

        template<typename T, typename Int>
        std::string LookupName( cref<Tensor1<T,Int>> s_mac_leod ) const
        {
            return LookupName( &s_mac_leod[0], s_mac_leod.Size() );
        }
        
        
        template<typename T, typename Int>
        std::pair<ID_T,ID_T> LookupID( cref<std::vector<T,Int>> s_mac_leod ) const
        {
            return LookupID( &s_mac_leod[0], s_mac_leod.size() );
        }

        template<typename T, typename Int>
        std::string LookupName( cref<std::vector<T,Int>> s_mac_leod ) const
        {
            return LookupName( &s_mac_leod[0], s_mac_leod.size() );
        }
        
        
        std::pair<ID_T,ID_T> LookupID( cref<std::string> s_mac_leod ) const
        {
            return LookupID( &s_mac_leod[0], s_mac_leod.size() );
        }

        std::string LookupName( cref<std::string> s_mac_leod ) const
        {
            return LookupName( &s_mac_leod[0], s_mac_leod.size() );
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


