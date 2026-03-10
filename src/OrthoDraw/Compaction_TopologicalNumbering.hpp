void ComputeVertexCoordinates_TopologicalNumbering()
{
    Tensor1<Int,Int> x = Dv().TopologicalNumbering();
    Tensor1<Int,Int> y = Dh().TopologicalNumbering();
    
    if( x.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_TopologicalNumbering") + ": Graph Dv() is cyclic.");
    }
    
    if( y.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_TopologicalNumbering") + ": Graph Dh() is cyclic.");
    }
    
    ComputeVertexCoordinates(x,y);
}
