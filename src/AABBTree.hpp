#pragma once

namespace Knoodle
{
    
    // A simple bounding volume hierarchy with static ordering.
    // This is specifically written for line segment primitives for curves.
    // It won't work with more general clouds of primitives.
    
    template<
        int AmbDim_, typename Real_, typename Int_, typename BReal_ = Real_,
        bool precompute_rangesQ_   = true,
        bool change_rounding_modeQ = true
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
        
        using EContainer_T = Tiny::MatrixList_AoS<2,AmbDim,Real,Int>;
        using BContainer_T = Tiny::MatrixList_AoS<2,AmbDim,BReal,Int>;
        
        using UInt = ToUnsigned<Int>;
        
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
        
        template<Int point_count, Int dimP, Int inc = point_count * dimP>
        void ComputeBoundingBoxes( cptr<Real> P, mptr<BReal> B ) const
        {
            TOOLS_PTIMER(timer,MethodName("ComputeBoundingBoxes"));
            
            static_assert(dimP >= AmbDim,"");
            
            constexpr Int d = AmbDim;
            
            // We might round from double to float here. We have to guarantee that the boxes won't become smaller by this. This is why we set the rounding mode to "upwards".
            if constexpr( change_rounding_modeQ )
            {
                #pragma STDC FENV_ACCESS ON
                std::fesetround(FE_UPWARD);
//                change_rounding_mode
                auto primitive_to_box = []( cptr<Real> p, mptr<BReal> b )
                {
                    Tiny::Vector<AmbDim,Real,Int> lo(p);
                    Tiny::Vector<AmbDim,Real,Int> hi(p);
                    
                    for( Int i = 1; i < point_count; ++i )
                    {
                        lo.ElementwiseMin(&p[dimP * i]);
                        hi.ElementwiseMax(&p[dimP * i]);
                    }
                
                    // This is where the rounding happens.
                    for( Int k = 0; k < AmbDim; ++k )
                    {
                        b[    k] = -static_cast<BReal>(-lo[k]); // because of upward rounding mode
                        b[d + k] =  static_cast<BReal>( hi[k]);
                    }
                    
//                    // This would work, but it is much slower.
//                    for( Int k = 0; k < AmbDim; ++k )
//                    {
//                        b[    k] = PrevFloat(static_cast<BReal>(lo[k]));
//                        b[d + k] = nextFloat(static_cast<BReal>(hi[k]));
//                    }
                };

                // Compute bounding boxes of leave nodes (last row of tree).
                for( Int N = last_row_begin; N < node_count; ++N )
                {
                    const Int i = N - last_row_begin;
                    // Here is where the rounding takes place.
                    primitive_to_box( &P[inc * i], &B[BoxDim * N] );
                }
                
                // Compute bounding boxes of leave nodes (penultimate row of tree).
                for( Int N = int_node_count; N < last_row_begin; ++N )
                {
                    const Int i = N + offset;
                    // Here is where the rounding takes place.
                    primitive_to_box( &P[inc * i], &B[BoxDim * N] );
                }
                
                // end of #pragma STDC FENV_ACCESS ON
            }
            else
            {
                auto primitive_to_box = []( cptr<Real> p, mptr<BReal> b )
                {
                    Tiny::Vector<AmbDim,Real,Int> lo(p);
                    Tiny::Vector<AmbDim,Real,Int> hi(p);
                    
                    for( Int i = 1; i < point_count; ++i )
                    {
                        lo.ElementwiseMin(&p[dimP * i]);
                        hi.ElementwiseMax(&p[dimP * i]);
                    }
                    
                    // This would work, but it is much slower.
                    for( Int k = 0; k < AmbDim; ++k )
                    {
                        b[    k] = PrevFloat(static_cast<BReal>(lo[k]));
                        b[d + k] = nextFloat(static_cast<BReal>(hi[k]));
                    }
                };

                // Compute bounding boxes of leave nodes (last row of tree).
                for( Int N = last_row_begin; N < node_count; ++N )
                {
                    const Int i = N - last_row_begin;
                    // Here is where the rounding takes place.
                    primitive_to_box( &P[inc * i], &B[BoxDim * N] );
                }

                // Compute bounding boxes of leave nodes (penultimate row of tree).
                for( Int N = int_node_count; N < last_row_begin; ++N )
                {
                    const Int i = N + offset;
                    // Here is where the rounding takes place.
                    primitive_to_box( &P[inc * i], &B[BoxDim * N] );
                }
            }
            
            // Compute bounding boxes of internal nodes.
            for( Int N = int_node_count; N --> Int(0);  )
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
            for( Int k = 0; k < AmbDim; ++k )
            {
                if( (B_i[k] > B_j[AmbDim + k]) || (B_j[k] > B_i[AmbDim + k]) )
                {
                    return false;
                }
            }
            
            return true;
        }
        
        Real BoxesIntersectQ(
            cref<BContainer_T> B_0, const Int i, cref<BContainer_T> B_1, const Int j
        )
        {
            return BoxesIntersectQ( B_0.data(i), B_1.data(j) );
        }
        
        
        static constexpr Real BoxBoxSquaredDistance(
            const cptr<Real> B_i, const cptr<Real> B_j
        )
        {
            TOOLS_MAKE_FP_FAST();
            
            Real d2 = 0;
            
            for( Int k = 0; k < AmbDim; ++k )
            {
//                const Real x = Ramp(
//                    Max( B_i[2 * k + 0], B_j[2 * k + 0] )
//                    -
//                    Min( B_i[2 * k + 1], B_j[2 * k + 1] )
//                );
                
                const Real x = Ramp(
                    Max( B_i[AmbDim * 0 + k], B_j[AmbDim * 0 + k] )
                    -
                    Min( B_i[AmbDim * 1 + k], B_j[AmbDim * 1 + k] )
                );
                
                d2 += x * x;
            }
            return d2;
        }
        
        static constexpr Real BoxBoxSquaredDistance(
            cref<BContainer_T> B_0, const Int i, cref<BContainer_T> B_1, const Int j
        )
        {
            return BoxBoxSquaredDistance( B_0.data(i), B_1.data(j) );
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
