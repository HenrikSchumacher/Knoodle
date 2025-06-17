public:

Int TrfaceCount() const
{
    return TRF_count;
}

cref<Tensor1<Int,Int>> TrfaceTrdedgePointers() const
{
    return TRF_dTRE_ptr;
}

cref<Tensor1<Int,Int>> TrfaceTrdedgeIndices() const
{
    return TRF_dTRE_idx;
}

//Tensor1<Int,Int> TrfaceTrdedgePointers()
//{
//    TOOLS_PTIC(ClassName()+"::TrfaceTrdedgePointers");
//
//    Aggregator<Int,Int> agg ( vertex_count );
//    agg.Push(Int(0));
//
//    Int dE_counter = 0;
//
//    TRF_TraverseAll(
//        [](){},
//        [&dE_counter]( const Int de )
//        {
//            ++dE_counter;
//        },
//        [&agg,&dE_counter]()
//        {
//            agg.Push(dE_counter);
//        }
//    );
//
//    TOOLS_PTOC(ClassName()+"::TrfaceTrdedgePointers");
//
//    return agg.Get();
//}

//Tensor1<Int,Int> TrfaceTrdedgeIndices()
//{
//    TOOLS_PTIC(ClassName()+"::TrfaceTrdedgeIndices");
//
//    Aggregator<Int,Int> agg ( vertex_count );
//
//    TRF_TraverseAll(
//        [](){},
//        [&agg]( const Int de )
//        {
//            agg.Push(de);
//        },
//        [](){}
//    );
//
//    TOOLS_PTOC(ClassName()+"::TrfaceTrdedgeIndices");
//
//    return agg.Get();
//}
