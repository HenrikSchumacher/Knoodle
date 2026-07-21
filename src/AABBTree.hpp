#pragma once

namespace Knoodle
{

    /*!@brief A simple bounding volume hierarchy with static ordering. It works with both floating-point types and signed integral types.
     *
     *  This is specifically written for line segment primitives for curves. I may also deal with triangles or convex polytopes spanned by a few points, but that use case is not really intended. So we do not provide any guarantees.
     *
     *  @tparam AmbDim_ The ambient dimension, i.e., the dimension of the Euclidean space in which the tree is to be build.
     *
     *  @tparam Real_ The scalar type for the coordinates of points or primitives.
     *
     *  @tparam Int_ And integral type used for indices.
     *
     *  @tparam BReal_ The scalar type used for storing bounding volumes. If `Real_` is `double` one can safe a lot of memory here by using `float` as `BReal_. This won't worsen the accuracy of the computations in this class.
     *
     *  @tparam precompute_rangesQ_ If you make frequent use of `NodeBegin` and `NodeEnd`, then you might want to activate this to accelerate execution.
     *
     *  @tparam change_rounding_modeQ It only makes a difference when `Real_` is `double` and `BReal_` is float. If set to `true`, then the program alters the rounding mode of the CPU to accelerate computations. Since messing around with CPU flags is dangerous, we also provide as slightly an alternative; it is a bit slower, though.
     */
    
    template<
        int AmbDim_, typename Real_, IntQ Int_, typename BReal_ = Real_,
        bool precompute_rangesQ_   = true,
        bool change_rounding_modeQ = true
    >
    class alignas( ObjectAlignment ) AABBTree : public CompleteBinaryTree<Int_,precompute_rangesQ_>
    {
        static_assert( (FloatQ<Real_> && FloatQ<BReal_>) || (SignedIntQ<Real_> && SignedIntQ<BReal_>) , "Either both types are floating-point types or both are signed integral types.");
         
        static_assert(!IntQ<Real_> || SameQ<Real_,BReal_>, "In the integal case, the types need to coincide.");
        
        static_assert( !(SameQ<Real_,float> && SameQ<Real_,double>) , "While it would technically work fine, there is no point in storing the bounding boxes with higher precision than the geometric primitives.");
        
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
        
        /*!@brief Allocates and returns a container of type `BContainer_T` that is suitable to store the bounding boxes for all the nodes of the tree.
         */
        
        BContainer_T AllocateBoxes() const
        {
            return BContainer_T(NodeCount());
        }
        
        /*!@brief Computes the bounding boxes of the whole tree from the coordinates of a list of primitives. Primitives are assumed to be the convex hull of finitely many points.
         *
         * @tparam point_count Number of points in the primitive.
         *
         * @tparam dimP `P` is assumed to have dimensions `prim_count x point_count x dimP`. Here we require that `dimP >= AmbDim`. This sounds odd, but our main use case is creating a planar diagrams; here `dimP = 3` and `AmbDim = 2`.
         *
         * @tparam inc Normally, `inc` should not be changed. However, if one wants to handle edges of a polygon whose vertex coordinates are stored consecutively in `P` (with a duplicate of the first vertex coordinate at the end of `P`, then one can set `inc` to `dimP` to compute the bounding boxes without first making copies of the vertices in a container of type `EContainer_T`.
         *
         * @param P Array that represents the primitive coordinates. It is assumed to have size `prim_count x point_count x dimP`.
         *
         * @param B Represents the container for the boxes. It is assumed to be an 3D array of dimensions `prim_count x 2 x AmbDim`. Each box is stored in the format `{ { lo[0],...,lo[d-1]}, { hi[0],...,hi[d-1]} }`.
         */
        
