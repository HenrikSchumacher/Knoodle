#pragma once

#include "../deps/pcg-cpp/include/pcg_random.hpp"

// TODO: Use a structure ClisbyNode and hold it in stack memory.

// TODO: Check neighborhood about pivots more thoroughly.

// TODO: _Compute_ NodeRange more efficiently.

// TODO: Shave off one double form ClangQuaternionTransform at the cost of a sqrt.

// TODO: Update(p,q) -- Could thise be faster?
// TODO:    1. Start at the pivots, walp up to the root and track all the touched nodes;
// TODO:       We can also learn this way which nodes need an update.
// TODO:    2. Go down again and push the transforms out of the visited nodes.
// TODO:    3. Grab the pivot vertex positions and compute the transform.
// TODO:    4. Walk once again top down through the visited nodes and update their transforms.
// TODO:    5. Walk from the pivots upwards and update the balls.

namespace Knoodle
{
    struct ClisbyTree_TArgs
    {
        bool clang_matrixQ  = true;
        bool quaternionsQ   = true;
        bool countersQ      = false;   // debugging flag
        bool witnessesQ     = false;   // debugging flag
        bool manual_stackQ  = false;   // debugging flag
    };
    
    template<
        int AmbDim_,
        typename Real_, typename Int_, typename LInt_,
        ClisbyTree_TArgs targs = ClisbyTree_TArgs()
    >
    class alignas( ObjectAlignment ) ClisbyTree : public CompleteBinaryTree<Int_,true>
//    class alignas( ObjectAlignment ) ClisbyTree : public CompleteBinaryTree_Precomp<Int_>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(SignedIntQ<Int_>,"");
        static_assert(SignedIntQ<LInt_>,"");
        static_assert( AmbDim_ == 3, "Currently only implemented in dimension 3." );
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using LInt   = LInt_;
        
        using Tree_T = CompleteBinaryTree<Int,true>;
    //        using Tree_T = CompleteBinaryTree_Precomp<Int>;
    
        using SInt = typename Tree_T::SInt;
        
        using Tree_T::max_depth;
        
        static constexpr Int AmbDim = AmbDim_;
    
        static constexpr bool clang_matrixQ = targs.clang_matrixQ && MatrixizableQ<Real>;
        static constexpr bool quaternionsQ  = clang_matrixQ && targs.quaternionsQ;
        static constexpr bool manual_stackQ = targs.manual_stackQ;
        static constexpr bool witnessesQ    = targs.witnessesQ;
        
        using Transform_T
            = typename std::conditional_t<
                  clang_matrixQ,
                  typename std::conditional_t<
                      quaternionsQ,
                      ClangQuaternionTransform<Real,Int>,
                      ClangAffineTransform<AmbDim,Real,Int>
                  >,
                  AffineTransform<AmbDim,Real,Int>
              >;
    
        using NodeFlag_T               = AffineTransformFlag_T;
        using Vector_T                 = typename Transform_T::Vector_T;
        using Matrix_T                 = typename Transform_T::Matrix_T;
        using FoldFlag_T               = Int32;
        using FoldFlagCounts_T         = Tiny::Vector<5,LInt,FoldFlag_T>;
        
        using WitnessVector_T          = Tiny::Vector<2,Int,Int>;
        using WitnessCollector_T       = std::vector<Tiny::Vector<4,Int,Int>>;
        using PivotCollector_T         = std::vector<std::tuple<Int,Int,Real>>;
        
        
        using NodeFlagContainer_T      = Tensor1<NodeFlag_T,Int>;
        using NodeTransformContainer_T = Tensor2<Real,Int>;
        using NodeBallContainer_T      = Tensor2<Real,Int>;
    
        using NodeSplitFlagVector_T    = Tiny::Vector<2,bool,Int>;
        using NodeSplitFlagMatrix_T    = Tiny::Matrix<2,2,bool,Int>;

    //        using NodeSplitFlagVector_T = std::array<bool,2>;
    //        using NodeSplitFlagMatrix_T = std::array<NodeSplitFlagVector_T,2>;
    
        using RNG_T  = std::random_device;
    
        using PRNG_T = pcg64;
        static constexpr Size_T PRNG_T_state_size = 4;
    
