public:

Int DegenerateEdgeCount()
{
    return degenerate_edge_count;
}

private:
    
void CountDegenerateEdges()
{
    degenerate_edge_count = 0;
    
    Vector2_T x;
    Vector2_T y;
    
    for( Int edge = 0; edge < edge_count; ++edge )
    {
        x = EdgeVector2(edge,0);
        y = EdgeVector2(edge,1);
        
        const Real d2 = SquaredDistance(x,y);
        
        degenerate_edge_count += (d2 <= Real(0));
    }
}
