public:

using ChildContainer_T = Tiny::VectorList_AoS<2,Int,Int>;
using StateContainer_T = Tensor1<State_T,Int>;
using DataContainer_T  = Tensor1<Data_T,Int>;

private:

StateContainer_T node_state;
ChildContainer_T node_child;
DataContainer_T  node_data;

public:

Int NodeCapacity() const { return node_data.Size(); }

void Reserve( const Int size )
{
    // TODO: Add a check whether this size can be stored in the integer type.
    if( NodeCapacity() < size )
    {
        node_state.template Resize<true>(size);
        node_child.template Resize<true>(size);
        node_data .template Resize<true>(size);
    }
}

Int & Child( const Int node, const bool side )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_node; }
    }
    return node_child(node,side);
}

const Int & Child( const Int node, const bool side ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_node; }
    }
    return node_child(node,side);
}

State_T & State( const Int node )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_state; }
    }
    return node_state[node];
}

const State_T & State( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_state; }
    }
    return node_state[node];
}

Data_T & Data( const Int node )
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    return node_data[node];
}

const Data_T & Data( const Int node ) const
{
    if constexpr ( bound_checksQ )
    {
        if( !InRangeQ(node) ) { return dummy_data; }
    }
    return node_data[node];
}
