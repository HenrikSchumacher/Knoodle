#pragma once

#include "RedBlackTree.hpp"

namespace Knoodle
{
    template<
        typename Key_T_, typename Val_T_, IntQ Int_,
        typename Cmp = std::less<Key_T_>
    >
    class RedBlackMap
    {
        using Key_T = Key_T_;
        using Val_T = Val_T_;
        using Int   = Int_;
        
        struct KeyVal_T
        {
            Key_T key;
            Val_T val;
        };
        
        auto fun = []( cref<KeyVal_T> key_val )
        {
            return key_val.key;
        };
        
        using Tree_T = RedBlackTree<KeyVal_T,Int,decltype(fun),Cmp>;
        
    private:
        
        mutable Tree_T T;
        
    public:
        RedBlackMap() = default;
        
        ~RedBlackMap() = default;
        
        int Insert( cref<Key_T> key, cref<Val_T> key )
        {
            return T.Insert( KeyVal_T{key,val}, true );
        }
        
        bool ContainsQ( cref<Key_T> key  ) const
        {
            return T.ContainsQ(key);
        }

        Val_T & operator[]( cref<Key_T> key )
        {
            if( T.Current() == NIL )
            {
                Data_T & x = GetData(T.Current());
                
                if( x.key == key ) { return x.val; }
            }
            
            if( !T.Find(key) )
            {
                int flag = T.Insert( KeyVal_T{key,ValT{}} );
                // Here it would be nice if Insert would initialize the path correctly.
                if( flag == 1 ) { T.Find(key); }
            }
            return T.GetData(T.Current()).val;
        }
        
        const Val_T & operator[]( cref<Key_T> key ) const
        {
            if( T.Current() == NIL )
            {
                const Data_T & x = GetData(T.Current());
                
                if( x.key == key ) { return x.val; }
            }
            
            if( !T.Find(key) )
            {
                int flag = T.Insert( KeyVal_T{key,ValT{}} );
                // Here it would be nice if Insert would initialize the path correctly.
                if( flag == 1 ) { T.Find(key); }
            }
            return T.GetData(T.Current()).val;
        }
        
        void Delete( cref<Key_T> key )
        {
            (void)Delete( KeyVal_T{key,ValT{}} );
        }
        
        /*!@brief Return a string that identifies a class method specified by `tag`. Mostly used for logging and in error messages.*/
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*!@brief Return a string that identifies this class with type information. Mostly used for logging and in error messages.*/
        static constexpr std::string ClassName()
        {
            return std::string("RedBlackMap")
                + "<" + TypeName<Key_T>
                + "," + TypeName<Val_T>
                + "," + TypeName<Int>
                + ">";
        }
        
    }; // class RedBlackMap
    
} // namespace Knoodle
