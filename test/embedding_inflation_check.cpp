/**
 * @file embedding_inflation_check.cpp
 * @brief Regression test: re-aiming a projection must be a rigid motion. It must
 *        not walk the embedding's centre of mass away from the origin as the
 *        trial counts grow.
 *
 * The bug this pins (fixed in 00a8407). PlanarDiagramComplex::Rattle used to
 * re-aim each projection by rotating the live embedding in place:
 *
 *     LinkEmbedding_T emb = reapr.Embedding(pd,reapr.RandomRotation());
 *     for( rot ... ) { emb.Transform( reapr.RandomRotation() ); ... }
 *
 * LinkEmbedding::Transform writes the current coordinates out, applies the
 * matrix, and reads them back through ReadVertexCoordinates<true,shiftQ=true>,
 * which re-applies the Sterbenz shift. The catch is that WriteVertexCoordinates
 * hands back the raw edge_coords -- coordinates that ALREADY carry the previous
 * shift -- so each call shifts an already-shifted point cloud.
 *
 * The shift is `hi - 2*lo` per axis, which parks the cloud in [d, 2d] for
 * d = hi - lo. That is deliberate: it is the Sterbenz-lemma range, where
 * subtracting nearby coordinates is exact. But it also leaves the cloud a full
 * diameter from the origin. Rotating THAT about the origin scatters it across
 * both signs, so the next shift is computed from a much wider spread and pushes
 * it out further still. Iterate, and the coordinates random-walk outward.
 *
 * The clean way to say what is wrong: a rotation about the origin maps the
 * centre of mass c to A*c, so ||c|| is conserved. Transform does not conserve
 * it, because it folds a translation -- the re-shift -- into something
 * advertised as a rotation. The shift itself is legitimate and internal, so the
 * honest centre of mass is centroid(edge_coords) - SterbenzShift(); that is the
 * quantity checked below.
 *
 * Measured on the fixture here: re-aiming the way Rattle now does conserves
 * ||c|| to 2.1e-15 relative over 4096 rotations -- roundoff, nothing more. A
 * loop of Transform calls sends it from ~1.1e3 to ~4.3e6 in 200 rotations. The
 * tolerance below is 1e-12: three orders of magnitude above the noise floor and
 * about nineteen below the violation, so it is nowhere near finely tuned.
 *
 * The consequence, and why conservation is the right thing to assert: as the
 * cloud drifts outward, more and more of the 53-bit mantissa pays for position
 * rather than shape. At the magnitudes reached above, the geometry is left with
 * roughly 26 bits. That does not break FindIntersections by itself -- it raises
 * the odds that some pair of edges collapses into a degenerate configuration.
 * You still need bad luck to trip one; it just gets likelier the more precision
 * has been thrown away. A test that waited for a degeneracy would be slow and
 * flaky, so this one asserts the deterministic precondition instead.
 *
 * What each check pins, and how much it is worth:
 *
 *  1. Reapr::Embedding(pd,A) must RECORD A as the embedding's transformation
 *     matrix. This is the enabling half of the fix and the sharp discriminator:
 *     the old Embedding applied A through a transformation lambda and left
 *     TransformationMatrix() as the identity, so composing another Transform
 *     onto the live coordinates was the only way to re-aim. Verified to fail
 *     against the pre-fix tree.
 *
 *  2. Re-aiming conserves ||c|| and leaves the extent alone -- together, that it
 *     is a rigid motion about the origin. This is the invariant the fix exists
 *     to protect. It passes either side of the fix, because it exercises the
 *     replacement pattern rather than the code that changed; its value is
 *     guarding the Sterbenz/read path against a future change that would let the
 *     shift compound again.
 *
 *  3. Simplify still returns a valid complex, link components intact, when
 *     driven through Rattle at high embedding and rotation trial counts.
 *
 *  4. The same invariant for LinkEmbedding::Transform, the in-place spelling of
 *     the same rotation. 00a8407 routed Rattle around Transform rather than
 *     changing it, so this check failed until Transform was made to remove the
 *     shift its coordinates already carry before rotating. Run with --transform
 *     to print the walk.
 *
 * Provenance: the first diagrams seen to trip this in the wild were Nathan's
 * "congruence" examples. They are not needed to observe it -- the mechanism
 * belongs to the rotation loop, not to any particular diagram -- so this test
 * uses a small self-contained fixture and stays fast and data-free.
 *
 * Build: see test/Makefile (target: embedding_inflation_check).
 */

