private:
    
void CheckInputs() const
{
    std::string tag (MethodName("CheckInputs"));
    TOOLS_PTIMER(timer,tag);
    
    const EInt edge_count = edges.Dim(0);
    
    for( EInt e = 0; e < edge_count; ++e )
    {
        const VInt e_0 = edges(e,Tail);
        const VInt e_1 = edges(e,Head);


        if( e_0 < VInt(0) )
        {
            eprint(tag + ":  first entry " + ToString(e_0) + " of edge " + ToString(e) + " is negative.");

            return;
        }
        
        if( e_0 >= vertex_count )
        {
            eprint(tag + ":  first entry " + ToString(e_0) + " of edge " + ToString(e) + " is greater or equal to vertex_count = " + ToString(vertex_count) + ".");

            return;
        }

        if( e_1 < VInt(0) )
        {
            eprint(tag + ": second entry " + ToString(e_1) + " of edge " + ToString(e) + " is negative.");
            return;
        }
        
        if( e_1 >= vertex_count )
        {
            eprint(tag + ": second entry " + ToString(e_1) + " of edge " + ToString(e) + " is greater or equal to vertex_count = " + ToString(vertex_count) + ".");

            return;
        }
    }
}
