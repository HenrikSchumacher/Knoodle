public:

template<typename T = Real>
Tensor1<T,Int> LevelsMinHeight( cref<PlanarDiagram<Int>> pd )
{
    std::string tag = ClassName()+"::LevelsMinHeight" + "<" + TypeName<T> + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    Int m = pd.ArcCount();
    
    Tensor1<T,Int> L ( m );
    
    for( Int a = 0; a < m; ++ a )
    {
        if( pd.ArcActiveQ(a) )
        {
            L[a] = pd.ArcOverQ(a,PD_T::Tail);
        }
        else
        {
            L[a] = 0;
        }
    }
    
    return L;
}
