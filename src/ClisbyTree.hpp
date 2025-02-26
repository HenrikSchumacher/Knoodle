#pragma once

#include "../deps/pcg-cpp/include/pcg_random.hpp"

//TODO: shorten N_state

namespace KnotTools
{
    template<
        int AmbDim_,
        typename Real_, typename Int_, typename LInt_,
//        bool use_manual_stackQ_ = true
        bool use_manual_stackQ_ = false
    >
    class alignas( ObjectAlignment ) ClisbyTree : public CompleteBinaryTree<Int_,true>
//    class alignas( ObjectAlignment ) ClisbyTree : public CompleteBinaryTree_Precomp<Int_>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(IntQ<LInt_>,"");
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
    
        static constexpr bool use_clang_matrixQ = true && MatrixizableQ<Real>;
        static constexpr bool use_quaternionsQ  = true;
        static constexpr bool use_manual_stackQ = use_manual_stackQ_;
    
        using Transform_T
            = typename std::conditional_t<
                  use_clang_matrixQ,
                  typename std::conditional_t<
                      use_quaternionsQ,
                      ClangQuaternionTransform<Real,Int>,
                      ClangAffineTransform<AmbDim,Real,Int>
                  >,
                  AffineTransform<AmbDim,Real,Int>
              >;
    
        using Vector_T        = typename Transform_T::Vector_T;
        using Matrix_T        = typename Transform_T::Matrix_T;
        using Flag_T          = Int32;
        using FlagCountVec_T  = Tiny::Vector<5,LInt,Flag_T>;

        using NodeContainer_T = Tensor2<Real,Int>;
    
        using NodeSplitFlagVector_T = Tiny::Vector<2,bool,Int>;
        using NodeSplitFlagMatrix_T = Tiny::Matrix<2,2,bool,Int>;

    //        using NodeSplitFlagVector_T = std::array<bool,2>;
    //        using NodeSplitFlagMatrix_T = std::array<NodeSplitFlagVector_T,2>;
    
        using RNG_T  = std::random_device;
    
//        using PRNG_T = std::mt19937_64;
//        static constexpr Size_T PRNG_T_state_size = PRNG_T::state_size;
    
        using PRNG_T = pcg64;
        static constexpr Size_T PRNG_T_state_size = 4;
    
        static constexpr Size_T seed_size  = (PRNG_T_state_size * sizeof(PRNG_T::result_type)) / sizeof(RNG_T::result_type);
    
        using Seed_T = std::array<RNG_T::result_type,seed_size>;
        
//        using PRNG_FullState_T = std::array<std::string,3>;
        
        struct PRNG_FullState_T
        {
            std::string multiplier;
            std::string increment;
            std::string state;
        };
        
        // For center, radius, rotation, and translation.
        static constexpr Int TransformDim = Transform_T::Size();
        static constexpr Int BallDim      = AmbDim + 1;
//        static constexpr Int NodeDim      = BallDim + TransformDim;
        
//        static constexpr bool countersQ = true;
        static constexpr bool countersQ = false;
    
        
        enum class UpdateFlag_T : std::int_fast8_t
        {
            DoNothing = 0,
            Update    = 1,
            Split     = 2
        };
        
        enum class NodeState_T : std::int_fast8_t
        {
            Id    = 0,
            NonId = 1
        };
        
        ClisbyTree() = default;

