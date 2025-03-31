static Transform_T PivotTransform(
    cref<Vector_T> X, cref<Vector_T> Y, Real angle, bool reflectQ_
)
{
    Transform_T f;
    
    // Rotation axis.
    Vector_T u = (Y - X);
    u.Normalize();
    
    if constexpr ( quaternionsQ )
    {
        const Real theta_half = Scalar::Half<Real> * angle;
        const Real cos = std::cos(theta_half);
        const Real sin = std::sin(theta_half);
        
        Real Q [7] {cos, sin * u[0], sin * u[1], sin * u[2], 0, 0, 0 };
            
        f.Read( &Q[0], NodeFlag_T::NonId );
        
        Vector_T b = X - f.Matrix() * X;
        
        f.ForceReadVector(b);
    }
    else
    {
        const Real cos = std::cos(angle);
        const Real sin = std::sin(angle);
        
        Tiny::Matrix<3,3,Real,Int> a;
        
        const Real d = Scalar::One<Real> - cos;
        
        a[0][0] = u[0] * u[0] * d + cos       ;
        a[0][1] = u[0] * u[1] * d - sin * u[2];
        a[0][2] = u[0] * u[2] * d + sin * u[1];
        
        a[1][0] = u[1] * u[0] * d + sin * u[2];
        a[1][1] = u[1] * u[1] * d + cos       ;
        a[1][2] = u[1] * u[2] * d - sin * u[0];
        
        a[2][0] = u[2] * u[0] * d - sin * u[1];
        a[2][1] = u[2] * u[1] * d + sin * u[0];
        a[2][2] = u[2] * u[2] * d + cos       ;
        
        if( reflectQ_ )
        {
            Tiny::Vector<3,Real,Int> v;
            u.Write( v.data() );
            
            Tiny::Vector<3,Real,Int> w ( Real(0) );
            // Take the vector from the standard basis that makes the biggest angle with v.
            w[v.IAMin()] = Real(1);
            
            // Make w orthogonal to v.
            w -= v * InnerProduct(v,w);
            w -= v * InnerProduct(v,w);
            
            // Reflect in the hyperplane defined by w.
            a = a * HouseholderReflector(w);
        }
        
        Matrix_T A;
        
        A.Read(a.data());
        
        Vector_T b = X - A * X;
        
        f.Read( A, b, NodeFlag_T::NonId );
    }
    
    return f;
}

static void InvertTransform( mref<Transform_T> f )
{
    if constexpr ( quaternionsQ )
    {
        f.Invert();
    }
    else
    {
        Matrix_T A;
        Vector_T b;
        NodeFlag_T flag;
        
        f.Write( A, b, flag );
        
        Tiny::Matrix<3,3,Real,Int> a;
        A.Write(a.data());
        a = a.Transpose();
        A.Read(a.data());
        
        Vector_T c = Real(-1) * (A * b);
        
        f.Read( A, c, NodeFlag_T::NonId );
    }
}

Vector_T NodeCenterAbsoluteCoordinates( const Int node_ ) const
{
    Int node = node_;
    
    Vector_T x = NodeCenter(node);
    
    const Int root = Root();
    
    while( node != root )
    {
        node = Parent(node);
        
        if constexpr ( quaternionsQ )
        {
            // If we use quaternions, then we need to construct only the 3x3 matrix, not the 4x4 matrix. That saves a little time.
            
            x = Transform_T::Transform( NodeTransformPtr(node), NodeFlag(node), x );
        }
        else
        {
            x = NodeTransform(node)(x);
        }
    }
    
    return x;
}

Vector_T VertexCoordinates( const Int vertex ) const
{
    return NodeCenterAbsoluteCoordinates( VertexNode(vertex) );
}

// This version is measurably slower than the previous one.
//Vector_T VertexCoordinates( const Int vertex )
//{
//    Int node = VertexNode(vertex);
//
//    PullTransforms( Root(), node );
//
//    return NodeCenter(node);
//}

