public:

static constexpr UInt East  =  0;
static constexpr UInt North =  1;
static constexpr UInt West  =  2;
static constexpr UInt South =  3;
static constexpr UInt NoDir = 15;

static constexpr bool Left  = PlanarDiagram_T::Left;
static constexpr bool Right = PlanarDiagram_T::Right;
static constexpr bool Out   = PlanarDiagram_T::Out;
static constexpr bool In    = PlanarDiagram_T::In;
static constexpr bool Tail  = PlanarDiagram_T::Tail;
static constexpr bool Head  = PlanarDiagram_T::Head;


static constexpr UInt8 ActiveBit   = 0;
static constexpr UInt8 VisitedBit  = 1;
static constexpr UInt8 BoundaryBit = 2;
static constexpr UInt8 VirtualBit  = 3;

static constexpr UInt8 ActiveMask   = UInt8(1) << ActiveBit;
static constexpr UInt8 VisitedMask  = UInt8(1) << VisitedBit;
static constexpr UInt8 BoundaryMask = UInt8(1) << BoundaryBit;
static constexpr UInt8 VirtualMask  = UInt8(1) << VirtualBit;
