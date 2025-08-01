#pragma once

namespace Knoodle
{
    // TODO: Enable unsigned integers because they should be about 15% faster in tasks heavy bit manipulations.
    // TODO: Unify the compile-size stacks for pairs of indices. They work only with signed integers at the moment. Moreover, they are quite annoying to maintain.
    
    template<
        typename Int_,
        bool precompute_rangesQ_ = true,
        bool use_manual_stackQ_ = false
    >
    class alignas( ObjectAlignment ) CompleteBinaryTree : public CachedObject
    {
//        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Int  = Int_;
        
        
        static_assert(std::in_range<Int>(4 * 64 + 1),"");
        
        static constexpr Int max_depth = 64;
        static constexpr bool precompute_rangesQ = precompute_rangesQ_;
        static constexpr bool use_manual_stackQ  = use_manual_stackQ_;
        
        static constexpr Int Uninitialized = SignedIntQ<Int> ? Int(-1): std::numeric_limits<Int>::max();
        
        using UInt = ToUnsigned<Int>;

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
        
        // TODO: What to do if leaf_node_count_ == 0?
        
        explicit CompleteBinaryTree( const Int leaf_node_count_  )
        :         leaf_node_count ( leaf_node_count_                                   )
        ,              node_count ( Int(2) * leaf_node_count - Int(1)                  )
        ,          int_node_count { node_count - leaf_node_count                       }
        ,          last_row_begin { (Int(1) << Depth(node_count-Int(1))) - Int(1)      }
        ,                  offset { node_count - int_node_count - last_row_begin       }
        ,            actual_depth { Depth(node_count-Int(1))                           }
        , regular_leaf_node_count { int_cast<Int>(Int(1) << actual_depth) }
        ,          last_row_count { Int(2) * leaf_node_count - regular_leaf_node_count }
        {
            if( leaf_node_count <= Int(0) )
            {
                eprint(ClassName()+" initialized with 0 leaf nodes.");
            }

            if constexpr ( precompute_rangesQ )
            {
                N_ranges = Tiny::VectorList_AoS<2,Int,Int>(node_count);
                
                mptr<Int> r = N_ranges.data();
                
                // Compute range of leaf nodes in last row.
                for( Int N = last_row_begin; N < node_count; ++N )
                {
                    N_ranges(N,0) = N - last_row_begin    ;
                    N_ranges(N,1) = N - last_row_begin + 1;
                }
                
                // Compute range of leaf nodes in penultimate row.
                for( Int N = int_node_count; N < last_row_begin; ++N )
                {
                    N_ranges(N,0) = N + offset;
                    N_ranges(N,1) = N + offset + 1;
                }
                
                for( Int N = int_node_count; N --> Int(0); )
                {
                    const auto [L,R] = Children(N);
                    
                    N_ranges(N,0) = Min( r[2 * L + 0], r[2 * R + 0] );
                    N_ranges(N,1) = Max( r[2 * L + 1], r[2 * R + 1] );
                }
            }
        }
        
        // Default constructor
        CompleteBinaryTree() = default;
        // Destructor constructor (virtual because of inheritance)
        virtual ~CompleteBinaryTree() = default;
        // Copy constructor
        CompleteBinaryTree( const CompleteBinaryTree & other ) = default;
        // Copy assignment operator
        CompleteBinaryTree & operator=( const CompleteBinaryTree & other ) = default;
        // Move constructor
        CompleteBinaryTree( CompleteBinaryTree && other ) = default;
        // Move assignment operator
        CompleteBinaryTree & operator=( CompleteBinaryTree && other ) = default;
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        Int leaf_node_count = 0;
        
        Int node_count = 0;
        
        Int int_node_count = 0;

        Int last_row_begin = 0;
//        
        Int offset = 0;

        Tiny::VectorList_AoS<2,Int,Int> N_ranges;
        
        Int actual_depth            = 0;
        
        // A full binary tree with depth = actual_depth has this many leaf nodes.
        Int regular_leaf_node_count = 0;
        Int last_row_count          = 0;
        
        
    public:

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
        static constexpr Int MaxLevel()
        {
            return max_depth;
        }
        
        inline static constexpr Int LeftChild( const Int i )
        {
            return Int(2) * i + Int(1);
        }
        
