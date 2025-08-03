public:

template<typename T = Real>
Tensor1<T,Int> LevelsByHeight( cref<PlanarDiagram<Int>> pd )
{
    std::string tag = ClassName()+"::LevelsByHeight" + "<" + TypeName<T> + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    Int m = pd.ArcCount();
    
    Tensor1<T,Int> L ( m );
    
    for( Int a = 0; a < m; ++ a )
    {
        if( pd.ArcActiveQ(a) )
        {
            L[a] = pd.template ArcOverQ<PD_T::Tail>(a);
        }
        else
        {
            L[a] = 0;
        }
    }
    
    return L;
}
