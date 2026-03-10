void ComputeVertexCoordinates_TopologicalOrdering()
{
    Tensor1<Int,Int> x = Dv().TopologicalOrdering();
    Tensor1<Int,Int> y = Dh().TopologicalOrdering();
    
    if( x.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_TopologicalOrdering") + ": Graph Dv() is cyclic.");
    }
    
    if( y.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_TopologicalOrdering") + ": Graph Dh() is cyclic.");
    }

    ComputeVertexCoordinates(x,y);
}
