public:

void ComputeSpanningForest()
{
    TOOLS_PTIC(ClassName()+"::ComputeSpanningForest");
    
    Aggregator<Int,Int> C_pre           ( CrossingCount() );
    Aggregator<Int,Int> C_post          ( CrossingCount() );
    Aggregator<Int,Int> roots           ( Int(1)          );
    Aggregator<Int,Int> discovered_arcs ( ArcCount()      );
    Tensor1<Int,Int>    C_parent_A      ( CrossingCount() );
    
    
    DepthFirstSearch(
        [&discovered_arcs]( cref<DarcNode> A )
        {
            discovered_arcs.Push( FromDarc(A.da).first );
        },
        [&discovered_arcs]( cref<DarcNode> A )
        {
            discovered_arcs.Push( FromDarc(A.da).first );
        },
        [&C_pre,&C_parent_A,&roots]( cref<DarcNode> A )
        {
            C_pre.Push(A.head);
            C_parent_A[A.head] = A.da;
            
            if( A.da == Uninitialized )
            {
                roots.Push(A.head);
            }
        },
        [&C_post]( cref<DarcNode> A )
        {
            C_post.Push(A.head);
        }
    );
    
    this->SetCache( "DiagramComponentCount", roots.Size()               );
    this->SetCache( "CrossingPreOrdering",   C_pre.Disband()            );
    this->SetCache( "CrossingPostOrdering",  C_post.Disband()           );
    this->SetCache( "SpanningForestDarcs",   std::move(C_parent_A)      );
    this->SetCache( "SpanningForestRoots",   roots.Disband()            );
    this->SetCache( "DFSArcOrdering",        discovered_arcs.Disband()  );
    
    TOOLS_PTOC(ClassName()+"::ComputeSpanningForest");
}


/*! @brief Returns the list of crossings ordered in the way they are pre-visited by `DepthFirstSearch`.
 */

Tensor1<Int,Int> CrossingPreOrdering()
{
    std::string tag ("CrossingPreOrdering");
    TOOLS_PTIC(ClassName()+"::"+tag);
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName()+"::"+tag);
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

/*! @brief Returns the list of crossings ordered in the way they are post-visited by `DepthFirstSearch`.
 */

Tensor1<Int,Int> CrossingPostOrdering()
{
    std::string tag ("CrossingPostOrdering");
    TOOLS_PTIC(ClassName()+"::"+tag);
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName()+"::"+tag);
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

/*! @brief Returns a spanning forest in the following format: For every crossing `c` the entry `a = SpanningForestDarcs()[c]` is the _oriented_ arc of the spanning tree that points to `c`. If `c` is a root crossing, then `-1` is returned instead.
 */

Tensor1<Int,Int> SpanningForestDarcs()
{
    std::string tag ("SpanningForestDarcs");
    TOOLS_PTIC(ClassName()+"::"+tag);
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName()+"::"+tag);
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> SpanningForestRoots()
{
    std::string tag ("SpanningForestRoots");
    TOOLS_PTIC(ClassName()+"::"+tag);
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName()+"::"+tag);
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> DFSArcOrdering()
{
    std::string tag ("DFSArcOrdering");
    TOOLS_PTIC(ClassName()+"::"+tag);
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    TOOLS_PTOC(ClassName()+"::"+tag);
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}
