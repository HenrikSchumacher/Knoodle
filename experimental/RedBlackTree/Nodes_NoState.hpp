public:

struct Node_T
{
    Int child [2] = { NIL, NIL };
    Data_T data;
    
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


// TODO: All these have to be private.

Int GetChild( const Int node, const bool side ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return NIL; }
    }
    return (node_buffer[node][side] & NIL);
}

void SetChild( const Int node, const bool side, const Int child )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return; }
    }
    auto a = node_buffer[node][side] & ~ NIL;
    node_buffer[node][side] = a | (child & NIL);
}

State_T GetState( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return State_T::Invalid; }
    }
    return State_T{static_cast<bool>(node_buffer[node][Left] & ~NIL)};
}

void SetState( const Int node, const State_T state )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return; }
    }
    auto a = node_buffer[node][Left] & NIL;
    node_buffer[node][Left] = a | ((state == State_T::Red) ? ~NIL : Int{0});
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
        if( !InRangeQ(node) ) { return; }
    }
    node_buffer[node].data = data;
}

void SetData( const Int node, Data_T && data )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return; }
    }
    node_buffer[node].data = std::move(data);
}
