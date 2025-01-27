#pragma once

namespace KnotTools
{
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) ClisbyTree : public CompleteBinaryTree<Int_,true>
//    class alignas( ObjectAlignment ) ClisbyTree : public CompleteBinaryTree_Precomp<Int_>
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        using Tree_T = CompleteBinaryTree<Int,true>;
//        using Tree_T = CompleteBinaryTree_Precomp<Int>;
        using Tree_T::max_depth;
        
        static constexpr Int AmbDim  = AmbDim_;
        
        using Vector_T        = Tiny::Vector<AmbDim_,Real,Int>;
        using Matrix_T        = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        using Transform_T     = AffineTransform<AmbDim,Real,Int>;
        
        using NodeContainer_T = Tensor2<Real,Int>;
        
        using PRNG_T          = std::mt19937_64;
        using PRNG_Result_T   = PRNG_T::result_type;
        
        using FlagVec_T       = std::array<Size_T,5>;
        
        
        // For center, radius, rotation, and translation.
        static constexpr Int TransformDim = Transform_T::Size();
        static constexpr Int NodeDim      = AmbDim + 1 + TransformDim;
        
        
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
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal radius
        )
        :   Tree_T      { static_cast<Int>(vertex_count_)    }
        ,   N_data      { NodeCount(), NodeDim, 0            }
        ,   N_state     { NodeCount(), NodeState_T::Id       }
        ,   r           { static_cast<Real>(radius)          }
        ,   r2          { r * r                              }
        {
            id.SetIdentity();
            
            ReadVertexCoordinates( vertex_coords_ );
            
            InitializePRNG();
        }
        
        ~ClisbyTree() = default;
        
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
        
        Real r  = 0;                    // Hard sphere radius.
        Real r2 = 0;                    // Squared hard sphere radius.
        
        Int p = 0;                      // Lower pivot index.
        Int q = 0;                      // Greater pivot index.
        
        Int witness_0 = -1;
        Int witness_1 = -1;
    
        bool mid_changedQ = false;
        Real theta;                     // Rotation angle
        Vector_T    X_p;                // Pivot location.
        Vector_T    X_q;                // Pivot location.
        Transform_T id;
        Transform_T transform;          // Pivot transformation.
        
        PRNG_T random_engine;
        
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
        
        void InitializeNodeFromVertex( const Int node, cptr<Real> v )
        {
            copy_buffer<AmbDim>( v, NodeCenterPtr(node) );
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
        
        
#include "ClisbyTree/Access.hpp"
#include "ClisbyTree/Transformations.hpp"
#include "ClisbyTree/Update.hpp"
#include "ClisbyTree/CollisionsChecks.hpp"
        
//#########################################################################################
//##    Folding
//#########################################################################################

        int Fold( const Int p_, const Int q_, const Real theta_ )
        {
            int flag = Update(p_,q_,theta_);
            
            if( flag != 0 )
            {
                // Folding step failed because of silly reason.
                return 1;
            }
            else
            {
                if( OverlapQ() )
                {
                    // Folding step failed; undo the modifications.
                    Update(p_,q_,-theta_);
                    return 2;
                }
                else
                {
                    // Folding step succeeded.
                    return 0;
                }
            }
        }
        
        FlagVec_T FoldRandom( const Int step_count )
        {
            FlagVec_T counters {0};
            
            using unif_int  = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            const Int n = VertexCount();
            
            unif_int  u_int ( Int(0), n-3 );
            unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
            
            for( Int step = 0; step < step_count; ++step )
            {
                const Int  i     = u_int                        (random_engine);
                const Int  j     = unif_int(i+2,n-1-(i==Int(0)))(random_engine);
                const Real angle = u_real                       (random_engine);
                
                int flag = Fold( i, j, angle );
                
                ++counters[flag];
            }
            
            return counters;
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
                + ">";
        }

    }; // ClisbyTree
    
} // namespace KnotTools
