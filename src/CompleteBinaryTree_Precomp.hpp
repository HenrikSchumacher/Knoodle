#pragma once

namespace KnotTools
{
    
    template<typename Int_>
    class alignas( ObjectAlignment ) CompleteBinaryTree_Precomp
    {
        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Int  = Int_;
        
        using UInt = Scalar::Unsigned<Int>;
        
        using HelperTree_T = CompleteBinaryTree<Int,false>;
        
        static constexpr Int max_depth = HelperTree_T::max_depth;
        
        CompleteBinaryTree_Precomp() = default;
        
        // TODO: What to do if leaf_node_count_ == 0?
        
        explicit CompleteBinaryTree_Precomp( const Int leaf_node_count_  )
        :                       T { leaf_node_count_      }
        ,              N_children { NodeCount(), 2        }
        ,                N_ranges { NodeCount(), 2        }
        ,                N_parent { NodeCount()           }
        ,                 N_depth { NodeCount()           }
        ,                N_column { NodeCount()           }
        ,                  P_node { LeafNodeCount()       }
        ,           post_ordering { NodeCount()           }
        {
            const Int node_count = T.NodeCount();
            
            mptr<Int> children  = N_children.data();
            mptr<Int> ranges    = N_ranges.data();
            mptr<Int> depth     = N_depth.data();
            mptr<Int> column    = N_column.data();
            mptr<Int> parent    = N_parent.data();
            mptr<Int> leaf_node = P_node.data();
            
            Tensor1<Int,Int> p_buffer ( node_count );
            Tensor1<Int,Int> q_buffer ( node_count );
            
            mptr<Int> p = p_buffer.data();
            mptr<Int> q = q_buffer.data();
            
            // Compute DFS ordering.
            Int counter = 0;
            root = 0;
            T.DepthFirstSearch(
                [p,q,&counter]( const Int node )                // interior node previsit
                {
                    p[counter] = node;
                    q[node]    = counter;
                    ++counter;
                },
                []( const Int node )                            // interior node postvisit
                {},
                [p,q,&counter]( const Int node ) // leaf node previsit
                {
                    p[counter] = node;
                    q[node]    = counter;
                    ++counter;
                },
                []( const Int node )                            // leaf node postvisit
                {}
            );
            
//            // Compute preordering.
//            Int counter = 0;
//            root = node_count - 1;
//            T.DepthFirstSearch(
//                []( const Int node )                            // interior node previsit
//                {},
//                [p,q,&counter]( const Int node )                // interior node postvisit
//                {
//                    p[counter] = node;
//                    q[node]    = counter;
//                    ++counter;
//                },
//                []( const Int node )                            // leaf node previsit
//                {},
//                [p,q,&counter]( const Int node )                // leaf node postvisit
//                {
//                    p[counter] = node;
//                    q[node]    = counter;
//                    ++counter;
//                }
//            );
             
            // Permute the tree.
            for( Int node = 0; node < node_count; ++node )
            {
                const Int i       = p[node];
                auto  [L,R]       = T.Children(i);
                const Int P       = T.Parent(i);
                auto  [begin,end] = T.NodeRange(i);
                
                children[2 * node + 0] = (L < node_count) ? q[L] : -1;
                children[2 * node + 1] = (R < node_count) ? q[R] : -1;
                ranges  [2 * node + 0] = begin;
                ranges  [2 * node + 1] = end;
                parent[node] = ( P >= 0 ) ? q[P] : -1;
                depth [node] = T.Depth(i);
                column[node] = T.Column(i);
                
                if( begin + 1 == end )
                {
                    leaf_node[begin] = node;
                }
            }
            
            // Compute postordering.
            mptr<Int> post = post_ordering.data();
            counter = 0;
            this->DepthFirstSearch(
                []( const Int node )                            // interior node previsit
                {},
                [post,&counter]( const Int node )               // interior node postvisit
                {
                    post[counter++] = node;
                },
                []( const Int node )                            // leaf node previsit
                {},
                [post,&counter]( const Int node )               // leaf node postvisit
                {
                    post[counter++] = node;
                }
            );
        }
        
        ~CompleteBinaryTree_Precomp() = default;
        
        
    protected:
        
        HelperTree_T T;
        
        Int root = 0;
        
        Tensor2<Int,Int> N_children;
        Tensor2<Int,Int> N_ranges;
        Tensor1<Int,Int> N_parent;
        Tensor1<Int,Int> N_depth;
        Tensor1<Int,Int> N_column;
        Tensor1<Int,Int> P_node;
        Tensor1<Int,Int> post_ordering;
        
    public:
        