        inline static constexpr Int RightChild( const Int i )
        {
            return Int(2) * i + Int(2);
        }
        
        inline static constexpr std::pair<Int,Int> Children( const Int i )
        {
            return { Int(2) * i + Int(1), Int(2) * i + Int(2) };
        }
        
        inline static constexpr Int Parent( const Int i )
        {
            return (i > Int(0)) ? (i - Int(1)) / Int(2) : Uninitialized;
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
        
        
        inline static constexpr Int Level( const Int i )
        {
            // Level equals the position of the most significant bit of i+1.
            return static_cast<Int>( MSB( static_cast<UInt>(i) + UInt(1) ) ) - Int(1);
        }
        
        inline static constexpr Int Depth( const Int i )
        {
            return Level(i);
        }
        
        inline static constexpr Int Column( const Int i )
        {
            // The start of each column is the number with all bits < Depth() being set.
            
            const UInt k = static_cast<UInt>(i) + UInt(1);

            return i - static_cast<Int>(PrevPow(k) - UInt(1));
        }
        
        inline Int ActualDepth() const
        {
            return actual_depth;
        }
        
        inline Int LevelCount() const
        {
            return actual_depth + Int(1);
        }
        
        inline Int LevelPointer( const Int level ) const
        {
            return (Int(1) << level) - Int(1);
        }
        inline Int LevelBegin( const Int level ) const
        {
            return LevelPointer(level);
        }
        
        inline Int LevelEnd( const Int level ) const
        {
            return LevelPointer( level + Int(1) );
        }
        
        inline std::pair<Int,Int> LevelRange( const Int level ) const
        {
            const Int begin = LevelPointer(level);
            
            return { begin, (begin << Int(1)) + Int(1) };
        }
        
        inline  Int RegularLeafNodeCount( const Int i ) const
        {
            // I a full binary tree this node would contain this many leaf nodes.
            return regular_leaf_node_count >> Depth(i);
        }
        
    private:
        
        // Helper function for NodeBegin, NodeEnd etc.
        inline Int nodeBoundaryCorrection( const Int r ) const
        {
            return r >= last_row_count
            ? r - ((r - last_row_count) >> Int(1))
            : r;
        }
        
    public:
        
        inline Int NodeBegin( const Int i ) const
        {
            if constexpr ( precompute_rangesQ )
            {
                return N_ranges(i,0);
            }
            else
            {
                // This would be the beginning of the node's primitives if the node were a _full_ complete binary tree.
                const Int reg_begin = RegularLeafNodeCount(i) * Column(i);
                
                return nodeBoundaryCorrection(reg_begin);
            }
        }
        
        inline Int NodeEnd( const Int i ) const
        {
            if constexpr ( precompute_rangesQ )
            {
                return N_ranges(i,1);
            }
            else
            {
                // This would be the end of the node's primitives if the node were a _full_ complete binary tree.
                const Int reg_end = RegularLeafNodeCount(i) * (Column(i) + Int(1));
                
                return nodeBoundaryCorrection(reg_end);
            }
        }
        
        Tiny::VectorList_AoS<2,Int,Int> NodeRanges() const
        {
            if constexpr ( precompute_rangesQ )
            {
                return N_ranges;
            }
            else
            {
                return Tiny::VectorList_AoS<2,Int,Int>();
            }
        }

        inline std::pair<Int,Int> ComputeNodeRange( const Int i ) const
        {
            const Int reg_leaf_count = RegularLeafNodeCount(i);
            const Int reg_begin      = reg_leaf_count * Column(i);
            const Int reg_end        = reg_begin + reg_leaf_count;
            
            return {
                nodeBoundaryCorrection(reg_begin),
                nodeBoundaryCorrection(reg_end)
            };
        }
        
        inline std::pair<Int,Int> ComputeNodeRange( const Int depth, const Int column ) const
        {
            const Int reg_leaf_count = regular_leaf_node_count >> depth;
            const Int reg_begin      = reg_leaf_count * (column + Int(0));
            const Int reg_end        = reg_leaf_count * (column + Int(1));
            
            return {
                nodeBoundaryCorrection(reg_begin),
                nodeBoundaryCorrection(reg_end)
            };
        }
        
        inline std::pair<Int,Int> NodeRange( const Int i ) const
        {
            if constexpr ( precompute_rangesQ )
            {
                return { N_ranges(i,0), N_ranges(i,1) };
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
        
        inline bool InternalNodeQ( const Int node ) const
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
        
        Int InternalNodeCount() const
        {
            return int_node_count;
        }
        
        Int LeafNodeCount() const
        {
            return leaf_node_count;
        }
        
//###############################################################################
//##        Breadth first
//###############################################################################
        
        /*! Breadth first scan of the tree. Every node is visited _before_ its children.
         *
         * @param int_visit An instance of a functor class (e.g. a lamda). It must have a method `operator()( Int node )` that executes what ought to be done in an internal node.
         *
         * @param leaf_visit An instance of a functor class that specifies what ought to be done in a leaf node.
         *
         *
         */
        
        template< class Internal_T, class Leaf_T >
        void BreadthFirstSearch(
            Internal_T  && int_visit,
            Leaf_T      && leaf_visit
        )
        {
            TOOLS_PTIMER(timer,MethodName("BreadthFirstSearch"));
            
            for( Int node = Int(0); node < int_node_count; ++node )
            {
                int_visit(node);
            }
            
            for( Int node = int_node_count; node < leaf_node_count; ++node )
            {
                leaf_visit(node);
            }
        }
        
        // Pointless, but useful for debugging and performance estimation.
        cref<Tensor1<Int,Int>> BreadthFirstOrdering()
        {
            std::string tag = "BreadthFirstOrdering";
            
            if( !InCacheQ(tag) )
            {
                Tensor1<Int,Int> p ( node_count );
                Int counter = 0;
                
                mptr<Int> p_ptr = p.data();
                
                BreadthFirstSearch(
                    [p_ptr,&counter]( Int node )  // node visit
                    {
                        p_ptr[counter++] = node;
                    },
                    [p_ptr,&counter]( Int node )  // leaf visit
                    {
                        p_ptr[counter++] = node;
                    }
                );
                
                SetCache(tag,std::move(p));
            }
            
            return GetCache<Tensor1<Int,Int>>(tag);
        }
        
        /*! Reverse breadth first scan of the tree. Every node is visited _after_ its children.
         *
         * @param int_visit An instance of a functor class (e.g. a lamda). It must have a method `operator()( Int node )` that executes what ought to be done in an internal node.
         *
         * @param leaf_visit An instance of a functor class that specifies what ought to be done in a leaf node.
         *
         */
        
        
        template< class Internal_T, class Leaf_T >
        void ReverseBreadthFirstSearch(
            Internal_T  && int_visit,
            Leaf_T      && leaf_visit
        )
        {
            TOOLS_PTIMER(timer,MethodName("ReverseBreadthFirstSearch"));

            for( Int node = leaf_node_count; node --> int_node_count; )
            {
                leaf_visit(node);
            }
            
            for( Int node = int_node_count; node --> Int(0);  )
            {
                int_visit(node);
            }
        }
        
        // Pointless, but useful for debugging and performance estimation.
        cref<Tensor1<Int,Int>> ReverseBreadthFirstOrdering()
        {
            std::string tag = "ReverseBreadthFirstOrdering";
            
            if( !InCacheQ(tag) )
            {
                Tensor1<Int,Int> p ( node_count );
                Int counter = 0;
                
                mptr<Int> p_ptr = p.data();
                
                BreadthFirstSearch(
                    [p_ptr,&counter]( Int node )  // node visit
                    {
                        p_ptr[counter++] = node;
                    },
                    [p_ptr,&counter]( Int node )  // leaf visit
                    {
                        p_ptr[counter++] = node;
                    }
                );
                
                SetCache(tag,std::move(p));
            }
            
            return GetCache<Tensor1<Int,Int>>(tag);
        }
        
//###############################################################################
//##        Depth first
//###############################################################################
        
    public:
        
        enum class DFS : std::int_fast8_t
        {
            BreakNever      = 0,
            PostVisitAlways = 1,
            BreakEarly      = 2
        };
        
        static std::string ToString( DFS mode )
        {
            switch( mode )
            {
                case DFS::BreakNever:
                {
                    return "BreakNever";
                }
                case DFS::PostVisitAlways:
                {
                    return "PostVisitAlways";
                }
                case DFS::BreakEarly:
                {
                    return "BreakEarly";
                }
            }
        }
        
        
       /*! Depth first scan of the tree. Can used to implement pre-order and post-order tree traversal.
        *
        * @param int_pre_visit An instance of a functor class (e.g. a lamda). It must have a method `bool operator()( Int node )` that executes what ought to be done in an internal node _before_ the children are visited. The returned `bool` indicates whether the children shall be visited and whether `int_post_visit` shall be executed.
        *
        * @param int_post_visit An instance of a functor class that specifies what ought to be done in a node _after_ the children have been visited.
        *
        * @param leaf_visit An instance of a functor class that specifies what ought to be done in a leaf node.
        *
        * @param start_node The root of the subtree to be visited.
        *
        */
        
        template<DFS mode, class IntPre_T, class IntPost_T, class Leaf_T>
        void DepthFirstSearch(
            IntPre_T  && int_pre_visit,
            IntPost_T && int_post_visit,
            Leaf_T    && leaf_visit,
            const Int start_node = Uninitialized
        )
        {
            TOOLS_PTIMER(timer,ClassName()+"::DepthFirstSearch"
                + "<" + ToString(mode)
                + ">" );
            
            if constexpr ( use_manual_stackQ )
            {
                DepthFirstSearch_ManualStack<mode>(
                    int_pre_visit, int_post_visit, leaf_visit,
                    (start_node < Int(0)) ? Root() : start_node
                );
            }
            else
            {
                DepthFirstSearch_Recursive<mode>(
                    int_pre_visit, int_post_visit, leaf_visit,
                    (start_node < Int(0)) ? Root() : start_node
                );
            }
        }
        
        template< DFS mode, class IntPre_T, class IntPost_T, class Leaf_T >
        void DepthFirstSearch_Recursive(
            IntPre_T  && int_pre_visit,
            IntPost_T && int_post_visit,
            Leaf_T    && leaf_visit,
            const Int node
        )
        {
            // We visit this node for the first time.
            if( InternalNodeQ(node) )
            {
                if constexpr ( mode == DFS::BreakNever )
                {
                    (void)int_pre_visit(node);
                    
                    auto [L,R] = Children(node);
                    
                    DepthFirstSearch_Recursive<mode>(
                        int_pre_visit, int_post_visit, leaf_visit, L
                    );
                    DepthFirstSearch_Recursive<mode>(
                        int_pre_visit, int_post_visit, leaf_visit, R
                    );
                    
                    int_post_visit(node);
                }
                else
                {
                    bool continueQ = int_pre_visit(node);
                    
                    if( continueQ )
                    {
                        auto [L,R] = Children(node);
                        
                        DepthFirstSearch_Recursive<mode>(
                            int_pre_visit, int_post_visit, leaf_visit, L
                        );
                        DepthFirstSearch_Recursive<mode>(
                            int_pre_visit, int_post_visit, leaf_visit, R
                        );
                    }
                    
                    if constexpr ( mode == DFS::PostvisitAlways )
                    {
                        int_post_visit(node);
                    }
                    else
                    {
                        if( continueQ )
                        {
                            int_post_visit(node);
                        }
                    }
                }
            }
            else
            {
                // Things to be done when node is a leaf.
                leaf_visit(node);
            }
        }
        
        template<DFS mode, class IntPre_T, class IntPost_T, class Leaf_T>
        void DepthFirstSearch_ManualStack(
            IntPre_T  && int_pre_visit,
            IntPost_T && int_post_visit,
            Leaf_T    && leaf_visit,
            const Int start_node = Uninitialized
        )
        {
            constexpr Int stack_max_size = 2 * max_depth + 1;
            constexpr Int stack_limit    = stack_max_size - 2;
            
            Int stack [stack_max_size];
            Int stack_ptr = 0;
            stack[stack_ptr] = 0; // Dummy node.
            
            stack[++stack_ptr] = (start_node < Int(0)) ? Root() : start_node;
            
            while( (Int(0) < stack_ptr) && (stack_ptr < stack_limit) )
            {
                const Int code = stack[stack_ptr];
                const Int node = (code >> 1);
                
                const bool visitedQ = (code & Int(1));
                
                if( !visitedQ  )
                {
                    // We visit this node for the first time.
                    if( InternalNodeQ(node) )
                    {
                        // Mark node as visited.
                        stack[stack_ptr] = (code | Int(1)) ;

                        auto [L,R] = Children(node);
                        
                        if constexpr ( mode == DFS::BreakNever )
                        {
                            (void)int_pre_visit(node);
                            stack[++stack_ptr] = (R << 1);
                            stack[++stack_ptr] = (L << 1);
                        }
                        else
                        {
                            if( int_pre_visit(node) )
                            {
                                stack[++stack_ptr] = (R << 1);
                                stack[++stack_ptr] = (L << 1);
                            }
                            else
                            {
                                if constexpr ( mode == DFS::BreakEarly )
                                {
                                    // This prevents execution of int_post_visit.
                                    --stack_ptr;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Things to be done when node is a leaf.
                        leaf_visit(node);
                        
                        // Popping current node from the stack.
                        --stack_ptr;
                    }
                }
                else
                {
                    // We visit this node for the second time.
                    // Thus it cannot be a leave node.
                    // We are moving in direction towards the root.
                    // Hence all children have already been visited.
                    int_post_visit(node);

                    // Popping current node from the stack.
                    --stack_ptr;
                }
            }
            
            if( stack_ptr >= stack_max_size )
            {
                eprint(ClassName()+"::DepthFirstSearch_ManualStack: Stack overflow.");
            }
        }
        
        
//###############################################################################
//##        PreOrdering
//###############################################################################
        
        template< class Internal_T, class Leaf_T >
        void PreOrderScan(
            Internal_T  && int_visit,
            Leaf_T      && leaf_visit,
            const Int start_node = Uninitialized
        )
        {
            DepthFirstSearch<DFS::BreakNever>(
               [&int_visit]( Int node )   // pre visit
               {
                   int_visit(node);
               },
               []( Int node )              // post visit
               {
                   (void)node;
               },
               [&leaf_visit]( Int node )  // leaf visit
               {
                   leaf_visit(node);
               },
               start_node
            );
        }
        
        cref<Tensor1<Int,Int>> PreOrdering()
        {
            std::string tag = "PreOrdering";
            
            if( !InCacheQ(tag) )
            {
                Tensor1<Int,Int> p ( node_count );
                Int counter = 0;
                
                mptr<Int> p_ptr = p.data();
                
                PreOrderScan(
                   [p_ptr,&counter]( Int node )  // pre visit
                   {
                       p_ptr[counter++] = node;
                   },
                   [p_ptr,&counter]( Int node )  // leaf visit
                   {
                       p_ptr[counter++] = node;
                   }
                );
                
                SetCache(tag,std::move(p));
            }
            
            return GetCache<Tensor1<Int,Int>>(tag);
        }
        
//###############################################################################
//##        PostOrdering
//###############################################################################
        
        template< class Internal_T, class Leaf_T >
        void PostOrderScan(
            Internal_T  && int_visit,
            Leaf_T      && leaf_visit,
            const Int start_node = Uninitialized
        )
        {
            DepthFirstSearch<DFS::BreakNever>(
               []( Int node )             // pre visit
               {
                   (void)node;
               },
               [&int_visit]( Int node )   // post visit
               {
                   int_visit(node);
               },
               [&leaf_visit]( Int node )  // leaf visit
               {
                   leaf_visit(node);
               },
               start_node
            );
        }
        
        cref<Tensor1<Int,Int>> PostOrdering()
        {
            std::string tag = "PostOrdering";
            
            if( !InCacheQ(tag) )
            {
                Tensor1<Int,Int> p ( node_count );
                Int counter = 0;
                
                mptr<Int> p_ptr = p.data();
                
                PostOrderScan(
                   [p_ptr,&counter]( Int node )          // node visit
                   {
                       p_ptr[counter++] = node;
                   },
                   [p_ptr,&counter]( Int node )          // leaf visit
                   {
                       p_ptr[counter++] = node;
                   }
                );
                
                SetCache(tag,std::move(p));
            }
            
            return GetCache<Tensor1<Int,Int>>(tag);
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
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("CompleteBinaryTree")
                + "<" + TypeName<Int>
                + "," + Tools::ToString(precompute_rangesQ)
                + "," + Tools::ToString(use_manual_stackQ)
                + ">";
        }

    }; // CompleteBinaryTree
    
} // namespace Knoodle

