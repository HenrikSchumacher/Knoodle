#pragma once

namespace Knoodle
{
    template<typename Data_T_, IntQ Int_>
    class RedBlackTree
    {
    public:

        using Data_T  = Data_T_;
        using Int     = Int_;
        using UInt    = ToUnsigned<Int>;

        enum class State_T : UInt8
        {
            Inactive  = 0,
            Black     = 1,
            Red       = 2
        };
        
        friend std::string ToString( State_T state )
        {
            switch (state)
            {
                case State_T::Inactive: return "Inactive";
                case State_T::Black:    return "Black";
                case State_T::Red:      return "Red";
                default:                return "Unknown";
            }
        }
        
//        static constexpr Int Uninitialized = -1;
        static constexpr UInt PNIL      = ~UInt{0};
        static constexpr UInt node_mask = (PNIL >> 1);
        static constexpr UInt side_mask = ~node_mask;
        static constexpr Int NIL        = static_cast<Int>(node_mask);
        static constexpr bool debug     = false;
        static constexpr bool Left      = 0;
        static constexpr bool Right     = 1;
        
        static constexpr Int max_path_size = 64;
        
        struct Node_T
        {
            Int child [2] = { NIL, NIL };
            Data_T data;
            State_T state = State_T::Inactive;
            
            mref<Int> operator[]( bool side )       { return child[side]; }
            
            cref<Int> operator[]( bool side ) const { return child[side]; }
        };
        
    private:
        
        Tensor1<Node_T,Int> node_buffer;
        Stack<Int,Int>      deleted_nodes;
        Int node_count      = 0;
        Int node_end        = 0;
        
        UInt path [max_path_size+2] = {PNIL};
        Int  path_ptr = 2;
        Int  root     = NIL;
        Int  current  = NIL;
        
        Data_T dummy;
        
    public:
        
        RedBlackTree() { InitializePath(); }
        
        ~RedBlackTree() {}
        
    public:
        
        Int NodeCount() const { return node_count; }
        
        Int NodeCapacity() const { return node_buffer.Size(); }
        
        Int Root() const { return root; }
        
        static constexpr Int Nil() { return NIL; }
        
        static constexpr State_T Red() { return State_T::Red; }
        
        static constexpr State_T Black() { return State_T::Black; }
        
        void Clear()
        {
            deleted_nodes.Clear();
            root       = NIL;
            node_count = 0;
            node_end   = 0;
            ResetPath();
        }
        
        void Reserve( const Int size )
        {
            // TODO: Add a check whether this size can be stored in the integer type.
            
            if( node_buffer.Size() < size )
            {
                node_buffer.template Resize<true>(size);
            }
        }
        
        Node_T & Node( const Int node )
        {
            // This one is private because only the tree itself is allowed to change this.
            return node_buffer[node];
        }

        Int & Child( const Int node, const bool side )
        {
            // This one is private because only the tree itself is allowed to change this.
            return node_buffer[node][side];
        }
        
        Data_T & Data( const Int node )
        {
//            if( (node < Int{0}) || (node >= node_end) )
//            {
//                eprint(MethodName("Data") + ": node = " + ToString(node)+ " does not exist.");
//                return dummy;
//            }
            
            // This one is private because only the tree itself is allowed to change this.
            return node_buffer[node].data;
        }
        
        State_T & State( const Int node )
        {
            // This one is private because only the tree itself is allowed to change this.
            return node_buffer[node].state;
        }
        
    public:
        
        const Node_T & Node( const Int node ) const
        {
            return node_buffer[node];
        }
        
        const Int & Child( const Int node, const bool side ) const
        {
            return node_buffer[node][side];
        }
        
        const Data_T & Data( const Int node ) const
        {
            return node_buffer[node].data;
        }
        
        const State_T & State( const Int node ) const
        {
            return node_buffer[node].state;
        }
        
#include "RedBlackTree/Path.hpp"
#include "RedBlackTree/Insert.hpp"
#include "RedBlackTree/Find.hpp"
#include "RedBlackTree/Rotate.hpp"
        
    public:
        
        
        /*!@brief Return a string that identifies a class method specified by `tag`. Mostly used for logging and in error messages.*/
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*!@brief Return a string that identifies this class with type information. Mostly used for logging and in error messages.*/
        static constexpr std::string ClassName()
        {
            return std::string("RedBlackTree")
                + "<" + TypeName<Data_T>
                + "," + TypeName<Int>
                + ">";
        }
        
    }; // class RedBlackTree

} // namespace Knoodle
