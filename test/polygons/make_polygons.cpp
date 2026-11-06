// make_polygons -- one-off generator for the random-polygon corpus.
//
// NOT A TEST. It is built by `make polygons` and run by hand when the corpus
// needs regenerating; the corpus itself is the artifact and is checked in. This
// file is kept for provenance -- so the .crd files are reproducible rather than
// mysterious -- not because it runs.
//
// WHY THE CORPUS EXISTS. The simplifier's local-move tiers were almost
// untested: local_moves_check reports that they change the answer on 1 of 581
// diagrams in data/diagrams/hardunknots. That is not the corpus failing, it is
// the corpus being made of exactly the wrong thing -- hard unknots are
// *constructed* to resist local moves. GitHub #33 (R_IIa half-applying on a
// locked diagram and returning a different knot) survived in the tree because
// of that gap.
//
// Random equilateral polygons of a few hundred edges are the opposite: full of
// easy local structure, so every tier actually fires.
//
// Sampled with ActionAngleSampler, the same generator klut_bench --polygon-edges
// drives, from a fixed seed recorded in each file's header.
//
// Build: `make polygons` in test/.  Run: `./polygons/make_polygons`

#include "../../Knoodle.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using Int   = std::int64_t;
using Real  = double;
using PD_T  = Knoodle::PlanarDiagram<Int>;

using Sampler_T = Knoodle::ActionAngleSampler<Real, Int, Knoodle::PRNG_T, true>;

// Fixed, and recorded in every generated file. Changing it changes the corpus,
// which is a deliberate act, not a side effect of rerunning this.
static constexpr std::uint64_t kSeed = 20260815ULL;

// Mixed sizes: small enough that HOMFLY stays usable after simplification,
// large enough to carry real local structure. 20 polygons, ~100 KB total.
struct Batch { Int edges; int count; };
static const Batch kBatches[] = { {60, 8}, {120, 8}, {240, 4} };

int main( int argc, char ** argv )
{
    std::filesystem::path dir = (argc > 1) ? argv[1] : ".";

    Sampler_T sampler{ Knoodle::PRNG_T(kSeed) };

    std::printf("%-24s %6s %10s %8s\n", "file", "edges", "crossings", "bytes");
    std::printf("%s\n", std::string(52,'-').c_str());

    Int total_bytes = 0;
    int written = 0, skipped = 0;

    for( const Batch & b : kBatches )
    {
        for( int i = 0; i < b.count; ++i )
        {
            auto L = sampler.template RandomEquilateralLink<Real,Int,float>(Int(1), b.edges);

            // The diagram is not written out -- it is recomputed by the test
            // from the coordinates. It is built here only to report the
            // crossing count and to refuse a sample the projection cannot
            // handle, so the corpus contains nothing that fails before the
            // interesting part starts.
            auto [pd, unlinks] = PD_T::FromLinkEmbedding(L);
            (void)unlinks;
            if( !pd.ValidQ() )
            {
                std::printf("  (skipped a %lld-gon: projection declined it)\n",
                            (long long)b.edges);
                ++skipped;
                --i;                      // replace it, so counts stay exact
                continue;
            }

            char name[64];
            std::snprintf(name, sizeof name, "poly_%03lld_%02d.crd",
                          (long long)b.edges, i);
            const std::filesystem::path path = dir / name;

            if( !L.WriteToFile(path, /*colorQ=*/false) )
            {
                std::printf("  FAILED to write %s\n", name);
                return 1;
            }

            const Int bytes = Int(std::filesystem::file_size(path));
            total_bytes += bytes;
            ++written;
            std::printf("%-24s %6lld %10lld %8lld\n", name,
                        (long long)b.edges, (long long)pd.CrossingCount(),
                        (long long)bytes);
        }
    }

    std::printf("%s\n", std::string(52,'-').c_str());
    std::printf("%d files, %.1f KB total (%d samples replaced)\n",
                written, double(total_bytes)/1024.0, skipped);
    std::printf("seed = %llu\n", (unsigned long long)kSeed);
    return 0;
}
