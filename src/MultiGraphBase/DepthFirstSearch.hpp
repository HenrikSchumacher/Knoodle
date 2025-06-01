//########################################################
//##    Some Auxiliaries
//########################################################

public:

struct DirectedEdge
{
    VInt v = -1; // vertex we came from
    EInt e = -1; // direction is stored in lowest bit.
    VInt w = -1; // vertex we go to
};

constexpr static auto TrivialEdgeFunction = []( cref<DirectedEdge> de )
{
    (void)de;
};

constexpr static auto PrintDiscover = []( cref<DirectedEdge> de )
{
    const EInt e = de.e >> 1;
    const bool d = de.e | EInt(1);
    
    print("Discovering vertex " + ToString(de.w) + " from vertex " + ToString(de.v) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintRediscover = []( cref<DirectedEdge> de )
{
    const EInt e = de.e >> 1;
    const bool d = de.e | EInt(1);

    print("Rediscovering vertex " + ToString(de.w) + " from vertex " + ToString(de.v) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintPreVisit = []( cref<DirectedEdge> de )
{
    const EInt e = de.e >> 1;
    const bool d = de.e | EInt(1);
    
    print("Pre-visiting vertex " + ToString(de.w) + " from vertex " + ToString(de.v) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};

constexpr static auto PrintPostVisit = []( cref<DirectedEdge> de )
{
    const EInt e = de.e >> 1;
    const bool d = de.e | EInt(1);
    
    print("Post-visiting vertex " + ToString(de.w) + " from vertex " + ToString(de.v) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};


//########################################################
//##    DepthFirstSearch (fast)
//########################################################

public:

/*!@brief This traverses the graph in a slightly _modified_ depth-first order: In the case `dir == InOut::Undirected` suppose we are at the currently visited vertex `v`. Then new neighboring vertices  `w` are given precedence if they are reached from `v` via an outgoing edge. That means, an edge `e_0` of the form `{v,w_0}` will always be traversed before an edge `e_1` of the form `{w_1,v}`, no matter whether the edge index `e_0` is smaller or greater than `e_1`.  This is to allow the use of `InIncidenceMatrix` and `OutIncidenceMatrix`, which together are faster to assemble than `OrientedIncidenceMatrix`.
 */

template<
    InOut dir = InOut::Undirected,
    class DiscoverVertex_T,   // f( const DirectedEdge & de )
    class RediscoverVertex_T, // f( const DirectedEdge & de )
    class PreVisitVertex_T,   // f( const DirectedEdge & de )
    class PostVisitVertex_T   // f( const DirectedEdge & de )
>
void DepthFirstSearch(
    DiscoverVertex_T   && discover,
    RediscoverVertex_T && rediscover,
    PreVisitVertex_T   && pre_visit,
    PostVisitVertex_T  && post_visit
)
{
    if( vertex_count <= VInt(0) )
    {
        return;
    }
    
    TOOLS_PTIC( ClassName() + "::DepthFirstSearch<" + ToString(dir) + ">");
    
    const EInt * E_V = edges.data();
    
    const EInt * V_Out_ptr;
    const EInt * V_Out_idx;
    
    const EInt * V_In_ptr;
    const EInt * V_In_idx;
    
    if constexpr( dir != InOut::In )
    {
        auto & A = OutIncidenceMatrix();
        V_Out_ptr   = A.Outer().data();
        V_Out_idx   = A.Inner().data();
    }
    
    if constexpr( dir != InOut::Out )
    {
        auto & A = InIncidenceMatrix();
        V_In_ptr = A.Outer().data();
        V_In_idx = A.Inner().data();
    }
    
    // V_flag[v] == 0 means unvisited.
    // V_flag[v] == 1 means discovered.
    // V_flag[v] == 1 means visited, but not yet postvisited.
    // V_flag[v] == 2 means visited and postvisited.
    mptr<UInt8> V_flag = reinterpret_cast<UInt8 *>(V_scratch.data());
    fill_buffer(V_flag,UInt8(0),vertex_count);
    
    mptr<bool> E_visitedQ = reinterpret_cast<bool *>(E_scratch.data());
    fill_buffer(E_visitedQ,false,edges.Size());

    // In the worst case (2 vertices, n edges), we push a stack_node for every edge before we ever can pop a node.
    // Note: We could mark every newly discovered vertex w as "discovered, not visited"  and push only {v,e,w,d} with undiscovered vertex w.
    // The problem with this approach is that we have to do the marking in the order of discovery, but the pushing to stack in reverse order. That would take roughly twice as long. In constrast, memory is cheap nowadays. And we use a stack, so we interesting stuff will stay in hot memory. So we can afford allocating a potentially oversized stack in favor of faster execution.
    Stack<DirectedEdge,EInt> stack ( EdgeCount() );

    auto process = [V_flag,E_visitedQ,E_V,&stack,&discover,&rediscover](
        const DirectedEdge & de, const EInt e
    )
    {
        if constexpr ( dir == InOut::Undirected )
        {
            // We never walk back the same edge. (Not needed if traversal is directed.
            if( e == (de.e ^ EInt(1)) )
            {
                return;
            }
        }
            
        const EInt e_ud = (e >> 1);
        const VInt w = E_V[e];
        
        if( V_flag[w] == UInt8(0) )
        {
            if( E_visitedQ[e_ud] )
            {
                eprint("!!!");
            }
            E_visitedQ[e_ud] = true;
            DirectedEdge de_next {de.w,e,w};
            discover( de_next );
            stack.Push( std::move(de_next) );
        }
        else
        {
            // We need this check here to prevent loop edge being traversed more than once!
            if( !E_visitedQ[e_ud] )
            {
                E_visitedQ[e_ud] = true;
                rediscover({de.w,e,w});
            }
        }
    };

    for( VInt v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
        if( V_flag[v_0] != UInt8(0) )
        {
            continue;
        }
        
        {
            V_flag[v_0] = UInt8(1);
            DirectedEdge de {VInt(-1), EInt(-1), v_0};
            discover( de );
            stack.Push( std::move(de) );
        }

        while( !stack.EmptyQ() )
        {
            DirectedEdge & de = stack.Top();

            const VInt v = de.w;
            
            if( V_flag[v] == UInt8(1) )
            {
                eprint("Undiscovered vertex on stack!");
            }
            
            if( V_flag[v] == UInt8(1) )
            {
                V_flag[v] = UInt8(2);
                pre_visit( de );
                
                // Process outgoing edges first.
                if constexpr ( dir != InOut::In )
                {
                    const EInt k_begin = V_Out_ptr[v          ];
                    const EInt k_end   = V_Out_ptr[v + VInt(1)];
                    
                    for( EInt k = k_end; k --> k_begin; )
                    {
                        const EInt e = ((V_Out_idx[k] << 1) | EInt(Head));
                        process( de, e );
                    }
                }
                
                // Process ingoing edges last.
                if constexpr ( dir != InOut::Out )
                {
                    const EInt k_begin = V_In_ptr[v          ];
                    const EInt k_end   = V_In_ptr[v + VInt(1)];
                    
                    for( EInt k = k_end; k --> k_begin; )
                    {
                        const EInt e = ((V_In_idx[k] << 1) | EInt(Tail));
                        process( de, e );
                    }
                }
            }
            else if( V_flag[v] == UInt8(2) )
            {
                V_flag[v] = UInt8(3);
                post_visit( de );
                (void)stack.Pop();
            }
            else // if( V_flag[v] == UInt8(3) )
            {
                (void)stack.Pop();
            }
        } // while( !stack.EmptyQ() )
        
    } // for( VInt v_0 = 0; v_0 < vertex_count; ++v_0 )
    
    TOOLS_PTOC( ClassName() + "::DepthFirstSearch<" + ToString(dir) + ">");
}

template< InOut dir = 0, class PreVisitVertex_T >
void DepthFirstSearch( PreVisitVertex_T && pre_visit )
{
    this->template DepthFirstSearch<dir>(
        TrivialEdgeFunction,    //discover
        TrivialEdgeFunction,    //rediscover
        pre_visit,              // f( const DirectedEdge & de )
        TrivialEdgeFunction     //postvisit
    );
}

template<InOut dir = InOut::Undirected>
void RequireDFSOrderings()
{
    TOOLS_PTIC(ClassName() + "::RequireDFSOrderings<" + ToString(dir) + ">");
    
    Tensor1<VInt,VInt> V_perm (VertexCount());
    VInt V_ptr = 0;
    
    Tensor1<EInt,VInt> V_parent_E (VertexCount());
    
    Aggregator<VInt,VInt> roots(VInt(1));
    
    this->template DepthFirstSearch<dir>(
        [&V_perm,&V_ptr,&V_parent_E,&roots]( cref<DirectedEdge> de )
        {
            V_perm[V_ptr++]  = de.w;
            V_parent_E[de.w] = de.e;
            
            if( de.e < EInt(0) )
            {
                roots.Push(de.w);
            }
            
        }
    );
    
    this->SetCache(
       "DFSVertexOrdering<" + ToString(dir) + ">", std::move(V_perm)
    );
    this->SetCache(
       "SpanningForestDirectedEdges<" + ToString(dir) + ">", std::move(V_parent_E)
    );
    this->SetCache(
        "SpanningForestRoots<" + ToString(dir) + ">", std::move(roots.Get())
    );
    
    TOOLS_PTOC(ClassName() + "::RequireDFSOrderings<" + ToString(dir) + ">");
}

/*! @brief Returns the list of vertices ordered in the way they are visited by  `DepthFirstSearch`.
 */

template<InOut dir = InOut::Undirected>
Tensor1<VInt,VInt> DFSVertexOrdering()
{
    std::string tag ("DFSVertexOrdering<" + ToString(dir) + ">");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        this->template RequireDFSOrderings<dir>();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<VInt,VInt>>(tag);
}

/*! @brief Returns a spanning forest in the following format: For every vertex `v` the entry `e = SpanningForestDirectedEdges()[v]` is the _oriented_ edge of the spanning tree that points to `v`. If `v` is a root vertex, then `-1` is returned instead.
 */

template<InOut dir = InOut::Undirected>
Tensor1<EInt,VInt> SpanningForestDirectedEdges()
{
    std::string tag ("SpanningForestDirectedEdges<" + ToString(dir) + ">");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        this->template RequireDFSOrderings<dir>();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<EInt,VInt>>(tag);
}

template<InOut dir = InOut::Undirected>
Tensor1<VInt,VInt> SpanningForestRoots()
{
    std::string tag ("SpanningForestRoots<" + ToString(dir) + ">");
    
    TOOLS_PTIC(ClassName() + "::" + tag);
    
    if( !this->InCacheQ( tag ) )
    {
        this->template RequireDFSOrderings<dir>();
    }
    
    TOOLS_PTOC(ClassName() + "::" + tag);
    
    return this->template GetCache<Tensor1<VInt,VInt>>(tag);
}

// TODO: Fix the bugs in the code below.

////########################################################
////##    DepthFirstSearch (strict)
////########################################################
//
///*!@brief Strict version of `DepthFirstSearch`. The new vertices are discovered on the order of the indices of the edges leading to them. This uses `OrientedIncidenceMatrix`, which is much slower to assemble than `InIncidenceMatrix` and `OutIncidenceMatrix` together. Morever, the signs from `OrientedIncidenceMatrix` have to be loaded and processes. So this overall slower than `DepthFirstSearch`.
// */
//
//template<
//    InOut dir = 0,
//    class DiscoverVertex_T,   // f( const DirectedEdge & de )
//    class RediscoverVertex_T, // f( const DirectedEdge & de )
//    class PreVisitVertex_T,   // f( const DirectedEdge & de )
//    class PostVisitVertex_T   // f( const DirectedEdge & de )
//>
//void StrictDepthFirstSearch(
//    DiscoverVertex_T   && discover,
//    RediscoverVertex_T && rediscover,
//    PreVisitVertex_T   && pre_visit,
//    PostVisitVertex_T  && post_visit
//)
//{
//    if( vertex_count <= VInt(0) )
//    {
//        return;
//    }
//    
//    TOOLS_PTIC( ClassName() + "::StrictDepthFirstSearch<" + ToString(dir) + ">");
//    
//    const EInt * E_V = edges.data();
//    
//    const EInt * V_E_ptr;
//    const EInt * V_E_idx;
//    const SInt * V_E_sign;
//    
//    if constexpr( dir == InOut::Undirected )
//    {
//        auto & A = OrientedIncidenceMatrix();
//        V_E_ptr  = A.Outer().data();
//        V_E_idx  = A.Inner().data();
//        V_E_sign = A.Values().data();
//    }
//    else if constexpr( dir == InOut::Out )
//    {
//        auto & A = OutIncidenceMatrix();
//        V_E_ptr  = A.Outer().data();
//        V_E_idx  = A.Inner().data();
//        V_E_sign = nullptr;
//    }
//    else if constexpr( dir == InOut::In )
//    {
//        auto & A = InIncidenceMatrix();
//        V_E_ptr  = A.Outer().data();
//        V_E_idx  = A.Inner().data();
//        V_E_sign = nullptr;
//    }
//    
//    mptr<bool> V_visitedQ = reinterpret_cast<bool *>(V_scratch.data());
//    fill_buffer(V_visitedQ,false,vertex_count);
//
//    // In the worst case (2 vertices, n edges), we push a stack_node for every edge before we ever can pop a node.
//    // Note: We could mark every newly discovered vertex w as "discovered, not visited"  and push only {v,e,w,d} with undiscovered vertex w.
//    // The problem with this approach is that we have to do the marking in the order of discovery, but the pushing to stack in reverse order. That would take roughly twice as long. In constrast, memory is cheap nowadays. And we use a stack, so we interesting stuff will stay in hot memory. So we can afford allocating a potentially oversized stack in favor of faster execution.
//    Stack<DirectedEdge,EInt> stack ( EdgeCount() );
//    
//    // `process(de,e)` does the discovery given `de` pointing to the currently visited vertex and the directed edge index `e` of the next edge to discover.
//    auto process = [V_visitedQ,E_V,&stack,&discover,&rediscover](
//        const DirectedEdge & de, const EInt e
//    )
//    {
//        if constexpr ( dir == InOut::Undirected )
//        {
//            // We never walk back the same edge. (Not needed if traversal is directed.
//            if( e == (de.e ^ EInt(1)) )
//            {
//                return;
//            }
//        }
//            
//        const VInt w = E_V[e];  // the (re)discovered vertex
//        
//        if( !V_visitedQ[w] )
//        {
//            DirectedEdge de_next {de.w,e,w};
//            discover( de_next );
//            stack.Push( std::move(de_next) );
//        }
//        else
//        {
//            rediscover({de.w,e,w});
//        }
//    };
//
//    for( VInt v_0 = 0; v_0 < vertex_count; ++v_0 )
//    {
//        if( !V_visitedQ[v_0] )
//        {
//            {
//                DirectedEdge de {VInt(-1), EInt(-1), v_0};
//                discover( de );
//                stack.Push( std::move(de) );
//            }
//         
//            while( !stack.EmptyQ() )
//            {
//                DirectedEdge & de = stack.Top();
//
//                const VInt v = de.w;
//                
//                if( !V_visitedQ[v] )
//                {
//                    V_visitedQ[v] = true;
//                    pre_visit( de );
//                    
//                    const EInt k_begin = V_E_ptr[v          ];
//                    const EInt k_end   = V_E_ptr[v + VInt(1)];
//                    
//                    for( EInt k = k_end; k --> k_begin; )
//                    {
//                        if constexpr ( dir == InOut::Undirected )
//                        {
//                            const bool d = V_E_sign[k] >= 0;
//                            const EInt e = ((V_E_idx[k] << 1) | EInt(d));
//                            process(de,e);
//                        }
//                        else if constexpr ( dir == InOut::Out )
//                        {
//                            const EInt e = ((V_E_idx[k] << 1) | EInt(Head));
//                            process(de,e);
//                        }
//                        else if constexpr ( dir == InOut::In )
//                        {
//                            const EInt e = ((V_E_idx[k] << 1) | EInt(Tail));
//                            process(de,e);
//                        }
//                        else
//                        {
//                            static_assert(Tools::DependentFalse<PreVisitVertex_T>,"");
//                        }
//                    } // for( EInt k = k_end; k --> k_begin; )
//                }
//                else
//                {
//                    post_visit( de );
//                    (void)stack.Pop();
//                }
//                
//            } // while( !stack.EmptyQ() )
//        }
//    } // for( VInt v_0 = 0; v_0 < vertex_count; ++v_0 )
//    
//    TOOLS_PTOC( ClassName() + "::StrictDepthFirstSearch<" + ToString(dir) + ">");
//}
//
//
//template< InOut dir = 0, class PreVisitVertex_T >
//void StrictDepthFirstSearch( PreVisitVertex_T && pre_visit )
//{
//    this->template StrictDepthFirstSearch<dir>(
//        TrivialEdgeFunction,    //discover
//        TrivialEdgeFunction,    //rediscover
//        pre_visit,              // f( const DirectedEdge & de )
//        TrivialEdgeFunction     //postvisit
//    );
//}
//
//template<InOut dir = InOut::Undirected>
//Tensor1<VInt,VInt> StrictDepthFirstSearchOrdering()
//{
//    TOOLS_PTIC(ClassName() + "::StrictDepthFirstSearchOrdering<" + ToString(dir) + ">");
//    
//    Tensor1<VInt,VInt> p (VertexCount());
//    VInt p_ptr = 0;
//    
//    this->template StrictDepthFirstSearch<dir>(
//        [&p_ptr,&p]( cref<DirectedEdge> de )
//        {
//            p[p_ptr++] = de.w;
//        }
//    );
//    
//    TOOLS_PTOC(ClassName() + "::StrictDepthFirstSearchOrdering<" + ToString(dir) + ">");
//    
//    return p;
//}

