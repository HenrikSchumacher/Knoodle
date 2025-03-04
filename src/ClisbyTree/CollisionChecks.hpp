#include "CollidingQ_Reference.hpp"
#include "CollidingQ_ManualStack.hpp"

#include "SubtreesCollideQ_Recursive.hpp"
#include "SubtreesCollideQ_Recursive2.hpp"


public:

bool OverlapQ()
{
    TOOLS_PTIC(ClassName()+"::OverlapQ");
    
    witness[0] = -1;
    witness[1] = -1;
        
    bool result;
    
    if constexpr ( use_manual_stackQ )
    {
        result = CollidingQ_ManualStack(); // 32.9882 s
    }
    else
    {
        result = SubtreesCollideQ_Recursive( Root() ); // 29.9892 s / ?? s.
        
//        result = SubtreesCollideQ_Rec2( Root(), NodeTransform(Root()) );
    }
    
////     DEBUGGING
//    {
//        Int w_0 = witness[0];
//        Int w_1 = witness[1];
//
//        bool resultReference = CollidingQ_Reference();
//
//        if( result != resultReference )
//        {
//            eprint("!!!");
//            dump(result)
//            dump(resultReference)
//            
//            dump(w_0);
//            dump(witness[0]);
//            dump(w_1);
//            dump(witness[1]);
//            exit(-1);
//        }
//    }

    TOOLS_PTOC(ClassName()+"::OverlapQ");
    
    return result;
    
}


static constexpr void MergeBalls( cptr<Real> N_L, cptr<Real> N_R, mptr<Real> N )
{
    const Vector_T c_L ( N_L );
    const Vector_T c_R ( N_R );
    
    const Real d = Distance(c_R,c_L);
    
    
    const Real r_L = N_L[AmbDim];
    const Real r_R = N_R[AmbDim];
    
    if( d + r_R <= r_L ) [[unlikely]]
    {
        N[AmbDim] = r_L;
        c_L.Write(N);
        
        return;
    }
    
    if( d + r_L <= r_R ) [[unlikely]]
    {
        N[AmbDim] = r_R;
        c_R.Write(N);
        
        return;
    }
    
    const Real s = Frac<Real>( r_R - r_L, Scalar::Two<Real> * d );
    
    const Real alpha_L = Scalar::Half<Real> - s;
    const Real alpha_R = Scalar::Half<Real> + s;
    
    const Vector_T c = alpha_L * c_L + alpha_R * c_R;
    c.Write(N);

    N[AmbDim] = Scalar::Half<Real> * ( d + r_L + r_R );
}

void ComputeBall( const Int node )
{
    auto [L,R] = Children(node);
    
    MergeBalls( NodeBallPtr(L), NodeBallPtr(R), NodeBallPtr(node) );
}

static constexpr Real NodeCenterSquaredDistance(
    const cptr<Real> N_0, const cptr<Real> N_1
)
{
    const Vector_T c_L ( N_0 );
    const Vector_T c_R ( N_1 );
    
    const Vector_T delta = c_R - c_L;
    
    const Real d2 = delta.SquaredNorm();
    
    return d2;
}


static constexpr Real SquaredDistance( cref<Vector_T> x, cref<Vector_T> y )
{
    Vector_T z = x - y;
    
    return z.SquaredNorm();
}
                
static constexpr bool BallsOverlapQ(
    const cptr<Real> N_0, const cptr<Real> N_1, const Real diam
)
{
    const Real d2 = NodeCenterSquaredDistance(N_0,N_1);
    
    const Real threshold = diam + N_0[AmbDim] + N_1[AmbDim];
    
    return (d2 < threshold * threshold);
}

bool BallsOverlapQ(
    cref<Vector_T> c_0, const Real r_0,
    cref<Vector_T> c_1, const Real r_1,
    const Real diam
)
{
    if constexpr ( countersQ )
    {
        ++call_counters.overlap;
    }
    
    const Real d2 = SquaredDistance(c_0,c_1);
    
    const Real threshold = diam + r_0 + r_1;
    
    return (d2 < threshold * threshold);
}

bool BallsOverlapQ( const Int node_0, const Int node_1) const
{
    if constexpr ( countersQ )
    {
        ++call_counters.overlap;
    }
    
    return BallsOverlapQ(
        NodeBallPtr(node_0), NodeBallPtr(node_1), hard_sphere_diam
    );
}

int CheckJoints()
{
    const Int n = VertexCount();
    
    const Int p_prev = (p == 0)     ? (n - 1) : (p - 1);
    const Int p_next = (p + 1 == n) ? Int(0)  : (p + 1);

    Vector_T X_p_prev = VertexCoordinates(p_prev);
    Vector_T X_p_next = VertexCoordinates(p_next);
    
    if( mid_changedQ )
    {
        X_p_next = transform(X_p_next);
    }
    else
    {
        X_p_prev = transform(X_p_prev);
    }
    
    
    if( SquaredDistance(X_p_next,X_p_prev) <= hard_sphere_squared_diam )
    {
        witness[0] = p_prev;
        witness[1] = p_next;
        return 2;
    }
    
    const Int q_prev = (q == 0)     ? (n - 1) : (q - 1);
    const Int q_next = (q + 1 == n) ? Int(0)  : (q + 1);
    
    Vector_T X_q_prev = VertexCoordinates(q_prev);
    Vector_T X_q_next = VertexCoordinates(q_next);
    
    if( mid_changedQ )
    {
        X_q_prev = transform(X_q_prev);
    }
    else
    {
        X_q_next = transform(X_q_next);
    }
    
    if( SquaredDistance(X_q_next,X_q_prev) <= hard_sphere_squared_diam )
    {
        witness[0] = q_prev;
        witness[1] = q_next;
        return 3;
    }
    
    return 0;
}


