#pragma once

namespace Knoodle
{
    
    // A simple bounding volume hierarchy with static ordering.
    // This is specifically written for edge and triangle primitives for curves.
    // It won't work with more general clouds of primitives.
    
    template<
        int AmbDim_, typename Real_, typename Int_, typename BReal_ = Real_,
        bool precompute_rangesQ_ = true
    >
    class alignas( ObjectAlignment ) AABBTree : public CompleteBinaryTree<Int_,precompute_rangesQ_>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(FloatQ<BReal_>,"");
        
    public:
        
        using Real  = Real_;
        using BReal = BReal_;
        using Int   = Int_;
        
        static constexpr bool precompute_rangesQ = precompute_rangesQ_;
        
        using Base_T = CompleteBinaryTree<Int,precompute_rangesQ>;
        using Base_T::max_depth;

        
        static constexpr Int AmbDim = AmbDim_;
        static constexpr Int BoxDim = 2 * AmbDim;
        
        using Vector_T     = Tiny::Vector<AmbDim_,Real,Int>;
        
        using BContainer_T = Tensor3<BReal,Int>;
        
//        using EContainer_T = Tensor3<Real,Int>;
        
//        using BContainer_T = Tiny::MatrixList_AoS<AmbDim,2,Real,Int>;
        
        using EContainer_T = Tiny::MatrixList_AoS<2,AmbDim,Real,Int>;
        
        using UInt = Scalar::Unsigned<Int>;
        
        explicit AABBTree( const Int prim_count_  )
        :   Base_T  ( prim_count_ )
        {}
        
        // Default constructor
        AABBTree() = default;
        // Destructor (virtual because of inheritance)
        virtual ~AABBTree() = default;
        // Copy constructor
        AABBTree( const AABBTree & other ) = default;
        // Copy assignment operator
        AABBTree & operator=( const AABBTree & other ) = default;
        // Move constructor
        AABBTree( AABBTree && other ) = default;
        // Move assignment operator
        AABBTree & operator=( AABBTree && other ) = default;

    protected:
        
        // Integer data for the combinatorics of the tree.
        
        using Base_T::leaf_node_count;
        using Base_T::node_count;
        using Base_T::int_node_count;
        using Base_T::last_row_begin;
        using Base_T::offset;
        
    public:
        
        using Base_T::MaxDepth;
        using Base_T::NodeCount;
        using Base_T::InternalNodeCount;
        using Base_T::LeafNodeCount;
        
        using Base_T::RightChild;
        using Base_T::LeftChild;
        using Base_T::Children;
        using Base_T::Parent;
        using Base_T::Depth;
        using Base_T::Column;
        using Base_T::NodeBegin;
        using Base_T::NodeEnd;
        
    private:
        
        /*!
         * @brief Computes the bounding boxes from the coordinates of a list of primitives. Primitives are assumed to be the convex hull of finitely many points. Only the leading `AmbDim` entries of each point are used.
         *
         *If the precision of `Breal`, the type for storing the bounding boxes, is lower than `Real`, then the boxes will be correctly enlarged to guarantee that they contain the boxes as if compute with `BReal = Real`.
         *
         * @tparam point_count Number of points in the primitive.
         *
         * @tparam dimP `dimP` is assumed to have dimensions `prim_count x point_count x dimP`. Here we require that `dimP >= AmbDim`. This sound off, but our main use case is creating planar diagrams; here `dimP = 3` and `AmbDim = 2`.
         *
         * @param P Array that represent a single primitive. It is assumed to have size `point_count x dimP`.
         *
         * @param B Represents a single box. It is assumed to be of dimensions `AmbDim x 2`.
         */
        
        template<Int point_count, Int dimP>
        static constexpr void PrimitiveToBox(
            cptr<Real> P, mptr<BReal> B
        )
        {
            static_assert(point_count > Int(0), "");
            
            // TODO: If we store upper and lower bounds as vectors, then this can be vectorized?
            
            columnwise_minmax<point_count,AmbDim>(
                P, dimP, &B[0], Size_T(2), &B[1], Size_T(2)
            );
            
            // If we have round, make sure that the boxes become a little bit bigger, so that it still contains all primitives.
            if constexpr ( Scalar::Prec<BReal> < Scalar::Prec<Real> )
            {
                for( Int k = 0; k < AmbDim; ++k )
                {
                    B[2 * k + 0] = PrevFloat(B[2 * k + 0]);
                    B[2 * k + 1] = NextFloat(B[2 * k + 1]);
                }
            }
        }
        
