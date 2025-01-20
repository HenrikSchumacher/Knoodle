#pragma once

namespace KnotTools
{
    
    // A simple bounding volume hiearchy with static ordering.
    // This is specifically written for edge and triangle primitives for curves.
    // It won't work with more general clouds of primitives.
    
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) AABBTree2 : public CompleteBinaryTree<Int_>
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
        
        AABBTree2() = default;
        
        explicit AABBTree2( const Int prim_count_  )
        :   Tree  ( prim_count_ )
        {}
        
        ~AABBTree2() = default;
        
        
    protected:
        
        // Integer data for the combinatorics of the tree.
        // Corners of the bounding boxes.

        using Tree::leaf_node_count;
        using Tree::node_count;
        using Tree::int_node_count;
        using Tree::last_row_begin;
        using Tree::offset;
        
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
        
        /*!
         * @brief Computes the bounding boxes from the coordinates of a list of primitives. Primitives are assumed to be the convex hull of finitely many points. Only the leading `AmbDim` entries of each point are used.
         *
         * @tparam point_count Number of points in the primitive.
         *
         * @tparam dimP `dimP` is assumed to have dimensions `prim_count x point_count x dimP`. Here we require that `dimP >= AmbDim`. This sound off, but our main use case is creating planar diagrams; here `dimP = 3` and `AmbDim = 2`.
         *
         * @param P Array that represent a single primitive. It is assumed to have size `point_count x dimP`.
         *
         * @param lo Represents the lower vector of a single box. It is assumed to be of dimensions `AmbDim`.
         *
         * @param hi Represents the upper vector of a single box. It is assumed to be of dimensions `AmbDim`.
         */
        
        template<Int point_count, Int dimP>
        static constexpr void PrimitiveToBox(
            cptr<Real> P, mptr<Real> lo, mptr<Real> hi
        )
        {
            static_assert(point_count > 0, "");
            
            columnwise_minmax<point_count,AmbDim>(
                P, dimP, lo, Size_T(1), hi, Size_T(1)
            );
        }
        
        static constexpr void BoxesToBox(
            cptr<Real> L_lo, cptr<Real> R_lo, mptr<Real> N_lo,
            cptr<Real> L_hi, cptr<Real> R_hi, mptr<Real> N_hi
        )
        {
            // TODO: Can this be vectorized?
            
            for( Int k = 0; k < AmbDim; ++k )
            {
                N_lo[k] = Min( L_lo[k], R_lo[k] );
                N_hi[k] = Max( L_hi[k], R_hi[k] );
            }
        }
        
    public:
        
        /*!
         * @brief Computes the bounding boxes of the whole tree from the coordinates of a list of primitives. Primitives are assumed to be the convex hull of finitely many points.
         *
         * @tparam point_count Number of points in the primitive.
         *
         * @tparam dimP `dimP` is assumed to have dimensions `prim_count x point_count x dimP`. Here we require that `dimP >= AmbDim`. This sound off, but our main use case is creating planar diagrams; here `dimP = 3` and `AmbDim = 2`.
         *
         * @param P Array that represents the primitive coordinates. It is assumed to have size `prim_count x point_count x dimP`.
         *
         * @param B Represents the container for the boxes. It is assumed to be a `Tensor3` of dimensions `prim_count x AmbDim x 2`.
         */
        
        template<Int point_count, Int dimP>
        void ComputeBoundingBoxes( cptr<Real> P, mref<BContainer_T> B ) const
        {
            ptic(ClassName()+"ComputeBoundingBoxes");
            
            static_assert(dimP >= AmbDim,"");
            
            constexpr Int sizeP = point_count * dimP;
            

            ptic("Compute bounding boxes of leave nodes.");
            // Compute bounding boxes of leave nodes (last row of tree).
            for( Int N = last_row_begin; N < node_count; ++N )
            {
                const Int i = N - last_row_begin;
                
                PrimitiveToBox<point_count,dimP>( &P[sizeP * i], B.data(0,N), B.data(1,N) );
            }
            
            // Compute bounding boxes of leave nodes (penultimate row of tree).
            for( Int N = int_node_count; N < last_row_begin; ++N )
            {
                const Int i = N + offset;

                PrimitiveToBox<point_count,dimP>( &P[sizeP * i], B.data(0,N), B.data(1,N) );
            }
            ptoc("Compute bounding boxes of leave nodes.");
            
            ptic("Compute bounding boxes of interior nodes.");
            // Compute bounding boxes of interior nodes.
            for( Int N = int_node_count; N --> 0;  )
            {
                const Int L = LeftChild (N);
                const Int R = L + 1;
                
                BoxesToBox(
                    B.data(0,L), B.data(0,R), B.data(0,N),
                    B.data(1,L), B.data(1,R), B.data(1,N)
                );
            }
            ptoc("Compute bounding boxes of interior nodes.");
            
            ptoc(ClassName()+"ComputeBoundingBoxes");
        }
        
        template<Int point_count, Int dimP>
        void ComputeBoundingBoxes( cref<EContainer_T> E, mref<BContainer_T> B )
        {
            ComputeBoundingBoxes<point_count,dimP>( E.data(), B );
        }
        
        static constexpr bool BoxesIntersectQ(
            const cptr<Real> lo_i, const cptr<Real> hi_i,
            const cptr<Real> lo_j, const cptr<Real> hi_j
        )
        {
            // B_i and B_j are assumed to have size AmbDim x 2.
            
            if constexpr ( VectorizableQ<Real> )
            {
                vec_T<2*AmbDim,Real> a;
                vec_T<2*AmbDim,Real> b;
                
                for( Int k = 0; k < AmbDim; ++k )
                {
                    a[2 * k + 0] = lo_i[k];
                    a[2 * k + 1] = lo_j[k];
                    b[2 * k + 0] = hi_j[k];
                    b[2 * k + 1] = hi_i[k];
                }
                
                return __builtin_reduce_and( a <= b );
            }
            else
            {
                for( Int k = 0; k < AmbDim; ++k )
                {
                    if(
                       (lo_i[k] > hi_j[k])
                       ||
                       (lo_j[k] > hi_i[k])
                    )
                    {
                        return false;
                    }
                }
                
                return true;
            }
        }
        
        static constexpr Real BoxBoxSquaredDistance(
            const cptr<Real> lo_i, const cptr<Real> hi_i,
            const cptr<Real> lo_j, const cptr<Real> hi_j
        )
        {
            Real d2 = 0;
            
            for( Int k = 0; k < AmbDim; ++k )
            {
                const Real x = Ramp(
                    Max( lo_i[k], lo_j[k] )
                    -
                    Min( hi_i[k], hi_j[k] )
                );
                
                d2 += x * x;
            }
            return d2;
        }
        
        Real BoxBoxSquaredDistance(
            cref<BContainer_T> B_0, const Int i, cref<BContainer_T> B_1, const Int j
        ) const
        {
            return BoxBoxSquaredDistance(
                B_0.data(0,i), B_0.data(1,i), B_1.data(0,j), B_1.data(1,j)
            );
        }
        
        BContainer_T AllocateBoxes()
        {
            ptic(ClassName()+"AllocateBoxes");
            
            BContainer_T B (2,NodeCount(),AmbDim);
            
            ptoc(ClassName()+"AllocateBoxes");
            
            return B;
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("AABBTree2")+"<"+ToString(AmbDim)+","+TypeName<Real>+","+TypeName<Int>+">";
        }

    }; // AABBTree2
    
} // namespace KnotTools