#include "../Knoodle.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

// Rattle's own loop shape: a fresh embedding per embedding trial, then this many
// random rotations within it. Both sit far above the defaults (0 and 25) because
// the trial counts are what multiply the drift -- that is the whole point.
constexpr int kEmbeddingTrials = 16;
constexpr int kRotationTrials  = 256;

// Conservation of ||c|| under rotation, relative. The measured noise floor is
// ~2e-15 over 4096 rotations; a compounding shift violates this by nineteen
// orders of magnitude.
constexpr Real kCentroidTol = 1e-12;

// Repeated LinkEmbedding::Transform. Against the unfixed version this many calls
// walk ||c|| off by three to four orders of magnitude; it typically breaks the
// tolerance above within one or two.
constexpr int kTransformRotations = 200;

// An 8-crossing connected diagram of the 2-component unlink, borrowed from
// component_check.cpp where it is already a known-good fixture.
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

// Centre of mass of the raw coordinate buffer.
Real MeanNorm( const Knoodle::Tensor2<Real,Int> & v, const Int m )
{
    Real c[3] = { Real(0), Real(0), Real(0) };
    for( Int i = 0; i < m; ++i )
    {
        for( int k = 0; k < 3; ++k ) { c[k] += v(i,k); }
    }
    Real n = Real(0);
    for( int k = 0; k < 3; ++k ) { c[k] /= static_cast<Real>(m); n += c[k] * c[k]; }
    return std::sqrt(n);
}

// Centre of mass of the embedding's actual geometry: the internal Sterbenz
// translation is removed, so what remains should be moved only by the rotation.
template<typename Emb_T>
Real CentroidNorm( Emb_T & emb, Knoodle::Tensor2<Real,Int> & scratch )
{
    const Int m = emb.EdgeCount();
    scratch.template RequireSize<false>(m, Int(3));
    emb.WriteVertexCoordinates(scratch.data());

    Real c[3] = { Real(0), Real(0), Real(0) };
    for( Int i = 0; i < m; ++i )
    {
        for( int k = 0; k < 3; ++k ) { c[k] += scratch(i,k); }
    }

    const auto shift = emb.SterbenzShift();
    Real n = Real(0);
    for( int k = 0; k < 3; ++k )
    {
        const Real t = c[k] / static_cast<Real>(m) - static_cast<Real>(shift[k]);
        n += t * t;
    }
    return std::sqrt(n);
}

// Longest axis of the axis-aligned bounding box.
template<typename Emb_T>
Real Span( Emb_T & emb, Knoodle::Tensor2<Real,Int> & scratch )
{
    const Int m = emb.EdgeCount();
    scratch.template RequireSize<false>(m, Int(3));
    emb.WriteVertexCoordinates(scratch.data());

    Real lo[3] = { scratch(0,0), scratch(0,1), scratch(0,2) };
    Real hi[3] = { scratch(0,0), scratch(0,1), scratch(0,2) };
    for( Int i = 0; i < m; ++i )
    {
        for( int k = 0; k < 3; ++k )
        {
            lo[k] = std::min(lo[k], scratch(i,k));
            hi[k] = std::max(hi[k], scratch(i,k));
        }
    }
    Real span = Real(0);
    for( int k = 0; k < 3; ++k ) { span = std::max(span, hi[k] - lo[k]); }
    return span;
}

PD_T MakeDiagram()
{
    const Int n = static_cast<Int>(sizeof(kPD) / sizeof(kPD[0]) / 5);
    return PD_T::FromSignedPDCode(kPD, n, false, true);
}

