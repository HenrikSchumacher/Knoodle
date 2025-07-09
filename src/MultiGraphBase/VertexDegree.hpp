protected:

template<InOut inout = InOut::Undirected>
VV_Vector_T VertexDegrees_impl() const
{
    // We have to initialize by to account for disconnected vertices.
    VV_Vector_T degree (vertex_count,VInt(0));

    for( EInt e = 0; e < EdgeCount(); ++e )
    {
        switch( inout )
        {
            case InOut::Undirected:
            {
                ++degree[edges(e,Tail)];
                ++degree[edges(e,Head)];
            }
            case InOut::Out:
            {
                ++degree[edges(e,Tail)];
            }
            case InOut::In:
            {
                ++degree[edges(e,Head)];
            }
        }
    }
    
    return degree;
}

public:

VInt VertexDegree( const VInt v ) const
{
    auto & A = OrientedIncidenceMatrix();
    
    return A.Outer(v+VInt(1)) - A.Outer(v);
}

VInt VertexInDegree( const VInt v ) const
{
    auto & A = InIncidenceMatrix();
    
    return A.Outer(v+VInt(1)) - A.Outer(v);
}

VInt VertexOutDegree( const VInt v ) const
{
    auto & A = OutIncidenceMatrix();
    
    return A.Outer(v+VInt(1)) - A.Outer(v);
}


VV_Vector_T VertexDegrees() const
{
    TOOLS_PTIC(this->ClassName()+"::VertexDegrees");
    
    VV_Vector_T result =
    this->template VertexDegrees_impl<InOut::Undirected>();
    
    TOOLS_PTOC(this->ClassName()+"::VertexDegrees");
    
    return result;
}

VV_Vector_T VertexInDegrees() const
{
    TOOLS_PTIC(this->ClassName()+"::VertexInDegrees");
    
    VV_Vector_T result
    = this->template VertexDegrees_impl<InOut::In>();
    
    TOOLS_PTOC(this->ClassName()+"::VertexInDegrees");
    
    return result;
}

VV_Vector_T VertexOutDegrees() const
{
    TOOLS_PTIC(this->ClassName()+"::VertexOutDegrees");
    
    VV_Vector_T result
    = this->template VertexDegrees_impl<InOut::Out>();
    
    TOOLS_PTOC(this->ClassName()+"::VertexOutDegrees");
    
    return result;
}
