#pragma  once


namespace Knoodle
{
    /*!@brief A class for a lookupt table whose keys are fixed-size byte sequences. This is optimized for low storage space and descently fast lookup. An insertion operation has been omitted intentionally.
     */
    
    template<IntQ Int_,typename Value_T_>
    class FrontEncodedLookupTable final
    {
        
    public:
        
        using Int     = Int_;
        using Byte_T  = unsigned char;
        using Value_T = Value_T_;
        
        using Class_T = FrontEncodedLookupTable;
        
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
        
        static constexpr Int default_block_size = 64;
        
    private:
        
        Int key_count   = 0;
        Int key_size    = 0;
        Int block_count = 0;
        Int block_size  = default_block_size;
        Int last_block_size = 0;
        
        // For each prefix byte `p` this table stores the first position of an entry greater or equal to `[p,0,0...]`.
        
        Tensor2<Byte_T ,Int> block_keys;
        Tensor1<Int    ,Int> block_ptr;
        Tensor1<Byte_T ,Int> buffer;
        Tensor1<Value_T,Int> values;
        mutable Tensor1<Byte_T,Int> last_key;
        mutable Tensor1<Byte_T,Int> this_key;
        
        bool loadedQ = false;
        bool failedQ = false;
        
    public:
        
        FrontEncodedLookupTable() = default;
        
        ~FrontEncodedLookupTable() = default;
        
        template<IntQ I_0, IntQ I_1, IntQ I_2>
        int Reset(
            const I_0 key_count_,
            const I_1 key_size_,
            const I_2 block_size_ = default_block_size
        )
        {
            loadedQ = false;
            failedQ = true;
            
            // TODO: We have to check whether Int is large enough.
            
            if( std::cmp_less(key_size_, 0) ) { return 1; }

            if( std::cmp_greater(key_size_, 256) ) { return 2; }
        
            if( std::cmp_less(key_count_, 0) ) { return 3; }
            
            if( std::cmp_less(block_size_, 0) ) { return 4; }
            
            key_count   = static_cast<Int>(Ramp(key_count_));
            key_size    = Max(Int{1},static_cast<Int>(Ramp(key_size_)));
            block_size  = Max(Int{1},static_cast<Int>(Ramp(block_size_)));
            block_count = CeilDivide(key_count,block_size);
            last_block_size = (block_count > Int{0}) ? (key_count - (block_count - Int{1}) * block_size) : Int{0};
            
            block_keys  = Tensor2<Byte_T ,Int>(block_count + Int{1}, key_size);
            block_ptr   = Tensor1<Int    ,Int>(block_count + Int{1});
            buffer      = Tensor1<Value_T,Int>((key_count - block_count) * key_size, -1);
            values      = Tensor1<Value_T,Int>(key_count);
            last_key    = Tensor1<Byte_T ,Int>(key_size);
            this_key    = Tensor1<Byte_T ,Int>(key_size);
        
            loadedQ     = false;
            failedQ     = false;
            
            return 0;
        }

        bool LoadedQ() const
        {
            return loadedQ;
        }
        
        bool FailedQ() const
        {
            return failedQ;
        }
        
        Int KeyCount() const
        {
            return key_count;
        }
        
        Int KeySize() const
        {
            return key_size;
        }
        
        Int BlockCount() const
        {
            return block_count;
        }
        
        Int BlockSize() const
        {
            return block_size;
        }

        Int BufferSize() const
        {
            return buffer.Size();
        }

        const Tensor1<Int,Int> & BlockPointers() const
        {
            return block_ptr;
        }
        
        const Tensor1<Byte_T,Int> & LastKey() const
        {
            return last_key;
        }

        const Tensor2<Byte_T,Int> & BlockKeys() const
        {
            return block_keys;
        }
        
        const Tensor1<Byte_T,Int> & Buffer() const
        {
            return buffer;
        }
        
        const Tensor1<Value_T,Int> & Values() const
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
            return ct_string("FrontEncodedLookupTable<") + TypeName<Int> + "," + TypeName<Value_T> + ">";
        }
    
    }; // class FrontEncodedLookupTable

} // namespace Knoodle
