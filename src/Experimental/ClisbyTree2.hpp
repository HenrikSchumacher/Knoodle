#pragma once

#include "../AffineTransforms/AffineTransformFlag.hpp"
#include "../AffineTransforms/AffineTransform.hpp"
#include "../AffineTransforms/ClangMatrix.hpp"
#include "../AffineTransforms/ClangAffineTransform.hpp"
#include "../AffineTransforms/ClangQuaternionTransform.hpp"

// This is a fork of ClisbyTree2 to experiment with thickness conditions.
// This is mostly for finding out whether this is suitable for Rhoslyn


// TODO: Use edge_length > 1.
// TODO: For allowing thickness checks:
// TODO:    - skip collision checks between nodes with
// TODO:      edge_length * node_distance < hard_sphere_radius
// TODO:    - rework the checking of joints
// DONE: Fold for a specific number of attempts vs. fold for a specific number of successes.


// Far future goals (maybe)
// TODO: Make this available also for open curves.
// TODO: Use capsule shapes, too.

namespace Knoodle
{
    struct ClisbyTree2_TArgs
    {
        bool clang_matrixQ            = true;
        bool quaternionsQ             = true;
        bool countersQ                = false;   // debugging flag
        bool witnessesQ               = false;   // debugging flag
    };
    
    
    template<
        Size_T AmbDim_,
        typename Real_, typename Int_, typename LInt_,
        ClisbyTree2_TArgs targs = ClisbyTree2_TArgs()
    >
    class alignas(ObjectAlignment) ClisbyTree2 final : public CompleteBinaryTree<Int_,true,true>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(IntQ<LInt_>,"");
        static_assert( AmbDim_ == 3, "Currently only implemented in dimension 3." );
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using LInt   = LInt_;
        
        static constexpr Int AmbDim = AmbDim_;
        
        static constexpr bool clang_matrixQ = targs.clang_matrixQ && MatrixizableQ<Real>;
        static constexpr bool quaternionsQ  = clang_matrixQ && targs.quaternionsQ;
        static constexpr bool witnessesQ    = targs.witnessesQ;
        
        using Base_T = CompleteBinaryTree<Int,true,true>;
        using DFS = Base_T::DFS;
        
        using Base_T::max_depth;
    
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
//        using FoldFlag_T               = Int32;
        
        
        enum struct FoldFlag_T : Int32
        {
            Accepted         = 0,
            RejectedByPivots = 1,
            RejectedByJoint0 = 2,
            RejectedByJoint1 = 3,
            RejectedByTree   = 4
        };
        
        using FoldFlagCounts_T         = Tiny::Vector<5,LInt,std::underlying_type_t<FoldFlag_T>>;
        
        using WitnessVector_T          = Tiny::Vector<2,Int,Int>;
        using WitnessCollector_T       = std::vector<Tiny::Vector<4,Int,Int>>;
        using PivotCollector_T         = std::vector<std::tuple<Int,Int,Real,bool>>;
        
        
        using NodeFlagContainer_T      = Tensor1<NodeFlag_T,Int>;
    
        // For rotation and translation.
        static constexpr Int TransfDim = Transform_T::Size();
        using NodeTransformContainer_T = Tiny::VectorList_AoS<TransfDim,Real,Int>;
    
        // For center and radius.
        static constexpr Int BallDim   = AmbDim + 1;
        using NodeBallContainer_T      = Tiny::VectorList_AoS<BallDim,Real,Int>;
    
        using NodeSplitFlagVector_T    = Tiny::Vector<2,bool,Int>;
        using NodeSplitFlagMatrix_T    = Tiny::Matrix<2,2,bool,Int>;
    
        using RNG_T  = std::random_device;
    
        using PRNG_T = pcg64;
        static constexpr Size_T PRNG_T_state_size = 4;
    
        static constexpr Size_T seed_size  = (PRNG_T_state_size * sizeof(PRNG_T::result_type)) / sizeof(RNG_T::result_type);
    
