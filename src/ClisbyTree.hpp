#pragma once

#include "../deps/pcg-cpp/include/pcg_random.hpp"

// TODO: Rotate DOFs on load or write.
// TODO: Recenter polygon on load or write.
// TODO: 2:1 Subdivision.
// TODO: BurnIn routine.
// TODO: Sample routine.

// TODO: _Compute_ NodeRange more efficiently.

// TODO: Shave off one double from ClangQuaternionTransform at the cost of a sqrt?

// TODO: Update(p,q) -- Could this be faster?
// TODO:    1. Start at the pivots, walk up to the root and track all the touched nodes;
// TODO:       We can also learn this way which nodes need an update.
// TODO:    2. Go down again and push the transforms out of the visited nodes.
// TODO:    3. Grab the pivot vertex positions and compute the transform.
// TODO:    4. Walk once again top down through the visited nodes and update their transforms.
// TODO:    5. Walk from the pivots upwards and update the balls.


// Done: Use a structure ClisbyNode and hold it in stack memory.
// ----> Seems to be slower.

// DONE: Check neighborhood around pivots more thoroughly.
// ----> Tried it; did not help.

namespace Knoodle
{
    struct ClisbyTree_TArgs
    {
        bool clang_matrixQ            = true;
        bool quaternionsQ             = true;
        bool countersQ                = false;   // debugging flag
        bool witnessesQ               = false;   // debugging flag
        bool manual_stackQ            = false;   // debugging flag
    };
    
    
    template<
        Size_T AmbDim_,
        typename Real_, typename Int_, typename LInt_,
        ClisbyTree_TArgs targs = ClisbyTree_TArgs()
    >
    class alignas( ObjectAlignment ) ClisbyTree
    : public CompleteBinaryTree<Int_,true,true>
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
        
        static constexpr Int AmbDim = AmbDim_;
        
        static constexpr bool clang_matrixQ = targs.clang_matrixQ && MatrixizableQ<Real>;
        static constexpr bool quaternionsQ  = clang_matrixQ && targs.quaternionsQ;
        static constexpr bool manual_stackQ = targs.manual_stackQ;
        static constexpr bool witnessesQ    = targs.witnessesQ;
        
        using Tree_T = CompleteBinaryTree<Int,true,true>;
        using DFS = Tree_T::DFS;
        
        using Tree_T::max_depth;
    
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
        using PivotCollector_T         = std::vector<std::tuple<Int,Int,Real,bool>>;
        
        
        using NodeFlagContainer_T      = Tensor1<NodeFlag_T,Int>;
        using NodeTransformContainer_T = Tensor2<Real,Int>;
        using NodeBallContainer_T      = Tensor2<Real,Int>;
    
        using NodeSplitFlagVector_T    = Tiny::Vector<2,bool,Int>;
        using NodeSplitFlagMatrix_T    = Tiny::Matrix<2,2,bool,Int>;
    
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
        :   Tree_T                      { int_cast<Int>(vertex_count_)         }
        ,   N_transform                 { InternalNodeCount(), TransformDim    }
        ,   N_state                     { InternalNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount(), BallDim                 }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_) }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam  }
        ,   level_moves_per_node        { this->ActualDepth() + Int(1)         }
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
        :   Tree_T                      { int_cast<Int>(vertex_count_)         }
        ,   N_transform                 { InternalNodeCount(), TransformDim    }
        ,   N_state                     { InternalNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount(), BallDim                 }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_) }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam  }
        ,   level_moves_per_node        { this->ActualDepth() + Int(1)         }
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
        :   Tree_T                      { int_cast<Int>(vertex_count_)         }
        ,   N_transform                 { InternalNodeCount(), TransformDim    }
        ,   N_state                     { InternalNodeCount(), NodeFlag_T::Id  }
        ,   N_ball                      { NodeCount(), BallDim                 }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_) }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam  }
        ,   random_engine               { prng                                 }
        ,   level_moves_per_node        { this->ActualDepth() + Int(1)         }
        {
            InitializeTransforms();
            ReadVertexCoordinates( vertex_coords_ );
        }

        ~ClisbyTree() = default;
    
    public:
        
        using Tree_T::MaxDepth;
        using Tree_T::NodeCount;
        using Tree_T::InternalNodeCount;
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
        using Tree_T::InternalNodeQ;
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
//        bool transforms_pushedQ = false;
        
        Tensor1<LInt,Int>  level_moves_per_node;
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
            
            for( Int node = 0; node < InternalNodeCount(); ++node )
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
            TOOLS_PTIC(ClassName()+"::ReadVertexCoordinates");
            
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
            
            TOOLS_PTOC(ClassName()+"::ReadVertexCoordinates");
        }

        
        void SetToCircle()
        {
            const Int n = VertexCount();
            
            Tensor2<Real,Int> X( n, AmbDim );
            
            mptr<Real> x = X.data();
            
            const double delta  = Frac<double>( Scalar::TwoPi<double>, n );
            const double radius = Frac<double>( 1, 2 * std::sin( Frac<double>(delta,2) ) );
            
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

#include "ClisbyTree/Access.hpp"
#include "ClisbyTree/Transformations.hpp"
#include "ClisbyTree/Update.hpp"
#include "ClisbyTree/CollisionChecks.hpp"
#include "ClisbyTree/Fold.hpp"
#include "ClisbyTree/FoldRandomHierarchical.hpp"
//#include "ClisbyTree/Subdvide.hpp"
    
//#include "ClisbyTree/ClisbyNode.hpp"
//#include "ClisbyTree/ModifiedClisbyTree.hpp"

    
    public:
    
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
            
            for(Int node = 0; node < InternalNodeCount(); ++node )
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
                + "<" + Tools::ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + "," + Tools::ToString(clang_matrixQ)
                + "," + Tools::ToString(quaternionsQ)
                + "," + Tools::ToString(countersQ)
                + "," + Tools::ToString(manual_stackQ)
                + "," + Tools::ToString(witnessesQ)
                + ">";
        }
        
    }; // ClisbyTree
    
} // namespace Knoodle
