public:
    
using Base_T          = CachedObject;

using VInt            = VInt_;
using EInt            = EInt_;
using SInt            = SInt_;
using Edge_T          = Tiny::Vector<2,VInt,EInt>;
using EdgeContainer_T = Tiny::VectorList_AoS<2,VInt,EInt>;

enum class InOut : SInt
{
    Undirected =  0,
    In         =  1,
    Out        = -1
};

enum class Direction : bool
{
    Forward  = 1,
    Backward = 0
};

template<typename Int, bool nonbinaryQ>
using SignedMatrix_T = std::conditional_t<
        nonbinaryQ,
        Sparse::MatrixCSR<SInt,Int,Int>,
        Sparse::BinaryMatrixCSR<Int,Int>
>;

using IncidenceMatrix_T = SignedMatrix_T<EInt,1>;

using VV_Vector_T       = Tensor1<VInt,VInt>;
using EE_Vector_T       = Tensor1<EInt,EInt>;
using EV_Vector_T       = Tensor1<EInt,VInt>;
using VE_Vector_T       = Tensor1<VInt,EInt>;
