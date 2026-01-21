public:

void Push( PD_T && pd )
{
    // TODO: Implement color checks.
        
    pd_list.push_back( std::move(pd) );
    ClearCache();
}

void Replace( const Int diagram_idx, PD_T && pd )
{
    // TODO: Implement color checks.
    
    if( (diagram_idx < 0) || (diagram_idx >= DiagramCount() ) )
    {
        wprint(MethodName("Replace") + ": Diagram index a = " + ToString(diagram_idx) + " is out of bounds. Doing nothing.");
        return;
    }
        
    pd_list[diagram_idx] = std::move(pd);
    ClearCache();
}
