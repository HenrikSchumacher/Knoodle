#pragma once

// Multi-crossing pass-move fixtures.
//
// The trefoil and the figure-eight cannot host a corridor with two or more
// crossings: that needs a strand of three or more arcs, and an exhaustive
// search over both found only L = 2, k = 1 moves once check 6 (no lengthening)
// is enforced. So every multi-crossing case has to come from a real diagram.
//
// This is the 73-crossing, determinant-5 knot from
// handoff/reroute-arc-label-aliasing/ -- middlestrands built it from a
// zerofriends corpus diagram (zf061098) by flipping five crossings so that
// arcs 40..48 form a genuine underpass, and used it to demonstrate the
// arc-label aliasing bug in PassSimplifier::Reroute (PR #30). Its k = 8
// descriptor is the one that triggers the aliasing, so it is the single most
// valuable pass-move fixture we have.

#include <vector>

// 5-column signed PD code, 0-based arcs.
inline const std::vector<std::int64_t> zf061098_underpass = {
      36,   11,   37,   12,   -1,
      35,   50,   36,   51,   -1,
      49,   11,   50,   10,    1,
       9,   49,   10,   48,    1,
      34,   76,   35,   75,    1,
      51,   74,   52,   75,   -1,
      47,   77,   48,   76,    1,
      52,  106,   53,  105,    1,
      33,  104,   34,  105,   -1,
      53,   32,   54,   33,   -1,
      31,   91,   32,   90,    1,
      89,  107,   90,  106,    1,
     107,   30,  108,   31,   -1,
     108,   92,  109,   91,    1,
      88,   73,   89,   74,   -1,
     109,   55,  110,   54,    1,
      72,   30,   73,   29,    1,
      87,   13,   88,   12,    1,
      13,   28,   14,   29,   -1,
      55,   92,   56,   93,   -1,
      71,   56,   72,   57,   -1,
      27,   86,   28,   87,   -1,
      26,   38,   27,   37,    1,
      70,   93,   71,   94,   -1,
      14,   58,   15,   57,    1,
      58,   86,   59,   85,    1,
      15,   95,   16,   94,    1,
      59,   38,   60,   39,   -1,
      16,  123,   17,  124,   -1,
      69,  125,   70,  124,    1,
     110,  125,  111,  126,   -1,
     126,  103,  127,  104,   -1,
      46,  128,   47,  127,    1,
      45,  102,   46,  103,   -1,
     128,   78,  129,   77,    1,
     122,   96,  123,   95,    1,
      96,   18,   97,   17,    1,
     121,   84,  122,   85,   -1,
      68,  111,   69,  112,   -1,
      44,   67,   45,   68,   -1,
     120,   40,  121,   39,    1,
      83,   18,   84,   19,   -1,
      97,  113,   98,  112,    1,
      40,   20,   41,   19,    1,
      82,  113,   83,  114,   -1,
      43,   99,   44,   98,    1,
      99,   66,  100,   67,   -1,
      42,   81,   43,   82,   -1,
      41,  115,   42,  114,    1,
      80,   66,   81,   65,    1,
     100,  141,  101,  142,   -1,
     142,  101,  143,  102,   -1,
      79,  141,   80,  140,    1,
     143,   79,  144,   78,    1,
     144,  130,  145,  129,    1,
     145,    8,    0,    9,   -1,
     130,    7,  131,    8,   -1,
       6,  139,    7,  140,   -1,
       5,   64,    6,   65,   -1,
     138,  131,  139,  132,   -1,
     132,   64,  133,   63,    1,
      62,  138,   63,  137,    1,
       4,  116,    5,  115,    1,
       3,   21,    4,   20,    1,
     119,    3,  120,    2,    1,
     133,  116,  134,  117,   -1,
      60,    1,   61,    2,   -1,
     134,   21,  135,   22,   -1,
     118,  135,  119,  136,   -1,
      22,  118,   23,  117,    1,
      23,  136,   24,  137,   -1,
      24,   61,   25,   62,   -1,
      25,    1,   26,    0,    1,
};

//==============================================================================
// LINK fixtures.
//
// Knots cannot exercise the case that matters most for a link-capable spec: a
// corridor belonging to one component passing over or under a DIFFERENT
// component. Every descriptor below does exactly that -- the strand's arcs and
// the crossed arcs live on disjoint link components -- and each was found by
// exhaustive search over all 1268 diagrams of `data/diagrams/linktable/`
// (every strand of length 2..6, every arc-disjoint dual corridor, both tag
// choices), then confirmed to route at four grid sizes and to survive
// `AfterDiagram` + `CheckAll`.
//
// Two facts about that search are worth recording, because they explain why
// the fixtures look the way they do:
//
//   * Every well-formed pass move in the whole table has k = L-1, i.e. it
//     PRESERVES the crossing count. Not one crossing-reducing pass move exists
//     anywhere in the table (0 well-formed at every k < L-1, out of >100k
//     candidates). So none of these fixtures shrink a diagram.
//   * k >= 2 occurs only on NON-alternating links. An alternating strand
//     alternates over and under at its interior crossings, so it has no
//     uniform role and check 5 refuses it. That is why the multi-crossing
//     cases below are all `L*n*` and none are `L*a*`.
//
// The Hopf link admits nothing at all, and cannot: with 2 crossings every
// 2-arc strand leaves and returns to the same crossing, which check 4's
// distinct-anchors clause refuses. It stays in the plain round-trip coverage.
//==============================================================================

