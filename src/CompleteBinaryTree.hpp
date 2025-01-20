#pragma once

//#define KNOTTOOLS_COMPLETEBINARYTREE_PRECOMPUTE_RANGES

namespace KnotTools
{
    template<typename Int_>
    class alignas( ObjectAlignment ) CompleteBinaryTree
    {
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Int  = Int_;
        
        static constexpr Int max_depth = 128;
        
        
        using UInt = Scalar::Unsigned<Int>;
        
        CompleteBinaryTree() = default;
        
        // TODO: What to do if leaf_node_count_ == 0?
        
        explicit CompleteBinaryTree( const Int leaf_node_count_  )
        :         leaf_node_count { leaf_node_count_                                   }
        ,              node_count { 2 * leaf_node_count - 1                            }
        ,          int_node_count { node_count - leaf_node_count                       }
        ,          last_row_begin { (Int(1) << Depth(node_count-1)) - 1                }
        ,                  offset { node_count - int_node_count - last_row_begin       }
#ifdef KNOTTOOLS_COMPLETEBINARYTREE_PRECOMPUTE_RANGES
        ,                 N_begin { node_count                                         }
        ,                   N_end { node_count                                         }
#endif
        ,            actual_depth { Depth(node_count-1)                                }
        , regular_leaf_node_count { Int(1) << actual_depth                             }
        ,          last_row_count { Int(2) * leaf_node_count - regular_leaf_node_count }
        {
            if( leaf_node_count <= Int(0) )
            {
                eprint(ClassName()+" initialized with 0 leaf nodes.");
            }
            
//            dump(actual_depth);
//            dump(regular_leaf_node_count);
//            dump(last_row_count);
//
#ifdef KNOTTOOLS_COMPLETEBINARYTREE_PRECOMPUTE_RANGES
            // Compute range of leaf nodes in last row.
            for( Int N = last_row_begin; N < node_count; ++N )
            {
                N_begin[N] = N - last_row_begin    ;
                N_end  [N] = N - last_row_begin + 1;
            }
            
            // Compute range of leaf nodes in penultimate row.
            for( Int N = int_node_count; N < last_row_begin; ++N )
            {
                N_begin[N] = N + offset;
                N_end  [N] = N + offset + 1;
            }
            
            for( Int N = int_node_count; N --> 0; )
            {
                const Int L = LeftChild (N);
                const Int R = L + 1;
                
                N_begin[N] = Min( N_begin[L], N_begin[R] );
                N_end  [N] = Max( N_end  [L], N_end  [R] );
            }
#endif
        }
        
        ~CompleteBinaryTree() = default;
        
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        Int leaf_node_count = 0;
        
        Int node_count = 0;
        
        Int int_node_count = 0;

        Int last_row_begin = 0;
//        
        Int offset = 0;

#ifdef KNOTTOOLS_COMPLETEBINARYTREE_PRECOMPUTE_RANGES
        Tensor1<Int,Int> N_begin;
        Tensor1<Int,Int> N_end;
#endif
        
        Int actual_depth            = 0;
        
        // A full binary tree with depth = actual_depth has this many leaf nodes.
        Int regular_leaf_node_count = 0;
        Int last_row_count          = 0;
        
    public:
        
        // See https://trstringer.com/complete-binary-tree/

        static constexpr Int MaxDepth()
        {
            return max_depth;
        }
        
        static constexpr Int LeftChild( const Int i )
        {
            return 2 * i + 1;
        }
        
        static constexpr Int RightChild( const Int i )
        {
            return 2 * i + 2;
        }
        
        static constexpr Int Parent( const Int i )
        {
            return (i > 0) ? (i - 1) / 2 : -1;
        }
        
        static constexpr Int Depth( const Int i )
        {
            // Depth equals the position of the most significant bit if i+1.
            constexpr UInt one = 1;
            
            return static_cast<Int>( MSB( static_cast<UInt>(i) + one ) - one );
        }
        
        
        static constexpr Int Column( const Int i )
        {
            // The start of each column is the number with all bits < Depth() being set.
            
            constexpr UInt one = 1;
            
            UInt k = static_cast<UInt>(i) + one;

            return i - (PrevPow(k) - one);
        }
        
        
        Int ActualDepth() const
        {
            return actual_depth;
        }
        
        Int RegularLeafNodeCount( const Int i ) const
        {
            // I a full binary tree this node would contain this many leaf nodes.
            return regular_leaf_node_count >> Depth(i);
        }
        
#ifdef KNOTTOOLS_COMPLETEBINARYTREE_PRECOMPUTE_RANGES
        Int NodeBegin( const Int i ) const
        {
            return N_begin[i];
        }
        
        Int NodeEnd( const Int i ) const
        {
            return N_end[i];
        }
#else
        Int NodeBegin( const Int i ) const
        {
            const Int regular_begin = RegularLeafNodeCount(i) * Column(i);
            
            return regular_begin - (Ramp(regular_begin - last_row_count) >> 1);
        }
        
        Int NodeEnd( const Int i ) const
        {
            const Int regular_end   = RegularLeafNodeCount(i) * (Column(i) + 1);
            
            return regular_end - (Ramp(regular_end - last_row_count) >> 1);
        }
#endif

        
        
        bool NodeContainsLeafNodeQ( const Int node, const Int leafnode ) const
        {
            return (Begin(node) <= leafnode) && (leafnode < End(node));
        }
        
        bool NodesContainLeafNodeQ(
            const Int node_0,      const Int node_1,
            const Int leafnode_0, const Int leafnode_1
        ) const
        {
            return (
                NodeContainsLeafNodeQ(node_0,leafnode_0)
                &&
                NodeContainsLeafNodeQ(node_1,leafnode_1)
            ) || (
                NodeContainsLeafNodeQ(node_0,leafnode_1)
                &&
                NodeContainsLeafNodeQ(node_1,leafnode_0)
            );
        }
        
        bool InteriorNodeQ( const Int node ) const
        {
            return (node < int_node_count);
        }
        
//###############################################################################
//##        Get functions
//###############################################################################

    public:

        Int NodeCount() const
        {
            return node_count;
        }
        
        Int InteriorNodeCount() const
        {
            return int_node_count;
        }
        
        Int LeafNodeCount() const
        {
            return leaf_node_count;
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("CompleteBinaryTree")+"<"+TypeName<Int>+">";
        }

    }; // CompleteBinaryTree
    
} // namespace KnotTools

