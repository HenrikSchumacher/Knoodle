public:

void ComputeSpanningForest()
{
    TOOLS_PTIC(ClassName() + "::ComputeSpanningForest");
    
    Aggregator<Int,Int> C_pre (CrossingCount());
    Aggregator<Int,Int> C_post (CrossingCount());
    
    Tensor1<Int,Int> C_parent_A (CrossingCount());
    
    Aggregator<Int,Int> roots(Int(1));
    Aggregator<Int,Int> discovered_arcs(ArcCount());
    
    DepthFirstSearch(
        [&discovered_arcs]( cref<DirectedArcNode> da )
        {
            discovered_arcs.Push(da.a >> 1);
        },
        [&discovered_arcs]( cref<DirectedArcNode> da )
        {
            discovered_arcs.Push(da.a >> 1);
        },
        [&C_pre,&C_parent_A,&roots]( cref<DirectedArcNode> da )
        {
            C_pre.Push(da.head);
            C_parent_A[da.head] = da.a;
            
            if( da.a < Int(0) )
            {
                roots.Push(da.head);
            }
        },
        [&C_post]( cref<DirectedArcNode> da )
        {
            C_post.Push(da.head);
        }
    );
    
    this->SetCache( "DiagramComponentCount",      roots.Size()            );
    this->SetCache( "CrossingPreOrdering",        std::move(C_pre.Get())  );
    this->SetCache( "CrossingPostOrdering",       std::move(C_post.Get()) );
    this->SetCache( "SpanningForestDirectedArcs", std::move(C_parent_A)   );
    this->SetCache( "SpanningForestRoots",        std::move(roots.Get())  );
    this->SetCache( "DFSArcOrdering",   std::move(discovered_arcs.Get())  );
    
    TOOLS_PTOC(ClassName() + "::ComputeSpanningForest");
}



/*! @brief Returns the list of crossings ordered in the way they are pre-visited by `DepthFirstSearch`.
 */

Tensor1<Int,Int> CrossingPreOrdering()
{
    std::string tag ("CrossingPreOrdering");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        ComputeSpanningForest();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

/*! @brief Returns the list of crossings ordered in the way they are post-visited by `DepthFirstSearch`.
 */

Tensor1<Int,Int> CrossingPostOrdering()
{
    std::string tag ("CrossingPostOrdering");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        ComputeSpanningForest();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

/*! @brief Returns a spanning forest in the following format: For every crossing `c` the entry `a = SpanningForestDirectedArcs()[c]` is the _oriented_ arc of the spanning tree that points to `c`. If `c` is a root crossing, then `-1` is returned instead.
 */

Tensor1<Int,Int> SpanningForestDirectedArcs()
{
    std::string tag ("SpanningForestDirectedArcs");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        ComputeSpanningForest();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> SpanningForestRoots()
{
    std::string tag ("SpanningForestRoots");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        ComputeSpanningForest();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> DFSArcOrdering()
{
    std::string tag ("DFSArcOrdering");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        ComputeSpanningForest();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}
