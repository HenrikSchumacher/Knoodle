public:

bool CheckAll()
{
    auto tag = [](){ return MethodName("CheckAll"); };
    
    bool passedQ = CheckValid();
    
    for( PD_T & pd : pd_list )
    {
        passedQ = passedQ && pd.CheckAll();
    }
    
    if( passedQ )
    {
        logprint(tag()+": passed.");
    }
    else
    {
        eprint(tag()+": failed.");
    }
    
    return passedQ;
}

bool CheckProvenMinimalQ()
{
    bool succeeded = true;
    
    for( PD_T & pd : pd_list )
    {
        succeeded = succeeded && pd.CheckProvenMinimalQ();
    }
    
    return succeeded;
}


bool CheckValid()
{
    auto tag = [](){ return MethodName("CheckValid"); };
    
    bool passedQ = true;
    
    for( PD_T & pd : pd_list )
    {
        if( !pd.ValidQ() )
        {
            eprint(tag()+": Found and invalid diagram in pd_list.");
            passedQ = false;
            break;
        }
    }
    
    for( PD_T & pd : pd_todo )
    {
        if( !pd.ValidQ() )
        {
            eprint(tag()+": Found and invalid diagram in pd_todo.");
            passedQ = false;
            break;
        }
    }
    
    for( PD_T & pd : pd_done )
    {
        if( !pd.ValidQ() )
        {
            eprint(tag()+": Found and invalid diagram in pd_done.");
            passedQ = false;
            break;
        }
    }
    
    return passedQ;
}
