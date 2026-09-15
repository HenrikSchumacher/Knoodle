#pragma once

// PassOracle -- test-only access to `PassSimplifier`'s rerouting entry points.
//
// `PassSimplifier::LoadDiagram` and `PassSimplifier::Reroute` are private, and
// deliberately so: `Reroute`'s doc comment says it is unsafe without the
// invariants `SimplifyStrands` establishes. We need to call them anyway, for
// one purpose only -- to check `OrthoDecorate::AfterDiagram` (which builds the
// after-diagram from the descriptor alone, never calling the applier) against
// the applier itself on the same move. Two independent implementations of one
// move; if they disagree, one is wrong, and the drawing is where that shows up.
//
// Henrik approved the friend access on 2026-08-12. The declaration lives in
// `Knoodle.hpp` and the `friend` line in `PassSimplifier`; both name this
// class, which is defined only here and never in a shipped translation unit.
//
// This is a driver, not a reimplementation: it forwards to the existing
// routines and adds nothing of its own.

#include "../Knoodle.hpp"

#include <string>

namespace Knoodle
{
    template<IntQ Int_>
    class PassOracle
    {
    public:

        using Int   = Int_;
        using PD_T  = PlanarDiagram<Int>;
        using PDC_T = PlanarDiagramComplex<Int>;
        using PS_T  = PassSimplifier<Int>;

        /*!@brief Hand `pd` to the pass simplifier. Mirrors what
         * `SimplifyPasses` does before it reroutes.
         */
        static void LoadDiagram( mref<PS_T> S, mref<PD_T> pd )
        {
            S.LoadDiagram(pd);
        }

        /*!@brief Run the applier under test on a prepared pass and path.
         *
         * CAUTION: `Reroute` assumes the corridor is a SHORTEST path returned
         * by `FindShortestPath` (Henrik, 2026-08-12). Handing it anything else
         * is outside its contract, and the arc-label aliasing is what that
         * looks like when it goes wrong. Callers that deliberately feed it a
         * non-conforming corridor are testing the boundary, not using the API.
         */
        template<typename Pass_T, typename Path_T>
        static bool Reroute( mref<PS_T> S, mref<Pass_T> pass, mref<Path_T> path )
        {
            return S.Reroute(pass,path);
        }

    }; // class PassOracle

} // namespace Knoodle
