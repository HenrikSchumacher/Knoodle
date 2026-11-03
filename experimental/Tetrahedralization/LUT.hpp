public:

  
/*             v3
 *             X
 *            /.\
 *           / . \
 *          /  .  \
 *         /  .X.  \
 *        / . v2  . \
 *       /.         .\
 *   v0 X-------------X v1
 */


static constexpr std::array<std::array<Int,3>,4> face_vertex_lut = {{
    {1,2,3},
    {0,3,2},
    {3,0,1},
    {2,1,0}
}};

static constexpr std::array<std::array<std::array<Int,3>,3>,4> glue_lut = {
    {
        {3,2,1}, // <-- faces_lut[0] reversed
        {1,3,2},
        {2,1,3}
    },
    {
        {2,3,0}, // <-- faces_lut[1] reversed
        {0,2,3},
        {3,0,2}
    },
    {
        {1,0,3}, // <-- faces_lut[2] reversed
        {3,1,0},
        {0,3,1}
    },
    {
        {0,1,2}, // <-- faces_lut[3] reversed
        {2,0,1},
        {1,2,0}
    }
};




//////////////////// Experimental from here on. ///////////////////////

// We need
//  - dedge-face lookup (for cycling around dedge)
//  - dedge-opp-dedge lookup (maybe)
//  - dedge-vertex lookup (maybe)

// cycling around the i-th triangle, generating dedges as we go.
static constexpr std::array<std::array<Int,3>,12> dedge_vertex_lut = {{
    {2,3},{3,1},{1,2},
    {3,2},{2,0},{0,3},
    {0,1},{1,3},{3,0},
    {1,0},{0,2},{2,1}
}};

//// No actual need to store this.
//static constexpr std::array<std::array<Int,3>,4> face_dedge_lut = {
//    { 0, 1, 2},
//    { 3, 4, 5},
//    { 6, 7, 8},
//    { 9,10,11}
//};

// this makes the dedge-to-face lookup easy:
static constexpr std::array<Int,12> dedge_face_lut = {
    0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3
};

static constexpr std::array<Z3,12> dedge_mod3 = {
    Z3{0}, Z3{1}, Z3{2}, Z3{0}, Z3{1}, Z3{2}, Z3{0}, Z3{1}, Z3{2}, Z3{0}, Z3{1}, Z3{2}
};


static constexpr std::array<Int,12> dedge_rev_dedge_lut = {
    3, 7, 11, 0, 10, 8, 9, 1, 5, 6, 4, 2
};

static constexpr std::array<Int,12> dedge_opp_dedge_lut = {
    6, 10, 5, 9, 7, 2, 0, 4, 11, 3, 1, 8
};

