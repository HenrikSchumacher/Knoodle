#pragma once

namespace Knoodle
{
    
    // A simple bounding volume hierarchy with static ordering.
    // This is specifically written for line segment primitives for curves.
    // It won't work with more general clouds of primitives.
    
    template<
        int AmbDim_, IntQ Real_, IntQ Idx_, /*IntQ BReal_ = Real_,*/
        bool precompute_rangesQ_   = true
    >
    class alignas( ObjectAlignment ) AABBTree_Int : public CompleteBinaryTree<Int_,precompute_rangesQ_>
    {
        
    public:
        
        using Real  = Real_;
//        using BReal = BReal_;
        using BReal = Real_;
        using Idx   = Idx_;
        
        static constexpr bool precompute_rangesQ = precompute_rangesQ_;
        
        using Base_T = CompleteBinaryTree<Idx,precompute_rangesQ>;
        using Base_T::max_depth;

        
        static constexpr Idx AmbDim = AmbDim_;
        static constexpr Idx BoxDim = 2 * AmbDim;
        
        using Vector_T     = Tiny::Vector<AmbDim_,Real,Idx>;
        
        using EContainer_T = Tiny::MatrixList_AoS<2,AmbDim,Real ,Idx>;
        using BContainer_T = Tiny::MatrixList_AoS<2,AmbDim,BReal,Idx>;
        
        using UInt = ToUnsigned<Idx>;
        
        explicit AABBTree_Int( const Idx prim_count_  )
        :   Base_T  ( prim_count_ )
        {}
        
        // Default constructor
        AABBTree_Int() = default;
        // Destructor (virtual because of inheritance)
        virtual ~AABBTree_Int() = default;
        // Copy constructor
        AABBTree_Int( const AABBTree_Int & other ) = default;
        // Copy assignment operator
        AABBTree_Int & operator=( const AABBTree_Int & other ) = default;
        // Move constructor
        AABBTree_Int( AABBTree_Int && other ) = default;
        // Move assignment operator
        AABBTree_Int & operator=( AABBTree_Int && other ) = default;

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
        
    public:
        
        BContainer_T AllocateBoxes() const
        {
            return BContainer_T(NodeCount());
        }
        
        // TODO: Transpose this in the description.
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
         * @param B Represents the container for the boxes. It is assumed to be an 3D array of dimensions `prim_count x 2 x AmbDim`.
         */
        
        template<Idx point_count, Idx dimP, Idx inc = point_count * dimP>
        void ComputeBoundingBoxes( cptr<Real> P, mptr<BReal> B ) const
        {
            TOOLS_PTIMER(timer,MethodName("ComputeBoundingBoxes"));
            
            static_assert(dimP >= AmbDim,"");
            
            constexpr Idx d = AmbDim;
            
            auto primitive_to_box = []( cptr<Real> p, mptr<BReal> b )
            {
                Vector_T lo(p);
                Vector_T hi(p);
                
                for( Idx i = 1; i < point_count; ++i )
                {
                    lo.ElementwiseMin(&p[dimP * i]);
                    hi.ElementwiseMax(&p[dimP * i]);
                }
            
                // This is where the rounding would happens.
                for( Idx k = 0; k < AmbDim; ++k )
                {
                    b[    k] = lo[k];
                    b[d + k] = hi[k];
                }
            };

            // Compute bounding boxes of leave nodes (last row of tree).
            for( Idx N = last_row_begin; N < node_count; ++N )
            {
                const Idx i = N - last_row_begin;
                // Here is where the rounding takes place.
                primitive_to_box( &P[inc * i], &B[BoxDim * N] );
            }
            
            // Compute bounding boxes of leave nodes (penultimate row of tree).
            for( Idx N = int_node_count; N < last_row_begin; ++N )
            {
                const Idx i = N + offset;
                // Here is where the rounding takes place.
                primitive_to_box( &P[inc * i], &B[BoxDim * N] );
            }
            
            // Compute bounding boxes of internal nodes.
            for( Idx N = int_node_count; N --> Idx(0);  )
            {
                const auto [L,R] = Children(N);
                
                elementwise_min<d>(&B[BoxDim * L + 0], &B[BoxDim * R + 0], &B[BoxDim * N + 0]);
                elementwise_max<d>(&B[BoxDim * L + d], &B[BoxDim * R + d], &B[BoxDim * N + d]);
            }
            
        }
        
        static constexpr bool BoxesIntersectQ(
            const cptr<BReal> B_i, const cptr<BReal> B_j
        )
        {
            // B_i and B_j are assumed to have size 2 x AmbDim.
            for( Idx k = 0; k < AmbDim; ++k )
            {
                if( (B_i[k] > B_j[AmbDim + k]) || (B_j[k] > B_i[AmbDim + k]) )
                {
                    return false;
                }
            }
            
            return true;
        }
        
        bool BoxesIntersectQ(
            cref<BContainer_T> B_0, const Idx i, cref<BContainer_T> B_1, const Idx j
        )
        {
            return BoxesIntersectQ( B_0.data(i), B_1.data(j) );
        }
        
    public:
        
        Size_T AllocatedByteCount() const
        {
            return Base_T::N_ranges.AllocatedByteCount();
        }
        
        Size_T ByteCount() const
        {
            return sizeof(AABBTree_Int) + AllocatedByteCount();
        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("AABBTree_Int")
            + "<" + ToString(AmbDim)
            + "," + TypeName<Real>
            + "," + TypeName<Idx>
            + "," + TypeName<BReal>
            + "," + ToString(precompute_rangesQ)
            + ">";
        }

    }; // AABBTree_Int

} // namespace Knoodle
