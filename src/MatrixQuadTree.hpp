#pragma once

namespace Knoodle
{
    
    template<typename Int_>
    class alignas( ObjectAlignment ) MatrixQuadTree final
    {
        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Int = Int_;
        
//        using ID_T = std::array<Int,2>;
        
        using Entry_T = std::array<Int,2>;
        using EntryContainer_T = std::vector<Entry_T>;
        
        using Interval_T = std::array<Int,2>;
        using Box_T = std::array<Interval_T,2>;
        using BoxContainer_T = std::vector<Box_T>;
        
        using Tree_T = CompleteBinaryTree<Int,false>;

        struct Node final
        {
            Node * c[2][2] = {{nullptr,nullptr},{nullptr,nullptr}};
            Box_T box;

            bool touchedQ = false;
            
            Node() = delete;

            Node( Int x_a, Int x_b, Int y_a, Int y_b )
            :   box { {{x_a,x_b},{y_a,y_b}} }
            {}
            
            Node( cref<Box_T> box_ )
            :   box { box_ }
            {}
            
//            Node( cref<ID_T> id_, cref<Box_T> box_ )
//            :   id  { id_ }
//            ,   box { box_ }
//            {}
            
//            Node( ID_T && id_, Box_T && box_ )
//            :   id  { std::move(id_) }
//            ,   box { std::move(box_) }
//            {}
            
            ~Node()
            {
//                logvalprint("Deleting node",id);
//                logvalprint("this",this);
//                logvalprint("c",ArrayToString(&c[0][0],{2,2}));
//                logvalprint("box",ArrayToString(&box[0][0],{2,2}));
//                logvalprint("touchedQ",touchedQ);
                
                if( c[1][1] != nullptr ) delete c[1][1];
                if( c[1][0] != nullptr ) delete c[1][0];
                if( c[0][1] != nullptr ) delete c[0][1];
                if( c[0][0] != nullptr ) delete c[0][0];
            }
            
            Node( Node & N ) = delete;
            Node( const Node & N ) = delete;
            
        public:
            
            bool NoChildrenQ() const
            {
                return (c[0][0] == nullptr)
                    && (c[0][1] == nullptr)
                    && (c[1][0] == nullptr)
                    && (c[1][1] == nullptr);
            }
        };
        
        MatrixQuadTree( Int n_, Int d_ = Int(0) )
        {
            Reset(n_,d_);
        }
        
        // Default constructor
        MatrixQuadTree() = default;
        
        // TODO: A nast class this is!
        // Destructor
        ~MatrixQuadTree()
        {
            DeleteNode(root);
        }
        
        MatrixQuadTree( const MatrixQuadTree & other ) = delete;
        // Copy assignment operator
        MatrixQuadTree & operator=( const MatrixQuadTree & other ) = delete;
        // Move constructor
        MatrixQuadTree( MatrixQuadTree && other ) = delete;
        // Move assignment operator
        MatrixQuadTree & operator=( MatrixQuadTree && other ) = delete;
        
        
    private:
        
        Int n = 0;
        
        Node * root = nullptr;
        
        mutable Box_T R;
        
        mutable EntryContainer_T valid_entries;
        mutable BoxContainer_T   boxes;
        
        Int node_count = 0;
        Int max_node_count = 0;
        Int d = 0;

    public:
        
