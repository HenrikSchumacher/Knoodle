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

