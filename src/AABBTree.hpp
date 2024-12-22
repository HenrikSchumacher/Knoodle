#pragma once

namespace KnotTools
{
    
    // A simple bounding volume hiearchy with static ordering.
    // This is specifically written for edge and triangle primitives for curves.
    // It won't work with more general clouds of primitives.
    
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) AABBTree : public CompleteBinaryTree<Int_>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        using Tree = CompleteBinaryTree<Int>;
        using Tree::max_depth;

        
        static constexpr Int AmbDim = AmbDim_;
        
        using Vector_T     = Tiny::Vector<AmbDim_,Real,Int>;
        
        using BContainer_T = Tensor3<Real,Int>;
        
        using EContainer_T = Tensor3<Real,Int>;
        
        using PContainer_T = Tensor3<Real,Int>;
        
        using UInt = Scalar::Unsigned<Int>;
        
        AABBTree() = default;
        
        explicit AABBTree( const Int prim_count_  )
        :   Tree  ( prim_count_ )
        ,   N_box ( 2 * LeafNodeCount() - 1, AmbDim, 2 )
        {}
        
        ~AABBTree() = default;
        
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        using Tree::leave_count;
        using Tree::node_count;
        using Tree::int_node_count;
        using Tree::last_row_begin;
        using Tree::offset;
        using Tree::N_begin;
        using Tree::N_end;
        
        BContainer_T N_box;
        
    public:
        
        using Tree::MaxDepth;
        using Tree::NodeCount;
        using Tree::InteriorNodeCount;
        using Tree::LeafNodeCount;
        
        using Tree::RightChild;
        using Tree::LeftChild;
        using Tree::Parent;
        using Tree::Depth;
        using Tree::Column;
        using Tree::NodeBegin;
        using Tree::NodeEnd;
        
        
    private:
        
        template<Int point_count, Int ldP>
        static constexpr void PrimitiveToBox(
            cptr<Real> P, mptr<Real> B
        )
        {
            // P is assumed to represent a matrix of size prim_size x ldP,
            // where ldP >= AmbDim. This sound off, but our main use case is creating planar diagrams; here ldP = 3 and AmbDim = 2.
            
            // The geometric primitive is assumed to be the complex hull of the row vectors.
            // Only the AmbDim leading entries of each row are read in.
            
            static_assert(point_count > 0, "");
            
            Real lo [AmbDim];
            Real up [AmbDim];
            
            columnwise_minmax<point_count,AmbDim>(
                P, ldP, &lo[0], Size_T(1), &up[0], Size_T(1)
            );
            
            for( Int k = 0; k < AmbDim; ++k )
            {
                B[2*k+0] = lo[k];
                B[2*k+1] = up[k];
            }
            
//            if constexpr ( point_count == 1 )
//            {
//                for( Int k = 0; k < AmbDim; ++k )
//                {
//                    B[2*k+0] = P[k];
//                    B[2*k+1] = P[k];
//                }
//            }
//            if constexpr ( point_count > 1 )
//            {
//                Real lo [AmbDim];
//                Real up [AmbDim];
//                
//                elementwise_minmax<AmbDim>(&P[ldP * 0], &P[ldP * 1], &lo[0], &up[0]);
//                
//                for( Int i = 2; i < point_count; ++i )
//                {
//                    elementwise_minmax_update<AmbDim>(&P[ldP * i], &lo[0], &up[0]);
//                }
//                
//                for( Int k = 0; k < AmbDim; ++k )
//                {
//                    B[2*k+0] = lo[k];
//                    B[2*k+1] = up[k];
//                }
//            }
        }
        
        static constexpr void BoxesToBox(
            cptr<Real> B_L, cptr<Real> B_R, mptr<Real> B_N
        )
        {
            for( Int k = 0; k < AmbDim; ++k )
            {
                B_N[2*k+0] = Min( B_L[2*k+0], B_R[2*k+0] );
                B_N[2*k+1] = Max( B_L[2*k+1], B_R[2*k+1] );
            }
        }
        
    public:
        
        template<Int point_count, Int ldP>
        void ComputeBoundingBoxes( cref<PContainer_T> P, mref<BContainer_T> B ) const
        {
//            ptic(ClassName()+"ComputeBoundingBoxes");
            
            // P represents the primitive coordinates.
            // It is assumed to have size prim_count x point_count x ldP;
            
            // B represents the box coordinates.
            // It is assumed to have size prim_count x AmbDim x 2;
            
            // Here we require that ldP >= AmbDim. This sound off, but our main use case is creating planar diagrams; here ldP = 3 and AmbDim = 2.
            
            // Compute bounding boxes of leave nodes (last row of tree).
            for( Int N = last_row_begin; N < node_count; ++N )
            {
                const Int i = N - last_row_begin;
                
                PrimitiveToBox<point_count,ldP>( P.data(i), B.data(N) );
            }
            
            // Compute bounding boxes of leave nodes (penultimate row of tree).
            for( Int N = int_node_count; N < last_row_begin; ++N )
            {
                const Int i = N + offset;

                PrimitiveToBox<point_count,ldP>( P.data(i), B.data(N) );
            }
            
            // Compute bounding boxes of interior nodes.
            for( Int N = int_node_count; N --> 0;  )
            {
                const Int L = LeftChild (N);
                const Int R = RightChild(N);
                
                BoxesToBox( B.data(L), B.data(R), B.data(N) );
            }
            
//            ptoc(ClassName()+"ComputeBoundingBoxes");
        }
        
        template<Int point_count, Int ldP>
        void ComputeBoundingBoxes( cref<EContainer_T> E )
        {
            ComputeBoundingBoxes<point_count,ldP>( E, N_box );
        }
        
        static constexpr bool BoxesIntersectQ( const cptr<Real> B_i, const cptr<Real> B_j )
        {
            // B_i and B_j are assumed to have size AmbDim x 2.
            
            if constexpr ( VectorizableQ<Real> )
            {
                vec_T<2*AmbDim,Real> a;
                vec_T<2*AmbDim,Real> b;
                
                for( Int k = 0; k < AmbDim; ++k )
                {
                    a[2 * k + 0] = B_i[2 * k + 0];
                    a[2 * k + 1] = B_j[2 * k + 0];
                    b[2 * k + 0] = B_j[2 * k + 1];
                    b[2 * k + 1] = B_i[2 * k + 1];
                }
                
                return __builtin_reduce_and( a <= b );
            }
            else
            {
                for( Int k = 0; k < AmbDim; ++k )
                {
                    if(
                       (B_i[2 * k + 0] > B_j[2 * k + 1])
                       ||
                       (B_j[2 * k + 0] > B_i[2 * k + 1])
                    )
                    {
                        return false;
                    }
                }
                
                return true;
            }
        }
        
        
    public:
        
        BContainer_T & NodeBoxes()
        {
            return N_box;
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("AABBTree")+"<"+ToString(AmbDim)+","+TypeName<Real>+","+TypeName<Int>+">";
        }

    }; // AABBTree
    
} // namespace KnotTools
