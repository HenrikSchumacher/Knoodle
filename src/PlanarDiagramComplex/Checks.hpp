public:

bool CheckAll()
{
    bool succeeded = true;
    
    for( PD_T & pd : pd_list )
    {
        succeeded = succeeded && pd.CheckAll();
    }
    
    return succeeded;
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
