#define MallocNanoZone 0

#include "../submodules/Tensors/Accelerate.hpp"

#include "../KnotTools.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

using Real = double;
using Int  = Int64;

constexpr Int AmbDim = 3;

using Link_T = Link_2D<Real,Int,Int8>;
using PD_T   = PlanarDiagram<Int>;

int main(void)
{
    
    const Real r       = 1;
    const Int  n       = 100000;

    const Int  burn_in = 1000000;
    const Int  skip    = 100000;
    
    
    const Int sample_count = 100;

    Tensor2<Real,Size_T> x ( n, AmbDim, Real(0) );
    
//    Tensor3<Real,Size_T> samples ( sample_count, n, AmbDim, Real(0) );
    
//    dump( Real(samples.Size() * sizeof(double)) / 1000000. );
    
    ClisbyTree<AmbDim,Real,Int> T ( n, r );
    
    Int steps_taken = 0;
    
    
    tic("Burn-in");
    T.FoldRandom(burn_in);
    steps_taken += burn_in;
    toc("Burn-in");
    print("");
    
    
    Link_T L ( n );
    
    std::vector<Tensor2<Int,Int>> pd_codes;
        
    for( Int i = 0; i < sample_count; ++i )
    {
        tic("sample " + ToString(i));
        
        auto counts = T.FoldRandom(skip);
        
        dump(counts)
        
        steps_taken += skip;
        
        T.WriteVertexCoordinates( x.data() );
        
        L.ReadVertexCoordinates ( x.data() );

        L.FindIntersections();

        PD_T PD ( L );
        
        std::vector<PD_T> PD_list;
        
        PD.Simplify5( PD_list );
        
        dump(PD_list.size());
        
        std::stringstream s;
        
        s << "Cr = { " << PD.CrossingCount();
        
        pd_codes.push_back(PD.PDCode());
        
        for( auto & P : PD_list )
        {
            s << ", " << P.CrossingCount();
            
            pd_codes.push_back(P.PDCode());
        }
        
        s << " }";
        
        print(s.str());
        
//        T.WriteVertexCoordinates(samples.data(i));
//        dump( T.VertexCoordinates(0) );
        toc("sample " + ToString(i));
        print("");
    }
    
    return 0;
}
