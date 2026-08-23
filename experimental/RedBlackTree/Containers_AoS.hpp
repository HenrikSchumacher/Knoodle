public:

struct Node_T
{
    Int child [2] = { NIL, NIL };
    Data_T data;
    State_T state = State_T::Inactive;
    
    mref<Int> operator[]( bool side )       { return child[side]; }
    
    cref<Int> operator[]( bool side ) const { return child[side]; }
};

private:

Tensor1<Node_T,Int> node_buffer;

public:

Int NodeCapacity() const { return node_buffer.Size(); }

void Reserve( const Int size )
{
    // TODO: Add a check whether this size can be stored in the integer type.
    if( NodeCapacity() < size )
    {
        node_buffer.template Resize<true>(size);
    }
}

Int & Child( const Int node, const bool side )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_node; }
    }
    return node_buffer[node][side];
}

const Int & Child( const Int node, const bool side ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_node; }
    }
    return node_buffer[node][side];
}

State_T & State( const Int node )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_state; }
    }
    return node_buffer[node].state;
}

const State_T & State( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_state; }
    }
    return node_buffer[node].state;
}

Data_T & Data( const Int node )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    return node_buffer[node].data;
}

const Data_T & Data( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    return node_buffer[node].data;
}