// L4a1_0 -- 4 crossings, 2 components.
// strand=1,3 depart=0 cross=12:o land=2   (k=1; arcs 0,1 on cpt 0, arc 6 on cpt 1)
inline const std::vector<std::int64_t> L4a1_0 = {
       3,    6,    0,    7,   -1,
       5,    0,    6,    1,   -1,
       1,    4,    2,    5,   -1,
       7,    2,    4,    3,   -1,
};

// L6a1_0 -- 6 crossings, 2 components.
// strand=11,13 depart=10 cross=1:u land=12  (k=1, under-tagged; arcs 5,6 on
// cpt 1, arc 0 on cpt 0)
inline const std::vector<std::int64_t> L6a1_0 = {
       3,    8,    0,    9,   -1,
       5,    0,    6,    1,   -1,
       1,    4,    2,    5,   -1,
       9,    2,   10,    3,   -1,
      11,    7,    4,    6,    1,
       7,   11,    8,   10,    1,
};

// L6n1_0_0 -- 6 crossings, THREE components.
// strand=1,3,5 depart=0 cross=12:o,17:o land=4
// k=2, and the two crossed arcs sit on two DIFFERENT foreign components
// (arc 6 on cpt 1, arc 8 on cpt 2, strand on cpt 0).
inline const std::vector<std::int64_t> L6n1_0_0 = {
       3,   11,    0,   10,    1,
       5,    0,    6,    1,   -1,
       8,    2,    9,    1,    1,
       2,    7,    3,    4,   -1,
       4,   10,    5,    9,    1,
      11,    7,    8,    6,    1,
};

// L7n1_0 -- 7 crossings, 2 components.
// strand=1,3,5 depart=0 cross=12:o,22:o land=4  (k=2)
inline const std::vector<std::int64_t> L7n1_0 = {
       3,   12,    0,   13,   -1,
       5,    0,    6,    1,   -1,
      10,    1,   11,    2,   -1,
       2,    7,    3,    8,   -1,
       8,   13,    9,    4,   -1,
       4,    9,    5,   10,   -1,
      11,    6,   12,    7,   -1,
};

// L10n104_0_0_0 -- 10 crossings, FOUR components. The richest case we have:
// strand=15,17,19,9 depart=14 cross=33:o,0:o,22:o land=8
// k=3, and the corridor crosses one arc of EACH of the other three components.
inline const std::vector<std::int64_t> L10n104_0_0_0 = {
       3,   16,    0,   17,   -1,
       5,    0,    6,    1,   -1,
      14,    1,   15,    2,   -1,
       2,    7,    3,    8,   -1,
      18,    4,   19,    9,    1,
       4,   11,    5,   12,   -1,
       6,   16,    7,   15,    1,
      13,    8,   10,    9,   -1,
      10,   17,   11,   18,   -1,
      19,   12,   14,   13,   -1,
};

// L10a11_0 -- 10 crossings, 2 components. THE SPLIT-OFF FIXTURE.
//
// A pass move here frees a crossingless component: the transversal closes up
// through nothing but interior crossings of the strand, so deleting the strand
// takes away every crossing that loop had. Two descriptors use it:
//
//   k=0: strand=9,11,13,15,17,19,21 depart=8 land=20            (L=7)
//   k=1: strand=9,11,13,15,17,19,21,23 depart=8 cross=32:u land=22   (L=8)
//
// Both need `middlepassQ = true` -- the strand is not uniformly over/under, so
// check 5 refuses them as a classical pass -- and both take 10 crossings to 4,
// freeing one component of colour 0.
//
// HONEST FRAMING: these are well-formed and routable, which is what a fixture
// for the split-off machinery needs. They are NOT sound. Dropping 10 crossings
// to 4 is not an isotopy, and middlepass soundness would need a feasibility
// witness that nothing on this path checks. Do not describe them as isotopies;
// the spec deliberately allows drawing well-formed moves that are
// topologically infeasible, and this is one.
//
// Found by exhaustive search after L6a1_0 turned out to be structurally
// incapable of hosting the case: there a transversal cycle only closes when
// the strand consumes EVERY crossing, leaving an after-diagram of 0 crossings
// and 1 arc that CheckAll rightly refuses.
inline const std::vector<std::int64_t> L10a11_0 = {
       3,    8,    0,    9,   -1,
       5,    0,    6,    1,   -1,
       1,    4,    2,    5,   -1,
       9,    2,   10,    3,   -1,
      19,   16,    4,   17,   -1,
      13,    6,   14,    7,   -1,
       7,   14,    8,   15,   -1,
      15,   10,   16,   11,   -1,
      11,   19,   12,   18,    1,
      17,   13,   18,   12,    1,
};

// L10n104_1_0_0 -- the under-tagged counterpart of the above, on a different
// orientation variant of the same link.
// strand=13,15,17,19 depart=12 cross=31:u,2:u,24:u land=18   (k=3)
inline const std::vector<std::int64_t> L10n104_1_0_0 = {
       3,   16,    0,   17,   -1,
       7,    1,    8,    0,    1,
      14,    1,   15,    2,   -1,
       2,    6,    3,    5,    1,
      18,    9,   19,    4,   -1,
      13,    5,   10,    4,    1,
       6,   15,    7,   16,   -1,
       8,   12,    9,   11,    1,
      10,   17,   11,   18,   -1,
      19,   12,   14,   13,   -1,
};