Reapr_T MakeReapr( const PDC_T::Simplify_Args_T & sargs )
{
    return Reapr_T({
        .permute_randomQ     = sargs.permute_randomQ,
        .energy              = sargs.energy,
        .ortho_draw_settings = {
            .randomize_bends          = sargs.randomize_bends,
            .randomize_virtual_edgesQ = sargs.randomize_virtual_edgesQ,
            .compaction_method        = sargs.compaction_method
        },
        .scaling             = sargs.scaling
    });
}

// Repeated LinkEmbedding::Transform must conserve the centre of mass too: it is
// the in-place spelling of the same rotation, and a rotation about the origin
// cannot move ||c||. Returns the worst relative deviation; `trace` prints the
// walk, which is what a bug report wants to see.
Real TransformCentroidDrift( PD_T & pd, Reapr_T & reapr, const bool trace )
{
    Knoodle::Tensor2<Real,Int> scratch;
    auto emb = reapr.Embedding(pd);

    const Real c0 = CentroidNorm(emb, scratch);
    if( c0 <= Real(0) ) { return Real(0); }

    if( trace )
    {
        std::printf("\nLinkEmbedding::Transform, repeated:\n");
        std::printf("  %6s  %16s  %12s\n", "rot", "||c||", "rel. change");
        std::printf("  %6d  %16.6g  %12s\n", 0, c0, "-");
    }

    Real worst = Real(0);
    for( int k = 1; k <= kTransformRotations; ++k )
    {
        emb.Transform( reapr.RandomRotation() );

        const Real c   = CentroidNorm(emb, scratch);
        const Real dev = std::abs(c - c0) / c0;
        worst = std::max(worst, dev);

        if( trace && (k <= 3 || k == 10 || k == 50 || k == 100 || k == kTransformRotations) )
        {
            std::printf("  %6d  %16.6g  %12.3g\n", k, c, dev);
        }
    }
    return worst;
}

} // namespace

