private:

template<bool must_be_activeQ = true, bool silentQ = false>
bool CheckLeftDarc( const Int da )
{
    if constexpr ( lutQ )
    {
        return true;
    }
    
    bool passedQ = true;
    
    auto [a,dir] = PD_T::FromDarc(da);
    
    if( !pd->ArcActiveQ(a) )
    {
        if constexpr( must_be_activeQ )
        {
            eprint(MethodName("CheckLeftDarc")+": Inactive " + ArcString(a) + ".");
            return false;
        }
        else
        {
            return true;
        }
    }
    
    const Int da_l_true = pd->LeftDarc(da);
    const Int da_l      = LeftDarc(da);
    
    if( da_l != da_l_true )
    {
        if constexpr (!silentQ)
        {
            eprint(MethodName("CheckLeftDarc")+": Incorrect at " + ArcString(a) );
            
            auto [a_l_true,dir_l_true] = PD_T::FromDarc(da_l_true);
            auto [a_l     ,dir_l     ] = PD_T::FromDarc(da_l);
            
            logprint("Head is        {" + ToString(a_l)   + "," +  ToString(dir_l)  + "}.");
            logprint("Head should be {" + ToString(a_l_true) + "," +  ToString(dir_l_true) + "}.");
        }
        passedQ = false;
    }
    
    return passedQ;
}

public:

bool CheckLeftDarc()
{
    TOOLS_PTIMER(timer,"CheckLeftDarc");
    
    std::vector<Int> duds;
    
    for( Int a = 0; a < pd->max_crossing_count; ++a )
    {
        if( pd->ArcActiveQ(a) )
        {
            const Int da = ToDarc(a,Head);
            
            if( !CheckLeftDarc<false,true>(da) )
            {
                duds.push_back(da);
            }
            
            const Int db = ToDarc(a,Tail);
            
            if( !CheckLeftDarc<false,true>(db) )
            {
                duds.push_back(db);
            }
        }
    }
    
    if( duds.size() > Size_T(0) )
    {
        std::sort(duds.begin(),duds.end());
        
        TOOLS_LOGDUMP(duds);
        
//                std::sort(touched.begin(),touched.end());
//
//                TOOLS_LOGDUMP(touched);
        
        logprint(MethodName("CheckLeftDarc")+" failed.");
        
        return false;
    }
    else
    {
        return true;
    }
}


private:
        


bool CheckStrand( const Int a, const Int b, const bool overQ_ ) const
{
    if( pd == nullptr )
    {
        eprint(MethodName("CheckStrand") + ": No diagram loaded.");
        return false;
    }
    
    if( a == b )
    {
        wprint(MethodName("CheckStrand") + ": Strand is trivial: a == b.");
        return true;
    }
    
    auto arc_failsQ = [this,overQ_]( const Int e, const bool headtail )
    {
        if( pd->ArcOverQ(e,headtail) != overQ_ )
        {
            eprint(MethodName("CheckStrand") + ": Current strand is supposed to be an " + OverQString(overQ_) + "strand. " + ArcString(e) + " does not go " + OverQString(overQ_) + " its " + (headtail ? "head" : "tail" )+ ".");
            return true;
        }
        return false;
    };
    
    Int e = a;
        
    if( arc_failsQ(e,Head) ) { return false; }
    
    const Int pd_max_arc_count = pd->MaxArcCount();
    Int arc_counter = 0;
    
    e = pd->NextArc(a,Head);

    while( (e != b) && (arc_counter < pd_max_arc_count ) )
    {
        if( arc_failsQ(e,Tail) ) { return false; }
        if( arc_failsQ(e,Head) ) { return false; }
        
        // We use the safe implementation of NextArc here.
        e = pd->NextArc(e,Head);
    }
    
    if( e != b )
    {
        eprint(MethodName("CheckStrand") + ": After traversing strand for MaxArcCount() =  " + ToString(pd_max_arc_count) + " steps the end is still not reached.");
        
        return false;
    }
    
    if( arc_failsQ(b,Tail) ) { return false; }
    
    logprint(MethodName("CheckStrand") + ": Passed.");

    return true;
}


bool CheckStrand( const Int a, const Int b )
{
    return CheckStrand(a,b,overQ);
}
