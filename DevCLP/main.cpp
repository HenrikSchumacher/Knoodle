#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
#include "../submodules/Tensors/Clp.hpp"
#include "ClpSimplex.hpp"
#include "ClpSimplexDual.hpp"
//#include "CoinHelperFunctions.hpp"

#include "../Reapr.hpp"

//#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

using Real  = double;          // scalar type used for positions of polygon
using BReal = double;          // scalar type used for bounding boxes
using Int   = int;             // integer type used, e.g., for indices
using LInt  = CoinBigIndex;    // integer type for counting objects

int main( int argc, char** argv )
{
    TOOLS_MAKE_FP_STRICT();
    
    TOOLS_DUMP(TypeName<CoinBigIndex>);
    
    auto fp_formatter = [](double x){ return ToStringFPGeneral(x); };
    
    if( sizeof(CoinBigIndex) != 8 )
    {
        wprint("COIN-OR does not use 64 bit integers for CoinBigIndex!");
    }
    
    const Int n = 100;

    auto A = Sparse::MatrixCSR<Real,Int,LInt>::IdentityMatrix(n);

    
    Tensor1<Real,Int> col_lower_bnd ( A.ColCount(), -Scalar::Infty<Real> );
    Tensor1<Real,Int> col_upper_bnd ( A.ColCount(),  Scalar::Infty<Real> );
    
    Tensor1<Real,Int> row_lower_bnd ( A.RowCount(),  Scalar::One  <Real> );
    Tensor1<Real,Int> row_upper_bnd ( A.RowCount(),  Scalar::Two  <Real> );
    
    Tensor1<Real,Int> obj_vec ( A.ColCount() );
    obj_vec.Randomize();

    
    // https://coin-or.github.io/Clp/Doxygen/classClpSimplex.html
    ClpSimplex LP;
    LP.setMaximumIterations(1000000);
    LP.setOptimizationDirection(1); // +1 - minimize; -1 - maximize
    
    TOOLS_DUMP(LP.optimizationDirection());
    
    print("");
    
    // This variant of `loadProblem` expects  matrix in COLUMN-MAJOR form.
    tic("ClpSimplex::loadProblem from pointers.");
    
    auto AT = A.Transpose();
    
    
    LP.loadProblem(
        AT.RowCount(), AT.ColCount(),
        AT.Outer().data(), AT.Inner().data(), AT.Values().data(),
        col_lower_bnd.data(), col_upper_bnd.data(),
        obj_vec.data(),
        row_lower_bnd.data(), row_upper_bnd.data()
    );
    toc("ClpSimplex::loadProblem from pointers.");

    tic("ClpSimplex::primal");
    LP.primal();
    toc("ClpSimplex::primal");
    
    print("");
    valprint("Successful",      BoolString(LP.statusOfProblem()) );
    valprint("Iteration Count", LP.getIterationCount()           );
    
    print("");
    
    Tensor1<Real,Int> solution( LP.getNumCols() );
    
    solution.Read( LP.primalColumnSolution() );
    
    valprint("solution",ToString(solution,fp_formatter));

    tic("ClpSimplex::loadProblem from CoinPackedMatrix.");
    auto B = MatrixCSR_to_CoinPackedMatrix(A);
    
    LP.loadProblem(
        B,
        col_lower_bnd.data(), col_upper_bnd.data(),
        obj_vec.data(),
        row_lower_bnd.data(), row_upper_bnd.data()
    );
    
    toc("ClpSimplex::loadProblem from CoinPackedMatrix.");
    
    print("");
    
    LP.initialBarrierSolve();
    
    tic("ClpSimplex::dual");
    LP.dual();
    toc("ClpSimplex::dual");

    print("");
    valprint("Successful",      BoolString(LP.statusOfProblem()) );
    valprint("Iteration Count", LP.getIterationCount()           );
    print("");
    
    Tensor1<Real,Int> solution_2( LP.getNumCols() );
    
    solution_2.Read( LP.primalColumnSolution() );    
    valprint("solution_2",ToString(solution_2,fp_formatter));
    return 0;
}