int main( int argc, char ** argv )
{
    bool show_transform = false;
    for( int i = 1; i < argc; ++i )
    {
        if( std::strcmp(argv[i], "--transform") == 0 ) { show_transform = true; }
    }

    std::cout << "embedding_inflation_check\n";

    PD_T pd = MakeDiagram();
    if( pd.InvalidQ() )
    {
        std::cout << "FAIL: could not build the test diagram\n";
        return 1;
    }
    const Int components_before = pd.LinkComponentCount();

    PDC_T::Simplify_Args_T sargs;
    Reapr_T reapr = MakeReapr(sargs);

    // ---------------------------------------------------------------------
    // 1. The embedding must remember the transformation it was built with.
    //    Without that, the only way to re-aim is to compose another rotation
    //    onto the live (already shifted) coordinates -- which is what lets the
    //    shift compound.
    // ---------------------------------------------------------------------
    {
        const auto A = reapr.RandomRotation();
        auto A_arg   = A;                       // Embedding consumes an rvalue
        auto emb     = reapr.Embedding(pd, std::move(A_arg));
        const auto R = emb.TransformationMatrix();

        Real worst = Real(0);
        for( int i = 0; i < 3; ++i )
        {
            for( int j = 0; j < 3; ++j )
            {
                worst = std::max(worst, std::abs(A(i,j) - R(i,j)));
            }
        }

        std::cout << "  |A - TransformationMatrix()| = " << worst << "\n";
        Check(worst <= Real(1e-12),
              "Embedding(pd,A) records A instead of baking it into coordinates");
    }

    // ---------------------------------------------------------------------
    // 2. Re-aiming is a rigid motion: it moves the centre of mass only by the
    //    rotation, and does not change the size.
    // ---------------------------------------------------------------------
    Knoodle::Tensor2<Real,Int> scratch;
    Knoodle::Tensor2<Real,Int> kept;

    Real worst_centroid_dev = Real(0);
    Real worst_span_factor  = Real(1);
    int  worst_trial        = -1;
    int  worst_rotation     = -1;
    bool measured           = false;

    for( int trial = 0; trial < kEmbeddingTrials; ++trial )
    {
        // A fresh embedding per embedding trial, exactly as Rattle does, and a
        // kept copy of its coordinates. Every rotation below is applied to that
        // copy, never to the result of the previous rotation.
        auto emb = reapr.Embedding(pd);

        const Real base_span = Span(emb, scratch);
        if( base_span <= Real(0) )
        {
            std::cout << "FAIL: degenerate embedding (zero extent)\n";
            return 1;
        }

        const Int m = emb.EdgeCount();
        kept.template RequireSize<false>(m, Int(3));
        emb.WriteVertexCoordinates(kept.data());

        // ||A*c|| == ||c||, so the centre of mass of the kept coordinates is the
        // reference every rotation of them must reproduce.
        const Real reference = MeanNorm(kept, m);
        if( reference <= Real(0) )
        {
            std::cout << "FAIL: kept coordinates have a centre of mass at the origin\n";
            return 1;
        }

        for( int rot = 0; rot < kRotationTrials; ++rot )
        {
            emb.SetTransformationMatrix( reapr.RandomRotation() );
            emb.template ReadVertexCoordinates<true>( kept.data() );
            measured = true;

            const Real dev = std::abs(CentroidNorm(emb, scratch) - reference) / reference;
            if( dev > worst_centroid_dev )
            {
                worst_centroid_dev = dev;
                worst_trial        = trial;
                worst_rotation     = rot;
            }

            const Real span = Span(emb, scratch);
            worst_span_factor = std::max(worst_span_factor,
                                         std::max(span / base_span, base_span / span));
        }
    }

    std::cout << "  " << kEmbeddingTrials << " embedding trials x "
              << kRotationTrials << " rotation trials\n"
              << "  worst |d||c||| / ||c|| = " << worst_centroid_dev
              << " (trial " << worst_trial << ", rotation " << worst_rotation << ")\n"
              << "  worst extent change factor = " << worst_span_factor << "\n";

    Check(measured, "the trial loops actually ran");
    Check(worst_centroid_dev <= kCentroidTol,
          "rotation conserves the centre of mass");
    Check(worst_span_factor <= Real(4),
          "rotation does not change the embedding's size");

    // ---------------------------------------------------------------------
    // 3. The real thing: drive Simplify through Rattle at high trial counts.
    // ---------------------------------------------------------------------
    PDC_T::Simplify_Args_T hi_args;
    hi_args.embedding_trials = 32;
    hi_args.rotation_trials  = 128;

    PDC_T pdc{ MakeDiagram() };
    pdc.Simplify(hi_args);

    Check(pdc.ValidQ(), "Simplify returns a valid complex at high trial counts");

    Int components_after = 0;
    if( pdc.ValidQ() )
    {
        for( Int i = 0; i < pdc.DiagramCount(); ++i )
        {
            components_after += pdc.Diagram(i).LinkComponentCount();
        }
    }
    Check(components_after == components_before,
          "link components survive Rattle at high trial counts ("
          + std::to_string(static_cast<long long>(components_before)) + ")");

    // ---------------------------------------------------------------------
    // 4. The same invariant for the in-place spelling. Transform is advertised
    //    as a rotation, so it must not move the centre of mass either.
    // ---------------------------------------------------------------------
    const Real transform_dev = TransformCentroidDrift(pd, reapr, show_transform);
    std::cout << "  worst |d||c||| / ||c|| over " << kTransformRotations
              << " Transform calls = " << transform_dev << "\n";
    Check(transform_dev <= kCentroidTol,
          "repeated Transform conserves the centre of mass");

    std::cout << (failures == 0
                  ? "PASS: re-aiming a projection is a rigid motion\n"
                  : "FAIL: " + std::to_string(failures) + " check(s) failed\n");
    return (failures == 0) ? 0 : 1;
}
