#pragma once

#define KNOODLE_H

#include "deps/pcg-cpp/include/pcg_random.hpp"

#include "submodules/Tensors/Tensors.hpp"
#include "submodules/Tensors/submodules/Tools/Oriented2D.hpp"

namespace Knoodle
{
    using namespace Tensors;
    using namespace Tools;
    
    using Tools::ToString;
}


#include "src/Containers.hpp"
#include "src/Types.hpp"

#include "src/Link.hpp"

#include "src/CompleteBinaryTree.hpp"
#include "src/AABBTree.hpp"

#include "src/Intersection.hpp"

#include "src/PlanarLineSegmentIntersector.hpp"
#include "src/Link_2D.hpp"
#include "src/Knot_2D.hpp"

#include "src/MultiGraphBase.hpp"
#include "src/MultiGraph.hpp"
#include "src/MultiDiGraph.hpp"

#include "src/Debugging.hpp"
#include "src/Binarizer.hpp"

#include "src/PlanarDiagram.hpp"

#include "src/PlanarDiagram/CrossingSimplifier.hpp"
#include "src/PlanarDiagram/ArcSimplifier.hpp"
#include "src/PlanarDiagram/StrandSimplifier.hpp"

namespace Knoodle
{
    template<typename Int> class PlanarDiagram2;
    template<typename Int> class PlanarDiagramComplex;
}

#include "src/PlanarDiagramComplex/LoopRemover.hpp"
#include "src/PlanarDiagramComplex/ArcSimplifier2.hpp"
#include "src/PlanarDiagramComplex/StrandSimplifier2.hpp"

#include "src/PlanarDiagram2.hpp"
#include "src/PlanarDiagramComplex.hpp"

//
//#include "src/Seifert.hpp"    // TODO: Needs debugging.

#include "src/Link_3D.hpp"

#include "src/LinearHomotopy_3D.hpp"

#include "src/KnotInvariants/AlexanderStrandMatrix.hpp"
#include "src/KnotInvariants/AlexanderFaceMatrix.hpp"

//#include "src/Alexander.hpp"  // Uses my own Cholesky factorization.
                                // Not favorable compared to Alexander_UMFPACK.hpp

//#include "src/Alexander_UMFPACK.hpp" // Improved version of the former.


//#include "src/KnotLookupTable.hpp"
