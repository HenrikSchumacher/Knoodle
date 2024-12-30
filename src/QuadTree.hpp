#pragma once

namespace KnotTools
{
    template<typename Int_>
    class alignas( ObjectAlignment ) QuadTree
    {
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Int = Int_;
        
        static constexpr Int max_depth = 128;
        
        
        using UInt = Scalar::Unsigned<Int>;
        
        CompleteBinaryTree() = default;
        
        explicit CompleteBinaryTree( const Int leave_count_  )
        :       leave_count ( leave_count_                                   )
        ,       node_count  ( 2 * leave_count - 1                            )
        ,   int_node_count  ( node_count - leave_count                       )
            // Leaves are precisely the last prim_count nodes.
        ,   last_row_begin  ( (Int(1) << Depth(node_count-1)) - 1            )
        ,           offset  ( node_count - int_node_count - last_row_begin   )
        ,          N_begin  ( node_count                                     )
        ,            N_end  ( node_count                                     )
        {
            
            // Compute range of leave nodes in last row.
            for( Int N = last_row_begin; N < node_count; ++N )
            {
                N_begin[N] = N - last_row_begin    ;
                N_end  [N] = N - last_row_begin + 1;
            }
            
            // Compute range of leave nodes in penultimate row.
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
        }
        
        ~CompleteBinaryTree() = default;
        
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        const Int leave_count = 0;
        
        const Int node_count = 0;
        
        const Int int_node_count = 0;

        const Int last_row_begin = 0;
        
        const Int offset = 0;
        
        Tensor1<Int,Int> N_begin;
        Tensor1<Int,Int> N_end;
        
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
        
        Int NodeBegin( const Int i ) const
        {
            return N_begin[i];
        }
        
        Int NodeEnd( const Int i ) const
        {
            return N_end[i];
        }
        
        
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
            return leave_count;
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("CompleteBinaryTree")+"<"+TypeName<Int>+">";
        }

    }; // CompleteBinaryTree
    
} // namespace KnotTools
