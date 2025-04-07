#pragma once

namespace Knoodle
{
    
    template<typename Int_>
    class alignas( ObjectAlignment ) MatrixQuadTree
    {
        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Int = Int_;
        
        using NodeId_T = Int;
        
        using Children_T = std::array<std::array<Int,2>,2>;
        
        using Entry_T = std::array<Int,2>;
        using EntryContainer_T = std::vector<Entry_T>;
        
        using Interval_T = std::array<Int,2>;
        using Box_T = std::array<Interval_T,2>;
        using BoxContainer_T = std::vector<Box_T>;
        
        using Tree_T = CompleteBinaryTree<Int,false>;
        
        static constexpr NodeId_T NoNode = -1;

        enum class NodeState : Int8
        {
            Deleted   =  0,
            Touched   =  1,
            Untouched = -1
        };
        
        struct Node
        {
            Children_T c {{{NoNode,NoNode},{NoNode,NoNode}}};
            Box_T box;

            NodeState state = NodeState::Untouched;
            
            Node() = delete;

            Node( Int x_a, Int x_b, Int y_a, Int y_b )
            :   box { {{x_a,x_b},{y_a,y_b}} }
            {}
            
            Node( cref<Box_T> box_ )
            :   box { box_ }
            {}
            
            ~Node() = default;
            
//            Node( Node & N ) = delete;
//            Node( const Node & N ) = delete;
            
        public:
            
            bool NoChildrenQ() const
            {
                return (c[0][0] == NoNode)
                    && (c[0][1] == NoNode)
                    && (c[1][0] == NoNode)
                    && (c[1][1] == NoNode);
            }
        };
        
        MatrixQuadTree() = default;
        
        MatrixQuadTree( Int n_, Int dist_ = Int(0) )
        {
            Reset(n_,dist_);
        }
        
        ~MatrixQuadTree() = default;
        
        
    private:
        
        Int n = 0;
        
        std::vector<Node> nodes;
        
        mutable Box_T R;
        
        mutable EntryContainer_T valid_entries;
        mutable BoxContainer_T   boxes;
        
        Int node_count = 0;
        Int max_node_count = 0;
        Int dist = 0;
        
        Node NullNode {0,0,0,0};
        
        static constexpr NodeId_T root = 0;

    public:
        
        void Reset( Int n_, Int dist_ = Int(0) )
        {
            NullNode.state = NodeState::Deleted;
            
            n = n_;
            dist = dist_;
            
            nodes.clear();
            
            node_count = 0;
            CreateNode( Int(0), n, Int(0), n );
            max_node_count = node_count;
            
            valid_entries.clear();
            boxes.clear();
        }
        
        Int RowCount() const
        {
            return n;
        }
        
        Int ColCount() const
        {
            return n;
        }
        
        Int NodeCount() const
        {
            return node_count;
        }
        
        Int MaxNodeCount() const
        {
            return max_node_count;
        }
        
        Int Threshold() const
        {
            return dist;
        }
        
        
        // This checks whether interval I = [a,b[ lies in interval J = [c,f[
        static bool SubintervalQ( cref<Interval_T> I, cref<Interval_T> J  )
        {
            return (J[0] <= I[0]) && (I[1] <= J[1]);
        }
        
        // This checks whether intervals I = [a,b[ and J = [c,f[ are disjoint.
        static bool DisjointQ( cref<Interval_T> I, cref<Interval_T> J  )
        {
            return (I[1] <= J[0]) || (I[0] >= J[1]);
        }
        
    public:
        
        bool DeactivateReactangle( Int x_a, Int x_b, Int y_a, Int y_b )
        {
            TOOLS_PTIC(ClassName()+"::DeactivateReactangle");
            
            R[0][0] = x_a; R[0][1] = x_b;
            R[1][0] = y_a; R[1][1] = y_b;
            
            TouchNode(root);
            
            TOOLS_PTOC(ClassName()+"::DeactivateReactangle");
            
            return (root != NoNode);
        }
        
    private:
        
        Node GetChild( cref<Node> N, bool i, bool j )
        {
            if( N.c[i][j] == NoNode )
            {
                return NullNode;
            }
            else
            {
                return nodes[N.c[i][j]];
            }
        }

        NodeId_T CreateNode( Int x_a, Int x_b, Int y_a, Int y_b )
        {
//
//            {x_a,y_b}   {x_b,y_b}
//                +-----------+
//                |           |
//                |           |
//                |           |
//                +-----------+
//            {x_a,y_a}   {x_b,y_a}
            
            if(
               (x_a < x_b)
               &&
               (y_a < y_b)
               &&
               ( (ModDistance(n,x_a,y_b) > dist) || (ModDistance(n,x_b,y_a) > dist) )
            )
            {
                nodes.push_back( Node( x_a, x_b, y_a, y_b ) );
                ++node_count;
                
                return static_cast<NodeId_T>(nodes.size());
            }
            else
            {
                return NoNode;
            }
        }

        void DeleteNode( NodeId_T id )
        {
            if( id == NoNode ) return;
            
            mref<Node> N = nodes[id];
            
            N.state = NodeState::Deleted;
            --node_count;
            DeleteNode( N.c[1][1] );
            DeleteNode( N.c[1][0] );
            DeleteNode( N.c[0][1] );
            DeleteNode( N.c[0][0] );
        }
    
