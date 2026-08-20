#pragma once

namespace Knoodle
{
    template<typename Label_T_>
    class RedBlackTree
    {
    public:
        
        using Label_T = Label_T_;
        
        static constexpr bool black = true;
        static constexpr bool red   = false;
        
        struct Node
        {
            Node * parent = nullptr;
            Node * left   = nullptr;
            Node * right  = nullptr;
            Label_T label;
            bool color = red ;
            
            Node( const T & label_ )
            :   label  { label_  }
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
            
            root = NIL
            
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
        
        
    public:
        
        cptr<Node> Nil() const
        {
            return NIL;
        }
        
        template<typename F, typename C>
        Node * Find( cref<Label_T> label_, F && f, C && cmp )
        {
            return find(
                root,
                std::invoke(f,label_),
                std::forward<F>(f),
                std::forward<C>(cmp)
            );
        }
        
    private:
        
        template<typename Value_T, typename F, typename C = std::less>
        Node * Find_imp( Node * node, cref<Value_T> target_value, F && f, C && cmp )
        {
            if( node == NIL ) { return node; }
            
            Value_T value = f(node->label);
            
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