        template<Int point_count, Int dimP, Int inc = point_count * dimP>
        void ComputeBoundingBoxes( cptr<Real> P, mptr<BReal> B ) const
        {
            TOOLS_PTIMER(timer,MethodName("ComputeBoundingBoxes"));
            
            static_assert(dimP >= AmbDim,"");
            
            constexpr Int d = AmbDim;
            
            constexpr bool rounding_neededQ = SameQ<Real,double> && SameQ<BReal,float>;
            // If Real and BReal are integral, then no rounding is needed.
            // If both Real and BReal are floating-point types, then each of them is either float or double.
            // Rounding is needed only if we convert from the higher precision (double) to the lower (float).

            
            // We might round from double to float here. We have to guarantee that the boxes won't become smaller by this. There are various ways to do that. The fastes way seems to change the rounding mode.
            
            if constexpr( rounding_neededQ && change_rounding_modeQ )
            {
                ScopedRoundingMode mode (FE_UPWARD);
                
                auto primitive_to_box = []( cptr<Real> p, mptr<BReal> b )
                {
                    Vector_T lo(p);
                    Vector_T hi(p);
                    
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
            else
            {
                TOOLS_MAKE_FP_STRICT()
                
                constexpr BReal down = std::numeric_limits<BReal>::lowest();
                constexpr BReal up   = std::numeric_limits<BReal>::max();
                
                auto primitive_to_box = []( cptr<Real> p, mptr<BReal> b )
                {
                    Vector_T lo(p);
                    Vector_T hi(p);
                    
                    for( Int i = 1; i < point_count; ++i )
                    {
                        lo.ElementwiseMin(&p[dimP * i]);
                        hi.ElementwiseMax(&p[dimP * i]);
                    }
                    
                    if constexpr ( rounding_neededQ )
                    {
                        for( Int k = 0; k < AmbDim; ++k )
                        {
                            // We artificially enlarge the boxes by the smallest possible offset.
                            b[    k] = std::nextafter(static_cast<BReal>(lo[k]), down);
                            b[d + k] = std::nextafter(static_cast<BReal>(hi[k]), up  );
                        }
                    }
                    else
                    {
                        for( Int k = 0; k < AmbDim; ++k )
                        {
                            b[    k] = lo[k];
                            b[d + k] = hi[k];
                        }
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
        
        /*! @brief Checks whether two bounding boxes intersect.
         *
         *  @param B_i Pointer to first box stored in the format [ lo[0],...,lo[d-1], hi[0],...,hi[d-1] ]
         *
         *  @param B_j Pointer to second box stored in the format [ lo[0],...,lo[d-1], hi[0],...,hi[d-1] ]
         */
        
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
        
        /*! @brief Checks whether two bounding boxes intersect.
         *
         *  @param B_0 One instance of BContainer_T that stores multiple bounding boxes
         *
         *  @param i Index of a box in `B_0`.
         *
         *  @param B_1 One instance of BContainer_T that stores multiple bounding boxes
         *
         *  @param j Index of a box in `B_1`.
         */
        
        bool BoxesIntersectQ(
            cref<BContainer_T> B_0, const Int i, cref<BContainer_T> B_1, const Int j
        )
        {
            return BoxesIntersectQ( B_0.data(i), B_1.data(j) );
        }
        
        
        /*! @brief Compute the squared distance between two boxes. CAUTION: This may overflow if integral types are used. It is in the user's responsitbility to prevent this!
         *
         *  @param B_i Pointer to first box stored in the format [ lo[0],...,lo[d-1], hi[0],...,hi[d-1] ]
         *
         *  @param B_j Pointer to second box stored in the format [ lo[0],...,lo[d-1], hi[0],...,hi[d-1] ]
         */
        
        static constexpr Real BoxBoxSquaredDistance(
            const cptr<Real> B_i, const cptr<Real> B_j
        )
        {
            TOOLS_MAKE_FP_FAST();
            
            Real d2 = 0;
            
            for( Int k = 0; k < AmbDim; ++k )
            {
                const Real x = Ramp(
                    Max( B_i[AmbDim * 0 + k], B_j[AmbDim * 0 + k] )
                    -
                    Min( B_i[AmbDim * 1 + k], B_j[AmbDim * 1 + k] )
                );
                
                d2 += x * x;
            }
            return d2;
        }
        
        /*! @brief Compute the squared distance between two boxes. CAUTION: This may overflow if integral types are used. It is in the user's responsitbility to prevent this!
         *
         *  @param B_0 One instance of BContainer_T that stores multiple bounding boxes
         *
         *  @param i Index of a box in `B_0`.
         *
         *  @param B_1 One instance of BContainer_T that stores multiple bounding boxes
         *
         *  @param j Index of a box in `B_1`.
         */
        
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
