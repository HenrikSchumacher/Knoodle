#pragma once

namespace Knoodle
{
    template<typename Data_T_>
    class RedBlackTree
    {
    public:
        
        // Code learnt from https://www.geeksforgeeks.org/dsa/introduction-to-red-black-tree/
        using Data_T = Data_T_;
        
        static constexpr bool black = true;
        static constexpr bool red   = false;
        
        struct Node
        {
            Node * parent = nullptr;    // Probably not needed.
            Node * left   = nullptr;
            Node * right  = nullptr;
            Data_T data;
            bool color = red ;
            
            Node( const T & data_ )
            :   data   { data_   }
            ,   parent { nullptr }
            ,   left   { nullptr }
            ,   right  { nullptr }
            ,   color  { red     }
            {}
        }
        
    private:
        
        Node * root = nullptr;
        Node * NIL  = nullptr;
        
    public:
        
        RedBlackTree()
        {
            NIL = new Node( Label_T{0} );
            NIL->color = black;
            NIL->left  = NIL;
            NIL->right = NIL;
            
            root = NIL;
        }
        
        ~RedBlackTree()
        {
            DeleteNode(root);
            
            if( NIL != nullptr ) { delete NIL; }
        }
        
    private:
        
        void DeleteNode( Node * node )
        {
            if( node == nullptr ) { return; }
            
            // First delete the children, but we have to make sure that NIL is not deleted.
            if( (node->right != nullptr) && (node->right != NIL) )
            {
                DeleteNode(node->right);
            }
            
            if( (node->left != nullptr) && (node->left != NIL) )
            {
                DeleteNode(node->left);
            }
            
            delete node;
        }
        
    private:

        void RotateLeft( Node * x )
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
            
            Node * y = x;
            Node * a = y->left;
            Node * p = x->parent;
            
            x.right = a;
            if( a != NIL ) { a->parent = x; }
            
            y->parent = p;
            if( p == nullptr )
            {
                // x is root ->y becomes root.
                root = y;
            }
            else
            {
                if( p->left == x )
                {
                    p->left = y;
                }
                else
                {
                    p->right = y;
                }
            }
            
            y->left   = x;
            x->parent = y;
        }
        
        void RotateRight( Node * x )
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
            
            Node * y = x;
            Node * b = y->right;
            Node * p = x->parent;
            
            x->left = b;
            if( b != nullptr ) { b->parent = x; }
            
            if( p == nullptr )
            {
                // x is root ->y becomes root.
                root = y;
                y->parent = nullptr;
            }
            else
            {
                y->parent = p;
                
                if( p->left == x )
                {
                    p->left = y;
                }
                else
                {
                    p->right = y;
                }
            }
            
            y->right  = x;
            x->parent = y;
        }
        
        
        template<typename F, typename C>
        void Insert( cref<Data_T> data, F && f, C && cmp )
        {
            Node * node    = new Node(data);
            node->left     = NIL;
            node->right    = NIL;
            
            Node * parent  = nullptr;
            Node * current = root;
            
            // Step 1: Standard Binary Search Tree insertion
            while( current != NIL )
            {
                parent = current;
                if( cmp( f(node->data), f(current->data) ) )
                {
                    current = current->left;
                }
                else
                {
                    current = current->right;
                }
            }
            
            node->parent = parent;

            if( parent == nullptr )
            {
                root = node;
            }
            else if( cmp( f(node->data), f(parent->data) ) )
            {
                parent->left = node;
            }
            else
            {
                parent->right = node;
            }
            
            // Step 2: Handle edge cases and fix RB properties
            if (node->parent == nullptr)
            {
                node->color = black;
                return;
            }
            
            if (node->parent->parent == nullptr)
            {
                return;
            }

            Repair(node);
        }
        
    private:
        
        
        void Repair( Node * node )
        {
            // Continue while the parent is Red (violates RB property)
            while( node != root && node->parent->color == red )
            {
                if( node->parent == node->parent->parent->left )
                {
                    Node * uncle = node->parent->parent->right;
                    
                    if( uncle->color == red )
                    {
                        // Case 1: Uncle is Red -> Recolor parent, uncle, and grandparent
                        node->parent->color = black;
                        uncle->color = black;
                        node->parent->parent->color = red;
                        node = node->parent->parent;
                    }
                    else
                    {
                        if( node == node->parent->right )
                        {
                            // Case 2: Triangle shape -> Left rotate parent to form a line
                            node = node->parent;
                            RotateLeft(node);
                        }
                        // Case 3: Line shape -> Recolor and right rotate grandparent
                        node->parent->color = black;
                        node->parent->parent->color = red;
                        RotateRight(node->parent->parent);
                    }
                }
                else
                {
                    // Mirror Case: Parent is the right child
                    Node * uncle = node->parent->parent->left;
                    
                    if( uncle->color == red )
                    {
                        node->parent->color = black;
                        uncle->color = black;
                        node->parent->parent->color = red;
                        node = node->parent->parent;
                    }
                    else
                    {
                        if( node == node->parent->left )
                        {
                            node = node->parent;
                            RotateRight(node);
                        }
                        node->parent->color = black;
                        node->parent->parent->color = red;
                        RotateLeft(node->parent->parent);
                    }
                }
            }
            
            root->color = black; // Root must always be black.
        }
        
    public:
        
        Node * Root()
        {
            return root;
        }
        
        Node * Nil()
        {
            return NIL;
        }

        Node * FindMin()
        {
            Node node = root;
            
            if( node = nullptr ) { return node }
            
            while( node.left != NIL ) { node = node.left }
            
            return node;
        }
        
        Node * FindMax()
        {
            Node node = root;
            
            if( node = nullptr ) { return node }
            
            while( node.right != NIL ) { node = node.right }
            
            return node;
        }
        
        template<typename F, typename C>
        Node * Find( cref<Label_T> data_, F && f, C && cmp )
        {
            return find(
                root,
                std::invoke(f,data_),
                std::forward<F>(f),
                std::forward<C>(cmp)
            );
        }
        
    private:
        
        template<typename Value_T, typename F, typename C = std::less>
        Node * Find_imp( Node * node, cref<Value_T> target_value, F && f, C && cmp )
        {
            if( node == NIL ) { return node; }
            
            const Value_T value = f(node->data);
            
            if( value == target_value ) { return node; }
            
            if( cmp(value,target_value) )
            {
                Find_imp(
                    node->left,
                    target_value,
                    std::forward<F>(f),
                    std::forward<C>(cmp)
                );
            }
            else
            {
                Find_imp(
                    node->right,
                    target_value,
                    std::forward<F>(f),
                    std::forward<C>(cmp)
                );
            }
        }
    }

} // namespace Knoodle
