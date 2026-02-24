public:

struct Path_T
{
    // The first and last arc in a path_container are the start and end arcs.
    // All the other arcs path_container stand for arcs we need to cross.
    Tensor1<Int,Int> container;
    Int size = 0;
    
    Path_T() = default;
    ~Path_T() = default;
    
    Path_T( Int max_arc_count )
    : container { max_arc_count }
    {}
    
    mref<Int> operator[]( const Int i )
    {
        return container[i];
    }
    
    cref<Int> operator[]( const Int i ) const
    {
        return container[i];
    }
    
    Int Size() const
    {
        return size;
    }
    
    Int Capacity() const
    {
        return container.Size();
    }
    
    void Resize( const Int new_size )
    {
        if( new_size > Capacity() )
        {
            container.template Resize<false> ( new_size );
        }
        size = new_size;
    }
    
    Int CrossingCount() const
    {
        return (Size() >= Int(2)) ? Size() - Int(2) : Int(0);
    }
    
    friend std::string ToString( cref<Path_T> path )
    {
        return ToString(path.container);
//        if( path.Size() <= 0 )
//        {
//            return "{ }";
//        }
//        
//        std::string s = "{ ";
//        
//        {
//            auto [a,d] = PD_T::FromDarc(path[0]);
//            s += "{" + ToString(a) + "," + ToString(d) + "}";
//        }
//        for( Int p = 1; p < path.Size(); ++p )
//        {
//            auto [a,d] = PD_T::FromDarc(path[0]);
//            s += ", {" + ToString(a) + "," + ToString(d) + "}";
//        }
//        
//        s += " }";
//        
//        return s;
    }
    
    friend std::string PathString( cref<Path_T> path )
    {
        if( path.Size() <= 0 )
        {
            return "{ }";
        }
        
        std::string s = "{ ";
    
        s += ToString(ArcOfDarc(path[0]));
        
        for( Int p = 1; p < path.Size(); ++p )
        {
            s += ", " + ToString(ArcOfDarc(path[p]));
        }
        s += " }";
        
        return s;
    }

}; // struct Path_T


std::string PathDetails( cref<Path_T> path ) const
{
    if( path.Size() <= 0 )
    {
        return "{ }";
    }
    
    std::string s;
    
    for( Int p = 0; p < path.Size(); ++p )
    {
        const Int a = path[p];

        s += ToString(p  ) + " : " +  ArcString(a)
           + " (" + ToString( Sign(D_mark(a)) ) + ")\n";
    }
    
    return s;
}

const Tensor1<Int,Int> & PathToArray( cref<Path_T> path ) const
{
    return path.container;
}
