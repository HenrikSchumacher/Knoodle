public:

static constexpr Dir_T East      =  0;
static constexpr Dir_T North     =  1;
static constexpr Dir_T West      =  2;
static constexpr Dir_T South     =  3;

static constexpr Dir_T NorthEast =  4;  // only for softened virtual edges
static constexpr Dir_T NorthWest =  5;  // only for softened virtual edges
static constexpr Dir_T SouthWest =  6;  // only for softened virtual edges
static constexpr Dir_T SouthEast =  7;  // only for softened virtual edges

static constexpr Dir_T NoDir     = Scalar::Max<Dir_T>;

// The translation between PlanarDiagram's ports and the the cardinal directions under the assumption that C_dir[c] == North;
static constexpr Dir_T dir_lut [2][2] = { {North,East}, {West,South} };

static constexpr bool Left  = PD_T::Left;
static constexpr bool Right = PD_T::Right;
static constexpr bool Out   = PD_T::Out;
static constexpr bool In    = PD_T::In;
static constexpr bool Tail  = PD_T::Tail;
static constexpr bool Head  = PD_T::Head;


static constexpr int EdgeActiveBit        = 0;
static constexpr int EdgeVisitedBit       = 1;
static constexpr int EdgeExteriorBit      = 2;
static constexpr int EdgeVirtualBit       = 3;
static constexpr int EdgeUnconstrainedBit = 4;

static constexpr EdgeFlag_T EdgeActiveMask
                            = EdgeFlag_T(1) << EdgeActiveBit;
static constexpr EdgeFlag_T EdgeVisitedMask
                            = EdgeFlag_T(1) << EdgeVisitedBit;
static constexpr EdgeFlag_T EdgeExteriorMask
                            = EdgeFlag_T(1) << EdgeExteriorBit;
static constexpr EdgeFlag_T EdgeVirtualMask
                            = EdgeFlag_T(1) << EdgeVirtualBit;
static constexpr EdgeFlag_T EdgeUnconstrainedMask
                            = EdgeFlag_T(1) << EdgeUnconstrainedBit;
