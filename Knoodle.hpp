#pragma once

#define KNOODLE_H

#include <cfenv>

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
#include "src/Debugging.hpp"

#include "src/Link.hpp"

#include "src/CompleteBinaryTree.hpp"
#include "src/AABBTree.hpp"

#include "src/Intersection.hpp"

#include "src/PlanarLineSegmentIntersector.hpp"
#include "src/LinkEmbedding.hpp" // Meant to supercede the misnomer Link_2D.
#include "src/KnotEmbedding.hpp"

#include "src/MultiGraphBase.hpp"
#include "src/MultiGraph.hpp"
#include "src/MultiDiGraph.hpp"

#include "src/Binarizer.hpp"

namespace Knoodle
{
    template<IntQ Int> class PlanarDiagram;
    template<IntQ Int> class PlanarDiagramComplex;
    template<FloatQ Real, IntQ Int, FloatQ BReal> class Reapr;
    template<typename PD_T> class OrthoDraw;
}

#include "src/PlanarDiagramComplex/LoopRemover.hpp"
#include "src/PlanarDiagramComplex/ArcSimplifier.hpp"
#include "src/PlanarDiagramComplex/PassSimplifier.hpp"

#include "src/PlanarDiagram.hpp"
#include "src/PlanarDiagramComplex.hpp"
#include "src/OrthoDraw.hpp"
#include "src/Reapr.hpp"

#include "src/KnotInvariants/AlexanderStrandMatrix.hpp"
#include "src/KnotInvariants/AlexanderFaceMatrix.hpp"

//#include "src/Alexander.hpp"  // Uses my own Cholesky factorization.
                                // Not favorable compared to Alexander_UMFPACK.hpp

//#ifdef KNOODLE_USE_UMFPACK
//#include "src/KnotInvariants/Alexander_UMFPACK.hpp" // Improved version of the former.
//#endif

//
//#include "src/Seifert.hpp"    // TODO: Needs debugging.

#include "src/Link_3D.hpp"
#include "src/LinearHomotopy_3D.hpp"


#include "src/ActionAngleSampler.hpp"
#include "src/ConformalBarycenterSampler.hpp"


//#include "src/KnotLookupTable.hpp"