        // See https://trstringer.com/complete-binary-tree/

        Int Root() const
        {
            return root;
        }

        Int MaxDepth() const
        {
            return T.max_depth;
        }
        
        Int LeftChild( const Int i ) const
        {
            return N_children.data()[2 * i + 0];
        }
        
        Int RightChild( const Int i ) const
        {
            return N_children.data()[2 * i + 1];
        }
        
        std::pair<Int,Int> Children( const Int i ) const
        {
            return { N_children.data()[2 * i + 0], N_children.data()[2 * i + 1] };
        }
        
        Int Parent( const Int i ) const
        {
            return N_parent[i];
        }
        
        Int Depth( const Int i ) const
        {
            return N_depth[i];
        }
        
        Int Column( const Int i ) const
        {
            return N_column[i];
        }

        Int ActualDepth() const
        {
            return T.ActualDepth();
        }
        
        Int NodeBegin( const Int i ) const
        {
            return N_ranges.data()[2 * i + 0];
        }
        
        Int NodeEnd( const Int i ) const
        {
            return N_ranges.data()[2 * i + 1];
        }
        
        std::pair<Int,Int> NodeRange( const Int i) const
        {
            return { N_ranges.data()[2 * i + 0], N_ranges.data()[2 * i + 1] };
        }
        
        bool NodeContainsLeafNodeQ( const Int node, const Int leafnode ) const
        {
            auto [begin,end] = NodeRange(node);
            
            return (begin <= leafnode) && (leafnode < end);
        }
        
        bool InteriorNodeQ( const Int node ) const
        {
//            return (node < T.InteriorNodeCount());
            
            return (LeftChild(node) >= Int(0));
        }
        
        bool LeafNodeQ( const Int node ) const
        {
            return (LeftChild(node) < Int(0));
        }
        
        Int PrimitiveNode( const Int primitive ) const
        {
            return P_node[primitive];
        }
        
//###############################################################################
//##        Get functions
//###############################################################################

    public:

        Int NodeCount() const
        {
            return T.NodeCount();
        }
        
        Int InteriorNodeCount() const
        {
            return T.InteriorNodeCount();
        }
        
        Int LeafNodeCount() const
        {
            return T.LeafNodeCount();
        }
        
        cref<Tensor2<Int,Int>> NodeChildren() const
        {
            return N_children;
        }
        
        cref<Tensor2<Int,Int>> NodeRanges() const
        {
            return N_ranges;
        }
        
        
        
        template<
            class Lambda_IntPreVisit,  class Lambda_IntPostVisit,
            class Lambda_LeafPreVisit, class Lambda_LeafPostVisit
        >
        void DepthFirstSearch(
            Lambda_IntPreVisit   int_pre_visit,  Lambda_IntPostVisit  int_post_visit,
            Lambda_LeafPreVisit  leaf_pre_visit, Lambda_LeafPostVisit leaf_post_visit
        )
        {
            ptic(ClassName()+"::DepthFirstSearch");
            
            Int stack [2 * max_depth];

            Int stack_ptr = Int(0);
            stack[stack_ptr] = Int(0);
            
            while( (Int(0) <= stack_ptr) && (stack_ptr < Int(2) * max_depth - Int(2) ) )
            {
                const Int code = stack[stack_ptr];
                const Int node = (code >> 1);
                
                const bool visitedQ = (code & Int(1));
                
                if( !visitedQ  )
                {
                    auto [L,R] = Children(node);
                    
                    // We visit this node for the first time.
                    if( InteriorNodeQ(node) )
                    {
                        int_pre_visit(node);

                        // Mark node as visited.
                        stack[stack_ptr] = (code | Int(1)) ;
                        
                        ++stack_ptr;
                        stack[stack_ptr] = (R << 1);
                        
                        ++stack_ptr;
                        stack[stack_ptr] = (L << 1);
                    }
                    else
                    {
                        // Things to be done when node is a leaf.
                        leaf_pre_visit(node);
                        
                        // There are no children to be visited first.
                        leaf_post_visit(node);
                        
                        // Popping current node from the stack.
                        --stack_ptr;
                    }
                }
                else
                {
                    // We visit this node for the second time.
                    // Thus node cannot be a leave node.
                    // We are moving in direction towards the root.
                    // Hence all children have already been visited.
                    
                    int_post_visit(node);

                    // Popping current node from the stack.
                    --stack_ptr;
                }
            }
            
            ptoc(ClassName()+"::DepthFirstSearch");
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("CompleteBinaryTree_Precomp")+"<"+TypeName<Int>+">";
        }

    }; // CompleteBinaryTree_Precomp
    
} // namespace KnotTools