        void TouchNode( NodeId_T id )
        {
            if( id == NoNode ) return;
            
            
            mref<Node> N = nodes[id];
            
            bool x_disjointQ = DisjointQ( N.box[0], R[0] );
            bool y_disjointQ = DisjointQ( N.box[1], R[1] );
            
            if( x_disjointQ || y_disjointQ ) return;
            
            bool x_includedQ = SubintervalQ( N.box[0], R[0] );
            bool y_includedQ = SubintervalQ( N.box[1], R[1] );
            
            if( x_includedQ && y_includedQ )
            {
                DeleteNode(id);
                return;
            }
            
            if( N.state != NodeState::Untouched )
            {
                return;
            }
            
            const Int x_a = N.box[0][0];
            const Int x_c = N.box[0][1];
            const Int x_b = x_a + (x_c - x_a)/2;
            
            const Int y_a = N.box[1][0];
            const Int y_c = N.box[1][1];
            const Int y_b = y_a + (y_c - y_a)/2;
            
            N.state = NodeState::Touched;
        
            // Might might invalidate reference N.
            const Int id_0 = nodes[id].c[0][0] = CreateNode( x_a, x_b, y_a, y_b );
            const Int id_1 = nodes[id].c[0][1] = CreateNode( x_a, x_b, y_b, y_c );
            const Int id_2 = nodes[id].c[1][0] = CreateNode( x_b, x_c, y_a, y_b );
            const Int id_3 = nodes[id].c[1][1] = CreateNode( x_b, x_c, y_b, y_c );
            max_node_count = Max(node_count,max_node_count);
            
            TouchNode(id_0); // Might might invalidate references.
            TouchNode(id_1); // Might might invalidate references.
            TouchNode(id_2); // Might might invalidate references.
            TouchNode(id_3); // Might might invalidate references.
            
            // If all the children are deallocated, then we know that they cannot contain valid matrix extries. Then we can delete this node as well.
            
            if( nodes[id].NoChildrenQ() )
            {
                DeleteNode(id);
            }
        }
        
    public:
        
        cref<EntryContainer_T> ValidEntries( Int dist_ = Int(0) )
        {
            TOOLS_PTIC(ClassName()+"::ValidEntries");
         
            dist = dist_;
            
            valid_entries.clear();
            
            if( dist > 0 )
            {
                FindValidEntriesAtDistance( root );
            }
            else
            {
                FindValidEntries( root );
            }
            
            TOOLS_PTOC(ClassName()+"::ValidEntries");
            
            return valid_entries;
        }
        
    private:
        
        void FindValidEntries( NodeId_T id ) const
        {
            if( id == NoNode ) return;
            
            cref<Node> N = nodes[id];

            if( N.state == NodeState::Touched )
            {
                FindValidEntries(N.c[0][0]);
                FindValidEntries(N.c[0][1]);
                FindValidEntries(N.c[1][0]);
                FindValidEntries(N.c[1][1]);
            }
            else
            {
                for( Int i = N.box[0][0]; i < N.box[0][1]; ++i )
                {
                    for( Int j = N.box[1][0]; j < N.box[1][1]; ++j )
                    {
                        valid_entries.push_back( std::array<Int,2>({i,j}) );
                    }
                }
            }
        }
        
        
        void FindValidEntriesAtDistance( NodeId_T id  ) const
        {
            if( id == NoNode ) return;
            
            cref<Node> N = nodes[id];

            if( N.state == NodeState::Touched )
            {
                FindValidEntriesAtDistance(N.c[0][0]);
                FindValidEntriesAtDistance(N.c[0][1]);
                FindValidEntriesAtDistance(N.c[1][0]);
                FindValidEntriesAtDistance(N.c[1][1]);
            }
            else
            {
                for( Int i = N.box[0][0]; i < N.box[0][1]; ++i )
                {
                    for( Int j = N.box[1][0]; j < N.box[1][1]; ++j )
                    {
                        if( ModDistance(n,i,j) >= dist )
                        {
                            valid_entries.push_back( std::array<Int,2>({i,j}) );
                        }
                    }
                }
            }
        }
        
    public:
        
        cref<BoxContainer_T> NodeBoxes() const
        {
            TOOLS_PTIC(ClassName()+"::NodeBoxes");
            
            boxes.clear();
            
            FindNodeBoxes( root );
            
            TOOLS_PTOC(ClassName()+"::NodeBoxes");
            
            return boxes;
        }
        
    private:
        
        void FindNodeBoxes( NodeId_T id ) const
        {
            if( id == NoNode ) return;
            
            cref<Node> N = nodes[id];
            
            boxes.push_back( N.box );
            
            if( N.state == NodeState::Touched )
            {
                FindNodeBoxes(N.c[0][0]);
                FindNodeBoxes(N.c[0][1]);
                FindNodeBoxes(N.c[1][0]);
                FindNodeBoxes(N.c[1][1]);
            }
        }
        
    public:
        
        cref<BoxContainer_T> LeafBoxes() const
        {
            TOOLS_PTIC(ClassName()+"::LeafBoxes");
            
            boxes.clear();
            
            FindLeafBoxes( root );
            
            TOOLS_PTOC(ClassName()+"::LeafBoxes");
            
            return boxes;
        }
        
    private:
        
            
        void FindLeafBoxes( NodeId_T id ) const
        {
            if( id == NoNode ) return;
            
            cref<Node> N = nodes[id];
                
            if( N.state == NodeState::Touched )
            {
                FindLeafBoxes(N.c[0][0]);
                FindLeafBoxes(N.c[0][1]);
                FindLeafBoxes(N.c[1][0]);
                FindLeafBoxes(N.c[1][1]);
            }
            else
            {
                boxes.push_back( N.box );
            }
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("MatrixQuadTree") + "<" + TypeName<Int> + ">";
        }

    }; // MatrixQuadTree
    
} // namespace Knoodle

