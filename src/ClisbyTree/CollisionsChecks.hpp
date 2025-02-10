//#########################################################################################
//##    Collision checks
//#########################################################################################

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
        
//        copy_buffer<AmbDim>(N_L,N);
        
        c_L.Write(N);
        
        return;
    }
    
    if( d + r_L <= r_R ) [[unlikely]]
    {
        N[AmbDim] = r_R;
        
//        copy_buffer<AmbDim>(N_R,N);
        c_R.Write(N);
        
        return;
    }
    
    const Real s = Frac<Real>( r_R - r_L, Scalar::Two<Real> * d );
    
    const Real alpha_L = Scalar::Half<Real> - s;
    const Real alpha_R = Scalar::Half<Real> + s;
    
//    for( Int k = 0; k < AmbDim; ++k )
//    {
//        N[k] = alpha_L * N_L[k] + alpha_R * N_R[k];
//    }
    
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

//bool NodeContainsOnlyUnchangedVerticesQ( const Int node )
//{
//    auto [begin,end] = NodeRange(node);
//    
//  
//    if( mid_changedQ )
//    {
//        // Vertices [0,1,..,p-1,p] U [q,q+1,..,n-1] are unchanged.
//        return (end <= p+1) || (begin >= q);
//    }
//    else
//    {
//        // Vertices [p,p+1,...,q-1,q] are unchanged.
//        return (begin >= p) && (end <= q+1);
//    }
//}
//
//bool NodeContainsChangedVerticesQ( const Int node )
//{
//    return !NodeContainsOnlyUnchangedVerticesQ(node);
//}
//
//bool NodeContainsOnlyChangedVerticesQ( const Int node )
//{
//    auto [begin,end] = NodeRange(node);
//
//    if( mid_changedQ )
//    {
//        // Vertices [p+1,...,q-2,q-1] are changed.
//        return (begin >= p+1) && (end <= q);
//    }
//    else
//    {
//        // Vertices [0,1,..,p-2,p-1] U [q+1,q+2..,n-1] are changed.
//        return (end <= p) || (begin >= q+1);
//    }
//}
//
//bool NodeContainsUnchangedVerticesQ( const Int node )
//{
//    return !NodeContainsOnlyChangedVerticesQ(node);
//}

