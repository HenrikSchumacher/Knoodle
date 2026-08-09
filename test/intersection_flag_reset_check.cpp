/**
 * @file intersection_flag_reset_check.cpp
 * @brief Regression test: LinkEmbedding's intersection flag counters must
 *        describe ONE projection, not the lifetime of the embedding.
 *
 * The bug this pins: intersection_flag_counts was initialised once at
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
 * directly: running FindIntersections twice on the SAME unchanged embedding must
 * produce identical counters, because the second call describes the same single
 * projection. Under the bug the second call returns doubled counts.
 *
 * Build: see test/Makefile (target: intersection_flag_reset_check).
 */

#include "../Knoodle.hpp"

#include <cstdint>
#include <iostream>
#include <string>

using Int     = std::int64_t;
using Real    = double;
using BReal   = float;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, BReal>;

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
    const int flag1 = emb.template FindIntersections<false>();
    const auto counts1 = emb.IntersectionFlagCounts();

    // Same embedding, untouched: the second call describes the same projection,
    // so it must produce the same counters -- not counters twice as large.
    const int flag2 = emb.template FindIntersections<false>();
    const auto counts2 = emb.IntersectionFlagCounts();

    Check(flag1 == flag2, "repeating FindIntersections gives the same status flag");

    bool same = true;
    long total = 0;
    for (std::size_t i = 0; i < counts1.Size(); ++i)
    {
        total += static_cast<long>(counts1[i]);
        if (counts1[i] != counts2[i])
        {
            same = false;
            std::cout << "    flag " << i << ": first call " << counts1[i]
                      << ", second call " << counts2[i] << "\n";
        }
    }

    Check(total > 0, "the test diagram actually produced intersections to count");
    Check(same, "flag counters describe one projection, not an accumulating total");

    std::cout << (failures == 0
                  ? "PASS: intersection flag counters reset per call\n"
                  : "FAIL: " + std::to_string(failures) + " check(s) failed\n");
    return (failures == 0) ? 0 : 1;
}
