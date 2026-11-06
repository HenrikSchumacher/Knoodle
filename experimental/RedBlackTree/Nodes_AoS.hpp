public:

struct Node_T
{
    Int child [2] = { NIL, NIL };
    Data_T data;
    State_T state = State_T::Red;
    
    mref<Int> operator[]( bool side )       { return child[side]; }
    
    cref<Int> operator[]( bool side ) const { return child[side]; }
};

using NodeContainer_T = Tensor1<Node_T,Int>;

private:

NodeContainer_T node_buffer;

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


// TODO: All these have to be private.

Int GetChild( const Int node, const bool side ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_node; }
    }
    return node_buffer[node][side];
}

void SetChild( const Int node, const bool side, const Int child )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_node; }
    }
    node_buffer[node][side] = child;
}

State_T GetState( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_state; }
    }
    return node_buffer[node].state;
}

void SetState( const Int node, const State_T state )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_state; }
    }
    node_buffer[node].state = state;
}

const Data_T & GetData( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    return node_buffer[node].data;
}

void SetData( const Int node, cref<Data_T> data )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    node_buffer[node].data = data;
}

void SetData( const Int node, Data_T && data )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    node_buffer[node].data = std::move(data);
}
