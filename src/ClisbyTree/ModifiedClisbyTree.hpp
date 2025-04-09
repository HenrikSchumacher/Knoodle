public:

class ModifiedClisbyTree
{
    friend class ClisbyTree;
    
    cref<ClisbyTree> T;
    
    Stack<ClisbyNode,Int> mod_nodes;
    Tensor1<Int,Int> node_lookup;
    
    Real hard_sphere_diam;
    Real hard_sphere_squared_diam;
    
    Seed_T seed;
    PRNG_T random_engine;
    
    Int  p;
    Int  q;
    Real theta;
    Vector_T X_p;
    Vector_T X_q;
    Transform_T transform;
    
    WitnessVector_T witness {{-1,-1}};

    bool reflectQ;
    bool mid_changedQ;
    
    // TODO: Needs also random engine.
    
public:
    
    ModifiedClisbyTree( cref<ClisbyTree> T_ )
    :   T                           ( T_                         )
    ,   mod_nodes                   ( T.NodeCount()              )
    ,   node_lookup                 ( T.NodeCount(), Int(-1)     )
    ,   hard_sphere_diam            ( T.HardSphereDiameter()     )
    ,   hard_sphere_squared_diam    ( T.hard_sphere_squared_diam )
    {
        std::generate( seed.begin(), seed.end(), RNG_T() );
        
        std::seed_seq seed_sequence ( seed.begin(), seed.end() );
        
        random_engine = PRNG_T( seed_sequence );
    }
    
    Int ModifiedNodeCount() const
    {
        return mod_nodes.Size();
    }
    
    cref<Stack<ClisbyNode,Int>> ModifiedNodes() const
    {
        return mod_nodes;
    }
    
    mref<ClisbyNode> GetNode( const Int id )
    {
//        print("GetNode");
//        TOOLS_DUMP(id);
        
        Int local_id = node_lookup[id];

        if( local_id < Int(0) )
        {
            local_id = ModifiedNodeCount();
            
            node_lookup[id] = local_id;

            mod_nodes.Push( ClisbyNode( id, local_id, T, p, q, mid_changedQ ) );
        }
        
        return mod_nodes[local_id];
    }
    
    void Clear()
    {
        witness[0] = Int(-1);
        witness[1] = Int(-1);
        
        if( mod_nodes.Size() > Int(0) )
        {
            for( ClisbyNode & N : mod_nodes )
            {
                node_lookup[N.id] = Int(-1);
            }
            mod_nodes.Reset();
        }
    }
    
    mref<ClisbyNode> Root()
    {
        return GetNode( T.Root() );
    }
    
//    mref<ClisbyNode> GetLeftChild( cref<ClisbyNode> N )
//    {
//        return GetNode( T.LeftChild(N.id) );
//    }
//    
//    mref<ClisbyNode> GetRightChild( cref<ClisbyNode> N )
//    {
//        return GetNode( T.RightChild(N.id) );
//    }
    
    std::pair<mref<ClisbyNode>,mref<ClisbyNode>> GetChildren( mref<ClisbyNode> N )
    {
//        print("GetChildren");
//        TOOLS_DUMP(N.id);
        mref<ClisbyNode> L = GetNode( T.LeftChild (N.id) );
        mref<ClisbyNode> R = GetNode( T.RightChild(N.id) );
        PushTransform( N, L, R );

//        TOOLS_DUMP(N.id);
//        TOOLS_DUMP(L.id);
//        TOOLS_DUMP(R.id);
        return std::pair<mref<ClisbyNode>,mref<ClisbyNode>>(L,R);
    }
    
    void PushTransform( mref<ClisbyNode> N, mref<ClisbyNode> L, mref<ClisbyNode> R )
    {
        if( N.f.IdentityQ() )
        {
            return;
        }
        
        UpdateNode(N.f,L);
        UpdateNode(N.f,R);
        N.f.Reset();
    }

#include "ModifiedClisbyTree/Update.hpp"
#include "ModifiedClisbyTree/CollisionChecks.hpp"
#include "ModifiedClisbyTree/Fold.hpp"
    
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
            + ">::ModifiedClisbyTree";
    }
}; // ClisbyNode

public:

//void LoadModifications( mref<ModifiedClisbyTree> S )
//{
//    // TODO: Needs also state of random engine?
//    
//    p            = S.p;
//    q            = S.q;
//    theta        = S.theta;
//    reflectQ     = S.reflectQ;
//    mid_changedQ = S.mid_changedQ;
//    transform    = S.transform;
//    witness      = S.witness;
//    
//    // DEBUGGING
////    TOOLS_DUMP( S.mod_nodes.Size() );
////    
////    TOOLS_DUMP(p);
////    TOOLS_DUMP(q);
////    TOOLS_DUMP(theta);
////    TOOLS_DUMP(reflectQ);
////    TOOLS_DUMP(mid_changedQ);
////    
////    TOOLS_DUMP(S.node_lookup);
////    
////    TOOLS_DUMP(S.mod_nodes.begin());
////    TOOLS_DUMP(S.mod_nodes.end());
//    
//    for( ClisbyNode & N : S.mod_nodes )
//    {
////        TOOLS_DUMP(N.id);
//        
//        N.center.Write( NodeCenterPtr(N.id) );
//        NodeRadius(N.id) = N.radius;
//        N.f.Write( NodeTransformPtr(N.id), N_state[N.id] );
//    }
//}

void LoadModifications( mref<ModifiedClisbyTree> S )
{
    // TODO: Needs also state of random engine?
    
    (void)Fold( S.p, S.q, S.theta, S.reflectQ, false );
}