        static constexpr void BoxesToBox(
            cptr<BReal> B_L, cptr<BReal> B_R, mptr<BReal> B_N
        )
        {
            // TODO: Can this be vectorized?
            // Yes, if we store upper and lower bounds as vectors!
            
            for( Int k = 0; k < AmbDim; ++k )
            {
                B_N[2 * k + 0] = Min( B_L[2 * k + 0], B_R[2 * k + 0] );
                B_N[2 * k + 1] = Max( B_L[2 * k + 1], B_R[2 * k + 1] );
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
         * @tparam inc Normally, `inc` should not be changed. However, if one wants to handle edges of a polygon whose vertex coordinates are stored consecutively in `P` (with a dupplicate of the first vertex coordinate at the end of `P`, then one can set `inc` to `dimP` to to compute the bounding boxes without first makeing copies of the vertices in to a `EContainer_T`.
         *
         * @param P Array that represents the primitive coordinates. It is assumed to have size `prim_count x point_count x dimP`.
         *
         * @param B Represents the container for the boxes. It is assumed to be a `Tensor3` of dimensions `prim_count x AmbDim x 2`.
         */
        
        template<Int point_count, Int dimP, Int inc = point_count * dimP>
        void ComputeBoundingBoxes( cptr<Real> P, mptr<BReal> B ) const
        {
            TOOLS_PTIC(ClassName()+"::ComputeBoundingBoxes");
            
            static_assert(dimP >= AmbDim,"");
            
            TOOLS_PTIC("Compute bounding boxes of leave nodes.");
            // Compute bounding boxes of leave nodes (last row of tree).
            for( Int N = last_row_begin; N < node_count; ++N )
            {
                const Int i = N - last_row_begin;
                
                PrimitiveToBox<point_count,dimP>( &P[inc * i], &B[BoxDim * N] );
            }
            
            // Compute bounding boxes of leave nodes (penultimate row of tree).
            for( Int N = int_node_count; N < last_row_begin; ++N )
            {
                const Int i = N + offset;

                PrimitiveToBox<point_count,dimP>( &P[inc * i], &B[BoxDim * N] );
            }
            TOOLS_PTOC("Compute bounding boxes of leave nodes.");
            
            TOOLS_PTIC("Compute bounding boxes of internal nodes.");
            // Compute bounding boxes of internal nodes.
            for( Int N = int_node_count; N --> Int(0);  )
            {
                const auto [L,R] = Children(N);
                
                BoxesToBox( &B[BoxDim * L], &B[BoxDim * R], &B[BoxDim * N] );
            }
            TOOLS_PTOC("Compute bounding boxes of internal nodes.");
            
            TOOLS_PTOC(ClassName()+"::ComputeBoundingBoxes");
        }
        
        template<Int point_count, Int dimP>
        void ComputeBoundingBoxes( cref<EContainer_T> E, mref<BContainer_T> B )
        {
            ComputeBoundingBoxes<point_count,dimP,point_count*dimP>( E.data(), B.data() );
        }
        
        static constexpr bool BoxesIntersectQ( const cptr<BReal> B_i, const cptr<BReal> B_j )
        {
            // B_i and B_j are assumed to have size AmbDim x 2.
            
//            if constexpr ( VectorizableQ<BReal> )
//            {
//                vec_T<2*AmbDim,BReal> a;
//                vec_T<2*AmbDim,BReal> b;
//                
//                for( Int k = 0; k < AmbDim; ++k )
//                {
//                    a[2 * k + 0] = B_i[2 * k + 0];
//                    a[2 * k + 1] = B_j[2 * k + 0];
//                    b[2 * k + 0] = B_j[2 * k + 1];
//                    b[2 * k + 1] = B_i[2 * k + 1];
//                }
//                
//                return __builtin_reduce_and( a <= b );
//            }
//            else
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
        
        static constexpr Real BoxBoxSquaredDistance(
            const cptr<Real> B_i, const cptr<Real> B_j
        )
        {
            Real d2 = 0;
            
            for( Int k = 0; k < AmbDim; ++k )
            {
                const Real x = Ramp(
                    Max( B_i[2 * k + 0], B_j[2 * k + 0] )
                    -
                    Min( B_i[2 * k + 1], B_j[2 * k + 1] )
                );
                
                d2 += x * x;
            }
            return d2;
        }
        
        static constexpr Real BoxBoxSquaredDistance(
            cref<BContainer_T> B_0, const Int i, cref<BContainer_T> B_1, const Int j
        )
        {
            return BoxBoxSquaredDistance(
                &B_0.data()[BoxDim * i], &B_1.data()[BoxDim * j]
            );
        }
        
        BContainer_T AllocateBoxes()
        {
            TOOLS_PTIC(ClassName()+"::AllocateBoxes");
            
            BContainer_T B (NodeCount(),AmbDim,2);
            
            TOOLS_PTOC(ClassName()+"::AllocateBoxes");
            
            return B;
        }
        
    public:
        
        Size_T AllocatedByteCount() const
        {
            return Base_T::N_ranges.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(AABBTree) + AllocatedByteCount();
        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("AABBTree")
            + "<" + ToString(AmbDim)
            + "," + TypeName<Real>
            + "," + TypeName<Int>
            + "," + TypeName<BReal>
            + "," + ToString(precompute_rangesQ)
            + ">";
        }

    }; // AABBTree

} // namespace Knoodle
