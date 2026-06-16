// Lock the hot-path optimization: does declining the final Canonicalize change
// the looked-up MacLeod code / KLUT id? For sampled KLUT minimals, perturb each
// into a non-minimal diagram (one Reapr reproject), then pass-only Simplify it
// TWICE with deterministic pass order (permute_randomQ=false) — once with
// canonicalizeQ=true, once false — and compare the resulting MacLeod code + id.
// If they match for every diagram, WriteMacLeodCode is canonical-by-construction
// and skipping Canonicalize on the hot path is safe.
#include "../Knoodle.hpp"
#include <algorithm>
#include <array>
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
using CodeInt = Klut::CodeInt;

static std::uint64_t mix(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// Pass-only Simplify with the given canonicalizeQ (deterministic pass order).
// Returns false unless the result is a single 3..13-crossing knot; else fills
// the MacLeod code, id, and crossing count.
static bool reduce(const std::vector<Int>& code, bool canon, Klut& klut,
                   std::array<CodeInt, Klut::max_crossing_count>& mc,
                   Klut::ID_T& id, Int& c)
{
    PD_T pd = PD_T::FromSignedPDCode(code.data(),
                  static_cast<Int>(code.size() / 5), false, true);
    PDC_T pdc{ std::move(pd) };
    PDC_T::Simplify_Args_T a{};
    a.embedding_trials = 0;
    a.permute_randomQ  = false;
    a.canonicalizeQ    = canon;
    pdc.Simplify(a);
    if (!pdc.ValidQ() || pdc.DiagramCount() != Int(1)) { return false; }
    PD_T s = pdc.Diagram(0);   // copy (probe only) so we can call non-const WriteMacLeodCode
    c = s.CrossingCount();
    if (c < Int(3) || c > static_cast<Int>(Klut::max_crossing_count)) { return false; }
    s.template WriteMacLeodCode<CodeInt>(mc.data());
    auto [cc, i] = klut.FindID(mc.data(), c);
    (void)cc; id = i;
    return true;
}

int main(int argc, char** argv)
{
    std::string dir = "../data/Klut";
    Int c_max = 13, per_c = 60;
    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        if      (a.rfind("--klut-dir=", 0) == 0) dir   = a.substr(11);
        else if (a.rfind("--per-c=", 0)    == 0) per_c = std::stoll(a.substr(8));
        else if (a.rfind("--c-max=", 0)    == 0) c_max = std::stoll(a.substr(8));
    }

    Klut klut{ std::filesystem::path(dir), static_cast<Knoodle::Size_T>(c_max) };
    klut.LoadSubtables();
    Reapr_T reapr{};

    auto found = [](Klut::ID_T id) {
        return id != Klut::not_found && id != Klut::error && id != Klut::invalid;
    };
    // The optimization is safe iff declining Canonicalize never turns a lookup HIT
    // into a miss or a different knot. Byte-different codes are fine as long as they
    // resolve to the same id (the table holds many codes per knot).
    long tested = 0, id_match = 0, code_diff = 0, regression = 0;
    for (Int c = 3; c <= c_max; ++c)
    {
        const std::string path = dir + "/Klut_Keys_"
                               + (c < 10 ? "0" : "") + std::to_string(c) + ".bin";
        std::ifstream f(path, std::ios::binary);
        if (!f) { continue; }
        for (Int k = 0; k < per_c; ++k)
        {
            std::vector<Knoodle::UInt8> key(static_cast<std::size_t>(c));
            f.seekg(static_cast<std::streamoff>(k) * c);
            if (!f.read(reinterpret_cast<char*>(key.data()), c)) { break; }
            PD_T m = PD_T::FromMacLeodCode(key.data(), Int(c), Int(0));
            if (!m.ValidQ()) { continue; }

            // perturb into a non-minimal diagram of the same knot
            reapr.RandomEngine() = Knoodle::PRNG_T(mix(static_cast<std::uint64_t>(c * 1000 + k)));
            auto emb = reapr.Embedding(m);
            emb.Rotate(reapr.RandomRotation());
            auto [P, unlinks] = PD_T::FromLinkEmbedding(emb);
            if (!P.ValidQ() || unlinks.Size() > Int(0)
                || P.LinkComponentCount() > Int(1)
                || P.DiagramComponentCount() > Int(1)) { continue; }

            auto pc = P.template PDCode<Int, {.signQ = true, .colorQ = false}>();
            const Int nP = P.CrossingCount();
            std::vector<Int> pcode; pcode.reserve(static_cast<std::size_t>(5 * nP));
            for (Int i = 0; i < nP; ++i) for (int j = 0; j < 5; ++j) pcode.push_back(pc(i, j));

            std::array<CodeInt, Klut::max_crossing_count> mc_off{}, mc_on{};
            Klut::ID_T id_off, id_on; Int c_off, c_on;
            if (!reduce(pcode, false, klut, mc_off, id_off, c_off)) { continue; }
            if (!reduce(pcode, true,  klut, mc_on,  id_on,  c_on )) { continue; }
            ++tested;

            const bool cm = (c_off == c_on)
                && std::equal(mc_off.begin(), mc_off.begin() + c_off, mc_on.begin());
            if (!cm) { ++code_diff; }
            if (id_off == id_on) { ++id_match; }
            // Regression = canon-on found a knot but canon-off missed it / got a
            // different one. This is the only thing that would break the hot path.
            if (found(id_on) && id_off != id_on)
            {
                ++regression;
                if (regression <= 5)
                    std::cout << "  REGRESSION c_off=" << c_off << " c_on=" << c_on
                              << " id_off=" << id_off << " id_on=" << id_on
                              << "  (canon-on hit, canon-off didn't match)\n";
            }
        }
    }
    std::cout << "canon on/off: tested=" << tested << "  id_match=" << id_match
              << "  code_diff=" << code_diff << " (benign: same id)"
              << "  regressions=" << regression << "\n";
    std::cout << (regression == 0
                  ? "PASS: declining Canonicalize never turns a lookup hit into a miss/mismatch\n"
                  : "*** FAIL: canon-off loses or changes a lookup — hot-path skip NOT safe ***\n");
    return regression == 0 ? 0 : 1;
}
