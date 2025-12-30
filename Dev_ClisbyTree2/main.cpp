static constexpr bool clang_matrixQ = true;
static constexpr bool quaternionsQ  = true;

#include "Knoodle.hpp"
#include "../src/ClisbyTree2.hpp"

using namespace Knoodle;

using Real = double;
using Int  = Int64;
using LInt = Int64;

using Clisby_T = ClisbyTree2<
    3,Real,Int,LInt,{.clang_matrixQ=clang_matrixQ,.quaternionsQ=quaternionsQ}
>;

int main()
{
    print("Hello, this is a test program for ClisbyTree2.");
    print("");
    
    const Int  n                = 1024;
    const Real edge_length      = 1;
    const Real hard_sphere_diam = 1;
    const LInt attempt_count    = 1'000'000;
    const LInt accept_count     = 1'000'000;
    const Real reflection_prob  = 0.0;
    
    TOOLS_DUMP(n);
    TOOLS_DUMP(edge_length);
    TOOLS_DUMP(hard_sphere_diam);
    TOOLS_DUMP(attempt_count);
    TOOLS_DUMP(accept_count);
    TOOLS_DUMP(reflection_prob);
    
    // Initialize the tree.
    // By default, a convex regular equilateral n-gon in the x-y-plane is used for initialization.
    Clisby_T T ( n, edge_length, hard_sphere_diam );
    
    TOOLS_DUMP(T.Gap());
    
    // Do a thorough and relatively quick collision check.
    if( T.CollisionQ() )
    {
        auto [collosionQ,witnesses] = T.CollisionQ_Debug();
        
        TOOLS_DUMP(collosionQ);
        TOOLS_DUMP(witnesses);
        
        eprint("Initial polygon has self-intersections.");
        exit(1);
    }
    // Note: During sampling the tree runs a more cost efficient collisions check because it may exploit that the polygon before a move is already collision-free.
    
    // This helps us to time the code.
    TimeInterval t;
    
    print("");
    print("Attempting " + ToString(attempt_count) + " pivot moves.");
    t.Tic(); // start timer
        // Make attempt_count attempts of pivot moves.
        auto flag_counts = T.FoldRandom(attempt_count,reflection_prob);
        // The return value is a short vector with counts of certain events during sampling.
        // The sampled polygon is still stored implicitly in T.
    t.Toc(); // stop timer
    TOOLS_DUMP(t.Duration());

    print("");
    print("Sampling until "+ToString(accept_count)+" accepted pivot moves have been made.");
    t.Tic(); // start timer
        // Sample until accept_count many successful pivot moves have been made.
        flag_counts += T.FoldRandomUntil(accept_count,reflection_prob);
        // The return value is a short vector with counts of certain events during sampling.
        // We add them to the counts that we obtained before.
        // The sampled polygon is still stored implicitly in T.
    t.Toc(); // stop timer
    TOOLS_DUMP(t.Duration());
    
    // We print the meaning of the event counts in human-readable form.
    print("");
    print(T.FoldFlagCounts_ToString(flag_counts)); // print meaning of flags
    print("");
    
    // Create a container to store the vertex coordinates.
    // It behaves like a matrix of size n x 3. The coordinates will be stored in interleaved form, i.e., the 3 coordinates of the i-th vertex are stored in the i-th row.
    Tensor2<Real,Int> v_coords ( n, 3 );
   
    // v_coords.data() returns a pointer that can be handed to other routines.
    // Beware v_coords manages its own storage, so do not free this pointer!
    // Here, we hand this pointer over to T's routine WriteVertexCoordinates, which will write the coordinates of the current polygon to it.
    T.WriteVertexCoordinates( v_coords.data() );
    
    // Now you can retrieve the j-th coordinate of the i-th vertex by v_coords(i,j);
    
    // You could also read in the polygon with T.ReadVertexCoordinates( v_coords.data() ).
    
    // Check the relative deviations of edge lengths from the prescribed edge length
    valprint("Lowest and highest relative edge length deviation",T.MinMaxEdgeLengthDeviation(v_coords.data()));
    
    // Do a thorough and relatively quick collision check.
    if( T.CollisionQ() )
    {
        auto [collosionQ,witnesses] = T.CollisionQ_Debug();
        
        TOOLS_DUMP(collosionQ);
        TOOLS_DUMP(witnesses);
        
        eprint("Initial polygon has self-intersections.");
        exit(1);
    }
    
    return 0;
}