        static constexpr Size_T seed_size  = (PRNG_T_state_size * sizeof(PRNG_T::result_type)) / sizeof(RNG_T::result_type);
    
        using Seed_T = std::array<RNG_T::result_type,seed_size>;
        
        // For center, radius, rotation, and translation.
        static constexpr Int TransformDim = Transform_T::Size();
        static constexpr Int BallDim      = AmbDim + 1;
        
        static constexpr bool countersQ = targs.countersQ;
        
        struct CallCounters_T
        {
            Size_T overlap        = 0;
            
            Size_T mm             = 0;
            Size_T mv             = 0;
            Size_T load_transform = 0;
        };
    
        
        enum class UpdateFlag_T : std::int_fast8_t
        {
            DoNothing = 0,
            Update    = 1,
            Split     = 2
        };
        
        ClisbyTree() = default;

        template<typename ExtReal, typename ExtInt>
        ClisbyTree(
            const ExtInt vertex_count_,
            const ExtReal hard_sphere_diam_
        )
        :   Tree_T                      { static_cast<Int>(vertex_count_)       }
        ,   N_transform                 { InteriorNodeCount(), TransformDim     }
        ,   N_state                     { InteriorNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount(), BallDim                  }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_)  }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam   }
        {
            InitializeTransforms();
            SetToCircle();
            InitializePRNG();
        }
        
        template<typename ExtReal, typename ExtInt>
        ClisbyTree(
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal hard_sphere_diam_
        )
        :   Tree_T                      { static_cast<Int>(vertex_count_)       }
        ,   N_transform                 { InteriorNodeCount(), TransformDim     }
        ,   N_state                     { InteriorNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount(), BallDim                  }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_)  }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam   }
        {
            InitializeTransforms();
            ReadVertexCoordinates( vertex_coords_ );
            InitializePRNG();
        }
    
        template<typename ExtReal, typename ExtInt>
        ClisbyTree(
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal hard_sphere_diam_,
            PRNG_T prng
        )
        :   Tree_T                      { static_cast<Int>(vertex_count_)       }
        ,   N_transform                 { InteriorNodeCount(), TransformDim     }
        ,   N_state                     { InteriorNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount(), BallDim                  }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_)  }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam   }
        ,   random_engine               { prng                                  }
        {
            InitializeTransforms();
            ReadVertexCoordinates( vertex_coords_ );
        }

        ~ClisbyTree() = default;
    
