//###############################################
//##    Some Auxiliaries
//###############################################

public:

struct DedgeNode
{
    VInt tail = -1; // vertex we came from
    EInt de   = -1; // direction is stored in lowest bit.
    VInt head = -1; // vertex we go to
};

constexpr static auto TrivialEdgeFunction = []( cref<DedgeNode> E )
{
    (void)E;
};

constexpr static auto PrintDiscover = []( cref<DedgeNode> E )
{
    auto [e,d] = FromDedge(E.de);
    
    print("Discovering vertex " + ToString(E.head) + " from vertex " + ToString(E.tail) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintRediscover = []( cref<DedgeNode> E )
{
    auto [e,d] = FromDedge(E.de);

    print("Rediscovering vertex " + ToString(E.head) + " from vertex " + ToString(E.tail) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintPreVisit = []( cref<DedgeNode> E )
{
    auto [e,d] = FromDedge(E.de);
    
    print("Pre-visiting vertex " + ToString(E.head) + " from vertex " + ToString(E.tail) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};

constexpr static auto PrintPostVisit = []( cref<DedgeNode> E )
{
    auto [e,d] = FromDedge(E.de);
    
    print("Post-visiting vertex " + ToString(E.head) + " from vertex " + ToString(E.tail) + " along edge " + ToString(e) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};


//###############################################
//##    DepthFirstSearch (fast)
//###############################################

public:

/*!@brief This traverses the graph in a slightly _modified_ depth-first order: In the case `dir == InOut::Undirected` suppose we are at the currently visited vertex `v`. Then new neighboring vertices  `w` are given precedence if they are reached from `v` via an outgoing edge. That means, an edge `e_0` of the form `{v,w_0}` will always be traversed before an edge `e_1` of the form `{w_1,v}`, no matter whether the edge index `e_0` is smaller or greater than `e_1`.  This is to allow the use of `InIncidenceMatrix` and `OutIncidenceMatrix`, which together are faster to assemble than `OrientedIncidenceMatrix`.
 */

template<
    InOut dir = InOut::Undirected,
    class DiscoverVertex_T,   // f( const DedgeNode & E )
    class RediscoverVertex_T, // f( const DedgeNode & E )
    class PreVisitVertex_T,   // f( const DedgeNode & E )
    class PostVisitVertex_T   // f( const DedgeNode & E )
>
void DepthFirstSearch(
    DiscoverVertex_T     && discover,
    RediscoverVertex_T   && rediscover,
    PreVisitVertex_T     && pre_visit,
    PostVisitVertex_T    && post_visit
)
{
    if( vertex_count <= VInt(0) ) { return; }
    
    std::string tag = ClassName()+"::DepthFirstSearch<" + ToString(dir) + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    cptr<EInt> dE_V = edges.data();
    
    const EInt * V_Out_ptr = nullptr;
    const EInt * V_Out_idx = nullptr;
    
    const EInt * V_In_ptr  = nullptr;
    const EInt * V_In_idx  = nullptr;
    
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
    fill_buffer(E_visitedQ,false,edges.Dim(0));

    // In the worst case (2 vertices, n edges), we push a stack_node for every edge before we ever can pop a node.
    // Note: We could mark every newly discovered vertex w as "discovered, not visited"  and push only {v,e,w,d} with undiscovered vertex w.
    // The problem with this approach is that we have to do the marking in the order of discovery, but the pushing to stack in reverse order. That would take roughly twice as long. In constrast, memory is cheap nowadays. And we use a stack, so we interesting stuff will stay in hot memory. So we can afford allocating a potentially oversized stack in favor of faster execution.
    Stack<DedgeNode,EInt> stack ( EdgeCount() );

    auto conditional_push = [V_flag,E_visitedQ,dE_V,&stack,&discover,&rediscover,&tag](
        const DedgeNode & E, const EInt de
    )
    {
        if constexpr ( dir == InOut::Undirected )
        {
            // We never walk back the same edge. (Not needed if traversal is directed.
            // Also, we have to handle the case where E.de = -1.
            if( (E.de >= EInt(0)) && (de == FlipDiEdge(E.de)) )
            {
                return;
            }
        }
        
        // E.de may be virtual, but e may not.
        if( de < EInt(0) )
        {
            eprint(tag + ": Virtual edge on stack.");
            return;
        }
        
        const VInt w = dE_V[de];
        auto [e,d] = FromDedge(de);
        
        if( V_flag[w] <= UInt8(0) )
        {
            V_flag[w] = UInt8(1);
            E_visitedQ[e] = true;
            DedgeNode de_next {E.head,de,w};
//            logprint("discover vertex " + ToString(w) + "; edge = " + ToString(e));
            discover( de_next );
            stack.Push( std::move(de_next) );
        }
        else
        {
            // We need this check here to prevent loop edge being traversed more than once!
            if( !E_visitedQ[e] )
            {
                E_visitedQ[e] = true;
//                logprint("rediscover vertex " + ToString(w) + "; edge = " + ToString(e));
                rediscover({E.head,de,w});
            }
            else
            {
//                logprint("skip rediscover vertex " + ToString(w) + "; edge = " + ToString(e));
            }
        }
    };
    
    for( VInt v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
        if( V_flag[v_0] != UInt8(0) ) { continue; }
        
        {
            V_flag[v_0] = UInt8(1);
            DedgeNode E {UninitializedVertex, UninitializedEdge, v_0};
            discover( E );
//            logprint("discover vertex " + ToString(v_0));
            stack.Push( std::move(E) );
        }

        while( !stack.EmptyQ() )
        {
            DedgeNode & E = stack.Top();
            const VInt v = E.head;
            
            if( V_flag[v] == UInt8(0) )
            {
                eprint(tag + ": Undiscovered vertex on stack!");
                (void)stack.Pop();
            }
            else if( V_flag[v] == UInt8(1) )
            {
                V_flag[v] = UInt8(2);
//                logprint("pre-visit vertex " + ToString(v) + "; edge = " + ((E.de < 0) ? "-1" : ToString(E.de/2)) );
                pre_visit( E );
                // Process outgoing edges first.
                if constexpr ( dir != InOut::In )
                {
                    const EInt k_begin = V_Out_ptr[v          ];
                    const EInt k_end   = V_Out_ptr[v + VInt(1)];
                    
                    for( EInt k = k_end; k --> k_begin; )
                    {
                        const EInt e = ToDedge<Head>(V_Out_idx[k]);
                        conditional_push( E, e );
                    }
                }
                
                // Process ingoing edges last.
                if constexpr ( dir != InOut::Out )
                {
                    const EInt k_begin = V_In_ptr[v          ];
                    const EInt k_end   = V_In_ptr[v + VInt(1)];
                    
                    for( EInt k = k_end; k --> k_begin; )
                    {
                        const EInt e = ToDedge<Tail>(V_In_idx[k]);
                        conditional_push( E, e );
                    }
                }
            }
            else if( V_flag[v] == UInt8(2) )
            {
                V_flag[v] = UInt8(3);
                post_visit( E );
//                logprint("post-visit vertex " + ToString(v) + "; edge = " + ((E.de < 0) ? "-1" : ToString(E.de/2)) );
                (void)stack.Pop();
            }
            else // if( V_flag[v] == UInt8(3) )
            {
//                logprint("pop garbage edge " + ((E.de < 0) ? "-1" : ToString(E.de/2)));
                (void)stack.Pop();
            }
            
        } // while( !stack.EmptyQ() )
        
    } // for( VInt v_0 = 0; v_0 < vertex_count; ++v_0 )
}

template< InOut dir = InOut::Undirected, class PreVisitVertex_T >
void DepthFirstSearch( PreVisitVertex_T && pre_visit )
{
    this->template DepthFirstSearch<dir>(
        TrivialEdgeFunction,    //discover
        TrivialEdgeFunction,    //rediscover
        pre_visit,              // f( const DedgeNode & E )
        TrivialEdgeFunction     //postvisit
    );
}
