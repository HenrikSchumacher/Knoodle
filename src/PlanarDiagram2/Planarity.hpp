public:

bool PlanarGraphQ() const
{
    TOOLS_PTIMER(timer,MethodName("PlanarGraphQ"));
    
    using Graph_T = boost::adjacency_list<
        boost::vecS,
        boost::vecS,
        boost::undirectedS,
        boost::property<boost::vertex_index_t,Int>
    >;
    
    const Size_T n = ToSize_T(Crossings().Dim(0));
    const Size_T m = ToSize_T(Arcs().Dim(0));
    
    Graph_T G(n);
    
    for( Size_T a = 0; a < m; ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        
        add_edge( A_cross(a,Tail), A_cross(a,Head), G ) ;
    }
    
    bool result = boyer_myrvold_planarity_test(G);
    
    return result;
}
