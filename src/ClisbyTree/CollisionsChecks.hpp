//#########################################################################################
//##    Collision checks
//#########################################################################################

#include "OverlapQ_Reference.hpp"
#include "OverlapQ_ManualStack.hpp"

#include "SubtreesOverlapQ_Recursive.hpp"


public:

bool OverlapQ()
{
    ptic(ClassName()+"::OverlapQ");
    
    witness_0 = -1;
    witness_1 = -1;
        
    bool result;
    
    if constexpr ( use_manual_stackQ )
    {
        result = OverlapQ_ManualStack(); // 32.9882 s
    }
    else
    {
        result = SubtreesOverlapQ_Recursive( Root() ); // 29.9892 s / ?? s.
    }
    
////     DEBUGGING
//    {
//        bool resultReference = OverlapQ_Reference();
//        
//        if( result != resultReference )
//        {
//            eprint("!!!");
//            exit(-1);
//        }
//    }

    ptoc(ClassName()+"::OverlapQ");
    
    return result;
    
}


static constexpr void MergeBalls( cptr<Real> N_L, cptr<Real> N_R, mptr<Real> N )
{
//    Real d2 = 0;
//    
//    for( Int k = 0; k < AmbDim; ++k )
//    {
//        const Real delta = N_R[k] - N_L[k];
//        
//        d2 += delta * delta;
//    }
//
//    const Real d   = Sqrt(d2);
    
    
    const Vector_T c_L ( N_L );
    const Vector_T c_R ( N_R );
    
    const Real d = (c_R - c_L).Norm();
    
    
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
    
    MergeBalls( NodeData(L), NodeData(R), NodeData(node) );
}

static constexpr Real NodeCenterSquaredDistance(
    const cptr<Real> N_0, const cptr<Real> N_1
)
{
    const Vector_T c_L ( N_0 );
    const Vector_T c_R ( N_1 );
    
    const Real d2 = (c_R - c_L).SquaredNorm();
    
    return d2;
}
                
static constexpr bool NodesOverlapQ(
    const cptr<Real> N_0, const cptr<Real> N_1, const Real r
)
{
    const Real d2 = NodeCenterSquaredDistance(N_0,N_1);
    
    const Real R  = r + N_0[AmbDim] + N_1[AmbDim];
    
    const bool overlapQ = d2 < R * R;
    
    return overlapQ;
}

bool NodesOverlapQ( const Int node_0, const Int node_1, const Real radius ) const
{
    return NodesOverlapQ( NodeData(node_0), NodeData(node_1), radius );
}

int CheckJoints() const
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
    
    if( (X_p_next - X_p_prev).SquaredNorm() <= r2 )
    {
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
    
    if( (X_q_next - X_q_prev).SquaredNorm() <= r2 )
    {
        return 3;
    }
    
    return 0;
}


// First Boolean: whether node contains unchanged vertices.
// Second Boolean: whether node contains changed vertices.
__attribute__((hot)) std::pair<bool,bool> NodeSplitFlags( const Int node ) const
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

__attribute__((hot)) NodeSplitFlagVector_T NodeSplitFlagVector( const Int node ) const
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

__attribute__((hot)) void NodeSplitFlagVector( const Int node, mptr<bool> f ) const
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

__attribute__((hot)) NodeSplitFlagMatrix_T NodeSplitFlagMatrix( const Int i, const Int j ) const
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

__attribute__((hot)) force_inline void NodeSplitFlagMatrix( const Int i, const Int j, mptr<bool> F ) const
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
__attribute__((hot)) force_inline void NodeSplitFlags( cptr<Int> nodes, mptr<bool> F ) const
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


__attribute__((hot)) force_inline void NodeSplitFlags_3( cptr<Int> nodes, mptr<bool> F ) const
{
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;
    
    auto [begin_0,end_0] = NodeRange(nodes[0]);
    auto [begin_1,end_1] = NodeRange(nodes[1]);
    auto [begin_2,end_2] = NodeRange(nodes[2]);
    
    const bool not_only_midQ_0 = (begin_0 < p_) || (end_0   > q_);
    const bool not_no_midQ_0   = (end_0   > p_) && (begin_0 < q_);
    const bool not_only_midQ_1 = (begin_1 < p_) || (end_1   > q_);
    const bool not_no_midQ_1   = (end_1   > p_) && (begin_1 < q_);
    const bool not_only_midQ_2 = (begin_2 < p_) || (end_2   > q_);
    const bool not_no_midQ_2   = (end_2   > p_) && (begin_2 < q_);
    
    F[0+ mid_changedQ] = not_no_midQ_0  ;
    F[0+!mid_changedQ] = not_only_midQ_0;
    F[2+ mid_changedQ] = not_no_midQ_1  ;
    F[2+!mid_changedQ] = not_only_midQ_1;
    F[4+ mid_changedQ] = not_no_midQ_2  ;
    F[4+!mid_changedQ] = not_only_midQ_2;
}

__attribute__((hot)) force_inline void NodeSplitFlags_4( cptr<Int> nodes, mptr<bool> F ) const
{
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;
    
    auto [begin_0,end_0] = NodeRange(nodes[0]);
    auto [begin_1,end_1] = NodeRange(nodes[1]);
    auto [begin_2,end_2] = NodeRange(nodes[2]);
    auto [begin_3,end_3] = NodeRange(nodes[3]);
    
    const bool not_only_midQ_0 = (begin_0 < p_) || (end_0   > q_);
    const bool not_no_midQ_0   = (end_0   > p_) && (begin_0 < q_);
    const bool not_only_midQ_1 = (begin_1 < p_) || (end_1   > q_);
    const bool not_no_midQ_1   = (end_1   > p_) && (begin_1 < q_);
    const bool not_only_midQ_2 = (begin_2 < p_) || (end_2   > q_);
    const bool not_no_midQ_2   = (end_2   > p_) && (begin_2 < q_);
    const bool not_only_midQ_3 = (begin_3 < p_) || (end_3   > q_);
    const bool not_no_midQ_3   = (end_3   > p_) && (begin_3 < q_);
    
    F[0+ mid_changedQ] = not_no_midQ_0  ;
    F[0+!mid_changedQ] = not_only_midQ_0;
    F[2+ mid_changedQ] = not_no_midQ_1  ;
    F[2+!mid_changedQ] = not_only_midQ_1;
    F[4+ mid_changedQ] = not_no_midQ_2  ;
    F[4+!mid_changedQ] = not_only_midQ_2;
    F[6+ mid_changedQ] = not_no_midQ_3  ;
    F[6+!mid_changedQ] = not_only_midQ_3;
}
