#pragma once


namespace Knoodle
{
    class Binarizer
    {
    public:
        
        using Char  = char;
        using Digit = UInt8;
        
//        static constexpr UInt64 digit_bit_count = 8;

        static constexpr UInt64 digit_bit_count = 6;
        
        static constexpr UInt64 base = (UInt64(1) << digit_bit_count);
        static constexpr UInt64 digit_mask = base - UInt64(1);
        
        // Using the encoding of Base64 without padding.
        static constexpr Char to_char [64] = {
            'A','B','C','D','E','F','G','H',
            'I','J','K','L','M','N','O','P',
            'Q','R','S','T','U','V','W','X',
            'Y','Z','a','b','c','d','e','f',
            'g','h','i','j','k','l','m','n',
            'o','p','q','r','s','t','u','v',
            'w','x','y','z','0','1','2','3',
            '4','5','6','7','8','9','+','/'
        };
        
        static constexpr Digit from_char [128] = {
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0,62, 0, 0, 0,63,
            52,53,54,55,56,57,58,59,
            60,61, 0, 0, 0, 0, 0, 0,
             0, 0, 1, 2, 3, 4, 5, 6,
             7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,
            23,24,25, 0, 0, 0, 0, 0,
             0,26,27,28,29,30,31,32,
            33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,
            49,50,51, 0, 0, 0, 0, 0,
        };
        
    public:
        
        static constexpr Digit FromChar( const Char c )
        {
            return from_char[static_cast<UInt8>(c)];
        }
        
        static constexpr Char ToChar( const Digit k )
        {
            return to_char[k];
        }
        
//        static constexpr Digit FromChar( const Char c )
//        {
//            return static_cast<Digit>(c);
//        }
//        
//        static constexpr Char ToChar( const Digit k )
//        {
//            return static_cast<Char>(k);
//        }
        
        template<typename UInt>
        static constexpr Size_T DigitCount( const UInt max_number )
        {
            Size_T m = static_cast<Size_T>(max_number);
            
            Size_T digit_count = 0;
            
            while( m )
            {
                ++digit_count;
                m = (m >> digit_bit_count);
            }
            
            return digit_count;
        }
        
        
        template<typename UInt, typename Int>
        static void WriteCharSequence(
            cptr<UInt> a, const Int int_count, const Size_T digit_count, mptr<Char> s
        )
        {
            // Only grabing the lower bits of entries in a.
            // Hence, negative number are not handled correctly!!
            
            static_assert(UnsignedIntQ<UInt>);
            static_assert(IntQ<Int>);
            
            for( Int i = 0; i < int_count; ++i )
            {
                UInt64 k = static_cast<UInt64>(a[i]);
                
//                TOOLS_DUMP(k);
                
                for( Size_T j = 0; j < digit_count; ++j )
                {
                    Digit d = static_cast<Digit>(k & digit_mask);

                    Char c  = ToChar(d);
                    
                    if( FromChar(c) != d )
                    {
                        eprint("A!");
                        TOOLS_DUMP(d);
                        TOOLS_DUMP(c);
                        print(std::string("c  = ") + c);
                        TOOLS_DUMP(FromChar(c));
                    }
                    
                    s[digit_count * i + j] = c;
                    
                    k = (k >> digit_bit_count);
                }
            }
        }
        
        template<typename UInt, typename Int, typename Int2>
        static void ReadCharSequence(
            mptr<UInt> a, const Int int_count, const Int2 digit_count, cptr<Char> s
        )
        {
            static_assert(UnsignedIntQ<UInt>);
            static_assert(IntQ<Int>);
            static_assert(IntQ<Int2>);
             
            for( Int i = 0; i < int_count; ++i )
            {
                UInt64 k = 0;
                
                for( Size_T j = digit_count; j -->Size_T(0); )
                {
                    k = (k << digit_bit_count);
                    
                    Digit d = FromChar(s[digit_count * i + j]);
                    
                    k = k | d;
                }
                
                a[i] = k;
            }
        }
        
        template<typename UInt, typename Int>
        static Tensor1<Char,Int> ToCharSequence(
            cptr<UInt> a, const Int int_count, const Size_T digit_count
        )
        {
            // Only grabing the lower bits of entries in a.
            // Hence, negative number are not handled correctly!!
            
            static_assert(UnsignedIntQ<UInt>);
            
            Tensor1<Char,Int> s ( int_count * digit_count );
            
            WriteCharSequence( a, int_count, digit_count, &s[0] );
            
            return s;
        }
        
        template<typename UInt, typename Int>
        static std::string ToString(
            cptr<UInt> a, const Int int_count, const Size_T digit_count
        )
        {
            // Only grabing the lower bits of entries in a.
            // Hence, negative number are not handled correctly!!
            
            static_assert(UnsignedIntQ<UInt>);
            
            std::string s ( int_count * digit_count, 'A' );
            
            WriteCharSequence( a, int_count, digit_count, &s[0] );
            
            return s;
        }
        
        template<typename UInt, typename Int, typename Int2>
        static Tensor1<UInt,Int> FromCharSequence(
            cptr<Char> s, const Int int_count, const Int2 digit_count
        )
        {
            static_assert(UnsignedIntQ<UInt>);
            static_assert(IntQ<Int>);
            static_assert(IntQ<Int2>);
            
            Tensor1<UInt,Int> a ( int_count );
            
            ReadCharSequence( &a[0], int_count, digit_count, &s[0] );
            
            return a;
        }
        
        template<typename UInt, typename Int, typename Int2>
        static Tensor1<UInt,Int> FromCharSequence(
            cref<Tensor1<Char,Int>> s, const Int char_count, const Int2 digit_count
        )
        {
            return FromCharSequence( &s[0], char_count, digit_count );
        }
        
        
        template<typename UInt, typename Int, typename Int2>
        static Tensor1<UInt,Size_T> FromString(
            cref<std::string> s, const Int2 digit_count
        )
        {
            return FromCharSequence(
                &s[0], int_cast<Int>(s.size())/digit_count, digit_count
            );
        }
        
    }; // class Binarizer
    
}
