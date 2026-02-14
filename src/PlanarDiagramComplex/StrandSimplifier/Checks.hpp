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
        

bool CheckStrand( const Int a_begin, const Int a_end )
{
    bool passedQ = true;
    
    Int arc_counter = 0;
    
    Int a = a_begin;
    
    if( a_begin == a_end )
    {
        wprint(MethodName("CheckStrand")+"<" + (overQ ? "over" : "under" ) + ">: Strand is trivial: a_begin == a_end.");
    }
    
    const Int pd_max_arc_count = pd->MaxArcCount();
    
    while( (a != a_end) && (arc_counter < pd_max_arc_count ) )
    {
        ++arc_counter;
        
        passedQ = passedQ && (pd->ArcOverQ(a,Head) == overQ);
        
        // We use the safe implementation of NextArc here.
        a = pd->NextArc(a,Head);
    }
    
    if( a != a_end )
    {
        pd_eprint(MethodName("CheckStrand")+"<" + (overQ ? "over" : "under" ) + ">: After traversing strand for MaxArcCount() steps the end is still not reached.");
    }
    
    if( !passedQ )
    {
        pd_eprint(MethodName("CheckStrand")+"<" + (overQ ? "over" : "under" ) + ">: Strand is not an" + (overQ ? "over" : "under" ) + "strand.");
    }
    
    return passedQ;
}