        template<typename ExtReal, typename ExtInt>
        ClisbyTree(
            const ExtInt vertex_count_,
            const ExtReal hard_sphere_diam_
        )
        :   Tree_T                      { static_cast<Int>(vertex_count_)           }
        ,   N_transform                 { InteriorNodeCount(), TransformDim, 0      }
        ,   N_state                     { NodeCount(), NodeState_T::Id              }
        ,   N_ball                      { NodeCount(), BallDim, 0                   }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_)      }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam       }
        {
            id.SetIdentity();
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
        ,   N_transform                 { InteriorNodeCount(), TransformDim, 0  }
        ,   N_state                     { NodeCount(), NodeState_T::Id          }
        ,   N_ball                      { NodeCount(), BallDim, 0               }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_)      }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam       }
        {
            id.SetIdentity();
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
        ,   N_transform                 { InteriorNodeCount(), TransformDim, 0  }
        ,   N_state                     { NodeCount(), NodeState_T::Id          }
        ,   N_ball                      { NodeCount(), BallDim, 0               }
        ,   hard_sphere_diam            { static_cast<Real>(hard_sphere_diam_)      }
        ,   hard_sphere_squared_diam    { hard_sphere_diam * hard_sphere_diam       }
        ,   random_engine               { prng                                  }
        {
            id.SetIdentity();
            ReadVertexCoordinates( vertex_coords_ );
        }
        

        ~ClisbyTree() = default;
    
        // TODO: Copy + move semantics.
    
        
        
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
        
        NodeContainer_T N_transform;
        Tensor1<NodeState_T,Int> N_state;
        NodeContainer_T N_ball;
        
        Real hard_sphere_diam           = 0;
        Real hard_sphere_squared_diam   = 0;
        Real prescribed_edge_length     = 1;
        
        Int p = 0;                      // Lower pivot index.
        Int q = 0;                      // Greater pivot index.
        
        Int witness_0 = -1;
        Int witness_1 = -1;
    
        Real theta;                     // Rotation angle
        Vector_T   X_p;                 // Pivot location.
        Vector_T   X_q;                 // Pivot location.
        Transform_T id;
        Transform_T transform;          // Pivot transformation.
        
        Seed_T seed;
        PRNG_T random_engine;
    
        mutable LInt mm_counter   = 0;
        mutable LInt mv_counter   = 0;
        mutable LInt load_counter = 0;
    
        bool mid_changedQ = false;
        bool transforms_pushedQ = false;
        
    private:

        void ResetTransform( mptr<Real> f_ptr )
        {
            id.Write(f_ptr);
        }
        
        void ResetTransform( const Int node )
        {
            N_state[node] = NodeState_T::Id;
        }
        
        void InitializeNodeFromVertex( const Int node, cptr<Real> x )
        {
            copy_buffer<AmbDim>( x, NodeCenterPtr(node) );
            NodeRadius(node) = 0;
            ResetTransform(node);
        }
        
        void InitializePRNG()
        {
            std::generate( seed.begin(), seed.end(), RNG_T() );
            SeedBy( seed );
        }
    
    public:
        
        PRNG_T GetRandomEngine()
        {
            return random_engine;
        }
        
        void SetRandomEngine( cref<PRNG_T> prng )
        {
            random_engine = prng;
        }
        
        
        int SetRandomEngine( cref<PRNG_FullState_T> full_state )
        {
            std::string state = full_state.multiplier + " " + full_state.increment + " " + full_state.state;
            
            std::stringstream s (state);
            
            s >> random_engine;
            
            std::string actual_state = RandomEngineFullString();
            
            if( s.fail() )
            {
                return 1;
            }
            
//            if( actual_state != state )
//            {
//                eprint( ClassName() + "::SetRandomState: Failed.");
//                logprint(state);
//                logprint(actual_state);
//            }
            
            return 0;
        }
        
        std::string RandomEngineFullString() const
        {
            std::stringstream s;
            
            s << random_engine;
            
            return s.str();
        }
        
        
        PRNG_FullState_T RandomEngineFullState() const
        {
            std::stringstream s;
            
            s << random_engine;
            
            PRNG_FullState_T full_state;
            
            s >> full_state.multiplier;
            s >> full_state.increment;
            s >> full_state.state;
            
            return full_state;
        }

        std::string RandomEngineMultiplier() const
        {
            return RandomEngineFullState()[0];
        }
        
        std::string RandomEngineIncrement() const
        {
            return RandomEngineFullState()[1];
        }
        
        std::string RandomEngineState() const
        {
            return RandomEngineFullState()[2];
        }
    
//    multiplier_string + " " + increment_string + " " + state_string;
    
    
        void SeedBy( Seed_T & seed_ )
        {
            seed = seed_;
            
            std::seed_seq seed_sequence ( seed.begin(), seed.end() );
            
            random_engine = PRNG_T( seed_sequence );
            
//            random_engine = PRNG_T( pcg_extras::seed_seq_from<std::random_device>() );
        }
    
//        cref<Seed_T> Seed() const
//        {
//            return seed;
//        }
//    
//        std::string SeedToString()
//        {
//            std::string seed_string;
//            
//            for( auto s : seed )
//            {
//                std::string str = std::format("{0:08X}",s);
////                TOOLS_DUMP(str);    
//                
//                seed_string += str;
//            }
//            
//            return seed_string;
//        }
//    
//        static bool SeedStringOkayQ( cref<std::string> seed_string )
//        {
//            if( seed_string.size()  != 2 * sizeof(Seed_T) )
//            {
//                return false;
//            }
//            
//            bool b = true;
//            
//            for( char c : seed_string )
//            {
//                b = b && (
//                      ( ('0' <= c) && (c <= '9') ) || ( ('A' <= c) && (c <= 'F') )
//                );
//            }
//            
//            return b;
//        }
//    
//        static Seed_T StringToSeed( cref<std::string> seed_string )
//        {
//            Seed_T seed = {};
//            
//            if( !SeedStringOkayQ(seed_string) )
//            {
//                eprint(ClassName()+"::StringToSeed: Seed string " + seed_string + " is invalid.");
//                return seed;
//            }
//            
//            Size_T chuck_size = 2 * sizeof(RNG_T::result_type);
//
//            for( Size_T i = 0; i < seed.size(); ++i )
//            {
//                std::string chunk (
//                    &seed_string[chuck_size * i],
//                    &seed_string[chuck_size * (i+1)]
//                );
//
//                seed[i] = static_cast<RNG_T::result_type>(
//                    std::stoul(chunk, nullptr, 16)
//                );
//            }
//            
//            return seed;
//        }
        
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

        
#include "ClisbyTree/Access.hpp"
#include "ClisbyTree/Transformations.hpp"
#include "ClisbyTree/Update.hpp"
#include "ClisbyTree/CollisionsChecks.hpp"
        
//#########################################################################################
//##    Folding
//#########################################################################################

    public:
    
        Flag_T Fold( const Int p_, const Int q_, const Real theta_ )
        {
            int pivot_flag = LoadPivots(p_,q_,theta_);
            
            if( pivot_flag != 0 )
            {
                // Folding step aborted because pivots indices are too close.
                return pivot_flag;
            }
            
            int joint_flag = CheckJoints();
            
            if( joint_flag != 0 )
            {
                // Folding step failed because neighbors of pivot touch.
                return joint_flag;
            }
            
            Update();

            if( OverlapQ() )
            {
                // Folding step failed; undo the modifications.
                Update(p_,q_,-theta_);
                return 4;
            }
            else
            {
                // Folding step succeeded.
                return 0;
            }
        }
        
        FlagCountVec_T FoldRandom( const LInt success_count )
        {
            FlagCountVec_T counters;
            
            counters.SetZero();
            
            using unif_int  = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            const Int n = VertexCount();
            
            unif_int  u_int ( Int(0), n-3 );
            unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
            
            while( counters[0] < success_count )
            {
                const Int  i     = u_int                        (random_engine);
                const Int  j     = unif_int(i+2,n-1-(i==Int(0)))(random_engine);
                const Real angle = u_real                       (random_engine);
                
                Flag_T flag = Fold( i, j, angle );
                
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
    
        
//#########################################################################################
//##    Standard interface
//#########################################################################################
        
    public:
    
        Size_T AllocatedByteCount() const
        {
            return N_transform.AllocatedByteCount() + N_ball.AllocatedByteCount() + N_state.AllocatedByteCount() + Tree_T::N_ranges.AllocatedByteCount();
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
            return std::string("ClisbyTree")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + "," + ToString(use_manual_stackQ)
                + ">";
        }

    }; // ClisbyTree
    
} // namespace KnotTools
