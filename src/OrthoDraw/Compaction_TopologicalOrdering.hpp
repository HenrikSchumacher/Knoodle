void ComputeVertexCoordinates_TopologicalOrdering()
{
    Tensor1<Int,Int> x;
    Tensor1<Int,Int> y;
    
    ParallelDo(
        [&x,&y,this](const Int thread)
        {
            if( thread == Int(0) )
            {
                x = Dv().TopologicalOrdering();
            }
            else if( thread == Int(1) )
            {
                y = Dh().TopologicalOrdering();
            }
        },
        Int(2),
        (settings.parallelizeQ ? Int(2) : (Int(1)))
    );
    
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
