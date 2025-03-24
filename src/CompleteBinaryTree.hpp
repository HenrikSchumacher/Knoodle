#pragma once

namespace Knoodle
{
    
    template<typename Int_, bool precompute_rangesQ_ = true>
    class alignas( ObjectAlignment ) CompleteBinaryTree
    {
        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Int  = Int_;
        using SInt = std::int_fast32_t;
        
        static constexpr Int max_depth = 64;
        static constexpr bool precompute_rangesQ = precompute_rangesQ_;
        
        using UInt = Scalar::Unsigned<Int>;

        struct Node
        {
            Int idx    = 0;
            Int depth  = 0;
            Int column = 0;
            Int begin  = 0;
            Int end    = 0;
            
            Node( Int idx_, Int depth_, Int column_, Int begin_, Int end_ )
            : idx       { idx_      }
            , depth     { depth_    }
            , column    { column_   }
            , begin     { begin_    }
            , end       { end_      }
            {}
        };
        
        CompleteBinaryTree() = default;
        
        // TODO: What to do if leaf_node_count_ == 0?
        
        explicit CompleteBinaryTree( const Int leaf_node_count_  )
        :         leaf_node_count ( leaf_node_count_                                   )
        ,              node_count ( Int(2) * leaf_node_count - Int(1)                  )
        ,          int_node_count { node_count - leaf_node_count                       }
        ,          last_row_begin { (Int(1) << Depth(node_count-Int(1))) - Int(1)      }
        ,                  offset { node_count - int_node_count - last_row_begin       }
        ,            actual_depth { Depth(node_count-Int(1))                           }
        , regular_leaf_node_count { static_cast<Int>(Int(1) << actual_depth)           }
        ,          last_row_count { Int(2) * leaf_node_count - regular_leaf_node_count }
        {
            if( leaf_node_count <= Int(0) )
            {
                eprint(ClassName()+" initialized with 0 leaf nodes.");
            }
            
//            TOOLS_DUMP(actual_depth);
//            TOOLS_DUMP(regular_leaf_node_count);
//            TOOLS_DUMP(last_row_count);
//
            if constexpr ( precompute_rangesQ )
            {
                N_ranges = Tensor2<Int,Int>(node_count,2);
                
                mptr<Int> r = N_ranges.data();
                
                // Compute range of leaf nodes in last row.
                for( Int N = last_row_begin; N < node_count; ++N )
                {
                    r[2 * N + 0] = N - last_row_begin    ;
                    r[2 * N + 1] = N - last_row_begin + 1;
                }
                
                // Compute range of leaf nodes in penultimate row.
                for( Int N = int_node_count; N < last_row_begin; ++N )
                {
                    r[2 * N + 0] = N + offset;
                    r[2 * N + 1] = N + offset + 1;
                }
                
                for( Int N = int_node_count; N --> 0; )
                {
                    const auto [L,R] = Children(N);
                    
                    r[2 * N + 0] = Min( r[2 * L + 0], r[2 * R + 0] );
                    r[2 * N + 1] = Max( r[2 * L + 1], r[2 * R + 1] );
                }
            }
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

        Tensor2<Int,Int> N_ranges;
        
        Int actual_depth            = 0;
        
        // A full binary tree with depth = actual_depth has this many leaf nodes.
        Int regular_leaf_node_count = 0;
        Int last_row_count          = 0;




        Node Left( const Node node) const
        {
            
            Int m = node.begin; // TODO: Correct this!
            
            return Node(
                2 * node.idx + 1,
                node.depth + 1,
                2 * node.column,
                node.begin,
                m
            );
        }

        Node Right( const Node node) const
        {
            Int m = node.end; // TODO: Correct this!
            
            return Node(
                2 * node.idx + 2,
                node.depth + 1,
                2 * node.column + 1,
                m,
                node.end
            );
        }
        
        std::pair<Node,Node> Children( const Node node) const
        {
            Int m = node.end; // TODO: Correct this!
            
            return std::pair<Node,Node>(
                Node(
                    2 * node.idx + 1,
                    node.depth + 1,
                    2 * node.column,
                    node.begin,
                    m
                    )
                ,
                Node(
                    2 * node.idx + 2,
                    node.depth + 1,
                    2 * node.column + 1,
                    m,
                    node.end
                )
            );
        }
        
//        Node Parent( const Node node) const
//        {
//            // TODO: Correct this!
//            
//            const bool evenQ = node.idx | Int(1);
//            
//            Int m = evenQ ? node.end : node.begin;
//
//            return Node(
//                ( node.idx - 1 )/2,
//                node.depth - 1,
//                node.column / 2,
//                evenQ ? node.begin : m,
//                evenQ ? m          : node.end
//            );
//        }
        
    public:
        
        // See https://trstringer.com/complete-binary-tree/

        static constexpr Int Root()
        {
            return 0;
        }
        
        static constexpr Int MaxDepth()
        {
            return max_depth;
        }
        
        inline static constexpr Int LeftChild( const Int i )
        {
            return 2 * i + 1;
        }
        
        inline static constexpr Int RightChild( const Int i )
        {
            return 2 * i + 2;
        }
        
        inline static constexpr std::pair<Int,Int> Children( const Int i )
        {
            return { 2 * i + 1, 2 * i + 2 };
        }
        
        inline static constexpr Int Parent( const Int i )
        {
            return (i > 0) ? (i - 1) / 2 : -1;
        }
        
        inline static constexpr Int Ancestor( const Int i, const Int generations )
        {
            Int p = i;
            for( Int gen = 1; gen < generations; ++gen )
            {
                p = Parent(i);
            }
            
            return p;
        }
        
        inline static constexpr Int Depth( const Int i )
        {
            // Depth equals the position of the most significant bit if i+1.
            return static_cast<Int>( MSB( static_cast<UInt>(i) + UInt(1) ) ) - Int(1);
        }
        
        
        inline static constexpr Int Column( const Int i )
        {
            // The start of each column is the number with all bits < Depth() being set.
            
            constexpr UInt one = 1;
            
            UInt k = static_cast<UInt>(i) + one;

            return i - static_cast<Int>(PrevPow(k) - one);
        }
        
        
        inline Int ActualDepth() const
        {
            return actual_depth;
        }
        
        inline  Int RegularLeafNodeCount( const Int i ) const
        {
            // I a full binary tree this node would contain this many leaf nodes.
            return regular_leaf_node_count >> Depth(i);
        }
        
        inline Int NodeBegin( const Int i ) const
        {
            if constexpr ( precompute_rangesQ )
            {
                return N_ranges.data()[2 * i + 0];
            }
            else
            {
                const Int regular_begin = RegularLeafNodeCount(i) * Column(i);
                
                return regular_begin - (Ramp(regular_begin - last_row_count) >> 1);
            }
        }
        
        inline Int NodeEnd( const Int i ) const
        {
            if constexpr ( precompute_rangesQ )
            {
                return N_ranges.data()[2 * i + 1];
            }
            else
            {
                const Int regular_end = RegularLeafNodeCount(i) * (Column(i) + 1);
                
                return regular_end - (Ramp(regular_end - last_row_count) >> 1);
            }
        }

        inline std::pair<Int,Int> ComputeNodeRange( const Int i ) const
        {
            const Int reg_leaf_count = RegularLeafNodeCount(i);
            const Int regular_begin  = reg_leaf_count * Column(i);
            const Int regular_end    = regular_begin + reg_leaf_count;
            
            return {
                regular_begin - (Ramp(regular_begin - last_row_count) >> 1),
                regular_end   - (Ramp(regular_end   - last_row_count) >> 1)
            };
        }
        
        inline std::pair<Int,Int> ComputeNodeRange( const Int depth, const Int column ) const
        {
            const Int reg_leaf_count = regular_leaf_node_count >> depth;
            const Int reg_begin      = reg_leaf_count * (column + 0);
            const Int reg_end        = reg_leaf_count * (column + 1);
            
            return {
                reg_begin - (Ramp(reg_begin - last_row_count) >> 1),
                reg_end   - (Ramp(reg_end   - last_row_count) >> 1)
            };
        }
        
        inline std::pair<Int,Int> NodeRange( const Int i ) const
        {
            if constexpr ( precompute_rangesQ )
            {
                return { N_ranges.data()[2 * i + 0], N_ranges.data()[2 * i + 1] };
            }
            else
            {
                return ComputeNodeRange(i);
            }
        }
        
        bool NodeContainsLeafNodeQ( const Int node, const Int leafnode ) const
        {
            auto [begin,end] = NodeRange(node);
            
            return (begin <= leafnode) && (leafnode < end);
        }
        
        inline bool InteriorNodeQ( const Int node ) const
        {
            return (node < int_node_count);
        }
        
        inline bool LeafNodeQ( const Int node ) const
        {
            return node >= int_node_count;
        }
        
        inline Int PrimitiveNode( const Int primitive ) const
        {
            if( primitive < last_row_count )
            {
                return last_row_begin + primitive;
            }
            else
            {
                return int_node_count + (primitive - last_row_count);
            }
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
        
        template<
            class Lambda_IntPreVisit,  class Lambda_IntPostVisit,
            class Lambda_LeafPreVisit, class Lambda_LeafPostVisit
        >
        void DepthFirstSearch(
            Lambda_IntPreVisit   int_pre_visit,  Lambda_IntPostVisit  int_post_visit,
            Lambda_LeafPreVisit  leaf_pre_visit, Lambda_LeafPostVisit leaf_post_visit,
            const Int start_node = -1
        )
        {
            TOOLS_PTIC(ClassName()+"::DepthFirstSearch");
            
            Int stack [2 * max_depth];

            SInt stack_ptr = 0;
            stack[stack_ptr] = (start_node < 0) ? Root() : start_node;
            
            while( (SInt(0) <= stack_ptr) && (stack_ptr < SInt(2) * max_depth - SInt(2) ) )
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
                        
                        stack[++stack_ptr] = (R << 1);
                        stack[++stack_ptr] = (L << 1);
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
            
            if( stack_ptr >= Int(2) * max_depth - Int(2) )
            {
                eprint(ClassName() + "::DepthFirstSearch: Stack overflow.");
            }
            
            TOOLS_PTOC(ClassName()+"::DepthFirstSearch");
        }
        
    public:
        
        Size_T AllocatedByteCount() const
        {
            return N_ranges.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(CompleteBinaryTree) + AllocatedByteCount();
        }
        
        static std::string ClassName()
        {
            return ct_string("CompleteBinaryTree")
                + "<" + TypeName<Int>
                + "," + ToString(precompute_rangesQ)
                + ">";
        }

    }; // CompleteBinaryTree
    
} // namespace Knoodle