        using Seed_T = std::array<RNG_T::result_type,seed_size>;
        
        enum class AngleRandomMethod_T
        {
            Uniform,
            WrappedGaussian
        };
        
        enum class PivotRandomMethod_T
        {
            Uniform,
//            Triangular,
            DiscreteWrappedGaussian,
            DiscreteWrappedLaplace,
            Clisby
        };

        
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
        
        template<typename ExtReal, typename ExtInt>
        ClisbyTree2(
            const ExtInt vertex_count_,
            const ExtReal edge_length_,
            const ExtReal hard_sphere_diam_
        )
        :   Base_T                      { int_cast<Int>(vertex_count_)         }
        ,   N_transform                 { InternalNodeCount()                  }
        ,   N_state                     { InternalNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount()                          }
        ,   edge_length                 { static_cast<Real>(edge_length_)      }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_) }
        {
            InitializeConstants();
            InitializeTransforms();
            SetToCircle();
            InitializePRNG();
        }
        
        template<typename ExtReal, typename ExtInt>
        ClisbyTree2(
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal edge_length_,
            const ExtReal hard_sphere_diam_
        )
        :   Base_T                      { int_cast<Int>(vertex_count_)         }
        ,   N_transform                 { InternalNodeCount()                  }
        ,   N_state                     { InternalNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount()                          }
        ,   edge_length                 { static_cast<Real>(edge_length_)      }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_) }
        {
            InitializeConstants();
            InitializeTransforms();
            ReadVertexCoordinates( vertex_coords_ );
            InitializePRNG();
        }
    
        template<typename ExtReal, typename ExtInt>
        ClisbyTree2(
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal edge_length_,
            const ExtReal hard_sphere_diam_,
            PRNG_T prng
        )
        :   Base_T                      { int_cast<Int>(vertex_count_)         }
        ,   N_transform                 { InternalNodeCount()                  }
        ,   N_state                     { InternalNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount()                          }
        ,   edge_length                 { static_cast<Real>(edge_length_)      }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_) }
        ,   random_engine               { prng                                 }
        {
            InitializeConstants();
            InitializeTransforms();
            ReadVertexCoordinates( vertex_coords_ );
        }

        // Default constructor
        ClisbyTree2() = default;
        // Destructor
        virtual ~ClisbyTree2() override = default;
        // Copy constructor
        ClisbyTree2( const ClisbyTree2 & other ) = default;
        // Copy assignment operator
        ClisbyTree2 & operator=( const ClisbyTree2 & other ) = default;
        // Move constructor
        ClisbyTree2( ClisbyTree2 && other ) = default;
        // Move assignment operator
        ClisbyTree2 & operator=( ClisbyTree2 && other ) = default;
    
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
        using Base_T::NodeRange;
        using Base_T::LeafNodeQ;
        using Base_T::InternalNodeQ;
        using Base_T::PrimitiveNode;
        using Base_T::Root;
        
    private:
        
        NodeTransformContainer_T N_transform;
        NodeFlagContainer_T      N_state;
        NodeBallContainer_T      N_ball;
        
        Real edge_length                = 1;
        Real hard_sphere_diam           = 1;
        Real hard_sphere_squared_diam   = 1;
        
        Int gap                         = 0;
        Int p = 0;                      // Lower pivot index.
        Int q = 0;                      // Greater pivot index.
        Int p_shifted = 0;              // Lower pivot index.
        Int q_shifted = 0;              // Greater pivot index.
        WitnessVector_T witness {{-1,-1}};
    
        Real theta;                     // Rotation angle
        Vector_T   X_p;                 // Pivot location.
        Vector_T   X_q;                 // Pivot location.
        Transform_T transform;          // Pivot transformation.
        
        Seed_T seed;
        PRNG_T random_engine;
        
        mutable CallCounters_T call_counters;
    
        bool mid_changedQ       = false;
        bool reflectQ           = false; // Whether we multiply the pivot move with -1.
        
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
        
        void InitializeConstants()
        {
            hard_sphere_squared_diam = hard_sphere_diam * hard_sphere_diam;
            gap = Int(std::ceil(Frac<Real>(hard_sphere_diam,edge_length))) - Int(1);
        }
        
        void InitializeTransforms()
        {
            Transform_T id;
            id.SetIdentity();
            
            const Int n_count = InternalNodeCount();
            
            for( Int node = 0; node < n_count; ++node )
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
            TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates"));
            
            this->PostOrderScan(
                [this]( const Int node )                // internal node postvisit
                {
                    ComputeBall(node);
                    ResetTransform(node);
                },
                [this,x]( const Int node )              // leaf node visit
                {
                    const Int vertex = NodeBegin(node);

                    InitializeNodeFromVertex( node, &x[AmbDim * vertex] );
                }
            );
        }

        
        void SetToCircle()
        {
            const Int n = VertexCount();
            
            Tensor2<Real,Int> X( n, AmbDim );
            
            mptr<Real> x = X.data();
            
            const double delta  = Frac<double>( Scalar::TwoPi<double>, n );
            const double radius = Frac<double>( edge_length, 2 * std::sin( Frac<double>(delta,2) ) );
            
            Tiny::Vector<AmbDim,double,Int> v (double(0));
            
            for( Int vertex = 0; vertex < n; ++vertex )
            {
                const double angle = delta * vertex;
                
                v[0] = radius * std::cos(angle);
                v[1] = radius * std::sin(angle);
                
                v.Write(x,vertex);
            }
            
            ReadVertexCoordinates(x);
        }

