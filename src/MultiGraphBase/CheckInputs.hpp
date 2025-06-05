private:
    
void CheckInputs() const
{
    TOOLS_PTIMER( timer, ClassName() + "::CheckInputs" );
    
    const EInt edge_count = edges.Dimension(0);
    
    for( EInt e = 0; e < edge_count; ++e )
    {
        const VInt e_0 = edges(e,Tail);
        const VInt e_1 = edges(e,Head);


        if( e_0 < VInt(0) )
        {
            eprint("MultiGraphBase:  first entry " + ToString(e_0) + " of edge " + ToString(e) + " is negative.");

            return;
        }
        
        if( e_0 >= vertex_count )
        {
            eprint("MultiGraphBase:  first entry " + ToString(e_0) + " of edge " + ToString(e) + " is greater or equal to vertex_count = " + ToString(vertex_count) + ".");

            return;
        }

        if( e_1 < VInt(0) )
        {
            eprint("MultiGraphBase: second entry " + ToString(e_1) + " of edge " + ToString(e) + " is negative.");
            return;
        }
        
        if( e_1 >= vertex_count )
        {
            eprint("MultiGraphBase: second entry " + ToString(e_1) + " of edge " + ToString(e) + " is greater or equal to vertex_count = " + ToString(vertex_count) + ".");

            return;
        }
    }
}
