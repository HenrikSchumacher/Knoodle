// component_check — regression guard for the CollapseArcRange unlink-loss bug
// (fixed by Henrik's 5151f39). The embedded diagram is a connected 8-crossing
// diagram of the 2-component UNLINK, minimized from the n=9 plantri sweep
// (2026-06-15); pre-fix Simplify fused its two unlinked circles into a single
// unknot (dropping a component, changing HOMFLY). Run default Simplify N times
// and fail if the link-component count ever changes. Exit 0 = preserved.
#include "../Knoodle.hpp"
#include <cstdlib>
#include <iostream>

using Int   = std::int64_t;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;
using PD_T  = PDC_T::PD_T;

// 8-crossing connected diagram of the 2-component unlink (5-col signed PD).
static const Int kRepro[] = {
     6,  0,  7,  9,  1,
     0, 10,  1, 15,  1,
     1, 12,  2, 13, -1,
    11,  2, 12,  3, -1,
    10,  4, 11,  3,  1,
    13,  4, 14,  5, -1,
    14,  6, 15,  5,  1,
     7,  8,  8,  9, -1,
};

int main(int argc, char** argv)
{
    const Int n = static_cast<Int>(sizeof(kRepro) / sizeof(kRepro[0]) / 5);
    const int N = (argc > 1) ? std::atoi(argv[1]) : 200;

    Int before = 0; int lost = 0;
    for (int t = 0; t < N; ++t)
    {
        PD_T pd = PD_T::FromSignedPDCode(kRepro, n, false, true);
        before = pd.LinkComponentCount();
        PDC_T pdc{ std::move(pd) };
        pdc.Simplify(PDC_T::Simplify_Args_T{});             // default args (pass-only)
        Int after = 0;
        for (Int i = 0; i < (pdc.ValidQ() ? pdc.DiagramCount() : Int(0)); ++i)
            after += pdc.Diagram(i).LinkComponentCount();
        if (after != before) { ++lost; }
    }
    std::cout << "component_check: input components=" << before << "  runs=" << N
              << "  component-loss=" << lost << "\n";
    if (lost == 0) { std::cout << "PASS: Simplify preserved link-component count\n"; return 0; }
    std::cout << "FAIL: Simplify changed component count (CollapseArcRange regression?)\n";
    return 1;
}
