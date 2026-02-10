Int crossing_count      = 0;    // 8 bytes
Int arc_count           = 0;    // 8 bytes
Int max_crossing_count  = 0;    // 8 bytes
Int max_arc_count       = 0;    // 8 bytes

// Exposed to user via Crossings().
CrossingContainer_T      C_arcs;        // at least 8 bytes; currectly 40 bytes
// Exposed to user via CrossingStates().
CrossingStateContainer_T C_state;       // at least 8 bytes; currectly 24 bytes
// Some multi-purpose scratch buffers.
mutable Tensor1<Int,Int> C_scratch;     // at least 8 bytes; currectly 24 bytes

// Exposed to user via Arcs().
ArcContainer_T           A_cross;       // at least 8 bytes; currectly 32 bytes
// Exposed to user via ArcStates().
ArcStateContainer_T      A_state;       // at least 8 bytes; currectly 24 bytes
// Exposed to user via ArcColors().
ArcColorContainer_T      A_color;       // at least 8 bytes; currectly 24 bytes
// Some multi-purpose scratch buffers.
mutable Tensor1<Int,Int> A_scratch;     // at least 8 bytes; currectly 24 bytes

mutable Int last_color_deactivated = Uninitialized; // 8 bytes
mutable Int c_search_ptr = 0;                       // 8 bytes
mutable Int a_search_ptr = 0;                       // 8 bytes

bool proven_minimalQ = false;                       // 1 bytes

// Currently
// 7 * bytes for the ints
// 7 * bytes for the ints
