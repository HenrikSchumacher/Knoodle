#pragma once

namespace KnotTools
{
    
    // A simple bounding volume hiearchy with static ordering.
    // This is specifically written for edge and triangle primitives for curves.
    // It won't work with more general clouds of primitives.
    
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) AABBTree
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int max_depth = 128;
        
        static constexpr Int AmbDim = AmbDim_;
        
        using Vector_T     = Tiny::Vector<AmbDim_,Real,Int>;
        
        using BContainer_T = Tensor3<Real,Int>;
        
        using EContainer_T = Tensor3<Real,Int>;
        
        using PContainer_T = Tensor3<Real,Int>;
        
        using UInt = Scalar::Unsigned<Int>;
        
        AABBTree() = default;
        
        explicit AABBTree( const Int prim_count_  )
        :       prim_count ( prim_count_                                    )
        ,       node_count ( 2 * prim_count - 1                             )
        ,   int_node_count ( node_count - prim_count                        ) 
            // Leaves are precisely the last prim_count nodes.
        ,   last_row_begin ( (Int(1) << Depth(node_count-1)) - 1            )
        ,           offset ( node_count - int_node_count - last_row_begin   )
        ,            N_box ( 2 * prim_count - 1, AmbDim, 2                  )
        ,          N_begin ( node_count                                     )
        ,            N_end ( node_count                                     )
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
        
        
        bool NodeContainsPrimitiveQ( const Int node, const Int primitive ) const
        {
            return (Begin(node) <= primitive) && (primitive < End(node));
        }
        
        bool NodesContainPrimitiveQ(
            const Int node_0,      const Int node_1,
            const Int primitive_0, const Int primitive_1
        ) const
        {
            return (
                NodeContainsEdgeQ(node_0,primitive_0)
                &&
                NodeContainsEdgeQ(node_1,primitive_1)
            ) || (
                NodeContainsEdgeQ(node_0,primitive_1)
                &&
                NodeContainsEdgeQ(node_1,primitive_0)
            );
        }
        
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
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        const Int prim_count = 0;
        
        const Int node_count = 0;
        
        const Int int_node_count = 0;

        const Int last_row_begin = 0;
        
        const Int offset = 0;
        
        BContainer_T N_box;
        
        Tensor1<Int,Int> N_begin;
        Tensor1<Int,Int> N_end;
        
        
    public:

//###############################################################################
//##        Get functions
//###############################################################################

        BContainer_T & NodeBoxes()
        {
            return N_box;
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
