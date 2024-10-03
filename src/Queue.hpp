#pragma once


namespace KnotTools {
    
    template<typename Entry_T_, typename Int_>
    class Stack
    {
        static_assert(IntQ<Int_>, "");
        
    public:
        
        using Entry_T = Entry_T_;
        using Int     = Int_;

        Stack()
        :   a ( 1 )
        {
            a[0] = 0;
        }
        
        Stack( Int max_size )
        :   a ( max_size + 1 )
        {
            a[0] = 0;
        }
        
        ~Stack() = default;
        
        // Copy constructor
        Stack( const Stack & other )
        :   a   ( other.a   )
        ,   ptr ( other.ptr )
        {}
        
        // Move constructor
        Stack( Stack && other ) noexcept
        :   Stack()
        {
            swap(*this, other);
        }
        
        inline friend void swap( Stack & A, Stack & B) noexcept
        {
            using std::swap;
            
            if( &A == &B )
            {
                wprint( std::string("An object of type ") + ClassName() + " has been swapped to itself.");
            }
            else
            {
                swap( A.a  , B.a   );
                swap( A.ptr, B.ptr );
            }
        }
        
        
        Int Size() const
        {
            a.Size()-1;
        }
        
        void Reset()
        {
            ptr = 0;
        }
        
        void Push( cref<Entry_T> value )
        {
            PD_ASSERT( (0 <= ptr) && (ptr < a.Size()-1) );
            
            a[++ptr] = value;
        }

        Entry_T Top() const
        {
            return a[ptr];
        }
        
        Entry_T Pop()
        {
            PD_ASSERT( (0 <= ptr) && (ptr < a.Size()) );
            
            Entry_T r = a[ptr];
            
            ptr = (ptr > 0) ? (ptr-1) : 0;
            
            return r;
        }
        
        bool EmptyQ() const
        {
            return ptr <= 0;
        }
        
        Int ElementCount() const
        {
            return ptr;
        }
        
        
        std::string String() const
        {
            return ArrayToString( &a[1], {ptr} );
        }
        
        friend std::string ToString( const Stack & stack )
        {
            return stack.String();
        }
        
    private:
        
        Tensor1<Entry_T,Int> a;
        
        Int ptr = 0;
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Queue")
                + "<" + TypeName<Entry_T>
                + "," + TypeName<Int>
                + ">";
        }
        
        
    }; // class Stack
    
    template<typename Entry_T_, typename Int_>
    class Queue
    {
        static_assert(IntQ<Int_>, "");
        
    public:
        
        using Entry_T = Entry_T_;
        using Int     = Int_;

        Queue() = default;
        
        Queue( Int max_size )
        :   a ( max_size )
        {}
        
        ~Queue() = default;
        
        // Copy constructor
        Queue( const Queue & other )
        :   a         ( other.a         )
        ,   begin_ptr ( other.begin_ptr )
        ,   end_ptr   ( other.end_ptr   )
        {}
        
        // Move constructor
        Queue( Queue && other ) noexcept
        :   Queue()
        {
            swap(*this, other);
        }
        
        inline friend void swap( Queue & A, Queue & B) noexcept
        {
            using std::swap;
            
            if( &A == &B )
            {
                wprint( std::string("An object of type ") + ClassName() + " has been swapped to itself.");
            }
            else
            {
                swap( A.a        , B.a         );
                swap( A.begin_ptr, B.begin_ptr );
                swap( A.end_ptr  , B.end_ptr   );
            }
        }
        
        
        void Reset()
        {
            begin_ptr = 0;
            end_ptr = 0;
        }
        
        void Push( cref<Entry_T> value )
        {
            a[end_ptr++] = value;
        }
        
        Entry_T Pop()
        {
            return a[begin_ptr++];
        }
        
        bool EmptyQ() const
        {
            return begin_ptr >= end_ptr;
        }
        
    private:
        
        Tensor1<Entry_T,Int> a;
        
        Int begin_ptr = 0;
        Int end_ptr   = 0;
        
    public:
        
        static std::string ClassName()
        {
            return std::string("Queue")
                + "<" + TypeName<Entry_T>
                + "," + TypeName<Int>
                + ">";
        }
        
        
    }; // class Queue
    
    
} // namespace KnotTools



//    template<typename Priority_T_, typename Value_T_, typename Int_>
//    class BucketQueue
//    {
//        static_assert(UnsignedIntQ<Priority_T_>, "");
//        static_assert(UnsignedIntQ<Int_>, "");
//
//    public:
//
//        using Priority_T = Priority_T_;
//        using Value_T    = Value_T_;
//        using Int        = Int_;
//
//        using Bucket_T   = std::vector<SortedList<Value_T,Int>>;
//
//        BucketQueue( Priority_T max_priority_ )
//        :   max_priority( max_priority_ )
//        ,   min_priority( max_priority )
//        ,   buckets( max_priority )
//        {}
//
//        void Push( const Value_T value, const Priority_T priority )
//        {
//            min_priority = Min( priority, min_priority );
//
//            buckets[priority].Insert(value);
//        }
//
//        bool Drop( const Value_T value, const Priority_T priority )
//        {
//            if(  buckets[priority].EmptyQ() )
//            {
//                return false;
//            }
//            else
//            {
//                const bool foundQ = buckets[priority].Drop(value);
//
//                if( foundQ )
//                {
//                    if( priority == min_priority )
//                    {
//                        while(
//                            (buckets[min_priority].empty()) && (min_priority < max_priority)
//                        )
//                        {
//                            ++min_priority;
//                        }
//                    }
//                }
//
//                return foundQ;
//            }
//        }
//
//        bool ChangePriority(
//            const Value_T value,
//            const Priority_T old_priority,
//            const Priority_T new_priority
//        )
//        {
//            const bool foundQ = Drop(value, old_priority);
//
//            Push(value, new_priority);
//
//            return foundQ;
//        }
//
//        Priority_T MinPriority() const
//        {
////            while( (buckets[min_priority].empty()) && (min_priority < max_priority) )
////            {
////                ++min_priority;
////            }
////
////            // TODO: Returns max_priority if totally empty. We have to reset min_priority to  0 in this case, otherwise filling things up won't work later.
////            if( min_priority == max_priority )
////            {
////                min_priority = 0;
////            }
////
//            return min_priority;
//        }
//
//        Value_T Pop()
//        {
//            return buckets[MinPriority()].Pop();
//        }
//
//    private:
//
//        const Priority_T max_priority;
//
//        Priority_T min_priority = 0;
//
//        Bucket_T buckets;
//
//
//    }; // class BucketQueue





