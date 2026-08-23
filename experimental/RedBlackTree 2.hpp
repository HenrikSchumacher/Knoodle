#pragma once

namespace Knoodle
{
    template<typename Data_T_, IntQ Int_>
    class RedBlackTree
    {
    public:
        
        
        // Code learnt from https://www.geeksforgeeks.org/dsa/introduction-to-red-black-tree/
        using Data_T  = Data_T_;
        using Int     = Int_;
        
        enum class State_T : UInt8
        {
            Inactive  = 0,
            Black     = 1,
            Red       = 2
        };
        
        static constexpr Int Uninitialized = -1;
        static constexpr Int NIL           = -2;
        
        static constexpr bool Left         = 0;
        static constexpr bool Right        = 1;
        
        struct Node_T
        {
            Int child [2] = { Uninitialized, Uninitialized };
            Data_T data;
            State_T state = State_T::Inactive;
            
            Int & operator[]( bool side )
            {
                return child[side];
            }
            
            const Int & operator[]( bool side ) const
            {
                return child[side];
            }
        };
        
        struct NodePair
        {
            Int parent;
            Int node;
        };
        
    private:
        
        Tensor1<Node_T,Int> node_buffer;
        Stack<Int,Int>      deleted_nodes;
        Stack<Int,Int>      parent_stack;
        Int node_count      = 0;
        Int node_end        = 0;
        Int root            = NIL;
        
    public:
        
        RedBlackTree() = default;
        
        ~RedBlackTree() {}
        
    public:
        
        Int NodeCount() const
        {
            return node_count;
        }
        
        Int NodeCapacity() const
        {
            return node_buffer.Size();
        }
        
        Int Root() const
        {
            return root;
        }
        
        Int Nil() const
        {
            return NIL;
        }
        
        Int UninitializedValue() const
        {
            return Uninitialized;
        }
        
        
    public:
        
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
        
    private:
     
        
        void RotateLeft( const Int p, const Int x )
        {
            /*!     p                 p
             *      |                 |
             *      |                 |
             *      x                 y
             *       \               / \
             *        \             /   \
             *         y     ==>   x     b
             *        / \           \
             *       /   \           \
             *      a     b           a
             *
             */
            
            Int y = Child(x,Right);
            Int a = Child(y,Left );
            
            Child(x,Right) = a;
            
            if( p == Uninitialized )
            {
                // x is root ->y becomes root.
                root = y;
            }
            else
            {
                SetMatchingChild(p,x,y);
            }
            
            Child(y,Left) = x;
        }
        
        void RotateRight( const Int p, const Int x )
        {
            /*!           p            p
             *            |            |
             *            |            |
             *            x            y
             *           /            / \
             *          /            /   \
             *         y     ==>    a     x
             *        / \                /
             *       /   \              /
             *      a     b            b
             *
             */
            
            Int y = Cild(x,Left );
            Int b = Cild(y,Right);
            
            Child(x,Left) = b;
            
            if( p == Uninitialized )
            {
                // x is root ->y becomes root.
                root = y;
            }
            else
            {
                SetMatchingChild(p,x,y);
            }
            
            Child(y,Right) = x;
        }
        
        
    private:
        
        /*!@brief If right child of `node` coincides with `x`, then set it to `y`. Otherwise, set left child to `y`.
         *
         * This implicitly assumes that `node` is a valid node and that one of its children is `x`.
         */
        void SetMatchingChild( const Int node, const Int x, const Int y )
        {
            Child(node, Child(node,Right) == x ) = y;
        }
        
        Int NewNodeId()
        {
            ++node_count;
            
            Int node;
            if( !deleted_nodes.EmptyQ() )
            {
                node = deleted_nodes.Pop();
            }
            else
            {
                if( node_end >= node_buffer.Size() )
                {
                    node_buffer.template Resize<true>(
                        Int(2) * Max(node_buffer.Size(),Int(1))
                    );
                }
                node = node_end;
                ++node_end;
            }
            return node;
        }
        
        Int CreateNode( cref<Data_T> data )
        {
            const Int node = NewNodeId();
            Node_T & N     = node_buffer[node];
            N.child[Left ] = NIL;
            N.child[Right] = NIL;
            N.data         = data;
            N.state        = State_T::Red;
            return node;
        }
        
//        Int CreateNode( Data_T && data )
//        {
//            const Int node = NewNodeId();
//            Node_T & N     = node_buffer[node];
//            N.child[Left ] = NIL;
//            N.child[Right] = NIL;
//            N.data         = std::move(data);
//            N.state        = State_T::Red;
//            return node;
//        }
        