////        // Copy constructor
//        ClisbyTree( const ClisbyTree & other )
//        :   Tree_T                      { other                             }
//        ,   N_transform                 { other.N_transform                 }
//        ,   N_state                     { other.N_state                     }
//        ,   N_ball                      { other.N_ball                      }
//        ,   hard_sphere_diam            { other.hard_sphere_diam            }
//        ,   hard_sphere_squared_diam    { other.hard_sphere_squared_diam    }
//        ,   prescribed_edge_length      { other.prescribed_edge_length      }
//        ,   p                           { other.p                           }
//        ,   q                           { other.q                           }
//        ,   witness                     { other.witness                     }
//        ,   theta                       { other.theta                       }
//        ,   X_p                         { other.X_p                         }
//        ,   X_q                         { other.X_q                         }
//        ,   transform                   { other.transform                   }
//        ,   seed                        { other.seed                        }
//        ,   random_engine               { other.random_engine               }
//        ,   call_counters               { other.call_counters               }
//        ,   mid_changedQ                { other.mid_changedQ                }
//        ,   transforms_pushedQ          { other.transforms_pushedQ          }
//        {}
//
//        inline friend void swap( ClisbyTree & A, ClisbyTree & B) noexcept
//        {
//            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
//            using std::swap;
//            
//            if( &A == &B )
//            {
//                wprint( std::string("An object of type ") + ClassName() + " has been swapped to itself.");
//            }
//            else
//            {
//                swap( static_cast<Tree_T &>(A), static_cast<Tree_T &>(B) );
//                
//                swap( A.N_transform,                B.N_transform );
//                swap( A.N_state,                    B.N_state );
//                swap( A.N_ball,                     B.N_ball );
//                
//                swap( A.hard_sphere_diam,           B.hard_sphere_diam );
//                swap( A.hard_sphere_squared_diam,   B.hard_sphere_squared_diam );
//                swap( A.prescribed_edge_length,     B.prescribed_edge_length );
//                
//                swap( A.p,                          B.p );
//                swap( A.q,                          B.q );
//                
//                swap( A.witness,                    B.witness );
//
//                swap( A.theta,                      B.theta );
//                swap( A.X_p,                        B.X_p );
//                swap( A.X_q,                        B.X_q );
//                swap( A.transform,                  B.transform );
//                
//                swap( A.seed,                       B.seed );
//                swap( A.random_engine,              B.random_engine );
//                swap( A.call_counters,              B.call_counters );
//                
//                swap( A.mid_changedQ,               B.mid_changedQ );
//                swap( A.transforms_pushedQ,         B.transforms_pushedQ );
//            }
//        }
//    
////        /* Copy assignment operator */
////        ClisbyTree & operator=( ClisbyTree other ) noexcept
////        {
////            swap( *this, other );
////            return *this;
////        }
//
//        // Move constructor
//        ClisbyTree( ClisbyTree && other ) noexcept
//        :   ClisbyTree()
//        {
//            swap(*this, other);
//        }
//
//
//        /* Move-assignment operator */
//        mref<ClisbyTree> operator=( ClisbyTree && other ) noexcept
//        {
//            if( this == &other )
//            {
//                wprint("An object of type " + ClassName() + " has been move-assigned to itself.");
//            }
//            else
//            {
//                swap( *this, other );
//            }
//            return *this;
//        }
       
    public:
        
        using Tree_T::MaxDepth;
        using Tree_T::NodeCount;
        using Tree_T::InteriorNodeCount;
        using Tree_T::LeafNodeCount;
        
        using Tree_T::RightChild;
        using Tree_T::LeftChild;
        using Tree_T::Children;
        using Tree_T::Parent;
        using Tree_T::Depth;
        using Tree_T::Column;
        using Tree_T::NodeBegin;
        using Tree_T::NodeEnd;
        using Tree_T::NodeRange;
        using Tree_T::LeafNodeQ;
        using Tree_T::InteriorNodeQ;
        using Tree_T::PrimitiveNode;
        using Tree_T::Root;
        
        
    private:
        
        NodeTransformContainer_T N_transform;
        NodeFlagContainer_T      N_state;
        NodeBallContainer_T      N_ball;
        
        Real hard_sphere_diam           = 0;
        Real hard_sphere_squared_diam   = 0;
        Real prescribed_edge_length     = 1;
        
        Int p = 0;                      // Lower pivot index.
        Int q = 0;                      // Greater pivot index.
        
        WitnessVector_T witness {{-1,-1}};
    
        Real theta;                     // Rotation angle
        Vector_T   X_p;                 // Pivot location.
        Vector_T   X_q;                 // Pivot location.
        Transform_T transform;          // Pivot transformation.
        
        Seed_T seed;
        PRNG_T random_engine;
        
        mutable CallCounters_T call_counters;
    
        bool mid_changedQ = false;
        bool transforms_pushedQ = false;
        
        
        PivotCollector_T   pivot_collector;
        WitnessCollector_T witness_collector;
        
    private:
        
        void ResetTransform( const Int node )
        {
            N_state[node] = NodeFlag_T::Id;
        }
        
        void InitializeNodeFromVertex( const Int node, cptr<Real> x )
        {
            copy_buffer<AmbDim>( x, NodeCenterPtr(node) );
            NodeRadius(node) = 0;
        }
        
        void InitializeTransforms()
        {
            Transform_T id;
            id.SetIdentity();
            
            for( Int node = 0; node < InteriorNodeCount(); ++node )
            {
                id.ForceWrite( NodeTransformPtr(node), NodeFlag(node) );
            }
        }
        
        void InitializePRNG()
        {
            std::generate( seed.begin(), seed.end(), RNG_T() );
            SeedBy( seed );
        }
    
    public:
        
        PRNG_T RandomEngine()
        {
            return random_engine;
        }
        
        void SetRandomEngine( cref<PRNG_T> prng )
        {
            random_engine = prng;
        }
        
        void SeedBy( Seed_T & seed_ )
        {
            seed = seed_;
            
            std::seed_seq seed_sequence ( seed.begin(), seed.end() );
            
            random_engine = PRNG_T( seed_sequence );
        }
        
    public:
        
        void ReadVertexCoordinates( cptr<Real> x )
        {
            TOOLS_PTIC(ClassName() + "::ReadVertexCoordinates");
                
            this->DepthFirstSearch(
                []( const Int node )                    // interior node previsit
                {
                    (void)node;
                },
                [this]( const Int node )                // interior node postvisit
                {
                    ComputeBall(node);
                    ResetTransform(node);
                },
                [this,x]( const Int node )              // leaf node previsit
                {
                    const Int vertex = NodeBegin(node);

                    InitializeNodeFromVertex( node, &x[AmbDim * vertex] );
                },
                []( const Int node )                    // leaf node postvisit
                {
                    (void)node;
                }
            );
            
            TOOLS_PTOC(ClassName() + "::ReadVertexCoordinates");
        }

        
        void SetToCircle()
        {
            const Int n = VertexCount();
            
            Tensor2<Real,Int> X( n, AmbDim );
            
            mptr<Real> x = X.data();
            
            const double delta  = Frac<double>( Scalar::TwoPi<double>, n );
            const double radius = Frac<double>( 1, 2 * std::sin( Frac<double>(delta,2) ) );
            
            double v [AmbDim] = { double(0) };
            
            for( Int vertex = 0; vertex < n; ++vertex )
            {
                const double angle = delta * vertex;
                
                v[0] = radius * std::cos( angle );
                v[1] = radius * std::sin( angle );
                
                copy_buffer<AmbDim>( &v[0], &x[AmbDim * vertex] );
            }
            
            ReadVertexCoordinates(x);
        }

