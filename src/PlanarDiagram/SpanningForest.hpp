public:

void ComputeSpanningForest()
{
    TOOLS_PTIMER(timer,MethodName("ComputeSpanningForest"));
    
    Aggregator<Int,Int> C_pre           ( CrossingCount()    );
    Aggregator<Int,Int> C_post          ( CrossingCount()    );
    Aggregator<Int,Int> roots           ( Int(1)             );
    Aggregator<Int,Int> discovered_arcs ( ArcCount()         );
    Aggregator<Int,Int> tree_darcs      ( ArcCount()         );

    // DEBUGGING
    this->template DepthFirstSearch<true>(
        [&discovered_arcs,this]( cref<DarcNode> A )             // discover
        {
            if( ValidIndexQ(A.da) )
            {
                logprint("discover: Pushing " + this->DarcString(A.da));
                discovered_arcs.Push( FromDarc(A.da).first );
            }
        },
        [&discovered_arcs,this]( cref<DarcNode> A )             // rediscover
        {
            if( ValidIndexQ(A.da) )
            {
                logprint("rediscover: Pushing " + this->DarcString(A.da));
                discovered_arcs.Push( FromDarc(A.da).first );
            }
        },
        [&C_pre,&tree_darcs,&roots]( cref<DarcNode> A )         // pre_visit
        {
            C_pre.Push(A.head);
            
            if( A.da == Uninitialized )
            {
                roots.Push(A.head);
            }
            else
            {
                tree_darcs.Push(A.da);
            }
        },
        [&C_post]( cref<DarcNode> A )                           // post_visit
        {
            C_post.Push(A.head);
        }
    );
    
    if( C_pre.Size() > CrossingCount() )
    {
        eprint(MethodName("ComputeSpanningForest")+": C_pre.Size() > CrossingCount().");
    }
    if( C_post.Size() > CrossingCount() )
    {
        eprint(MethodName("ComputeSpanningForest")+": C_post.Size() > CrossingCount().");
    }
    if( discovered_arcs.Size() > ArcCount() )
    {
        eprint(MethodName("ComputeSpanningForest")+": discovered_arcs.Size() > ArcCount().");
    }
    if( tree_darcs.Size() > ArcCount() )
    {
        eprint(MethodName("ComputeSpanningForest")+": tree_darcs.Size() > ArcCount().");
    }
    
    // DEBUGGING
    
    TOOLS_LOGDUMP(discovered_arcs);

    this->SetCache( "DiagramComponentCount", roots.Size()               );
    this->SetCache( "CrossingPreOrdering",   C_pre.Disband()            );
    this->SetCache( "CrossingPostOrdering",  C_post.Disband()           );
    this->SetCache( "SpanningForestDarcs",   tree_darcs.Disband()       );
    this->SetCache( "SpanningForestRoots",   roots.Disband()            );
    this->SetCache( "DFSArcOrdering",        discovered_arcs.Disband()  );
}


/*! @brief Returns the list of crossings ordered in the way they are pre-visited by `DepthFirstSearch`.
 */

Tensor1<Int,Int> CrossingPreOrdering()
{
    std::string tag ("CrossingPreOrdering");
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }

    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

/*! @brief Returns the list of crossings ordered in the way they are post-visited by `DepthFirstSearch`.
 */

Tensor1<Int,Int> CrossingPostOrdering()
{
    std::string tag ("CrossingPostOrdering");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

/*! @brief Returns a spanning forest in the following format: For every crossing `c` the entry `a = SpanningForestDarcs()[c]` is the _oriented_ arc of the spanning tree that points to `c`. If `c` is a root crossing, then `-1` is returned instead.
 */

Tensor1<Int,Int> SpanningForestDarcs()
{
    std::string tag ("SpanningForestDarcs");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> SpanningForestRoots()
{
    std::string tag ("SpanningForestRoots");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> DFSArcOrdering()
{
    std::string tag ("DFSArcOrdering");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { ComputeSpanningForest(); }
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}
