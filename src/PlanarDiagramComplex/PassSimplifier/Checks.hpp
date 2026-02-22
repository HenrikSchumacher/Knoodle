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
        

template<bool trivial_strand_warningQ = true, bool check_strand_arc_countQ = false>
bool CheckStrand( const Int a, const Int b, const bool overQ_ ) const
{
    logprint(MethodName("CheckStrand") +" (" + ToString(a) + "," + ToString(b) + ")");
    
    if( pd == nullptr )
    {
        eprint(MethodName("CheckStrand") + ": No diagram loaded.");
        return false;
    }
    
    if( a == b )
    {
        if constexpr ( trivial_strand_warningQ )
        {
            wprint(MethodName("CheckStrand") + ": Strand is trivial: a == b.");
        }
        return true;
    }
    
    auto arc_passesQ = [this,overQ_]( const Int e, const bool headtail )
    {
        if( !ArcActiveQ(e) )
        {
            eprint(MethodName("CheckStrand") + ": " + ArcString(e) + " is inactive.");
            return false;
        }
        
        if( pd->ArcOverQ(e,headtail) != overQ_ )
        {
            eprint(MethodName("CheckStrand") + ": Current strand is supposed to be an " + OverQString(overQ_) + "strand. " + ArcString(e) + " does not go " + OverQString(overQ_) + " its " + (headtail ? "head" : "tail" )+ ".");
            return false;
        }
        return true;
    };
    
    const Int pd_max_arc_count = pd->MaxArcCount();
    
    bool passedQ = true;
    Int e = a;
    passedQ = passedQ && arc_passesQ(e,Head);
    
    e = pd->NextArc(a,Head);
    Int arc_counter = 1;
    
    while( (e != b) && (arc_counter < pd_max_arc_count ) )
    {
        passedQ = passedQ && arc_passesQ(e,Tail);
        passedQ = passedQ && arc_passesQ(e,Head);
        
        // We use the safe implementation of NextArc here.
        e = pd->NextArc(e,Head);
        ++arc_counter;
    }
    
    passedQ = passedQ && arc_passesQ(b,Tail);
    ++arc_counter;
    
    if( e != b )
    {
        eprint(MethodName("CheckStrand") + ": After traversing strand for MaxArcCount() =  " + ToString(pd_max_arc_count) + " steps the end is still not reached.");
        
        passedQ = false;
    }
    
    if constexpr ( check_strand_arc_countQ )
    {
        if( arc_counter != strand_arc_count )
        {
            eprint(MethodName("CheckStrand") + ": Counted " + ToString(arc_counter)+ " arcs in strand, but strand_arc_count = " + ToString(strand_arc_count) + ".");
            passedQ = false;
        }
    }

    if( passedQ )
    {
//        logprint(MethodName("CheckStrand") + " passed.");
//        
//        if constexpr ( check_strand_arc_countQ )
//        {
//            TOOLS_LOGDUMP(strand_arc_count);
//        }
//        
//        TOOLS_LOGDUMP(arc_counter);
//        logvalprint("strand",ShortArcRangeString(a,b));
        
        return true;
    }
    else
    {
        logprint(MethodName("CheckStrand") + " failed.");
        
        if constexpr ( check_strand_arc_countQ )
        {
            TOOLS_LOGDUMP(strand_arc_count);
        }
        
        TOOLS_LOGDUMP(arc_counter);
        logvalprint("strand",ShortArcRangeString(a,b));
        
        return false;
    }
}


template<bool trivial_strand_warningQ = true>
bool CheckStrand( const Int a, const Int b ) const
{
    return CheckStrand<trivial_strand_warningQ,true>(a,b,overQ);
}

bool CheckForDanglingMarkedArcs( const Int current_arc ) const
{
    bool passedQ = true;
    
    for( Int a = 0; a < pd->max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        if( !ArcMarkedQ(a) ) { continue; }
        if( a == current_arc) { continue; }
        
        if( !CrossingMarkedQ(pd->A_cross(a,Tail)) )
        {
            eprint(MethodName("CheckForDanglingMarkedArcs") + ": Found marked arc = "+ ArcString(a) + " whose tail is not marked.");
            passedQ = false;
        }
        
        if( !CrossingMarkedQ(pd->A_cross(a,Head)) )
        {
            eprint(MethodName("CheckForDanglingMarkedArcs") + ": Found marked arc = "+ ArcString(a) + " whose head is not marked.");
            passedQ = false;
        }
    }
    
    return passedQ;
}