//#include "ClisbyTree/ClisbyNode.hpp"
#include "ClisbyTree/Access.hpp"
#include "ClisbyTree/Transformations.hpp"
#include "ClisbyTree/Update.hpp"
#include "ClisbyTree/CollisionChecks.hpp"
        
        
//###################################################################################
//##    Folding
//###################################################################################

    public:
    
        template<bool check_overlapsQ = true>
        FoldFlag_T Fold( const Int p_, const Int q_, const Real theta_ )
        {
            int pivot_flag = LoadPivots(p_,q_,theta_);
            
            if( pivot_flag != 0 )
            {
                // Folding step aborted because pivots indices are too close.
                return pivot_flag;
            }
            
            if constexpr ( check_overlapsQ )
            {
                int joint_flag = CheckJoints();
                
                if( joint_flag != 0 )
                {
                    if constexpr ( witnessesQ )
                    {
                        // Witness checking
                        witness_collector.push_back(
                            Tiny::Vector<4,Int,Int>({p,q,witness[0],witness[1]})
                        );
                    }   
                    
                    // Folding step failed because neighbors of pivot touch.
                    return joint_flag;
                }
            }
            
            Update();

            if constexpr ( check_overlapsQ )
            {
                if( OverlapQ() )
                {
                    // Folding step failed; undo the modifications.
                    Update(p_,q_,-theta_);
                    
                    if constexpr ( witnessesQ )
                    {
                        // Witness checking
                        witness_collector.push_back(
                            Tiny::Vector<4,Int,Int>({p,q,witness[0],witness[1]})
                        );
                    }
                    return 4;
                }
                else
                {
                    if constexpr ( witnessesQ )
                    {
                        // Witness checking
                        pivot_collector.push_back(
                            std::tuple<Int,Int,Real>({p,q,theta})
                        );
                    }
                    
                    // Folding step succeeded.
                    return 0;
                }
            }
            else
            {
                if constexpr ( witnessesQ )
                {
                    // Witness checking
                    pivot_collector.push_back(
                        std::tuple<Int,Int,Real>({p,q,theta})
                    );
                }
                
                return 0;
            }
        }
        
        
