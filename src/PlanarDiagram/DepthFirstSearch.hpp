//###############################################
//##    Some Auxiliaries
//###############################################

struct DarcNode
{
    Int tail = Uninitialized;
    Int da   = Uninitialized;
    Int head = Uninitialized;
};

constexpr static auto TrivialArcFunction = []( cref<DarcNode> A )
{
    (void)A;
};

constexpr static auto PrintDiscover = []( cref<DarcNode> A )
{
    auto [a,d] = FromDarc(A.da);
    
    print("Discovering crossing " + ToString(A.head) + " from crossing " + ToString(A.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintRediscover = []( cref<DarcNode> A )
{
    auto [a,d] = FromDarc(A.da);

    print("Rediscovering crossing " + ToString(A.head) + " from crossing " + ToString(A.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};
constexpr static auto PrintPreVisit = []( cref<DarcNode> A )
{
    auto [a,d] = FromDarc(A.da);
    
    print("Pre-visiting crossing " + ToString(A.head) + " from crossing " + ToString(A.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};

constexpr static auto PrintPostVisit = []( cref<DarcNode> A )
{
    auto [a,d] = FromDarc(A.da);
    
    print("Post-visiting crossing " + ToString(A.head) + " from crossing " + ToString(A.tail) + " along arc " + ToString(a) + " in " + (d == Head ? "forward" : "backward" ) + " direction." );
};

//###############################################
//##    DepthFirstSearch
//###############################################

public:


/*!@brief This traverses the graph in a slightly _modified_ depth-first order: Suppose we are at the currently visited crossing `c`. Then the new crossings through arcs: `Crossings(c,Out,Right)`,`Crossings(c,Out,left)`,`Crossings(c,In,Left)`,`Crossings(c,In,Right)`. That is, we start with the right outgoing arc and then go counter-clockwise around the crossing `c`.
 */

template<
    class Discover_T,   // f( const DarcNode & da )
    class Rediscover_T, // f( const DarcNode & da )
    class PreVisit_T,   // f( const DarcNode & da )
    class PostVisit_T,   // f( const DarcNode & da )
    bool verboseQ = false
>
void DepthFirstSearch(
    Discover_T   && discover,
    Rediscover_T && rediscover,
    PreVisit_T   && pre_visit,
    PostVisit_T  && post_visit
)
{
    if( crossing_count <= Int(0) ) { return; }
    
    TOOLS_PTIMER(timer,MethodName("DepthFirstSearch"));
    
    cptr<Int> dA_C = A_cross.data();
    
    mptr<UInt8> C_flag = reinterpret_cast<UInt8 *>(C_scratch.data());
    fill_buffer(C_flag,UInt8(0),crossing_count);
    
    TOOLS_DUMP(crossing_count);
    
    mptr<bool> A_visitedQ = reinterpret_cast<bool *>(A_scratch.data());
    fill_buffer(A_visitedQ,false,arc_count);

    Stack<DarcNode,Int> stack ( arc_count );

    auto conditional_push = [A_visitedQ,C_flag,dA_C,&stack,&discover,&rediscover,this](
        const DarcNode & A, const Int db
    )
    {
        // We never walk back the same arc.
        if( this->ValidIndexQ(A.da) && (db == FlipDarc(A.da)) )
        {
            return;
        }
        
        // da.a may be virtual, but b may not.
        if( !this->ValidIndexQ(db) )
        {
            eprint(ClassName()+"::DepthFirstSearch: Virtual arc on stack.");
            return;
        }
        
        auto [b,dir] = this->FromDarc(db);
        const Int head = dA_C[db];

        if( C_flag[head] <= UInt8(0) )
        {
            C_flag[head] = UInt8(1);
            A_visitedQ[b] = true;
            DarcNode B {A.head,db,head};
            if constexpr ( verboseQ )
            {
                logprint("discover crossing " + ToString(head) + ";  darc = " + ToString(db)
                );
            }
            discover( B );
            stack.Push( std::move(B) );
        }
        else
        {
            // We need this check here to prevent loop arc being traversed more than once!
            if( !A_visitedQ[b] )
            {
                A_visitedQ[b] = true;
                if constexpr ( verboseQ )
                {
                    logprint("rediscover crossing " + ToString(head) + ";  darc = " + ToString(db)
                    );
                }
                rediscover({A.head,db,head});
            }
            else
            {
                if constexpr ( verboseQ )
                {
                    logprint("skip rediscover vertex " + ToString(head) + ";  darc = " + ToString(db)
                    );
                }
            }
        }
    };

    const Int c_count = C_arcs.Dim(0);
    
    TOOLS_LOGDUMP(c_count);
    TOOLS_LOGDUMP(crossing_count);
    
    for( Int c_0 = 0; c_0 < c_count; ++c_0 )
    {
        TOOLS_LOGDUMP(c_0);
        TOOLS_LOGDUMP(CrossingActiveQ(c_0));
        TOOLS_LOGDUMP(C_flag[c_0]);
        
        if( !CrossingActiveQ(c_0) || (C_flag[c_0] != UInt8(0)) )
        {
            continue;
        }
        
        {
            C_flag[c_0] = UInt8(1);
            DarcNode A {Uninitialized, Uninitialized, c_0};
            discover( A );
            if constexpr ( verboseQ )
            {
                logprint("discover crossing " + ToString(c_0));
            }
            stack.Push( std::move(A) );
        }
        
        while( !stack.EmptyQ() )
        {
            DarcNode & A = stack.Top();
            const Int c = A.head;
            
            if( C_flag[c] == UInt8(0) )
            {
                if constexpr ( verboseQ )
                {
                    eprint("Undiscovered crossing " + ToString(c) + " on stack!");
                }
                (void)stack.Pop();
            }
            else if( C_flag[c] == UInt8(1) )
            {
                
                C_flag[c] = UInt8(2);
                if constexpr ( verboseQ )
                {
                    logprint("pre-visit crossing " + ToString(c) + "; darc = " + ToString(A.da) );
                }
                pre_visit( A );

                // We process the arcs in reverse order so that they appear in correct order on the stack.
                
                conditional_push( A, ToDarc(C_arcs(c,In ,Right),Tail) );
                conditional_push( A, ToDarc(C_arcs(c,In ,Left ),Tail) );
                conditional_push( A, ToDarc(C_arcs(c,Out,Left ),Head) );
                conditional_push( A, ToDarc(C_arcs(c,Out,Right),Head) );
            }
            else if( C_flag[c] == UInt8(2) )
            {
                C_flag[c] = UInt8(3);
                if constexpr ( verboseQ )
                {
                    logprint("post-visit crossing " + ToString(c) + ";  darc = " + ToString(A.da) );
                }
                post_visit( A );
                (void)stack.Pop();
            }
            else // if( C_flag[c] == UInt8(3) )
            {
                if constexpr ( verboseQ )
                {
                    logprint("popping stack");
                }
                (void)stack.Pop();
            }

        } // while( !stack.EmptyQ() )
        
    } // for( Int c_0 = 0; c_0 < crossing_count; ++c_0 )
}

template<class PreVisitVertex_T>
void DepthFirstSearch( PreVisitVertex_T && pre_visit )
{
    DepthFirstSearch(
        TrivialArcFunction,    //discover
        TrivialArcFunction,    //rediscover
        pre_visit,             // f( const DarcNode & da )
        TrivialArcFunction     //postvisit
    );
}
