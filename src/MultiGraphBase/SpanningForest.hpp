public:

void ComputeSpanningForest()
{
    TOOLS_PTIC(ClassName() + "::ComputeSpanningForest");
    
    Aggregator<VInt,VInt> V_pre           ( VertexCount() );
    Aggregator<VInt,VInt> V_post          ( VertexCount() );
    Aggregator<VInt,VInt> roots           ( VInt(1)       );
    Aggregator<EInt,EInt> discovered_arcs ( EdgeCount()   );
    Tensor1<VInt,VInt>    V_parent_A      ( VertexCount() );

    DepthFirstSearch(
        [&discovered_arcs]( cref<DirectedEdge> E )
        {
            discovered_arcs.Push(E.de >> 1);
        },
        [&discovered_arcs]( cref<DirectedEdge> E )
        {
            discovered_arcs.Push(E.de >> 1);
        },
        [&V_pre,&V_parent_A,&roots]( cref<DirectedEdge> E )
        {
            V_pre.Push(E.head);
            V_parent_A[E.head] = E.de;
            
            if( E.de < EInt(0) )
            {
                roots.Push(E.head);
            }
        },
        [&V_post]( cref<DirectedEdge> E )
        {
            V_post.Push(E.head);
        }
    );
    
    this->SetCache( "DiagramComponentCount",       roots.Size()            );
    this->SetCache( "VertexPreOrdering",           std::move(V_pre.Get())  );
    this->SetCache( "VertexPostOrdering",          std::move(V_post.Get()) );
    this->SetCache( "SpanningForestDirectedEdges", std::move(V_parent_A)   );
    this->SetCache( "SpanningForestRoots",         std::move(roots.Get())  );
    this->SetCache( "DFSEdgeOrdering",   std::move(discovered_arcs.Get())  );
    
    TOOLS_PTOC(ClassName() + "::ComputeSpanningForest");
}


/*! @brief Returns the list of crossings ordered in the way they are pre-visited by `DepthFirstSearch`.
 */

VV_Vector_T VertexPreOrdering()
{
    std::string tag ("VertexPreOrdering");
    TOOLS_PTIC(ClassName() + "::" + tag);
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName() + "::" + tag);
    return this->template GetCache<VV_Vector_T>(tag);
}

/*! @brief Returns the list of vertices ordered in the way they are post-visited by `DepthFirstSearch`.
 */

VV_Vector_T VertexPostOrdering()
{
    std::string tag ("VertexPostOrdering");
    TOOLS_PTIC(ClassName() + "::" + tag);
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName() + "::" + tag);
    return this->template GetCache<VV_Vector_T>(tag);
}

/*! @brief Returns a spanning forest in the following format: For every vertex `v` the entry `e = SpanningForestDirectedEdges()[v]` is the _oriented_ edge of the spanning tree that points to `v`. If `v` is a root crossing, then `UninitializedEdge` is returned instead.
 */

EE_Vector_T SpanningForestDirectedEdges()
{
    std::string tag ("SpanningForestDirectedEdges");
    TOOLS_PTIC(ClassName() + "::" + tag);
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName() + "::" + tag);
    return this->template GetCache<EE_Vector_T>(tag);
}


VV_Vector_T SpanningForestRoots()
{
    std::string tag ("SpanningForestRoots");
    TOOLS_PTIC(ClassName() + "::" + tag);
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName() + "::" + tag);
    return this->template GetCache<VV_Vector_T>(tag);
}

EE_Vector_T DFSEdgeOrdering()
{
    std::string tag ("DFSEdgeOrdering");
    TOOLS_PTIC(ClassName() + "::" + tag);
    if( !this->InCacheQ(tag) ) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName() + "::" + tag);
    return this->template GetCache<EE_Vector_T>(tag);
}
