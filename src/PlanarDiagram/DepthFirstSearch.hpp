//########################################################
//##    DepthFirstSearch (fast)
//########################################################

struct DirectedArcNode
{
    Int tail = -1;
    Int a    = -1;
    Int head = -1;
};

constexpr static auto TrivialArcFunction = []( cref<DirectedArcNode> da )
{
    (void)da;
};

constexpr static auto PrintDiscover = []( cref<DirectedArcNode> da )
{
    const Int a = da.a >> 1;
    const bool d = da.a | Int(1);
    
    print("Discovering crossing " + ToString(da.head) + " from crossing " + ToString(da.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintRediscover = []( cref<DirectedArcNode> da )
{
    const Int a = da.a >> 1;
    const bool d = da.a | Int(1);

    print("Rediscovering crossing " + ToString(da.head) + " from crossing " + ToString(da.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintPreVisit = []( cref<DirectedArcNode> da )
{
    const Int a = da.a >> 1;
    const bool d = da.a | Int(1);
    
    print("Pre-visiting crossing " + ToString(da.head) + " from crossing " + ToString(da.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};

constexpr static auto PrintPostVisit = []( cref<DirectedArcNode> da )
{
    const Int a = da.a >> 1;
    const bool d = da.a | Int(1);
    
    print("Post-visiting crossing " + ToString(da.head) + " from crossing " + ToString(da.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};

public:


/*!@brief This traverses the graph in a slightly _modified_ depth-first order: Suppose we are at the currently visited crossing `c`. Then the new crossings through arcs: `Crossings(c,Out,Right)`,`Crossings(c,Out,left)`,`Crossings(c,In,Left)`,`Crossings(c,In,Right)`. That is, we start with the right outgoing arc and then go counter-clockwise around the crossing `c`.
 */

template<
    class Discover_T,   // f( const DirectedArcNode & da )
    class Rediscover_T, // f( const DirectedArcNode & da )
    class PreVisit_T,   // f( const DirectedArcNode & da )
    class PostVisit_T   // f( const DirectedArcNode & da )
>
void DepthFirstSearch(
    Discover_T   && discover,
    Rediscover_T && rediscover,
    PreVisit_T   && pre_visit,
    PostVisit_T  && post_visit
)
{
    if( crossing_count <= Int(0) )
    {
        return;
    }
    
    TOOLS_PTIC( ClassName() + "::DepthFirstSearch");
    
    mptr<UInt8> C_flag = reinterpret_cast<UInt8 *>(C_scratch.data());
    fill_buffer(C_flag,UInt8(0),crossing_count);
    
    mptr<bool> A_visitedQ = reinterpret_cast<bool *>(A_scratch.data());
    fill_buffer(A_visitedQ,false,arc_count);

    cptr<Int> A_C = A_cross.data();

    Stack<DirectedArcNode,Int> stack ( arc_count );
    
    TOOLS_LOGDUMP(stack.Capacity());

    auto fun = [A_visitedQ,C_flag,A_C,&stack,&discover,&rediscover](
        const DirectedArcNode & da, const Int b
    )
    {
        if( b == (da.a ^ Int(1)) )
        {
            return;
        }
        
        const Int head = A_C[b];
        
        const Int b_ud = (b >> 1);
        
        if( C_flag[head] == UInt8(0) )
        {
            logprint("--->discover " + ToString(b));
            C_flag[head] = UInt8(1);
            A_visitedQ[b_ud] = true;
            DirectedArcNode db {da.head,b,head};
            discover( db );
//            TOOLS_LOGDUMP("push vertex " + ToString(head));
            stack.Push( std::move(db) );
        }
        else
        {
            // We need this check here to prevent loop edge being traversed more than once!
            if( !A_visitedQ[b_ud] )
            {
//                logprint("rediscover " + ToString(b_ud));
                A_visitedQ[b_ud] = true;
                rediscover({da.head,b,head});
            }
            else
            {
//                logprint("skip rediscover " + ToString(b_ud));
            }
        }
    };

    for( Int c_0 = 0; c_0 < crossing_count; ++c_0 )
    {
        if( !CrossingActiveQ(c_0) || (C_flag[c_0] != UInt8(0)) )
        {
            continue;
        }
        
        {
            C_flag[c_0] = UInt8(1);
            DirectedArcNode da {Int(-1), Int(-1), c_0};
//            logprint("discover " + ToString(-1));
            discover( da );
//            TOOLS_LOGDUMP("push vertex " + ToString(c_0));
            stack.Push( std::move(da) );
        }
        
        while( !stack.EmptyQ() )
        {
            DirectedArcNode & da = stack.Top();
//            TOOLS_LOGDUMP(stack.Size());
            
//            TOOLS_LOGDUMP("top vertex " + ToString(da.head));

            const Int c = da.head;
            
//            if( C_flag[c] == UInt8(0) )
//            {
//                eprint("Undiscovered crossing " + ToString(c) + " on stack!");
//            }
            
            if( C_flag[c] == UInt8(1) )
            {
                C_flag[c] = UInt8(2);
                pre_visit( da );
//                logprint("pre_visit vertex " + ToString(da.head));
                // We process them here in reverse order so that they appear in correct order on the stack.
                fun( da, Int(2) * C_arcs(c,In ,Right) + Int(0) );
                fun( da, Int(2) * C_arcs(c,In ,Left ) + Int(0) );
                fun( da, Int(2) * C_arcs(c,Out,Left ) + Int(1) );
                fun( da, Int(2) * C_arcs(c,Out,Right) + Int(1) );
            }
            else if( C_flag[c] == UInt8(2) )
            {
                post_visit( da );
                (void)stack.Pop();
            }
            else
            {
                (void)stack.Pop();
            }
            
        } // while( !stack.EmptyQ() )
        
    } // for( Int c_0 = 0; c_0 < crossing_count; ++c_0 )
    
    TOOLS_PTOC( ClassName() + "::DepthFirstSearch");
}

template<class PreVisitVertex_T>
void DepthFirstSearch( PreVisitVertex_T && pre_visit )
{
    DepthFirstSearch(
        TrivialArcFunction,    //discover
        TrivialArcFunction,    //rediscover
        pre_visit,             // f( const DirectedArcNode & da )
        TrivialArcFunction     //postvisit
    );
}
