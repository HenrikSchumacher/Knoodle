#pragma once

namespace Knoodle
{
    template<typename Data_T_, IntQ Int_, bool bound_checksQ_ = false>
    class RedBlackTree
    {
    public:

        using Data_T  = Data_T_;
        using Int     = Int_;
        using UInt    = ToUnsigned<Int>;
        
        static constexpr bool bound_checksQ = bound_checksQ_;
        
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
        static constexpr Int  NIL       = static_cast<Int>(node_mask);
        static constexpr bool Left      = 0;
        static constexpr bool Right     = 1;
        
        static constexpr bool debugQ        = false;
        
        static constexpr Int max_path_size = 128;
        
    private:
        
//        Stack<Int,Int>
        std::vector<Int> deleted_nodes;
        
        Int node_count = 0;
        Int node_end   = 0;
        
        UInt path [max_path_size] = {PNIL};
        Int  path_ptr  = 2;
        Int  root      = NIL;
        Int  current   = NIL;
        
        UInt    dummy_path  = PNIL;
        Int     dummy_node  = NIL;
        Data_T  dummy_data;
        State_T dummy_state = State_T::Inactive;
        
    public:
        
        RedBlackTree()
        {
            InitializePath();
        }
        
        ~RedBlackTree() {}
        

//#include "RedBlackTree/Containers_SoA.hpp"
#include "RedBlackTree/Containers_AoS.hpp"
//#include "RedBlackTree/Containers_AoS2.hpp"
#include "RedBlackTree/Path.hpp"
#include "RedBlackTree/Rotate.hpp"
#include "RedBlackTree/Insert.hpp"
#include "RedBlackTree/Find.hpp"
#include "RedBlackTree/Delete.hpp"
        
    public:
        
        Int NodeCount() const { return node_count; }
        
        Int Root() const { return root; }
        
        static constexpr Int Nil() { return NIL; }
        
        static constexpr Int PathNil() { return PNIL; }
        
        static constexpr State_T Red() { return State_T::Red; }
        
        static constexpr State_T Black() { return State_T::Black; }
        
        bool NodeActiveQ( const Int node )
        {
            return (node == NIL) ? false : (node_state(node) != State_T::Inactive);
        }
        
        void Clear()
        {
            deleted_nodes.clear();
            root       = NIL;
            node_count = 0;
            node_end   = 0;
            ResetPath();
        }
        
        bool InRangeQ( const Int node ) const
        {
            if( (node < Int{0}) || (node >= node_end) )
            {
                eprint(this->MethodName("InRangeQ") + ": node = " + ToString(node)+ " is out of bounds.");
                
                return false;
            }
            return true;
        }
        
        bool RedQ( const Int node ) // const
        {
            return (node == NIL) ? false : (State(node) == State_T::Red);
        }
        
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
