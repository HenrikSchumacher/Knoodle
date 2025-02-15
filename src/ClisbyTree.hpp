#pragma once

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
    
        using SInt = typename Tree_T::SInt;
        
//        using Tree_T = CompleteBinaryTree_Precomp<Int>;
        using Tree_T::max_depth;
        
        static constexpr Int AmbDim  = AmbDim_;
    
        static constexpr bool use_clang_matrixQ = true && MatrixizableQ<Real>;
        static constexpr bool use_manual_stackQ = use_manual_stackQ_;
    
        using Transform_T     = typename std::conditional_t<
                                    use_clang_matrixQ,
                                    ClangAffineTransform<AmbDim,Real,Int>,
                                    AffineTransform<AmbDim,Real,Int>
                                >;
    
        using Vector_T        = typename Transform_T::Vector_T;
        using Matrix_T        = typename Transform_T::Matrix_T;
        
        using NodeContainer_T = Tensor2<Real,Int>;
        
        using PRNG_T          = std::mt19937_64;
        using PRNG_Result_T   = PRNG_T::result_type;
    
        using Flag_T          = Int32;
        using FlagVec_T       = Tiny::Vector<5,Flag_T,Int>;
    
        using NodeSplitFlagVector_T = Tiny::Vector<2,bool,Int>;
        using NodeSplitFlagMatrix_T = Tiny::Matrix<2,2,bool,Int>;
    
//        using NodeSplitFlagVector_T = std::array<bool,2>;
//        using NodeSplitFlagMatrix_T = std::array<NodeSplitFlagVector_T,2>;
        
        
        // For center, radius, rotation, and translation.
        static constexpr Int TransformDim = Transform_T::Size();
        static constexpr Int NodeDim      = AmbDim + 1 + TransformDim;
        
//        static constexpr bool perf_countersQ = true;
        static constexpr bool perf_countersQ = false;
    
        
        enum class UpdateFlag_T : UInt8
        {
            DoNothing = 0,
            Update    = 1,
            Split     = 2
        };
        
        enum class NodeState_T : UInt8
        {
            Id    = 0,
            NonId = 1
        };
        
        ClisbyTree() = default;

        template<typename ExtReal, typename ExtInt>
        ClisbyTree(
            const ExtInt vertex_count_,
            const ExtReal radius
        )
        :   Tree_T      { static_cast<Int>(vertex_count_)       }
        ,   N_data      { NodeCount(), NodeDim, 0               }
//        ,   N_data      { InteriorNodeCount(), NodeDim, 0       }
        ,   N_state     { NodeCount(), NodeState_T::Id          }
//        ,   N_state     { InteriorNodeCount(), NodeState_T::Id  }
        ,   E_lengths   { LeafNodeCount()                       }
        ,   r           { static_cast<Real>(radius)             }
        ,   r2          { r * r                                 }
        {
            id.SetIdentity();
            SetToCircle();
            InitializePRNG();
        }
        
        template<typename ExtReal, typename ExtInt>
        ClisbyTree(
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal radius
        )
        :   Tree_T      { static_cast<Int>(vertex_count_)    }
        ,   N_data      { NodeCount(), NodeDim, 0            }
//        ,   N_data      { InteriorNodeCount(), NodeDim, 0       }
        ,   N_state     { NodeCount(), NodeState_T::Id       }
//        ,   N_state     { InteriorNodeCount(), NodeState_T::Id  }
        ,   E_lengths   { LeafNodeCount()                    }
        ,   r           { static_cast<Real>(radius)          }
        ,   r2          { r * r                              }
        {
            id.SetIdentity();
            ReadVertexCoordinates( vertex_coords_ );
            InitializePRNG();
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
        
        NodeContainer_T N_data;
        Tensor1<NodeState_T,Int> N_state;
        Tensor1<Real,Int> E_lengths;
        
        Real r  = 0;                    // Hard sphere radius.
        Real r2 = 0;                    // Squared hard sphere radius.
        
        Int p = 0;                      // Lower pivot index.
        Int q = 0;                      // Greater pivot index.
        
        Int witness_0 = -1;
        Int witness_1 = -1;
    
        Real theta;                     // Rotation angle
        Vector_T   X_p;                 // Pivot location.
        Vector_T   X_q;                 // Pivot location.
        Transform_T id;
        Transform_T transform;          // Pivot transformation.
        
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
//            // Debugging
//            ResetTransform( NodeTransformPtr(node) );
            
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
            using RNG_T = std::random_device;
            
            constexpr Size_T state_size = PRNG_T::state_size;
            constexpr Size_T seed_size = state_size * sizeof(PRNG_T::result_type) / sizeof(RNG_T::result_type);
            
            std::array<RNG_T::result_type,seed_size> seed_array;
            
            std::generate( seed_array.begin(), seed_array.end(), RNG_T() );
            
            std::seed_seq seed ( seed_array.begin(), seed_array.end() );
            
            random_engine = PRNG_T( seed );   
        }
        
    public:
        
        void ReadVertexCoordinates( cptr<Real> x )
        {
            ptic(ClassName() + "::ReadVertexCoordinates");
                
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
            
            ptoc(ClassName() + "::ReadVertexCoordinates");
        }

        
        void SetToCircle()
        {
            const Int n = VertexCount();
            
            Tensor2<Real,Int> X( n, AmbDim );
            
            mptr<Real> x = X.data();
            
            const double delta  = Frac<double>( Scalar::TwoPi<double>, n );
            const double radius = Frac<double>( 1, 2 * std::sin( Frac<double>(delta,2) ) );
            
            Real v [AmbDim] = { Real(0) };
            
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
        
        FlagVec_T FoldRandom( const LInt success_count )
        {
            FlagVec_T counters;
            
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
                
                int flag = Fold( i, j, angle );
                
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
            
            const Real r_inv = Inv(r);
            
            {
                const V_T u ( &vertex_coordinates[0]              );
                const V_T v ( &vertex_coordinates[AmbDim * (n-1)] );
                
                const Real deviation = Distance(u,v) * r_inv - Real(1);
                
                min = deviation;
                max = deviation;
            }
            
            for( Int i = 1; i < n; ++i )
            {
                const V_T u ( &vertex_coordinates[AmbDim * (i-1)] );
                const V_T v ( &vertex_coordinates[AmbDim * i    ] );
                
                const Real deviation = Distance(u,v) * r_inv - Real(1);
                
                min = Min(min,deviation);
                max = Max(max,deviation);
            }
            
            return std::pair(min,max);
        }
        
//#########################################################################################
//##    Standard interface
//#########################################################################################
        
    public:
        
        static std::string ClassName()
        {
            return std::string("ClisbyTree")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + ToString(use_manual_stackQ)
                + ">";
        }

    }; // ClisbyTree
    
} // namespace KnotTools
