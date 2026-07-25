public:

/*!@brief **UNSAFE.** Append the `PlanarDiagram` `pd` to the internal list, taking ownership. It is in the user's discretion that this does not break the class's invariants, e.g., that the arc colors are consistent.
 *
 * @return `true` if succeeded, `false` otherwise.
 */
bool Push( PD_T && pd )
{
    if( LockedQ() ) { LockMessage("Push"); return false; }
        
    Push_Private(std::move(pd));
    
    return true;
}

private:

void Push_Private( PD_T && pd )
{
    // TODO: Implement color checks.
    // TODO: Check for unlink?
    pd_list.push_back( std::move(pd) );
    ClearCache();
}

public:

/*!@brief **UNSAFE.** Replace the diagram at position `diagram_idx` in the internal list by `PlanarDiagram` `pd`, taking ownership. It is in the user's discretion that this does not break the class's invariants, e.g., that the arc colors are consistent.
 *
 * @return `true` if succeeded, `false` otherwise.
 */
bool Replace( const Int diagram_idx, PD_T && pd )
{
    // TODO: Implement color checks.
    
    if( LockedQ() ) { LockMessage("Replace"); return false; }
    
    if( (diagram_idx < 0) || (diagram_idx >= DiagramCount() ) )
    {
        wprint(MethodName("Replace") + ": Diagram index = " + ToString(diagram_idx) + " is out of bounds. Doing nothing.");
        return false;
    }
        
    pd_list[diagram_idx] = std::move(pd);
    ClearCache();
    return true;
}

/*!@brief **UNSAFE.** Removes the last diagram in the internal list and returns it. It is in the user's discretion that this does not break the class's invariants, e.g., that the arc colors are consistent. */
PD_T Pop()
{
    if( LockedQ() ) { LockMessage("Pop"); return PD_T::InvalidDiagram(); }
    
    if( pd_list.empty() ) { return PD_T::InvalidDiagram(); }

    PD_T pd = std::move(pd_list.back());
    pd_list.pop_back();
    ClearCache();

    return pd;
}

/*!@brief **UNSAFE.** Erases all diagrams from the internal list.
 *
 * @return `true` if succeeded, `false` otherwise.
 */
bool Clear()
{
    if( LockedQ() ) { LockMessage("Clear"); return false; }
    
    pd_list.clear();
    ClearCache();
    return true;
}