#include "ClisbyTree2/Access.hpp"
#include "ClisbyTree2/Transformations.hpp"
#include "ClisbyTree2/Update.hpp"
#include "ClisbyTree2/CollisionChecks.hpp"
#include "ClisbyTree2/CollisionChecks_Debug.hpp"
#include "ClisbyTree2/Random.hpp"
#include "ClisbyTree2/Fold.hpp"
    
    public:
    
        std::pair<Real,Real> MinMaxEdgeLengthDeviation( cptr<Real> vertex_coordinates )
        {
            const Int n = VertexCount();

            using V_T = Tiny::Vector<AmbDim,Real,Int>;
            
            Real min;
            Real max;
            
            const Real ell_inv = Inv(edge_length);
            
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
            
            const Int n_count = InternalNodeCount();
            
            for( Int node = 0; node < n_count; ++node )
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
        
//###########################################################
//##    Standard interface
//###########################################################
        
    public:
    
        Size_T AllocatedByteCount() const
        {
            return N_transform.AllocatedByteCount() + N_ball.AllocatedByteCount() + N_state.AllocatedByteCount() + Base_T::N_ranges.AllocatedByteCount();
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
                + (",\n" + ct_tabs<t1>) + TOOLS_MEM_DUMP_STRING(Base_T::N_ranges)
                + ( "\n" + ct_tabs<t0> + "|>");
        }
        
        Size_T ByteCount() const
        {
            return sizeof(ClisbyTree2) + AllocatedByteCount();
        }
        
        std::string AllocatedByteCountString() const
        {
            return
                ClassName() + " allocations \n"
                + "\t" + TOOLS_MEM_DUMP_STRING(N_transform)
                + "\t" + TOOLS_MEM_DUMP_STRING(N_ball)
                + "\t" + TOOLS_MEM_DUMP_STRING(N_state)
                + "\t" + TOOLS_MEM_DUMP_STRING(Base_T::N_ranges.AllocatedByteCount());
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
    
        static std::string ClassName()
        {
            return ct_string("ClisbyTree2")
                + "<" + Tools::ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + "," + Tools::ToString(clang_matrixQ)
                + "," + Tools::ToString(quaternionsQ)
                + "," + Tools::ToString(countersQ)
                + "," + Tools::ToString(witnessesQ)
                + ">";
        }
        
    }; // ClisbyTree2
    
} // namespace Knoodle
