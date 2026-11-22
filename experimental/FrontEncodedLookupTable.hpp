#pragma  once


namespace Knoodle
{
    /*!@brief A class for a lookupt table whose keys are fixed-size byte sequences. This is optimized for low storage space and descently fast lookup. An insertion operation has been omitted intentionally.
     */
    
    template<Size_T key_length_, typename Value_T_>
    class FrontEncodedLookupTable final
    {
        
    public:
        
        using UInt = UInt8;
        
        
        static_assert( key_length_ <= 256,"" );
        
        static constexpr Size_T key_length = key_length_;
        
        using Class_T = FrontEncodedLookupTable;
        using Key_T   = std::array<UInt8,key_length>;
        using Value_T = Value_T_;
        
        
        struct FindResult_T
        {
            const Value_T & val;
            int             err = false;
            
            friend std::string ToString( const FindResult_T & r )
            {
                switch( r.err )
                {
                    case 0:  return ToString(r.val);
                    case 1:  return "NotLoaded";
                    case 2:  return "Failed";
                    case 3:  return "NotFound";
                    default: return "Failed due to unknown error.";
                }
            }
        };

        static constexpr Value_T DefaultValue = Value_T();
        
        static constexpr FindResult_T NotLoaded = {DefaultValue,1};
        static constexpr FindResult_T Failed    = {DefaultValue,2};
        static constexpr FindResult_T NotFound  = {DefaultValue,3};
        
        
        
    private:
        
        // For each prefix byte `p` this table stores the first position of an entry greater or equal to `[p,0,0...]`.
        std::array<Size_T,256> prefix_ptr = {};
        // For each prefix byte `p` this table stores the number of keys _before_ `[p,0,0...]`.
        std::array<Size_T,256> prefix_pos = {};
        std::vector<UInt8> buffer;
        std::vector<Value_T> values;
        Key_T last_key = {};
        bool loadedQ = false;
        bool failedQ = false;
        
    public:
        
        FrontEncodedLookupTable() = default;
        
        ~FrontEncodedLookupTable() = default;
        
        void Clear()
        {
            prefix_ptr.fill(UInt{0});
            prefix_pos.fill(UInt{0});
            buffer.clear();
            values.clear();
            last_key.fill(UInt{0});
            loadedQ = false;
            failedQ = false;
        }

        bool LoadedQ() const
        {
            return loadedQ;
        }
        
        bool FailedQ() const
        {
            return failedQ;
        }
        
        Size_T KeyCount() const
        {
            return values.size();
        }
        
        constexpr Size_T KeyLength() const
        {
            return key_length;
        }

        Size_T BufferSize() const
        {
            return buffer.size();
        }
        
        Size_T BufferCapacity() const
        {
            return buffer.capacity();
        }
        
        const Key_T & LastKey() const
        {
            return last_key;
        }

        const std::array<Size_T,256> & PrefixPointers() const
        {
            return prefix_ptr;
        }
        
        const std::array<Size_T,256> & PrefixPositions() const
        {
            return prefix_pos;
        }
        
        const std::vector<UInt> & Buffer() const
        {
            return buffer;
        }
        
        const std::vector<Value_T> & Values() const
        {
            return values;
        }
        
#include "FrontEncodedLookupTable/Load.hpp"
#include "FrontEncodedLookupTable/Find.hpp"
                       
    public:
        
        using Msgr = Tools::Messenger<Class_T>;

        template<typename A>
        static auto MethodName( const A & tag )
        {
            return Msgr::MethodName(tag);
        }
        
        static auto ClassName()
        {
            return std::string("FrontEncodedLookupTable<") + Tools::ToString(key_length) + ">";
        }
    
    }; // class FrontEncodedLookupTable

} // namespace Knoodle
