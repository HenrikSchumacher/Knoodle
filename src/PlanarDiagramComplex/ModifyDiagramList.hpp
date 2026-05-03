public:

void Push( PD_T && pd )
{
    // TODO: Implement color checks.
    // TODO: Check for unlink?
        
    pd_list.push_back( std::move(pd) );
    ClearCache();
}

void Replace( const Int diagram_idx, PD_T && pd )
{
    // TODO: Implement color checks.
    
    if( (diagram_idx < 0) || (diagram_idx >= DiagramCount() ) )
    {
        wprint(MethodName("Replace") + ": Diagram index = " + ToString(diagram_idx) + " is out of bounds. Doing nothing.");
        return;
    }
        
    pd_list[diagram_idx] = std::move(pd);
    ClearCache();
}

//void Pop()
//{
//    pd_list.pop_back();
//    ClearCache();
//}

void Clear()
{
    pd_list.clear();
    ClearCache();
}
