#pragma once

namespace Knoodle
{
    template<
        typename Data_T_, IntQ Int_, bool bound_checksQ_ = false,
        typename F = std::identity, typename C = std::less<>
    >
    class RedBlackTree
    {
    public:

        using Data_T  = Data_T_;
        using Int     = Int_;
        using UInt    = ToUnsigned<Int>;
        using Fun_T   = F;
        using Cmp_T   = C;
        
        static constexpr bool bound_checksQ = bound_checksQ_;
        
        enum class State_T : UInt8
        {
            Black     = 0,
            Red       = 1,
            Invalid   = 2
        };
        
        friend std::string ToString( State_T state )
        {
            switch (state)
            {
                case State_T::Black:    return "Black";
                case State_T::Red:      return "Red";
                case State_T::Invalid:  return "Invalid";
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
        
        using DeletedNodeContainer_T = std::vector<Int>;
        
    private:
        
        Fun_T f;
        Cmp_T cmp;

        DeletedNodeContainer_T deleted_nodes;
        
        Int node_count = 0;
        Int node_end   = 0;
        
        UInt path [max_path_size] = {PNIL};
        Int  path_ptr  = 2;
        Int  root      = NIL;
        Int  current   = NIL;
        
        UInt    dummy_path  = PNIL;
        Int     dummy_node  = NIL;
        Data_T  dummy_data;
        State_T dummy_state = State_T::Black;
        
    public:
        
        /*!@brief Initialize tree.
         *
         * @param f A function; it is assumed that the nodes `N` in the tree are sorted by `f(N.data)`
         *
         * @param cmp A user-defined comparison function for the type returned by `f`. `cmp(a,b)` should return true if `a` is considered less than `b`.
         *
         */
        
        RedBlackTree( const Fun_T & f_ = Fun_T(), const Cmp_T & cmp_ = Cmp_T() )
        :   f   {f_  }
        ,   cmp {cmp_}
        {
            InitializePath();
        }
        
        ~RedBlackTree() {}
        

//#include "RedBlackTree/Nodes_SoA.hpp"
#include "RedBlackTree/Nodes_AoS.hpp"
//#include "RedBlackTree/Nodes_NoState.hpp"
#include "RedBlackTree/Path.hpp"
#include "RedBlackTree/Rotate.hpp"
#include "RedBlackTree/Insert.hpp"
#include "RedBlackTree/Find.hpp"
#include "RedBlackTree/Delete.hpp"
        
    public:
        
        Size_T ByteCount() const
        {
            return sizeof(Node_T) * static_cast<Size_T>(node_buffer.Size())
                + sizeof(NodeContainer_T)
                + sizeof(Int) * deleted_nodes.size()
                + sizeof(DeletedNodeContainer_T)
                + sizeof(UInt) * max_path_size
                + sizeof(Fun_T)
                + sizeof(Cmp_T)
                + sizeof(std::vector<Int>)
                + sizeof(Int) * 5
                + sizeof(UInt)
                + sizeof(Data_T)
                + sizeof(State_T);
        }
        
        Int NodeCount() const { return node_count; }
        
        Int Root() const { return root; }
        
        static constexpr Int Nil() { return NIL; }
        
        static constexpr Int PathNil() { return PNIL; }
        
        static constexpr State_T Red() { return State_T::Red; }
        
        static constexpr State_T Black() { return State_T::Black; }
        
        static constexpr Int BoundChecksQ() { return bound_checksQ; }
        
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
            return (node != NIL) && (GetState(node) == State_T::Red);
        }
        
        
        void WriteSortedList( mptr<Data_T> a )
        {
            ResetPath();
            Int counter = 0;
            bool side  = false; // Only needed when going upwards.
            bool downQ = true;

            while( counter < node_count )
            {
                if( downQ )
                {
                    if( GetChild(Current(),Left) != NIL )
                    {
                        PushPath(Left);
                    }
                    else if( GetChild(Current(),Right) != NIL )
                    {
                        a[counter++] = GetData(Current());
                        PushPath(Right);
                    }
                    else
                    {
                        downQ = false;
                        a[counter++] = GetData(Current());
                        side  = ParentSide();
                        if( Current() != Root() ) { PopPath(); }
                    }
                }
                else // if( !downQ )
                {
                    if( side == Left )
                    {
                        a[counter++] = GetData(Current());
                        if( GetChild(Current(),Right) != NIL )
                        {
                            downQ = true;
                            PushPath(Right);
                        }
                        else
                        {
//                            downQ = false;
                            side  = ParentSide();
                            if( Current() != Root() ) { PopPath(); }
                        }
                    }
                    else // if( side == Right )
                    {
                        side  = ParentSide();
                        if( Current() != Root() ) { PopPath(); }
                    }
                }
            }
        }

        Tensor1<Data_T,Int> SortedList()
        {
            Tensor1<Data_T,Int> a ( node_count );
            WriteSortedList(a.data());
            return a;
        }
        
        /*!@brief Move to the next node after `Current()`' with respect to the ordering declared by `f` and `cmp`. Modify the internal path stack accordingly.
         *
         * @return If succeeded: `true`. Otherwise `false`.
         */
        bool Next() { return this->templatewalkToNext<Right>(); }
        
        /*!@brief Move to the previous node before `Current()`' with respect to the ordering declared by `f` and `cmp`. Modify the internal path stack accordingly.
         *
         * @return If succeeded: `true`. Otherwise `false`.
         */
        bool Prev() { return this->templatewalkToNext<Left>(); }
        
    private:
        
        template<bool side>
        bool walkToNext()
        {
            // Remember the current node in case we did not find its next node.
            const Int target = Current();

            if( GetChild(Current(),side) != NIL )
            {
                PushPath(side);
                WalkToEnd<!side>();
                foundQ = true;
                return true;
            }
            else
            {
                if( Current() == Root() ) { return false; }

                while( ParentSide() == side )
                {
                    PopPath();
                    if( Current() == Root() ) { return false; }
                }

                PopPath();
                
                return ( Current() != Root() );
            }
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