//        // Defect routine!!! Does not sample uniformly!
//        std::pair<Int,Int> RandomPivots_Legacy()
//        {
//            using unif_int  = std::uniform_int_distribution<Int>;
//
//            const Int n = VertexCount();
//
//            unif_int  u_int ( Int(0), n-3 );
//            
//            const Int i = u_int                        (random_engine);
//            const Int j = unif_int(i+2,n-1-(i==Int(0)))(random_engine);
//            
//            return std::pair<Int,Int>(i,j);
//        }
        
        std::pair<Int,Int> RandomPivots()
        {
            const Int n = VertexCount();
            
            using unif_int = std::uniform_int_distribution<Int>;
            
            unif_int u_int ( Int(0), n - Int(1) );
            
            
            Int i = u_int(random_engine);
            Int j = u_int(random_engine);

            while( ModDistance(n,i,j) < 2 )
            {
                i = u_int(random_engine);
                j = u_int(random_engine);
            }
            
            return MinMax(i,j);
        }

        
        template<bool check_overlapsQ = true>
        FoldFlagCounts_T FoldRandom( const LInt success_count )
        {
            FoldFlagCounts_T counters;
            
            counters.SetZero();
            
//            const Int n = VertexCount();
            
//            using unif_int = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
//            unif_int u_int ( Int(0), n - Int(1) );
            
            // Witness checking
            witness_collector.clear();
            
            
            while( counters[0] < success_count )
            {
                const Real angle = u_real(random_engine);
                
                auto [i,j] = RandomPivots();
                
                FoldFlag_T flag = Fold<check_overlapsQ>( i, j, angle );
                
                ++counters[flag];
            }
            
            return counters;
        }
    

        std::pair<Real,Real> MinMaxEdgeLengthDeviation( cptr<Real> vertex_coordinates )
        {
            const Int n = VertexCount();

            using V_T = Tiny::Vector<AmbDim,Real,Int>;
            
            Real min;
            Real max;
            
            const Real ell_inv = Inv(prescribed_edge_length);
            
            {
                const V_T u ( &vertex_coordinates[0]              );
                const V_T v ( &vertex_coordinates[AmbDim * (n-1)] );
                
                const Real deviation = Distance(u,v) * ell_inv - Real(1);
                
                min = deviation;
                max = deviation;
            }
            
            for( Int i = 1; i < n; ++i )
            {
                const V_T u ( &vertex_coordinates[AmbDim * (i-1)] );
                const V_T v ( &vertex_coordinates[AmbDim * i    ] );
                
                const Real deviation = Distance(u,v) * ell_inv - Real(1);
                
                min = Min(min,deviation);
                max = Max(max,deviation);
            }
            
            return std::pair(min,max);
        }
        
        Int ActiveNodeCount() const
        {
            Int counter = 0;
            
            for(Int node = 0; node < InteriorNodeCount(); ++node )
            {
                counter += (N_state[node] == NodeFlag_T::NonId);
            }
            
            return counter;
        }
    
        // Witness checking
        cref<WitnessCollector_T> WitnessCollector() const
        {
            return witness_collector;
        }
        
        // Witness checking
        cref<PivotCollector_T> PivotCollector() const
        {
            return pivot_collector;
        }
        
//###################################################################################
//##    Standard interface
//###################################################################################
        
    public:
    
        Size_T AllocatedByteCount() const
        {
            return N_transform.AllocatedByteCount() + N_ball.AllocatedByteCount() + N_state.AllocatedByteCount() + Tree_T::N_ranges.AllocatedByteCount();
        }
        
        template<int t0>
        std::string AllocatedByteCountDetails() const
        {
            constexpr int t1 = t0 + 1;
            return
                std::string("<|")
                + ( "\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(N_transform)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(N_ball)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(N_state)
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Tree_T::N_ranges)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        Size_T ByteCount() const
        {
            return sizeof(ClisbyTree) + AllocatedByteCount();
        }
        
        std::string AllocatedByteCountString() const
        {
            return
                ClassName() + " allocations \n"
                + "\t" + TOOLS_MEM_DUMP_STRING(N_transform)
                + "\t" + TOOLS_MEM_DUMP_STRING(N_ball)
                + "\t" + TOOLS_MEM_DUMP_STRING(N_state)
                + "\t" + TOOLS_MEM_DUMP_STRING(Tree_T::N_ranges.AllocatedByteCount());
        }
        
        static std::string ClassName()
        {
            return ct_string("ClisbyTree")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + "," + ToString(clang_matrixQ)
                + "," + ToString(quaternionsQ)
                + "," + ToString(countersQ)
                + "," + ToString(manual_stackQ)
                + "," + ToString(witnessesQ)
                + ">";
        }
        
    }; // ClisbyTree
    
} // namespace Knoodle