// First Boolean: whether node contains unchanged vertices.
// Second Boolean: whether node contains changed vertices.
std::pair<bool,bool> NodeSplitFlags( const Int node )
{
    auto [begin,end] = NodeRange(node);
    
    const bool a =  mid_changedQ;
    const bool b = !mid_changedQ;
    
    const Int p_ = p + a;
    const Int q_ = q + b;

    const bool only_midQ = (begin >= p_) && (end <= q_);
    const bool no_midQ   = (end <= p_) || (begin >= q_);
    
    if( mid_changedQ )
    {
        // Vertices [p+1,...,q-2,q-1] are changed.
        return { !only_midQ, !no_midQ };
    }
    else
    {
        // Vertices [0,1,..,p-2,p-1] U [q+1,q+2..,n-1] are changed.
        return { !no_midQ, !only_midQ };
    }
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


bool OverlapQ()
{
    ptic(ClassName()+"::OverlapQ");
    
//    const bool result = OverlapQ_implementation_0();
    
    const bool result = OverlapQ_implementation_1();
    
    ptoc(ClassName()+"::OverlapQ");
    
    return result;
    
}


private:

// Improved version of OverlapQ_implementation_1; we do the box-box checks of all the children at once; this saves us a couple of cache misses.
bool OverlapQ_implementation_1()
{
    const Int n = VertexCount();
    
    witness_0 = -1;
    witness_1 = -1;
    
    Int stack[4 * max_depth][2];
    Int stack_ptr = -1;

    // Helper routine to manage the pair_stack.
    auto push = [&stack,&stack_ptr]( const Int i, const Int j )
    {
        ++stack_ptr;
        stack[stack_ptr][0] = i;
        stack[stack_ptr][1] = j;
    };
    
    // Helper routine to manage the pair_stack.
    auto check_push = [&stack,&stack_ptr,this]( const Int i, const Int j )
    {
        const bool overlappingQ = ( (i==j) || this->NodesOverlapQ(i,j,this->r) );
        
        if( overlappingQ)
        {
            ++stack_ptr;
            stack[stack_ptr][0] = i;
            stack[stack_ptr][1] = j;
        }
    };
    
    // Helper routine to manage the pair_stack.
    auto pop = [&stack,&stack_ptr]()
    {
        auto result = MinMax( stack[stack_ptr][0], stack[stack_ptr][1] );
        stack_ptr--;
        return result;
    };
    
    auto continueQ = [&stack_ptr,this]()
    {
        const bool overflowQ = (stack_ptr >= 4 * max_depth - 4);
        
        if( (0 <= stack_ptr) && (!overflowQ) ) [[likely]]
        {
            return true;
        }
        else
        {
            if ( overflowQ ) [[unlikely]]
            {
                eprint(this->ClassName()+"::OverlapQ_implementation_1: Stack overflow.");
            }
            return false;
        }
    };
    
    auto [b_root_0,b_root_1] = NodeSplitFlags(0);
    
    if( b_root_0 && b_root_1 )
    {
        push(Root(),Root());
    }
    
    while( continueQ() )
    {
        auto [i,j] = pop();
        
        // "Interior node" means "not a leaf node".
        const bool i_interiorQ = InteriorNodeQ(i);
        const bool j_interiorQ = InteriorNodeQ(j);
        
        if( i_interiorQ || j_interiorQ )
        {
            if( i_interiorQ && j_interiorQ )
            {
                // Split both nodes.
                
                auto [L_i,R_i] = Children(i);
                auto [L_j,R_j] = Children(j);

                PushTransform( i, L_i, R_i );
                PushTransform( j, L_j, R_j );
                
                auto [b_L_i_0,b_L_i_1] = NodeSplitFlags(L_i);
                auto [b_R_i_0,b_R_i_1] = NodeSplitFlags(R_i);
                auto [b_L_j_0,b_L_j_1] = NodeSplitFlags(L_j);
                auto [b_R_j_0,b_R_j_1] = NodeSplitFlags(R_j);
                
                if( (b_R_i_0 && b_R_j_1) || (b_R_i_1 && b_R_j_0) )
                {
                    check_push(R_i,R_j);
                }
                
                if( (b_L_i_0 && b_L_j_1) || (b_L_i_1 && b_L_j_0) )
                {
                    check_push(L_i,L_j);
                }
                
                if( (b_L_i_0 && b_R_j_1) || (b_L_i_1 && b_R_j_0) )
                {
                    check_push(L_i,R_j);
                }
                
                // If i == j, we can skip (R_i,L_j) as (R_j,L_i) above is identical.
                if( (i != j) && ((b_R_i_0 && b_L_j_1) || (b_R_i_1 && b_L_j_0)) )
                {
                    check_push(R_i,L_j);
                }
            }
            else
            {
                // split only the interior node
                if ( i_interiorQ ) // !j_interiorQ follows from this.
                {
                    // Split node i.
                    
                    auto [L_i,R_i] = Children(i);
                    
                    PushTransform( i, L_i, R_i );
                    
                    auto [b_j_0  ,b_j_1  ] = NodeSplitFlags(j  );
                    auto [b_R_i_0,b_R_i_1] = NodeSplitFlags(R_i);
                    auto [b_L_i_0,b_L_i_1] = NodeSplitFlags(L_i);
                    
                    if( (b_R_i_0 && b_j_1) || (b_R_i_1 && b_j_0) )
                    {
                        check_push(R_i,j);
                    }
                    
                    if( (b_L_i_0 && b_j_1) || (b_L_i_1 && b_j_0) )
                    {
                        check_push(L_i,j);
                    }
                }
                else
                {
                    // Split node j.
                    
                    auto [L_j,R_j] = Children(j);
                    
                    PushTransform( j, L_j, R_j );

                    auto [b_i_0  ,b_i_1  ] = NodeSplitFlags(i  );
                    auto [b_R_j_0,b_R_j_1] = NodeSplitFlags(R_j);
                    auto [b_L_j_0,b_L_j_1] = NodeSplitFlags(L_j);
                    
                    if( (b_i_0 && b_R_j_1) || (b_i_1 && b_R_j_0) )
                    {
                        check_push(i,R_j);
                    }
                    
                    if( (b_i_0 && b_L_j_1) || (b_i_1 && b_L_j_0) )
                    {
                        check_push(i,L_j);
                    }
                }
            }
        }
        else // [[unlikely]]
        {
            // Nodes i and j are overlapping leaf nodes.
            
            // Rule out that tiny distance errors of neighboring vertices cause problems.
            const Int k = NodeBegin(i);
            const Int l = NodeBegin(j);
            
            const Int delta = Abs(k-l);
            
            if( Min( delta, n - delta ) > 1 )
            {
                witness_0 = k;
                witness_1 = l;
                
                return true;
            }
        }
    }
    
    return false;
    
} // OverlapQ_implementation_1



bool OverlapQ_implementation_0()
{
    const Int n = VertexCount();
    
    witness_0 = -1;
    witness_1 = -1;
    
    Int stack[4 * max_depth][2];
    Int stack_ptr = -1;

    // Helper routine to manage the pair_stack.
    auto push = [&stack,&stack_ptr]( const Int i, const Int j )
    {
        ++stack_ptr;
        stack[stack_ptr][0] = i;
        stack[stack_ptr][1] = j;
    };
    
    // Helper routine to manage the pair_stack.
    auto pop = [&stack,&stack_ptr]()
    {
        auto result = MinMax( stack[stack_ptr][0], stack[stack_ptr][1] );
        stack_ptr--;
        return result;
    };
    
    auto continueQ = [&stack_ptr,this]()
    {
        const bool overflowQ = (stack_ptr >= 4 * max_depth - 4);
        
        if( (0 <= stack_ptr) && (!overflowQ) ) [[likely]]
        {
            return true;
        }
        else
        {
            if ( overflowQ ) [[unlikely]]
            {
                eprint(this->ClassName()+"::OverlapQ_implementation_0: Stack overflow.");
            }
            return false;
        }
    };
    
    auto [b_root_0,b_root_1] = NodeSplitFlags(0);
    
    if( b_root_0 && b_root_1 )
    {
        push(Root(),Root());
    }
    
    while( continueQ() ) 
    {
        auto [i,j] = pop();
        
        const bool overlappingQ = ( (i==j) || NodesOverlapQ(i,j,r) );
        
        if( overlappingQ )
        {
            // "Interior node" means "not a leaf node".
            const bool i_interiorQ = InteriorNodeQ(i);
            const bool j_interiorQ = InteriorNodeQ(j);
            
            if( i_interiorQ || j_interiorQ )
            {
                if( i_interiorQ && j_interiorQ )
                {
                    // Split both nodes.
                    
                    auto [L_i,R_i] = Children(i);
                    auto [L_j,R_j] = Children(j);

                    PushTransform( i, L_i, R_i );
                    PushTransform( j, L_j, R_j );
                    
                    auto [b_L_i_0,b_L_i_1] = NodeSplitFlags(L_i);
                    auto [b_R_i_0,b_R_i_1] = NodeSplitFlags(R_i);
                    auto [b_L_j_0,b_L_j_1] = NodeSplitFlags(L_j);
                    auto [b_R_j_0,b_R_j_1] = NodeSplitFlags(R_j);
                    
                    if( (b_R_i_0 && b_R_j_1) || (b_R_i_1 && b_R_j_0) )
                    {
                        push(R_i,R_j);
                    }
                    
                    if( (b_L_i_0 && b_L_j_1) || (b_L_i_1 && b_L_j_0) )
                    {
                        push(L_i,L_j);
                    }
                    
                    if( (b_L_i_0 && b_R_j_1) || (b_L_i_1 && b_R_j_0) )
                    {
                        push(L_i,R_j);
                    }
                    
                    // If i == j, we can skip (R_i,L_j) as (R_j,L_i) above is identical.
                    if( (i != j) && ((b_R_i_0 && b_L_j_1) || (b_R_i_1 && b_L_j_0)) )
                    {
                        push(R_i,L_j);
                    }
                }
                else
                {
                    // split only the interior node
                    if ( i_interiorQ ) // !j_interiorQ follows from this.
                    {
                        // Split node i.
                        
                        auto [L_i,R_i] = Children(i);
                        
                        PushTransform( i, L_i, R_i );
                        
                        auto [b_j_0  ,b_j_1  ] = NodeSplitFlags(j  );
                        auto [b_R_i_0,b_R_i_1] = NodeSplitFlags(R_i);
                        auto [b_L_i_0,b_L_i_1] = NodeSplitFlags(L_i);
                        
                        if( (b_R_i_0 && b_j_1) || (b_R_i_1 && b_j_0) )
                        {
                            push(R_i,j);
                        }
                        
                        if( (b_L_i_0 && b_j_1) || (b_L_i_1 && b_j_0) )
                        {
                            push(L_i,j);
                        }
                    }
                    else
                    {
                        // Split node j.
                        
                        auto [L_j,R_j] = Children(j);
                        
                        PushTransform( j, L_j, R_j );

                        auto [b_i_0  ,b_i_1  ] = NodeSplitFlags(i  );

                        auto [b_R_j_0,b_R_j_1] = NodeSplitFlags(R_j);
                        auto [b_L_j_0,b_L_j_1] = NodeSplitFlags(L_j);
                        
                        if( (b_i_0 && b_R_j_1) || (b_i_1 && b_R_j_0) )
                        {
                            push(i,R_j);
                        }
                        
                        if( (b_i_0 && b_L_j_1) || (b_i_1 && b_L_j_0) )
                        {
                            push(i,L_j);
                        }
                    }
                }
            }
            else // [[unlikely]]
            {
                // Nodes i and j are overlapping leaf nodes.
                
                // Rule out that tiny distance errors of neighboring vertices cause problems.
                const Int k = NodeBegin(i);
                const Int l = NodeBegin(j);
                
                const Int delta = Abs(k-l);
                
                if( Min( delta, n - delta ) > 1 )
                {
                    witness_0 = k;
                    witness_1 = l;
                    
                    return true;
                }
            }
        }
    }
    
    return false;
    
} // OverlapQ_implementation_0
