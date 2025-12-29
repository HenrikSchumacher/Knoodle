#include "SubtreesCollideQ_Recursive.hpp"


public:


/*!@brief Checks for collisions in the tree.
 *
 * @tparam full_checkQ If `full_checkQ == false` (default), then this assumes that the tree has been collision-free before the last pivot move i.e., before the last call to `Fold`. If `full_checkQ == true`, then a full check is run on the whole tree. Note that this will take siginificantly longer.
 *
 * @tparam debugQ If `debugQ == true`, then the debugging mode is active: The result will be checked against the very thorough, but also very slow collision checker `CollisionQ_Debug`.
 */

template<bool full_checkQ = true, bool debugQ = false>
bool CollisionQ()
{
    TOOLS_PTIMER(timer,MethodName("CollisionQ")+"<" + BoolString(full_checkQ) + "," + BoolString(debugQ) + ">");
    
    witness[0] = -1;
    witness[1] = -1;
        
    bool result;
    
    if( mid_changedQ )
    {
        result = SubtreesCollideQ_Recursive<true,full_checkQ>( Root() );
    }
    else
    {
        result = SubtreesCollideQ_Recursive<false,full_checkQ>( Root() );
    }
    
    if constexpr ( debugQ )
    {
        auto [result_debug,witness_debug] = CollisionQ_Debug();
        
        if( result != result_debug )
        {
            eprint(MethodName("CollisionQ")+"<" + BoolString(full_checkQ) + "," + BoolString(debugQ) + ">" + ": Incorrect collision result.");
            TOOLS_DDUMP(result);
            TOOLS_DDUMP(result_debug);
            
            if( result_debug )
            {
                TOOLS_DDUMP(witness_debug);
            }
        }
    }

    return result;
}

/*!@brief Merges two input node into a third one. This computes the smallest ball wth center is on the line segment between the two nodes's centers that contains both node's bounding balls.
 *
 * @param N_L Pointer to data of first node to merge.
 * @param N_R Pointer to data of second node to merge.
 * @param N  Pointer to data of target node; the result will be written there.
 */

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
    const cptr<Real> N_ptr_0, const cptr<Real> N_ptr_1
)
{
    const Vector_T c_L ( N_ptr_0 );
    const Vector_T c_R ( N_ptr_1 );
    
    const Vector_T delta = c_R - c_L;
    
    const Real d2 = delta.SquaredNorm();
    
    return d2;
}


static constexpr Real SquaredDistance( cref<Vector_T> x, cref<Vector_T> y )
{
    Vector_T z = x - y;
    
    return z.SquaredNorm();
}
                
static constexpr bool BallsCollideQ(
    const cptr<Real> N_ptr_0, const cptr<Real> N_ptr_1, const Real diam
)
{
    const Real d2 = NodeCenterSquaredDistance(N_ptr_0,N_ptr_1);
    
    const Real threshold = diam + N_ptr_0[AmbDim] + N_ptr_1[AmbDim];
    
    return (d2 < threshold * threshold);
}

bool BallsCollideQ(
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

bool BallsCollideQ( const Int node_0, const Int node_1) const
{
    if constexpr ( countersQ )
    {
        ++call_counters.overlap;
    }
    
    return BallsCollideQ( NodeBallPtr(node_0), NodeBallPtr(node_1), hard_sphere_diam );
}

FoldFlag_T CheckJoints()
{
    const Int n = VertexCount();
    
    {
        const Int p_prev = (p == Int(0))     ? (n - Int(1)) : (p - Int(1));
        const Int p_next = (p + Int(1) == n) ? Int(0)       : (p + Int(1));
        
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
            return FoldFlag_T::RejectedByJoint0;
        }
    }
    
    {
        const Int q_prev = (q == Int(0))     ? (n - Int(1)) : (q - Int(1));
        const Int q_next = (q + Int(1) == n) ? Int(0)       : (q + Int(1));
        
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
            return FoldFlag_T::RejectedByJoint1;
        }
    }
    return FoldFlag_T::Accepted;
}


// First Boolean: whether node contains unchanged vertices.
// Second Boolean: whether node contains changed vertices.
template<bool mQ>
NodeSplitFlagVector_T NodeSplitFlagVector( const Int node ) const
{
    auto [begin,end] = NodeRange(node);
    
    const bool not_only_midQ = (begin < p_shifted) || (end   > q_shifted);
    const bool not_no_midQ   = (end   > p_shifted) && (begin < q_shifted);

    if constexpr ( mQ )
    {
        return NodeSplitFlagVector_T({not_only_midQ,not_no_midQ});
    }
    else
    {
        return NodeSplitFlagVector_T({not_no_midQ,not_only_midQ});
    }
}


// 1st Boolean: whether first node contains unchanged vertices.
// 2nd Boolean: whether first node contains changed vertices.
// 3rd Boolean: whether second node contains unchanged vertices.
// 4th Boolean: whether second node contains changed vertices.
template<bool mQ>
NodeSplitFlagMatrix_T NodeSplitFlagMatrix( const Int i, const Int j ) const
{
    auto [begin_i,end_i] = NodeRange(i);
    auto [begin_j,end_j] = NodeRange(j);
    
    const bool i_not_only_midQ = (begin_i < p_shifted) || (end_i   > q_shifted);
    const bool i_not_no_midQ   = (end_i   > p_shifted) && (begin_i < q_shifted);
    const bool j_not_only_midQ = (begin_j < p_shifted) || (end_j   > q_shifted);
    const bool j_not_no_midQ   = (end_j   > p_shifted) && (begin_j < q_shifted);
    
    if constexpr ( mQ )
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
