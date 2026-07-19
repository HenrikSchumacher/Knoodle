/**
 * @file link_alex_probe.cpp
 * @brief Prototype/validation for a single-variable Alexander |det| fingerprint
 *        that works for LINKS (where Alexander_UMFPACK::Alexander deliberately
 *        bails out).
 *
 * Why this works where the production value routine doesn't:
 *   Alexander_UMFPACK normalizes the raw strand-matrix determinant by dividing
 *   by its value at t = 1. For a knot Delta(1) = +/-1, so that's fine. For a
 *   mu-component link (mu >= 2) the reduced strand matrix degenerates at t = 1
 *   to the incidence matrix of mu disjoint strand-cycles (rank n - mu), so
 *   det(M(1)) = 0 and the normalization divides by zero.
 *
 *   But we don't need that normalization. The raw determinant of the reduced
 *   (n-1)x(n-1) strand matrix equals  +/- t^k * Delta_L(t)  -- the link's
 *   one-variable Alexander polynomial up to a unit. Evaluated on the UNIT
 *   CIRCLE (|t| = 1) the unit factor has modulus 1, so |det(M(t))| is a genuine
 *   diagram-independent invariant. We fingerprint log10|det| at a handful of
 *   irrational-angle points on the circle (avoiding roots of unity / common
 *   Alexander roots).
 *
 * This probe just validates the idea:
 *   (1) the fingerprint is preserved across Reapr embed+reproject (invariance), and
 *   (2) distinct links get distinct fingerprints (discrimination).
 * If it holds up, link_inflate_check.cpp uses it as the inflate tripwire.
 *
 * Build: see test/Makefile (target: link_alex_probe). Needs UMFPACK + Accelerate.
 */

#include "../Knoodle.hpp"
#include "link_alexander.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Int        = std::int64_t;
using Real       = double;
using Cplx       = std::complex<double>;
using PDC_T      = Knoodle::PlanarDiagramComplex<Int>;
using PD_T       = PDC_T::PD_T;
using Reapr_T    = Knoodle::Reapr<Real, Int, float>;
using LinkAlex_T = knoodle_test::LinkAlexander<Cplx, Int>;
using LinkFP     = LinkAlex_T::Value;

namespace {

const LinkAlex_T kLinkAlex{};

LinkFP LinkAlexander(const PD_T& pd) { return kLinkAlex(pd); }
bool   FPEqual(const LinkFP& a, const LinkFP& b) { return LinkAlex_T::Equal(a, b); }
std::string FPToString(const LinkFP& f) { return LinkAlex_T::ToString(f); }

// Load a 5-col signed PD code (one crossing per row) -> PD_T. Handles links.
PD_T LoadSignedPD(const std::string& path)
{
    std::ifstream in(path);
    std::vector<Int> ints;
    Int v;
    while (in >> v) { ints.push_back(v); }
    if (ints.empty() || ints.size() % 5 != 0) { return PD_T::InvalidDiagram(); }
    return PD_T::FromSignedPDCode(
        ints.data(), static_cast<Int>(ints.size() / 5), false, true);
}

std::uint64_t SplitMix64(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// One reproject round: Reapr embed -> random rotate -> back to PD. Returns an
// invalid diagram if the projection split or became multi-diagram.
PD_T Reproject(const PD_T& pd, Reapr_T& reapr, std::uint64_t seed)
{
    reapr.RandomEngine() = Knoodle::PRNG_T(seed);
    auto emb = reapr.Embedding(pd);
    emb.Rotate(reapr.RandomRotation());
    auto [pd_new, unlinks] = PD_T::FromLinkEmbedding(emb);
    if (!pd_new.ValidQ()) { return PD_T::InvalidDiagram(); }
    return pd_new;
}

// Validate the fingerprint on one link: invariance across several reprojections.
bool ProbeLink(const std::string& label, PD_T pd, int rounds = 6)
{
    if (!pd.ValidQ())
    {
        std::cout << "  " << label << ": INVALID diagram, skipping\n";
        return false;
    }

    std::cout << "  " << label
              << ": crossings=" << pd.CrossingCount()
              << " components=" << pd.LinkComponentCount() << "\n";

    const LinkFP fp0 = LinkAlexander(pd);
    if (!fp0.ok)
    {
        std::cout << "    FAIL: base fingerprint invalid (singular?)\n";
        return false;
    }
    std::cout << "    fp = [" << FPToString(fp0) << "]\n";

    Reapr_T reapr{};
    PD_T cur(pd);
    bool all_ok = true;
    for (int r = 1; r <= rounds; ++r)
    {
        const std::uint64_t seed = SplitMix64(0xABCDEF ^ static_cast<std::uint64_t>(r));
        PD_T next = Reproject(cur, reapr, seed);
        if (!next.ValidQ())
        {
            std::cout << "    round " << r << ": reproject produced invalid; skipping\n";
            continue;
        }
        const LinkFP fp = LinkAlexander(next);
        const bool eq = FPEqual(fp, fp0);
        std::cout << "    round " << r << ": crossings=" << next.CrossingCount()
                  << " components=" << next.LinkComponentCount()
                  << (eq ? "  MATCH" : "  *** MISMATCH ***") << "\n";
        if (!eq)
        {
            std::cout << "      base : [" << FPToString(fp0) << "]\n"
                      << "      round: [" << FPToString(fp)  << "]\n";
            all_ok = false;
        }
        cur = std::move(next);
    }
    return all_ok;
}

} // namespace

int main(int argc, char* argv[])
{
    std::string linktable = "../data/diagrams/linktable";
    std::vector<std::string> names;   // bare names like L2a1_0
    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        if (a.rfind("--linktable=", 0) == 0) { linktable = a.substr(12); }
        else { names.push_back(a); }
    }
    if (names.empty())
    {
        names = {"L2a1_0", "L4a1_0", "L5a1_0", "L6a4_0", "L6n1_0"};
    }

    std::cout << "link_alex_probe: validating single-variable |det| link fingerprint\n"
              << "  linktable dir: " << linktable << "\n\n";

    int fails = 0;
    std::vector<LinkFP> fps;
    std::vector<std::string> got_names;

    for (const auto& nm : names)
    {
        const std::string path = linktable + "/" + nm + ".tsv";
        PD_T pd = LoadSignedPD(path);
        std::cout << nm << " (" << path << ")\n";
        if (!ProbeLink(nm, pd)) { ++fails; }

        // Stash base fingerprint for the discrimination check.
        if (pd.ValidQ())
        {
            LinkFP fp = LinkAlexander(pd);
            if (fp.ok) { fps.push_back(fp); got_names.push_back(nm); }
        }
        std::cout << "\n";
    }

    // Discrimination: distinct links should (usually) get distinct fingerprints.
    std::cout << "discrimination check (distinct links -> distinct fingerprints):\n";
    int collisions = 0;
    for (std::size_t i = 0; i < fps.size(); ++i)
    {
        for (std::size_t j = i + 1; j < fps.size(); ++j)
        {
            if (FPEqual(fps[i], fps[j]))
            {
                std::cout << "  collision: " << got_names[i] << " == " << got_names[j] << "\n";
                ++collisions;
            }
        }
    }
    std::cout << "  " << collisions << " collision(s) among "
              << fps.size() << " links\n\n";

    std::cout << "link_alex_probe: " << (static_cast<int>(names.size()) - fails)
              << "/" << names.size() << " links passed invariance\n";
    return fails == 0 ? 0 : 1;
}