// First Boolean: whether node contains unchanged vertices.
// Second Boolean: whether node contains changed vertices.
std::pair<bool,bool> NodeSplitFlags( const Int node ) const
{
    auto [begin,end] = NodeRange(node);
    
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;

    const bool not_only_midQ = (begin < p_) || (end   > q_);
    const bool not_no_midQ   = (end   > p_) && (begin < q_);
    
    return mid_changedQ
            ? std::pair( not_only_midQ, not_no_midQ    )
            : std::pair( not_no_midQ  , not_only_midQ );
    
//    const bool only_midQ = (begin >= p_) && (end   <= q_);
//    const bool no_midQ   = (end   <= p_) || (begin >= q_);
//
//    if( mid_changedQ )
//    {
//        // Vertices [p+1,...,q-2,q-1] are changed.
//        return { !only_midQ, !no_midQ };
//    }
//    else
//    {
//        // Vertices [0,1,..,p-2,p-1] U [q+1,q+2..,n-1] are changed.
//        return { !no_midQ, !only_midQ };
//    }
}

NodeSplitFlagVector_T NodeSplitFlagVector( const Int node ) const
{
    auto [begin,end] = NodeRange(node);
    
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;

    const bool not_only_midQ = (begin < p_) || (end   > q_);
    const bool not_no_midQ   = (end   > p_) && (begin < q_);

    if( mid_changedQ )
    {
        return NodeSplitFlagVector_T({not_only_midQ,not_no_midQ});
    }
    else
    {
        return NodeSplitFlagVector_T({not_no_midQ,not_only_midQ});
    }
}

void NodeSplitFlagVector( const Int node, mptr<bool> f ) const
{
    auto [begin,end] = NodeRange(node);
    
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;

    const bool not_only_midQ = (begin < p_) || (end   > q_);
    const bool not_no_midQ   = (end   > p_) && (begin < q_);

    if( mid_changedQ )
    {
        f[0] = not_only_midQ;
        f[1] = not_no_midQ;
    }
    else
    {
        f[0] = not_no_midQ;
        f[1] = not_only_midQ;
    }
}

NodeSplitFlagMatrix_T NodeSplitFlagMatrix( const Int i, const Int j ) const
{
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;

    auto [begin_i,end_i] = NodeRange(i);
    auto [begin_j,end_j] = NodeRange(j);
    
    const bool i_not_only_midQ = (begin_i < p_) || (end_i   > q_);
    const bool i_not_no_midQ   = (end_i   > p_) && (begin_i < q_);
    const bool j_not_only_midQ = (begin_j < p_) || (end_j   > q_);
    const bool j_not_no_midQ   = (end_j   > p_) && (begin_j < q_);
    
    if( mid_changedQ )
    {
        return NodeSplitFlagMatrix_T({
            {i_not_only_midQ,i_not_no_midQ},
            {j_not_only_midQ,j_not_no_midQ},
        });
    }
    else
    {
        return NodeSplitFlagMatrix_T({
            {i_not_no_midQ,i_not_only_midQ},
            {j_not_no_midQ,j_not_only_midQ},
        });
    }
}

void NodeSplitFlagMatrix( const Int i, const Int j, mptr<bool> F ) const
{
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;

    auto [begin_i,end_i] = NodeRange(i);
    auto [begin_j,end_j] = NodeRange(j);
    
    const bool i_not_only_midQ = (begin_i < p_) || (end_i   > q_);
    const bool i_not_no_midQ   = (end_i   > p_) && (begin_i < q_);
    const bool j_not_only_midQ = (begin_j < p_) || (end_j   > q_);
    const bool j_not_no_midQ   = (end_j   > p_) && (begin_j < q_);
    
    if( mid_changedQ )
    {
        F[0] = i_not_only_midQ;
        F[1] = i_not_no_midQ;
        F[2] = j_not_only_midQ;
        F[3] = j_not_no_midQ;
    }
    else
    {
        F[0] = i_not_no_midQ;
        F[1] = i_not_only_midQ;
        F[2] = j_not_no_midQ;
        F[3] = j_not_only_midQ;
    }
}


template<Int N>
void NodeSplitFlags( cptr<Int> nodes, mptr<bool> F ) const
{
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;
    
    for( Int i = 0; i < N; ++i )
    {
        auto [begin,end] = NodeRange(nodes[i]);
        
        const bool not_only_midQ = (begin < p_) || (end   > q_);
        const bool not_no_midQ   = (end   > p_) && (begin < q_);
        
//        F[2*i+0] = mid_changedQ ? not_only_midQ : not_no_midQ  ;
//        F[2*i+1] = mid_changedQ ? not_no_midQ   : not_only_midQ;
        
        F[2*i+ mid_changedQ] = not_no_midQ  ;
        F[2*i+!mid_changedQ] = not_only_midQ;
    }
}