/*!
 * @brief Loads the transformation stored in node `node` into a `Transform_T` object.
 *
 */

Transform_T NodeTransform( const Int node ) const
{
    if constexpr ( countersQ )
    {
        call_counters.load_transform += (NodeFlag(node) == NodeFlag_T::NonId);
    }
    
    return Transform_T( NodeTransformPtr(node), NodeFlag(node) );
}

Vector_T NodeCenter( const Int node ) const
{
    return Vector_T( NodeCenterPtr(node) );
}

void UpdateNode( cref<Transform_T> f, const Int node )
{
    if constexpr ( countersQ )
    {
        call_counters.mv += f.TransformVector( NodeCenterPtr(node) );
    }
    else
    {
        (void)f.TransformVector( NodeCenterPtr(node) );
    }
    
    // Transformation of a leaf node never needs a change.
    if( LeafNodeQ( node ) )
    {
        return;
    }
    
    if constexpr ( countersQ )
    {
        bool info = f.TransformTransform( NodeTransformPtr(node), NodeFlag(node) );
        
        call_counters.load_transform += info;
        call_counters.mv             += info;
        call_counters.mm             += info;
    }
    else
    {
        (void)f.TransformTransform( NodeTransformPtr(node), NodeFlag(node) );
    }
}

void PushTransform( const Int node, const Int L, const Int R )
{
    if( N_state[node] == NodeFlag_T::Id )
    {
        return;
    }
    
    Transform_T f = NodeTransform(node);
    
    UpdateNode( f, L );
    UpdateNode( f, R );
    
    ResetTransform(node);
}

// Pushes down all the transformations in the subtree starting at `start_node`.
void PushAllTransforms()
{
    PushTransformsSubtree(Root());
}

void PushTransformsSubtree( const Int start_node )
{
    if constexpr ( manual_stackQ )
    {
        PushTransformsSubtree_ManualStack(start_node);
    }
    else
    {
        PushTransformsSubtree_Recursive(start_node);
    }
}

void PushTransformsSubtree_ManualStack( const Int start_node )
{
    this->DepthFirstSearch(
        [this]( const Int node )                    // interior node previsit
        {
            const auto [L,R] = Children( node );
            PushTransform( node, L, R );
        },
        []( const Int node ) { (void)node; },       // interior node postvisit
        []( const Int node ) { (void)node; },       // leaf node previsit
        []( const Int node ) { (void)node; },       // leaf node postvisit
        start_node
    );
}

void PushTransformsSubtree_Recursive( const Int node )
{
    if( InteriorNodeQ(node) )
    {
        const auto [L,R] = Children( node );
        PushTransform( node, L, R );
        PushTransformsSubtree_Recursive( L );
        PushTransformsSubtree_Recursive( R );
    }
}

void PullTransforms( const Int from, const Int to )
{
    if constexpr ( manual_stackQ )
    {
        PullTransforms_ManualStack(from,to);
    }
    else
    {
        PullTransforms_Recursive(from,to);
    }
}

// TODO: Test this. And try whether it improves the computation of the pivots.
void PullTransforms_Recursive( const Int from, const Int node )
{
    if( node != from )
    {
        PullTransforms_Recursive( from, Parent(node) );
        
        const auto [L,R] = Children( node );
        
        PushTransform( node, L, R );
    }
}

// TODO: Test this. And try whether it improves the computation of the pivots.
void PullTransforms_ManualStack( const Int from, const Int to )
{
    Int stack [max_depth];
    
    Int node = to;
    
    SInt stack_ptr = -1;
    
    while( (node != from) && (stack_ptr < max_depth) )
    {
        node = Parent(node);
        stack[++stack_ptr] = node;
    }
    
    if( stack_ptr >= max_depth )
    {
        eprint(ClassName() + "::PullTransforms_ManualStack: Stack overflow.");
        return;
    }
    
    while( Int(0) <= stack_ptr )
    {
        node = stack[stack_ptr--];
        const auto [L,R] = Children( node );
        PushTransform( node, L, R );
    }
}
