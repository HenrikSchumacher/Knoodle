/**
 * @file reapr_corner_probe.cpp
 * @brief Reproduce and diagnose the FindIntersections corner-degeneracy failure
 *        that makes Rattle bail with "returned invalid status flag for 10 random
 *        rotation matrices".
 *
 * Symptom (reported by a user, knoodlesimplify --max-reapr-attempts=200
 * --reapr-rotation-trials=200): a run emits
 *
 *   WARNING: ...ComputeEdgeIntersection: Edges 300 and 756 have common first corners.
 *   WARNING: ...FindIntersections: Detected 1 cases where the line-line
 *            intersection was a point in the corners of two line segments.
 *            Try to randomly rotate the input coordinates.          [x11]
 *   ERROR:   ...Rattle: ...FindIntersections returned invalid status flag for
 *            10 random rotation matrices. Something must be wrong.
 *
 * and returns an invalid diagram.
 *
 * The hypothesis this probe tests: CornerCorner (flag 4) is a *planar* test —
 * per Prosector_Float.hpp it fires when two edges' tails coincide
 * in the PROJECTION. Rattle's recovery is to re-randomize the rotation up to
 * max_projection_iter = 10 times. That recovery works only if the coincidence
 * is an artifact of the projection direction. If the two tails coincide in 3D,
 * every projection inherits the coincidence, the retry loop is futile, and the
 * bail-out is guaranteed rather than unlucky.
 *
 * So the probe asks two questions per embedding:
 *   (1) does the embedding contain two distinct vertices at the same 3D point?
 *   (2) does RequireIntersections keep returning 4 no matter how we rotate?
 * If (1) and (2) coincide, the defect is in the embedding, not the projection,
 * and no amount of rotating can fix it.
 *
 * Usage: ./reapr_corner_probe FILE.tsv [embeddings] [rotations]
 *   FILE.tsv    5-column signed PD code ('k'/'s'/'u' marker lines tolerated)
 * Exit 0 if no failure was observed, 1 if the failure reproduced.
 *
 * Build: see test/Makefile (target: reapr_corner_probe).
 *
 * STATUS (2026-08-13). The proximate bug this was written to chase has since
 * been fixed: the flag counters were never reset between projections, so a
 * single early CornerCorner kept every subsequent rotation looking failed and
 * Rattle's retry loop could not win. The reset is now at
 * src/LinkEmbedding/FindIntersections.hpp:241 and is pinned by
 * test/intersection_flag_reset_check.cpp.
 *
 * The probe is kept as a standing diagnostic for this failure class, not as a
 * live investigation. Its question -- "is this coincidence in the projection,
 * where rotating can help, or in 3-space, where it cannot?" -- is the right
 * first question whenever a Rattle bail-out shows up again, and it answers it
 * over a whole population of re-embeddings rather than one.
 *
 * Related, and a different layer: test/embedding_check.cpp settles the same
 * projection-vs-3-space question *exactly*, for a fixed curve, by reproducing
 * Prosector's symbolic perturbation as an integer shear; its census reports
 * coincident vertices and 3D intersections directly. Use that for "is this
 * curve degenerate and does the class handle it", and this probe for "can
 * Rattle's rotate-and-retry recovery escape".
 */

#include "../Knoodle.hpp"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using Real    = double;
using BReal   = float;
using PDC_T   = Knoodle::PlanarDiagramComplex<Int>;
using PD_T    = PDC_T::PD_T;
using Reapr_T = Knoodle::Reapr<Real, Int, BReal>;

namespace {

PD_T LoadPD(const std::string& path, bool& ok)
{
    ok = false;
    std::ifstream in(path);
    if (!in) { std::cerr << "cannot open " << path << "\n"; return PD_T::InvalidDiagram(); }

    std::vector<Int> ints;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) { continue; }
        const char c = line[0];
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+')) { continue; }
        std::istringstream ls(line);
        Int v;
        while (ls >> v) { ints.push_back(v); }
    }
    if (ints.empty() || ints.size() % 5 != 0)
    {
        std::cerr << path << ": expected a multiple of 5 integers, got " << ints.size() << "\n";
        return PD_T::InvalidDiagram();
    }
    ok = true;
    return PD_T::FromSignedPDCode(ints.data(), static_cast<Int>(ints.size() / 5), false, true);
}

/// Exact-duplicate 3D vertices in an embedding. Exact equality is the right
/// test: these come off a grid, and it is exact coincidence (not proximity)
/// that no rotation can separate.
struct DupReport
{
    Int  pairs = 0;
    std::vector<std::array<Real,3>> points;   // the coincident locations
};

