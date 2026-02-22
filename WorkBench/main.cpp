#include "../Knoodle.hpp"

using namespace Knoodle;

using Int = UInt64;
using Real = Real64;

int main()
{
    const Int m = 10'000;
    const Int n = 300;
    
    Knoodle::ConformalBarycenterSampler<3,Real,Int> S ( n );
    Tensors::Tensor3<Real,Int> vertex_coordinates( m, n + 1, 3 );
    Real K = 0;
    // Sometimes we want to copy the the first vertex on wrap-around; sometimes we don't.
    // That's why WriteRandomClosedPolygon has flag `wrap_aroundQ` for that.
    tic("WriteRandomClosedPolygon");
    for( Int i = 0; i < m; ++ i )
    {
        S.WriteRandomClosedPolygon(
            vertex_coordinates.data(i), K, true /* = with wrap-around*/
        );
    }
    toc("WriteRandomClosedPolygon");
    
    tic("WriteRandomClosedPolygon");
    for( Int i = 0; i < m; ++ i )
    {
        S.WriteRandomClosedPolygon(
            vertex_coordinates.data(i), K, true /* = with wrap-around*/
        );
    }
    toc("WriteRandomClosedPolygon");
    
    TOOLS_DUMP(vertex_coordinates(0,0,0));
    
    Knoodle::ActionAngleSampler<Real,Int> A;
    Tensors::Tensor3<Real,Int> vertex_coordinates2( m, n + 1, 3 );
    tic("ActionAngleSampler::WriteRandomEquilateralPolygon");
    for( Int i = 0; i < m; ++ i )
    {
        A.WriteRandomEquilateralPolygon(
          vertex_coordinates2.data(i), n, {.wrap_aroundQ = false}
        );
    }
    toc("ActionAngleSampler::WriteRandomEquilateralPolygon");
    
    tic("ActionAngleSampler::WriteRandomEquilateralPolygon");
    for( Int i = 0; i < m; ++ i )
    {
        A.WriteRandomEquilateralPolygon(
          vertex_coordinates2.data(i), n, {.wrap_aroundQ = false}
        );
    }
    toc("ActionAngleSampler::WriteRandomEquilateralPolygon");
    
    TOOLS_DUMP(vertex_coordinates(0,0,0));
    return 0;
}