        void Reset( Int n_, Int d_ = Int(0) )
        {
            n = n_;
            d = d_;
            
            DeleteNode(root);
            
            node_count = 0;
            root = new Node( Int(0), n, Int(0), n );
            max_node_count = node_count;
            
            TOOLS_LOGDUMP(root);
            
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
            return d;
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
        
        
    private:

            void CreateNode( Node * & N, Int x_a, Int x_b, Int y_a, Int y_b )
            {
                if( N != nullptr ) return;
    //
    //            {x_a,y_b}     {x_b,y_b}
    //                +-------------+
    //                |             |
    //                |             |
    //                |             |
    //                +-------------+
    //            {x_a,y_a}     {x_b,y_a}
                
                if(
                   (x_a < x_b) && (y_a < y_b)
                   &&
                   !(
                        ((y_b <= x_a + d + 1) && (y_a + d + 1 >= x_b))
                        ||
                        (y_a + d + 1 >= x_b + n     )
                        ||
                        (y_b + n     <= x_a + d + 1 )
                   )
                )
                {
                    N = new Node( x_a, x_b, y_a, y_b );
                    ++node_count;
                }
                else
                {
                    N = nullptr;
                }
            }
            
            void DeleteNode( Node * & N )
            {
                if( N == nullptr ) return;
                    
                DeleteNode(N->c[1][1]);
                DeleteNode(N->c[1][0]);
                DeleteNode(N->c[0][1]);
                DeleteNode(N->c[0][0]);
                
                delete N;
                N = nullptr;
                --node_count;
            }
        
    public:
        
        bool DeactivateReactangle( Int x_a, Int x_b, Int y_a, Int y_b )
        {
            // Don't do anything if the input rectangle is trivial.
            if( (x_b <= x_a) || (y_b <= y_a) )
            {
                return (root != nullptr);
            }
            
            R[0][0] = x_a; R[0][1] = x_b;
            R[1][0] = y_a; R[1][1] = y_b;
            
            TOOLS_LOGDUMP(root);
            
            TouchNode(root);
            
            return (root != nullptr);
        }
        
    private:

        void TouchNode( Node * & N )
        {
            if( N == nullptr )
            {
                return;
            }
            
            bool x_disjointQ = DisjointQ( N->box[0], R[0] );
            bool y_disjointQ = DisjointQ( N->box[1], R[1] );
            
            if( x_disjointQ || y_disjointQ )
            {
                return;
            }
                
            bool x_includedQ = SubintervalQ( N->box[0], R[0] );
            bool y_includedQ = SubintervalQ( N->box[1], R[1] );
            
            if( x_includedQ && y_includedQ )
            {
                DeleteNode(N);
                return;
            }
            
            RequireChildren(N);
            TouchNode(N->c[0][0]);
            TouchNode(N->c[0][1]);
            TouchNode(N->c[1][0]);
            TouchNode(N->c[1][1]);
            
            // If all the children are deallocated, then we know that they cannot contain valid matrix extries. Then we can delete this node as well.
            if( N->NoChildrenQ() )
            {
                DeleteNode(N);
            }
        }
        
        void RequireChildren( Node * & N )
        {
            if( N->touchedQ ) return;
            
            const Int x_a = N->box[0][0];
            const Int x_c = N->box[0][1];
            const Int x_b = x_a + (x_c - x_a)/2;
            
            const Int y_a = N->box[1][0];
            const Int y_c = N->box[1][1];
            const Int y_b = y_a + (y_c - y_a)/2;
            
            CreateNode( N->c[0][0], x_a, x_b, y_a, y_b );
            CreateNode( N->c[0][1], x_a, x_b, y_b, y_c );
            CreateNode( N->c[1][0], x_b, x_c, y_a, y_b );
            CreateNode( N->c[1][1], x_b, x_c, y_b, y_c );
            
            N->touchedQ = true;
            max_node_count = Max(node_count,max_node_count);
        }
        
    public:
        
        cref<EntryContainer_T> ValidEntries()
        {
            TOOLS_PTIC(ClassName()+"::ValidEntries");
         
            valid_entries.clear();
            
            if( d > Int(0) )
            {
                FindValidEntriesAtDistance(root);
            }
            else
            {
                FindValidEntries(root);
            }
            
            TOOLS_PTOC(ClassName()+"::ValidEntries");
            
            return valid_entries;
        }
        
    private:
        
        void FindValidEntries( Node * N ) const
        {
            if( N == nullptr ) return;

            if( N->touchedQ )
            {
                FindValidEntries(N->c[0][0]);
                FindValidEntries(N->c[0][1]);
                FindValidEntries(N->c[1][0]);
                FindValidEntries(N->c[1][1]);
            }
            else
            {
                for( Int i = N->box[0][0]; i < N->box[0][1]; ++i )
                {
                    for( Int j = N->box[1][0]; j < N->box[1][1]; ++j )
                    {
                        valid_entries.push_back( std::array<Int,2>({i,j}) );
                    }
                }
            }
        }
        
        
        void FindValidEntriesAtDistance( Node * N ) const
        {
            if( N == nullptr ) return;

            if( N->touchedQ )
            {
                FindValidEntriesAtDistance(N->c[0][0]);
                FindValidEntriesAtDistance(N->c[0][1]);
                FindValidEntriesAtDistance(N->c[1][0]);
                FindValidEntriesAtDistance(N->c[1][1]);
            }
            else
            {
                for( Int i = N->box[0][0]; i < N->box[0][1]; ++i )
                {
                    for( Int j = N->box[1][0]; j < N->box[1][1]; ++j )
                    {
                        if( ModDistance(n,i,j) >= d )
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
            
            FindNodeBoxes(root);
            
            TOOLS_PTOC(ClassName()+"::NodeBoxes");
            
            return boxes;
        }
        
    private:
        
        void FindNodeBoxes( Node * N ) const
        {
            if( N == nullptr ) return;
            
            boxes.push_back( N->box );
            
            if( N->touchedQ )
            {
                FindNodeBoxes(N->c[0][0]);
                FindNodeBoxes(N->c[0][1]);
                FindNodeBoxes(N->c[1][0]);
                FindNodeBoxes(N->c[1][1]);
            }
        }
        
    public:
        
        cref<BoxContainer_T> LeafBoxes() const
        {
            TOOLS_PTIC(ClassName()+"::LeafBoxes");
            
            boxes.clear();
            
            FindLeafBoxes(root);
            
            TOOLS_PTOC(ClassName()+"::LeafBoxes");
            
            return boxes;
        }
        
    private:
        
        void FindLeafBoxes( Node * N ) const
        {
            if( N == nullptr ) return;
            
            if( !N->touchedQ )
            {
                boxes.push_back( N->box );
            }
            else
            {
                FindLeafBoxes(N->c[0][0]);
                FindLeafBoxes(N->c[0][1]);
                FindLeafBoxes(N->c[1][0]);
                FindLeafBoxes(N->c[1][1]);
            }
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("MatrixQuadTree") + "<" + TypeName<Int> + ">";
        }

    }; // MatrixQuadTree
    
} // namespace Knoodle