DupReport FindDuplicateVertices(const std::vector<Real>& v, Int n)
{
    DupReport r;
    std::map<std::array<Real,3>, std::vector<Int>> at;
    for (Int i = 0; i < n; ++i)
    {
        at[{ v[3*i], v[3*i+1], v[3*i+2] }].push_back(i);
    }
    for (const auto& [pt, idx] : at)
    {
        if (idx.size() > 1)
        {
            r.pairs += static_cast<Int>(idx.size()) - 1;
            r.points.push_back(pt);
            std::cout << "    coincident 3D vertices at ("
                      << pt[0] << ", " << pt[1] << ", " << pt[2] << "): indices";
            for (Int i : idx) { std::cout << " " << i; }
            std::cout << "\n";
        }
    }
    return r;
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: reapr_corner_probe FILE.tsv [embeddings] [rotations]\n";
        return 2;
    }
    const long n_emb = (argc > 2) ? std::stol(argv[2]) : 200;
    const long n_rot = (argc > 3) ? std::stol(argv[3]) : 25;

    bool ok = false;
    PD_T pd = LoadPD(argv[1], ok);
    if (!ok) { return 2; }

    std::cout << "input: " << pd.CrossingCount() << " crossings, "
              << pd.LinkComponentCount() << " component(s)\n"
              << "probing " << n_emb << " embeddings x " << n_rot << " rotations\n\n";

    // Mirror how PlanarDiagramComplex::Simplify builds its Reapr. This matters:
    // OrthoDraw's own defaults are randomize_bends = 0 / randomize_virtual_edgesQ
    // = false, while Simplify_Args_T passes 2 / true, so a default-constructed
    // Reapr generates a MORE REGULAR embedding than any real run and misses the
    // randomized geometry where the degeneracy shows up.
    PDC_T::Simplify_Args_T sargs;   // the defaults knoodlesimplify starts from
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

    long  bad_embeddings = 0, total_flag4 = 0, dup_embeddings = 0;
    Int   min_edges = std::numeric_limits<Int>::max(), max_edges = 0;
    std::map<int,long> flag_counts;

    for (long e = 0; e < n_emb; ++e)
    {
        // Fresh embedding, exactly as Rattle builds one.
        auto emb = reapr.Embedding(pd, reapr.RandomRotation());

        const Int n = emb.EdgeCount();
        // Edge indices in a reported warning must be < max seen here, or the
        // reporter's diagram is not the one we are holding.
        if (n < min_edges) { min_edges = n; }
        if (n > max_edges) { max_edges = n; }
        std::vector<Real> vcoords(static_cast<std::size_t>(3 * n));
        emb.WriteVertexCoordinates(vcoords.data());

        // (1) Does this embedding have coincident vertices in 3D?
        bool dupQ = false;
        {
            std::map<std::array<Real,3>, int> seen;
            for (Int i = 0; i < n && !dupQ; ++i)
            {
                if (++seen[{ vcoords[3*i], vcoords[3*i+1], vcoords[3*i+2] }] > 1) { dupQ = true; }
            }
        }

        // (2) Rotate like Rattle does and watch the flag.
        long flag4_here = 0;
        bool stuckQ = false;
        for (long r = 0; r < n_rot; ++r)
        {
            emb.Transform(reapr.RandomRotation());
            const int flag = emb.template RequireIntersections<false>();  // quiet
            ++flag_counts[flag];
            if (flag != 0) { ++flag4_here; ++total_flag4; }
        }
        // Rattle gives up after 10 consecutive failures; approximate that here.
        stuckQ = (flag4_here >= 10);

        if (dupQ) { ++dup_embeddings; }
        if (flag4_here > 0)
        {
            ++bad_embeddings;
            std::cout << "embedding " << e << ": " << n << " edges, nonzero flag in "
                      << flag4_here << "/" << n_rot << " rotations"
                      << (stuckQ ? "  [would bail out]" : "")
                      << (dupQ ? "  DUPLICATE 3D VERTICES" : "  (no 3D duplicates)") << "\n";
            if (dupQ) { FindDuplicateVertices(vcoords, n); }
        }
    }

    std::cout << "\n=== summary ===\n"
              << "embeddings probed:                  " << n_emb << "\n"
              << "embedding edge count:               " << min_edges << " .. " << max_edges << "\n"
              << "embeddings with a flag-4 rotation:  " << bad_embeddings << "\n"
              << "embeddings with duplicate vertices: " << dup_embeddings << "\n"
              << "total flag-4 rotations:             " << total_flag4
              << " / " << (n_emb * n_rot) << "\n"
              << "flag histogram (0 = clean):\n";
    for (const auto& [f, c] : flag_counts)
    {
        std::cout << "   flag " << f << ": " << c << "\n";
    }

    return (bad_embeddings > 0) ? 1 : 0;
}
