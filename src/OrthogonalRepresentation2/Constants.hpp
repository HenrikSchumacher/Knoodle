public:

static constexpr Dir_T East  =  0;
static constexpr Dir_T North =  1;
static constexpr Dir_T West  =  2;
static constexpr Dir_T South =  3;
static constexpr Dir_T NoDir = 15;

static constexpr bool Left  = PlanarDiagram<Int>::Left;
static constexpr bool Right = PlanarDiagram<Int>::Right;
static constexpr bool Out   = PlanarDiagram<Int>::Out;
static constexpr bool In    = PlanarDiagram<Int>::In;
static constexpr bool Tail  = PlanarDiagram<Int>::Tail;
static constexpr bool Head  = PlanarDiagram<Int>::Head;


static constexpr int EdgeActiveBit   = 0;
static constexpr int EdgeVisitedBit  = 1;
static constexpr int EdgeExteriorBit = 2;
static constexpr int EdgeVirtualBit  = 3;

static constexpr EdgeFlag_T EdgeActiveMask   = EdgeFlag_T(1) << EdgeActiveBit;
static constexpr EdgeFlag_T EdgeVisitedMask  = EdgeFlag_T(1) << EdgeVisitedBit;
static constexpr EdgeFlag_T EdgeExteriorMask = EdgeFlag_T(1) << EdgeExteriorBit;
static constexpr EdgeFlag_T EdgeVirtualMask  = EdgeFlag_T(1) << EdgeVirtualBit;
