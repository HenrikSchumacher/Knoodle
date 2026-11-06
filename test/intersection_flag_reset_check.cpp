/**
 * @file intersection_flag_reset_check.cpp
 * @brief Regression test: an embedding's intersection counters must describe
 *        ONE projection, not the lifetime of the embedding.
 *
 * The bug this pins: prosector_flag_counts was initialised once at
 * declaration and incremented by ComputeEdgeIntersection, but never cleared at
 * the start of FindIntersections. The status checks at the top of
 * FindIntersections read those counters to decide the return code, so once any
 * projection produced a degeneracy the counter stayed nonzero and every later
 * call returned the same failure -- however clean the new projection was.
 *
 * PlanarDiagramComplex::Rattle recovers from a degenerate projection by
 * re-rotating and trying again, up to 10 times. With the stale counter that
 * recovery could not work: all 10 retries returned the original failure, Rattle
 * gave up, and returned an invalid diagram. Downstream that surfaced as a
 * 12-component link being reported as a single unknot, "proven minimal".
 *
 * The test does not need a degenerate diagram. It asserts the invariant
 * directly: computing the intersections twice on the SAME unchanged embedding
 * must produce identical counts, because the second call describes the same
 * single projection. Under the bug the second call returns doubled counts.
 *
 * 2026-11: Reapr now hands back the exact LinkEmbedding_Int (the float class
 * that had the bug was demoted to legacy/LinkEmbedding_Float). That class has
 * no flag counters -- simulation of simplicity leaves no degeneracies to count
 * -- but it keeps the same kind of per-projection bookkeeping:
 * IntersectionCount() and IntersectionCount3D(). The invariant is checked on
 * those, with RequireIntersections(true) forcing the recomputation.
 *
 * Build: see test/Makefile (target: intersection_flag_reset_check).
 */

#include "../Knoodle.hpp"

#include <cstdint>
#include <iostream>
#include <string>

using Int     = std::int64_t;
using Real    = double;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int>;

namespace {

int failures = 0;

void Check(bool ok, const std::string& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok) { ++failures; }
}

// An 8-crossing connected diagram of the 2-component unlink (borrowed from
// component_check.cpp, where it is already a known-good fixture). Any diagram
// with real crossings will do: the invariant under test is counter bookkeeping,
// not geometry.
const Int kPD[] = {
     6,  0,  7,  9,  1,
     0, 10,  1, 15,  1,
     1, 12,  2, 13, -1,
    11,  2, 12,  3, -1,
    10,  4, 11,  3,  1,
    13,  4, 14,  5, -1,
    14,  6, 15,  5,  1,
     7,  8,  8,  9, -1,
};

} // namespace

int main()
{
    std::cout << "intersection_flag_reset_check\n";

    const Int n = static_cast<Int>(sizeof(kPD) / sizeof(kPD[0]) / 5);
    PD_T pd = PD_T::FromSignedPDCode(kPD, n, false, true);
    if (pd.InvalidQ())
    {
        std::cout << "FAIL: could not build the test diagram\n";
        return 1;
    }

    PDC_T::Simplify_Args_T sargs;
    Reapr_T reapr ({
        .permute_randomQ     = sargs.permute_randomQ,
        .energy              = sargs.energy,
        .ortho_draw_settings = {
            .randomize_bends          = sargs.randomize_bends,
            .randomize_virtual_edgesQ = sargs.randomize_virtual_edgesQ,
            .compaction_method        = sargs.compaction_method
        },
        .scaling             = sargs.scaling
    });

    auto emb = reapr.Embedding(pd, reapr.RandomRotation());

    // First projection.
    const int  flag1    = emb.RequireIntersections(true);
    const Int  count1   = emb.IntersectionCount();
    const Int  count3d1 = emb.IntersectionCount3D();

    // Same embedding, untouched: the forced recomputation describes the same
    // projection, so it must produce the same counts -- not counts twice as large.
    const int  flag2    = emb.RequireIntersections(true);
    const Int  count2   = emb.IntersectionCount();
    const Int  count3d2 = emb.IntersectionCount3D();

    Check(flag1 == 0, "the test embedding projects without error");
    Check(flag1 == flag2, "repeating the intersection computation gives the same status flag");

    const long total = static_cast<long>(count1);
    const bool same  = (count1 == count2) && (count3d1 == count3d2);
    if (!same)
    {
        std::cout << "    first call: " << count1 << " crossings, " << count3d1
                  << " 3D intersections; second call: " << count2 << ", "
                  << count3d2 << "\n";
    }

    Check(total > 0, "the test diagram actually produced intersections to count");
    Check(same, "intersection counts describe one projection, not an accumulating total");

    std::cout << (failures == 0
                  ? "PASS: intersection counts reset per call\n"
                  : "FAIL: " + std::to_string(failures) + " check(s) failed\n");
    return (failures == 0) ? 0 : 1;
}
