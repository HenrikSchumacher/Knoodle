/**
 * @file key_roundtrip_probe.cpp
 * @brief Probe for the gating question in docs/knoodleidentify-design.md:
 *        does a PD-code round-trip (as knoodleidentify will perform it)
 *        preserve the MacLeod key used by the KLUT?
 *
 * Stage 0: simplify a trefoil via PDC_T::Simplify (as knoodlesimplify does),
 *          look the summand up in the Klut, expect a K[3,1,1,...] name.
 * Stage 1: for every key in every Klut_Keys_NN.bin: key -> FromMacLeodCode
 *          -> WriteMacLeodCode -> key' ; count key != key'.
 * Stage 2: same, but with the knoodleidentify round-trip inserted:
 *          key -> FromMacLeodCode -> PDCode<{signQ,!colorQ}> matrix
 *          -> FromPDCode (signed, compressQ=true, as CreateDiagramFromPDCode
 *          case 5 does) -> WriteMacLeodCode -> key'.
 *
 * Build (from test/):
 *   clang++ -Wall -Wextra -std=c++20 -O3 -march=native -fenable-matrix \
 *     -pthread -I$HOMEBREW_PREFIX/include -I../submodules/Tensors \
 *     key_roundtrip_probe.cpp -o key_roundtrip_probe
 * Run:
 *   ./key_roundtrip_probe ../data/Klut
 */

#include "../Knoodle.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using Int   = std::int64_t;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;
using PD_T  = PDC_T::PD_T;
using Klut  = Knoodle::Klut;

namespace {

// The exact reconstruction knoodleidentify will perform on 5-column input
// (mirrors tools/knoodle_io.hpp CreateDiagramFromPDCode, case 5).
PD_T RoundTripThroughPDCode(const PD_T& pd)
{
    auto pd_code = pd.template PDCode<Int, {.signQ = true, .colorQ = false}>();

    return PD_T::template FromPDCode<{.signQ = true, .colorQ = false}>(
        pd_code.data(), pd.CrossingCount(), false, true
    );
}

std::vector<Knoodle::UInt8> MacLeodOf(const PD_T& pd)
{
    std::vector<Knoodle::UInt8> code(static_cast<std::size_t>(pd.CrossingCount()));
    pd.template WriteMacLeodCode<Knoodle::UInt8>(code.data());
    return code;
}

} // namespace

int main(int argc, char** argv)
{
    const std::filesystem::path data_dir = (argc > 1) ? argv[1] : "../data/Klut";

    Klut klut(data_dir);

    //==========================================================================
    // Stage 0: simplified trefoil should be found and named.
    //==========================================================================
    {
        // Right-handed trefoil, 0-based arcs (same as test/run_tests.py).
        std::vector<Int> trefoil = {
            0, 4, 1, 3, 1,
            2, 0, 3, 5, 1,
            4, 2, 5, 1, 1,
        };

        PD_T pd = PD_T::FromSignedPDCode(trefoil.data(), Int(3), false, true);

        PDC_T pdc(std::move(pd));
        PDC_T::Simplify_Args_T args;
        args.splitQ = true;
        pdc.Simplify(args);

        if (pdc.DiagramCount() != 1)
        {
            std::cout << "Stage 0: FAIL (expected 1 summand, got "
                      << pdc.DiagramCount() << ")\n";
            return 1;
        }

        PD_T summand(pdc.Diagram(0));
        std::string direct = klut.FindName(summand);
        std::string rt     = klut.FindName(RoundTripThroughPDCode(summand));

        std::cout << "Stage 0: simplified trefoil -> direct lookup: " << direct
                  << ", after PD-code round-trip: " << rt << "\n";
        if (direct == "NotFound" || direct == "Invalid" || direct != rt)
        {
            std::cout << "Stage 0: FAIL\n";
        }
    }

    //==========================================================================
    // Stages 1 & 2: every key in the table.
    //==========================================================================
    std::size_t total = 0, stage1_bad = 0, stage2_bad = 0, invalid_pd = 0;

    for (std::size_t c = 3; c <= Klut::max_crossing_count; ++c)
    {
        std::ifstream stream(klut.KeyFile(c), std::ios::binary);
        if (!stream)
        {
            std::cout << "(skipping c = " << c << ": cannot open key file)\n";
            continue;
        }

        std::size_t n = 0, bad1 = 0, bad2 = 0;
        std::vector<Knoodle::UInt8> key(c);

        while (stream.read(reinterpret_cast<char*>(key.data()),
                           static_cast<std::streamsize>(c)))
        {
            ++n;

            PD_T pd = PD_T::FromMacLeodCode(key.data(), Int(c), Int(0));
            if (pd.InvalidQ()) { ++invalid_pd; continue; }

            if (MacLeodOf(pd) != key) { ++bad1; }

            PD_T pd2 = RoundTripThroughPDCode(pd);
            if (pd2.InvalidQ() || MacLeodOf(pd2) != key) { ++bad2; }
        }

        std::cout << "c = " << c << ": " << n << " keys, stage-1 mismatches: "
                  << bad1 << ", stage-2 mismatches: " << bad2 << "\n";

        total      += n;
        stage1_bad += bad1;
        stage2_bad += bad2;
    }

    std::cout << "\nTotal keys: " << total
              << "\n  invalid reconstructions: " << invalid_pd
              << "\n  stage-1 (key->PD->key) mismatches:           " << stage1_bad
              << "\n  stage-2 (key->PD->text-code->PD->key) mismatches: " << stage2_bad
              << "\n\nVerdict: "
              << ((stage1_bad == 0 && stage2_bad == 0 && invalid_pd == 0)
                      ? "round-trip is key-stable; knoodleidentify can rely on it"
                      : "round-trip is NOT key-stable; knoodleidentify must canonicalize after reading")
              << "\n";

    return (stage1_bad == 0 && stage2_bad == 0 && invalid_pd == 0) ? 0 : 2;
}
