#pragma once

// TODO: Make example.

namespace Knoodle
{
    class Klut
    {
    public:
        
        static constexpr Size_T max_crossing_count = 13;
        
        using CodeInt = UInt8;
        
        // Will work only up to 13-crossing knots!
        using ID_T   = UInt16; // TODO: Is this big/small enough?
        static constexpr ID_T not_found = std::numeric_limits<ID_T>::max();
        static constexpr ID_T error     = not_found - ID_T(1);
        static constexpr ID_T invalid   = not_found - ID_T(2);
        
        static constexpr Size_T KeyLength = 2;
        using KeyEntry_T  = UInt64;
        using Key_T       = std::array<KeyEntry_T,KeyLength>; // keys have length of 16 bytes
        using Hash_T      = Tools::array_hash;
        using KeySet_T    = SetContainer<Key_T,Hash_T>;
        using LUT_T       = AssociativeContainer<Key_T,ID_T,Hash_T>;
        using Path_T      = std::filesystem::path;
        using Name_T      = std::string;

        static constexpr Key_T InvalidKey = {0,0};
        
//        using LUT_T  = std::map<Key_T,ID_T>;
//        using LUT_T  = std::unordered_map<Key_T,ID_T,Hash_T>;
//        using LUT_T  = ankerl::unordered_dense::map<Key_T,ID_T,Hash_T>;
//        using LUT_T =  boost::unordered_flat_map<Key_T,ID_T,Hash_T>;
        
#include "Klut/Subtable.hpp"
        
    private:
        
        Path_T data_directory;
        
        std::vector<Subtable> subtables {1};
        
        Tensor1<CodeInt,Size_T> s_mac_leod_buffer { max_crossing_count };
        
    public:
        
        Klut() = default;
        
        Klut(
            cref<Path_T> data_directory_,
            Size_T crossing_count = max_crossing_count // build up to this number of crossings.
        )
        :   data_directory (data_directory_)
        {
            // TODO: Check size of max_crossing_count!
            
            Size_T c_count = std::min(max_crossing_count,crossing_count);
            
            subtables = std::vector<Subtable>( c_count + Size_T(1) );
            
            for( Size_T c = 3; c <= c_count; ++c )
            {
                subtables[c] = Subtable(c, KeyFile(c), ValueFile(c));
            }
        }
        
        Size_T CrossingCount() const
        {
            return subtables.size() - Size_T(1);
        }
        
        void LoadSubtables()
        {
            for( Size_T c = 3; c < subtables.size(); ++c )
            {
                subtables[c].RequireTable();
            }
        }
        
    public:

        Path_T KeyFile( Size_T crossing_count ) const
        {
            return data_directory / ("Klut_Keys_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".bin");
        }

        Path_T ValueFile( Size_T crossing_count ) const
        {
            return data_directory / ("Klut_Values_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".tsv");
        }
        
    public:
        
#include "Klut/Key.hpp"
#include "Klut/FindID.hpp"
#include "Klut/FindName.hpp"
#include "Klut/Export.hpp"
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return std::string("Klut");
        }
        
    }; // class Klut
    
} // namespace Knoodle


