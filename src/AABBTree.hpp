#pragma once

namespace KnotTools
{
    
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) AABBTree
    {
        ASSERT_FLOAT(Real_);
        ASSERT_INT(Int_);
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int max_depth = 128;
        
        static constexpr Int AmbDim = AmbDim_;
        
        using Vector_T     = Tiny::Vector<AmbDim_,Real,Int>;
        
        using BContainer_T = Tensor3<Real,Int>;
        
        using EContainer_T = Tensor3<Real,Int>;
        
        using UInt = Scalar::Unsigned<Int>;
        
        AABBTree() = default;
        
        explicit AABBTree( const Int edge_count_  )
        :       edge_count ( edge_count_                                    )
        ,       node_count ( 2 * edge_count - 1                             )
        ,   int_node_count ( node_count - edge_count                        ) 
            // Leaves are precisely the last edge count nodes.
        ,   last_row_begin ( (Int(1) << Depth(node_count-1)) - 1            )
        ,           offset ( node_count - int_node_count - last_row_begin   )
        ,            C_box ( 2 * edge_count - 1, AmbDim, 2                  )
        ,          C_begin ( node_count                                     )
        ,            C_end ( node_count                                     )
        {
            
            // Compute range of leave nodes in last row.
            for( Int C = last_row_begin; C < node_count; ++C )
            {
                C_begin[C] = C - last_row_begin    ;
                C_end  [C] = C - last_row_begin + 1;
            }
            
            // Compute range of leave nodes in penultimate row.
            for( Int C = int_node_count; C < last_row_begin; ++C )
            {
                C_begin[C] = C + offset;
                C_end  [C] = C + offset + 1;
            }
            
            for( Int C = int_node_count; C --> 0; )
            {
                const Int L = LeftChild (C);
                const Int R = L + 1;
                
                C_begin[C] = Min( C_begin[L], C_begin[R] );
                C_end  [C] = Max( C_end  [L], C_end  [R] );
            }
        }
        
        ~AABBTree() = default;
        
        // See https://trstringer.com/complete-binary-tree/
        
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
            return i > 0 ? (i - 1) / 2 : -1;
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
        
        Int Begin( const Int i ) const
        {
            return C_begin[i];
        }
        
        Int End( const Int i ) const
        {
            return C_end[i];
        }
        
        
        bool NodeContainsEdgeQ( const Int node, const Int edge ) const
        {
            return (Begin(node) <= edge) && (edge < End(node));
        }
        
        bool NodesContainEdgesQ( const Int node_0, const Int node_1, const Int edge_0, const Int edge_1 ) const
        {
            return ( NodeContainsEdgeQ(node_0,edge_0) && NodeContainsEdgeQ(node_1,edge_1) )
                   ||
                   ( NodeContainsEdgeQ(node_0,edge_1) && NodeContainsEdgeQ(node_1,edge_0) );
        }
        
        void ComputeBoundingBoxes( cref<EContainer_T> E, mref<BContainer_T> B ) const
        {
//            ptic(ClassName()+"ComputeBoundingBoxes");
            
//            dump(C_begin);
//            dump(int_node_count);
            
            // Compute bounding boxes of leave nodes (last row of tree).
            for( Int C = last_row_begin; C < node_count; ++C )
            {
                const Int i = C - last_row_begin;
                
                for( Int k = 0; k < AmbDim; ++k )
                {
                    std::tie( B(C,k,0), B(C,k,1) ) = MinMax( E(i,0,k), E(i,1,k) );
                }
            }
            
            
            // Compute bounding boxes of leave nodes (penultimate row of tree).
            for( Int C = int_node_count; C < last_row_begin; ++C )
            {
                const Int i = C + offset;
                
                for( Int k = 0; k < AmbDim; ++k )
                {
                    std::tie( B(C,k,0), B(C,k,1) ) = MinMax( E(i,0,k), E(i,1,k) );
                }
            }
            
            // Compute bounding boxes of interior nodes.
            for( Int C = int_node_count; C --> 0;  )
            {
                const Int L = LeftChild (C);
                const Int R = RightChild(C);
                
                for( Int k = 0; k < AmbDim; ++k )
                {
                    B(C,k,0) = Min( B(L,k,0), B(R,k,0) );
                    B(C,k,1) = Max( B(L,k,1), B(R,k,1) );
                }
            }
            
//            ptoc(ClassName()+"ComputeBoundingBoxes");
        }
        
        
        void ComputeBoundingBoxes( cref<EContainer_T> E )
        {
            ComputeBoundingBoxes( E, C_box );
        }
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        const Int edge_count = 0;
        
        const Int node_count = 0;
        
        const Int int_node_count = 0;

        const Int last_row_begin = 0;
        
        const Int offset = 0;
        
        BContainer_T C_box;
        
        Tensor1<Int,Int> C_begin;
        Tensor1<Int,Int> C_end;
        
        
    public:

//##########################################################################################
//##        Get functions
//##########################################################################################

        BContainer_T & ClusterBoxes()
        {
            return C_box;
        }
        
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
            return node_count - int_node_count;
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("AABBTree")+"<"+ToString(AmbDim)+","+TypeName<Real>+","+TypeName<Int>+">";
        }

    }; // AABBTree
    
} // namespace KnotTools
