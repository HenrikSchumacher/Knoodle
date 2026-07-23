#pragma once

// TODO: Make example.



namespace Knoodle
{
    /*!@brief A class to hold a prime knot table.
     *
     */
    
    class Klut
    {
    public:

        // We have one byte per crossing.
        // We need to store a number in [0,...,n[ per crossing.
        // And we need two extra bits for over/under and left/right-handed.
        // So 1 byte per crossings allows us to go up to 63 crossings?
        using CodeInt = UInt8;
        
//        // There are 9988 knots with 13 crossings.
//        static constexpr Size_T max_crossing_count = 13;
//        // So, an UInt16 should suffice for enumerating them.
//        using ID_T   = UInt16;
        
        // There are 1388705 knots with 16 crossings.
        static constexpr Size_T max_crossing_count = 16;
        // So, an UInt32 should suffice for enumerating them.
        
        /*!@brief An integral type to store IDs for all knot classes in the lookup table.*/
        using ID_T   = UInt32;
        
        /*!@brief The ID signaling that a MacLeod code could not be found in the lookup table.*/
        static constexpr ID_T not_found = std::numeric_limits<ID_T>::max();
        /*!@brief The ID signaling that an error occured during lookup.*/
        static constexpr ID_T error     = not_found - ID_T(1);
        /*!@brief The ID signaling that the input was invalid.*/
        static constexpr ID_T invalid   = not_found - ID_T(2);
        
        static constexpr Size_T KeyLength = 2;
        using KeyEntry_T  = UInt64;
        
        /*!@brief The type for the keys in the lookup table. It is a compressed encoding of a MacLeod code.*/
        using Key_T       = std::array<KeyEntry_T,KeyLength>; // keys have length of 16 bytes
//        using Hash_T      = Tools::Hash<Key_T>;
        using KeySet_T    = SetContainer<Key_T/*,Hash_T*/>;
        using LUT_T       = AssociativeContainer<Key_T,ID_T/*,Hash_T*/>;
        using Path_T      = std::filesystem::path;
        
        /*!@brief The type for the names of knots.*/
        using Name_T      = std::string;

        /*!@brief A key reserved for an invalid key.*/
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
        
        /*!@brief Initialize from path `data_directory_` and maximal number of crossings `crossing_count`. The tables are lazy-loaded. Call `RequireSubtables` to actually load their content.
         *
         * The tables can be a bit big; so we hesitate to load them already by the constructor. Instead, we do lazy-loading: a subtable is loaded only the first time it is needed.  In principle, this allows one to inspect the files for the subtables through the class's interface (e.g., find out their file sizes) and take appropriate measures.
         *
         * @param data_directory_ A path to a directory containing files with names of the form `Klut_Keys_??.bin` and `Klut_Values_??.bin`. These files hold the relevant information of the lookup tables and thei content will be loaded.
         *
         * @param crossing_count Provide tables for prime knots of up to this many crossings (if the corresponding files exist).
         */
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
        
        /*!@brief Return the maximal number of crossings for which a table exists in this data structure.*/
        Size_T CrossingCount() const
        {
            return subtables.size() - Size_T(1);
        }
        
        /*!@brief Ensure that all subtables are loaded.
         *
         *  The user should make sure to call this before any concurrent calls from several threads arrive at this instance of `Klut`; otherwise the lazy loading might incur a data race.
         */
        void RequireSubtables()
        {
            for( Size_T c = 3; c < subtables.size(); ++c )
            {
                subtables[c].RequireTable();
            }
        }
        
        /*!@brief Same as `RequireSubtables`. Only kept for backward compatibility.*/
        [[deprecated("Use RequireSubtables instead. That provides a better description on what is going on.")]]
        void LoadSubtables()
        {
            RequireSubtables();
        }
        
    public:

        /*!@brief Return the path to the file that contains the keys for all prime knots of exactly `crossing_count` crossings.*/
        Path_T KeyFile( Size_T crossing_count ) const
        {
            return data_directory / ("Klut_Keys_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".bin");
        }

        /*!@brief Return the path to the file that contains the values for all prime knots of exactly `crossing_count` crossings.*/
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


