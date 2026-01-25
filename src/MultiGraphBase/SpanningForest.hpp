public:

void ComputeSpanningForest()
{
    TOOLS_PTIMER(timer,MethodName("ComputeSpanningForest"));
    
    Aggregator<VInt,VInt> V_pre           ( VertexCount() );
    Aggregator<VInt,VInt> V_post          ( VertexCount() );
    Aggregator<VInt,VInt> roots           ( VInt(1)       );
    Aggregator<EInt,EInt> discovered_arcs ( EdgeCount()   );
    Tensor1<VInt,VInt>    V_parent_A      ( VertexCount() );

    DepthFirstSearch(
        [&discovered_arcs]( cref<DedgeNode> E )
        {
            discovered_arcs.Push(E.de >> 1);
        },
        [&discovered_arcs]( cref<DedgeNode> E )
        {
            discovered_arcs.Push(E.de >> 1);
        },
        [&V_pre,&V_parent_A,&roots]( cref<DedgeNode> E )
        {
            V_pre.Push(E.head);
            V_parent_A[E.head] = E.de;
            
            if( E.de < EInt(0) )
            {
                roots.Push(E.head);
            }
        },
        [&V_post]( cref<DedgeNode> E )
        {
            V_post.Push(E.head);
        }
    );
    
    this->template SetCache<false>( "DiagramComponentCount", roots.Size()               );
    this->template SetCache<false>( "VertexPreOrdering",     V_pre.Disband()            );
    this->template SetCache<false>( "VertexPostOrdering",    V_post.Disband()           );
    this->template SetCache<false>( "SpanningForestDedges",  V_parent_A                 );
    this->template SetCache<false>( "SpanningForestRoots",   roots.Disband()            );
    this->template SetCache<false>( "DFSEdgeOrdering",       discovered_arcs.Disband()  );
}


/*! @brief Returns the list of crossings ordered in the way they are pre-visited by `DepthFirstSearch`.
 */

VV_Vector_T VertexPreOrdering()
{
    std::string tag ("VertexPreOrdering");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    return this->template GetCache<VV_Vector_T>(tag);
}

/*! @brief Returns the list of vertices ordered in the way they are post-visited by `DepthFirstSearch`.
 */

VV_Vector_T VertexPostOrdering()
{
    std::string tag ("VertexPostOrdering");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    return this->template GetCache<VV_Vector_T>(tag);
}

/*! @brief Returns a spanning forest in the following format: For every vertex `v` the entry `e = SpanningForestDedges()[v]` is the _oriented_ edge of the spanning tree that points to `v`. If `v` is a root crossing, then `UninitializedEdge` is returned instead.
 */

EE_Vector_T SpanningForestDedges()
{
    std::string tag ("SpanningForestDedges");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    return this->template GetCache<EE_Vector_T>(tag);
}


VV_Vector_T SpanningForestRoots()
{
    std::string tag ("SpanningForestRoots");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    return this->template GetCache<VV_Vector_T>(tag);
}

EE_Vector_T DFSEdgeOrdering()
{
    std::string tag ("DFSEdgeOrdering");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    return this->template GetCache<EE_Vector_T>(tag);
}
