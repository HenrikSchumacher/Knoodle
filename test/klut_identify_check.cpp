// PR test for the KLUT Identify routine (klut_identify.hpp / docs/klut-identify-design.md).
// Cases:
//   (1) single knots          : perturb each KLUT minimal -> Identify -> the source id.
//   (2) connect sums          : K1#K2 (and K#K, triples) as BOTH a multi-diagram PDC
//                               and a single spliced diagram (ToSingleDiagram) ->
//                               Identify -> the exact multiset of summand ids. The
//                               single-diagram form exercises decomposition during
//                               the seed pass and/or the Reapr escalation split.
//   (3) the unknot            : -> Knot status, no summands.
//   (4) a multi-component link : two different-colored knots -> LinkOutOfScope.
#include "../Knoodle.hpp"
#include "../tools/klut_identify.hpp"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using Real    = double;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, float>;
using Klut    = Knoodle::Klut;
using Key     = std::vector<Knoodle::UInt8>;
namespace ki  = klut_identify;

static std::uint64_t mix(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

static std::string g_dir = "../data/Klut";

// k-th stored key for crossing number c (empty if absent).
static Key ReadKey(Int c, Int k)
{
    const std::string path = g_dir + "/Klut_Keys_"
                           + (c < 10 ? "0" : "") + std::to_string(c) + ".bin";
    std::ifstream f(path, std::ios::binary);
    if (!f) { return {}; }
    Key key(static_cast<std::size_t>(c));
    f.seekg(static_cast<std::streamoff>(k) * c);
    if (!f.read(reinterpret_cast<char*>(key.data()), c)) { return {}; }
    return key;
}

static PD_T FromKey(const Key& key, Int color = 0)
{
    return PD_T::FromMacLeodCode(key.data(), static_cast<Int>(key.size()), color);
}

// The ids of an Identified result, sorted (for order-independent multiset compare).
static std::vector<Klut::ID_T> SortedIds(const ki::IdentifyResult& r, bool& all_identified)
{
    std::vector<Klut::ID_T> ids;
    all_identified = true;
    for (const auto& s : r.summands)
    {
        if (s.kind != ki::Summand::Kind::Identified) { all_identified = false; }
        ids.push_back(s.id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

int main(int argc, char** argv)
{
    Int c_max = 13, per_c = 30;
    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        if      (a.rfind("--per-c=", 0)    == 0) per_c = std::stoll(a.substr(8));
        else if (a.rfind("--c-max=", 0)    == 0) c_max = std::stoll(a.substr(8));
        else if (a.rfind("--klut-dir=", 0) == 0) g_dir = a.substr(11);
    }

    Klut klut{ std::filesystem::path(g_dir), static_cast<Knoodle::Size_T>(c_max) };
    klut.LoadSubtables();
    Reapr_T reapr{};
    auto idOf = [&](const Key& key) { auto [c, id] = klut.FindID(FromKey(key)); (void)c; return id; };

    long fails = 0;

    // ---- (1) single knots: perturb each minimal, expect its source id ----------
    {
        long tested = 0, ok = 0, escalated = 0, total_reapr = 0;
        for (Int c = 3; c <= c_max; ++c)
            for (Int k = 0; k < per_c; ++k)
            {
                Key key = ReadKey(c, k);
                if (key.empty()) { break; }
                PD_T m = FromKey(key);
                if (!m.ValidQ()) { continue; }
                const Klut::ID_T src = idOf(key);

                reapr.RandomEngine() = Knoodle::PRNG_T(mix(static_cast<std::uint64_t>(c * 1000 + k)));
                auto emb = reapr.Embedding(m);
                emb.Transform( reapr.RandomRotation() );
                auto [P, unlinks] = PD_T::FromLinkEmbedding(emb);
                if (!P.ValidQ() || unlinks.Size() > Int(0)
                    || P.LinkComponentCount() > Int(1) || P.DiagramComponentCount() > Int(1)) { continue; }

                PDC_T pdc; pdc.Push(std::move(P));
                auto r = ki::Identify(klut, std::move(pdc), reapr);
                ++tested;
                if (r.reapr_calls > 0) { ++escalated; total_reapr += static_cast<long>(r.reapr_calls); }
                bool ai; auto ids = SortedIds(r, ai);
                if (r.status == ki::IdentifyResult::Status::Knot && ai
                    && ids.size() == 1 && ids[0] == src) { ++ok; }
            }
        const bool pass = (ok == tested && tested > 0);
        if (!pass) ++fails;
        std::cout << "(1) single knots          : " << ok << "/" << tested
                  << (pass ? "  PASS" : "  FAIL")
                  << "  [escalated " << escalated << " (" << total_reapr << " reapr calls); "
                  << (tested - escalated) << " hot-path]\n";
    }

    // ---- helper: identify a connect sum (multi-diagram PDC + single spliced PD) -
    auto check_connect_sum = [&](const std::vector<Key>& keys, const char* label) -> bool
    {
        std::vector<Klut::ID_T> expect;
        for (const auto& key : keys) { expect.push_back(idOf(key)); }
        std::sort(expect.begin(), expect.end());

        // form A: a multi-diagram PDC (all same color = one component = connect sum)
        PDC_T csA;
        for (const auto& key : keys) { csA.Push(FromKey(key, Int(0))); }
        bool aiA; auto idsA = SortedIds(ki::Identify(klut, std::move(csA), reapr), aiA);

        // form B: the single spliced diagram (farfalle), must decompose under Identify
        PDC_T tmp;
        for (const auto& key : keys) { tmp.Push(FromKey(key, Int(0))); }
        PDC_T csB; csB.Push(tmp.ToSingleDiagram());
        bool aiB; auto idsB = SortedIds(ki::Identify(klut, std::move(csB), reapr), aiB);

        const bool okA = aiA && idsA == expect;
        const bool okB = aiB && idsB == expect;
        std::cout << "    " << label << " : multi-diagram " << (okA ? "ok" : "FAIL")
                  << " | single-diagram " << (okB ? "ok" : "FAIL") << "\n";
        return okA && okB;
    };

    // ---- (2) connect sums ------------------------------------------------------
    std::cout << "(2) connect sums:\n";
    {
        Key k3 = ReadKey(3, 0), k4 = ReadKey(4, 0), k5 = ReadKey(5, 0), k5b = ReadKey(5, 1);
        bool pass = true;
        if (!k3.empty() && !k4.empty()) pass &= check_connect_sum({k3, k4},      "3_1 # 4_1");
        if (!k3.empty())                pass &= check_connect_sum({k3, k3},      "3_1 # 3_1 (multiplicity)");
        if (!k5.empty() && !k5b.empty())pass &= check_connect_sum({k5, k5b},     "5_1 # 5_2");
        if (!k3.empty() && !k4.empty() && !k5.empty())
                                        pass &= check_connect_sum({k3, k4, k5},  "3_1 # 4_1 # 5_1 (triple)");
        if (!pass) ++fails;
    }

    // ---- (3) the unknot --------------------------------------------------------
    {
        PDC_T pdc; pdc.Push(PD_T::Unknot(Int(0)));
        auto r = ki::Identify(klut, std::move(pdc), reapr);
        const bool pass = (r.status == ki::IdentifyResult::Status::Knot && r.summands.empty());
        if (!pass) ++fails;
        std::cout << "(3) unknot -> empty knot  : " << (pass ? "PASS\n" : "FAIL\n");
    }

    // ---- (4) multi-component link out of scope ---------------------------------
    {
        Key k3 = ReadKey(3, 0);
        PDC_T pdc; pdc.Push(FromKey(k3, Int(0))); pdc.Push(FromKey(k3, Int(1)));  // 2 colors = link
        auto r = ki::Identify(klut, std::move(pdc), reapr);
        const bool pass = (r.status == ki::IdentifyResult::Status::LinkOutOfScope);
        if (!pass) ++fails;
        std::cout << "(4) link -> out of scope  : " << (pass ? "PASS\n" : "FAIL\n");
    }

    std::cout << (fails == 0 ? "\nPASS: Identify handles single knots, connect sums, unknot, and links.\n"
                             : "\n*** " + std::to_string(fails) + " case group(s) FAILED ***\n");
    return fails == 0 ? 0 : 1;
}