    public:
        
        template<typename F, typename C>
        void Insert( cref<Data_T> data, F && f, C && cmp )
        {
            Int node    = CreateNode(data);
            Int parent  = Uninitialized;
            Int current = root;
            
            auto target_val = std::invoke(f, Data(node));
            
            bool side;
            
            // Step 1: Standard Binary Search Tree insertion
            while( current != NIL )
            {
                auto val = std::invoke(f, Data(current));
                
                parent = current;
                side   = std::invoke(cmp, val, target_val);
                current = Child(current,side);
            }

            if( parent == Uninitialized )
            {
                root = node;
            }
            else
            {
                Child(parent,side) = node;
            }
            
//            // Step 2: Handle edge cases and fix RB properties
//            if (node->parent == Uninitialized)
//            {
//                node->color = black;
//                return;
//            }
//            
//            if (node->parent->parent == nullptr)
//            {
//                return;
//            }

//            Repair(parent,node);
        }
        
    public:
        
        template<typename F, typename C>
        NodePair Find( cref<Data_T> data, F && f, C && cmp )
        {
            auto target_val = std::invoke(f, data);
            Int parent = Uninitialized;
            Int node   = root;
        
            while( node != NIL )
            {
                auto val = std::invoke(f, Data(node));
                
                if( val == target_val ) { return {parent,node}; }
                
                const bool side = std::invoke(cmp, val, target_val);
                
                parent = node;
                node   = Child(node, side);
            }
            
            return {parent,node};
        }
        
    public:

        Int MinNode() const
        {
            return FindMin().node;
        }
        
        NodePair FindMin() const
        {
            return FindMin(Uninitialized, root);
        }
        
        NodePair FindMin( const Int parent, const Int node ) const
        {
            return WalkToEnd<Left>(parent, node);
        }
        
        
        Int MaxNode() const
        {
            return FindMax().node;
        }
        
        NodePair FindMax() const
        {
            return FindMax(Uninitialized, root);
        }
        
        NodePair FindMax( const Int parent, const Int node ) const
        {
            return WalkToEnd<Right>(parent, node);
        }
        
    private:
        
        template<bool side>
        NodePair WalkToEnd( const Int parent_, const Int node_ ) const
        {
            Int parent = parent_;
            Int node   = node_;
            
            if( node == NIL ) { return {parent,node}; }
            
            Int child = Child(node,side);
            
            while( child != NIL )
            {
                parent = node;
                node   = child;
                child  = Child(node,side);
            }
            
            return {parent,node};
        }
        
    private:
        
        void Deactivate( Int node )
        {
            if( node == NIL ) { return; }
            
            if( node == Uninitialized ) { return; }
            
            node_buffer[node].state = State_T::Inactive;
            deleted_nodes.Push(node);
        }
        
//        template<typename F, typename C>
//        void Delete( cref<Data_T> data, F && f, C && cmp )
//        {
//            auto [parent, node] = Find( data, std::forward<F>(f), std::forward<C>(cmp) );
//            
//            if( node == NIL ) { return; }
//            
//            const bool leftQ  = (Child(node,Left ) != NIL);
//            const bool rightQ = (Child(node,Right) != NIL);
//            
//            if( leftQ )
//            {
//                if( parent != Uninitialized )
//                {
//                    const bool side = (Child(parent,Right) == node);
//                    Child(parent,side) = Child(node,Left );
//                }
//                Deactivate(node);
//                
//            }
//            else if( rightQ )
//            {
//                if( parent != Uninitialized )
//                {
//                    const bool side = (Child(parent,Right) == node);
//                    Child(parent,side) = Child(node,Right);
//                }
//                Deactivate(node);
//            }
//            else if( leftQ && rightQ )
//            {
//                // Node `node` has two children.
//                // Not so obvious what to do next!
//                
//                auto [prev_parent,prev] = FindMax( Child(node,Left) );
//                // v and prev cannot be NIL.
//                {
//                    const bool side = (Child(prev_parent,Right) == prev);
//                    Child(prev_parent,side) = Child(prev,Left);
//                }
//                
//                if( parent != Uninitialized )
//                {
//                    const bool side = (Child(parent,Right) == node);
//                    Child(parent,side) = prev;
//                }
//                
//                Deactivate(node);
//            }
//            else
//            {
//                if( parent != Uninitialized )
//                {
//                    const bool side = (Child(parent,Right) == node)
//                    Child(parent,side) = NIL;
//                }
//                Deactivate(node);
//            }
//        }
        
    }; // class RedBlackTree

} // namespace Knoodle
