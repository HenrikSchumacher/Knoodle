private:

bool CheckStrand( const Int a_begin, const Int a_end, const Int max_arc_count )
{
    bool passedQ = true;
    
    Int arc_counter = 0;
    
    Int a = a_begin;
    
    if( a_begin == a_end )
    {
        wprint(ClassName()+"::CheckStrand<" + (overQ ? "over" : "under" ) + ">: Strand is trivial: a_begin == a_end.");
    }
    
    while( (a != a_end) && (arc_counter < max_arc_count ) )
    {
        ++arc_counter;
        
        passedQ = passedQ && (pd.ArcOverQ(a,Head) == overQ);
        
        a = NextArc(a,Head);
    }
    
    if( a != a_end )
    {
        pd_eprint(ClassName()+"::CheckStrand<" + (overQ ? "over" : "under" ) + ">: After traversing strand for max_arc_count steps the end ist still not reached.");
    }
    
    if( !passedQ )
    {
        pd_eprint(ClassName()+"::CheckStrand<" + (overQ ? "over" : "under" ) + ">: Strand is not an" + (overQ ? "over" : "under" ) + "strand.");
    }
    
    return passedQ;
}

template<bool must_be_activeQ = true, bool silentQ = false>
bool CheckDarcLeftDarc( const Int da )
{
    bool passedQ = true;
    
    auto [a,dir] = PD_T::FromDarc(da);
    
    if( !pd.ArcActiveQ(a) )
    {
        if constexpr( must_be_activeQ )
        {
            eprint(MethodName("CheckDarcLeftDarc")+": Inactive " + ArcString(a) + ".");
            return false;
        }
        else
        {
            return true;
        }
    }
    
    const Int da_l_true = pd.LeftDarc(da);
    const Int da_l      = dA_left[da];
    
    if( da_l != da_l_true )
    {
        if constexpr (!silentQ)
        {
            eprint(MethodName("CheckDarcLeftDarc")+": Incorrect at " + ArcString(a) );
            
            auto [a_l_true,dir_l_true] = PD_T::FromDarc(da_l_true);
            auto [a_l     ,dir_l     ] = PD_T::FromDarc(da_l);
            
            logprint("dA_left[da] is        {" + ToString(a_l)   + "," +  ToString(dir_l)  + "}.");
            logprint("dA_left[da] should be {" + ToString(a_l_true) + "," +  ToString(dir_l_true) + "}.");
        }
        passedQ = false;
    }
    
    return passedQ;
}

public:

bool CheckDarcLeftDarc()
{
    duds.clear();
    
    const Int m = A_cross.Dim(0);
    
    
    if( !pd.CheckArcStates() )
    {
        eprint(MethodName("CheckDarcLeftDarc")+": failed.");
        return false;
    }
    
    for( Int a = 0; a < m; ++a )
    {
        if( pd.ArcActiveQ(a) )
        {
            const Int da = PD_T::ToDarc(a,Head);
            
            if( !CheckDarcLeftDarc<false,false>(da) )
            {
                duds.push_back(da);
            }
            
            const Int db = PD_T::ToDarc(a,Tail);
            
            if( !CheckDarcLeftDarc<false,false>(db) )
            {
                duds.push_back(db);
            }
        }
    }
    
    if( duds.size() > Size_T(0) )
    {
        std::sort(duds.begin(),duds.end());
        
        TOOLS_LOGDUMP(duds);
        
        std::sort(touched.begin(),touched.end());
        
        TOOLS_LOGDUMP(touched);
        
        logprint(MethodName("CheckDarcLeftDarc")+" failed.");
        
        return false;
    }
    else
    {
        
        return true;
    }
}
