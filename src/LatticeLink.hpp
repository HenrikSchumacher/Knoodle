#pragma once

#include <boost/unordered/unordered_flat_set.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace Knoodle
{
    // A self-avoiding link realized as a unit-step polygon on the integer lattice Z^3.
    //
    // Storage (see docs/lattice-bfacf-design.md):
    //   - EdgeList:    a dense std::vector acting as an intrusive doubly-linked list.
    //                  Each record owns its tail vertex (real lattice coords), a unit
    //                  direction, and next/prev indices. Every vertex is the tail of
    //                  exactly one edge, so the polygon is a set of disjoint directed
    //                  cycles (one per link component).
    //   - OccupiedSet: a boost::unordered_flat_set on the *doubled* lattice, storing BOTH
    //                  vertices (all-even doubled coords) and edge midpoints (one-odd).
    //                  Self-avoidance is one O(1) lookup per site.
    //   - S1,S2,N:     running accumulators giving R_g^2 = S2/N - |S1/N|^2 in O(1)/move.
    //
    // This class is self-contained: it does NOT modify LinkEmbedding. The round-trip is
    // done with a ctor (LinkEmbedding -> LatticeLink) and ToLinkEmbedding() (the reverse).
    //
    // BFACF moves are added in a later step; step 1 is the round-trip + accumulators only.

    template<FloatQ Real_ = Real64, IntQ Int_ = Int64, FloatQ BReal_ = Real32>
    class LatticeLink
    {
    public:

        using Real            = Real_;
        using Int             = Int_;
        using BReal           = BReal_;

        using LinkEmbedding_T = LinkEmbedding<Real,Int,BReal>;

        using LCoord          = std::int32_t;   // a single real lattice coordinate
        using EIdx            = std::int32_t;    // index into the EdgeList

        // A point on the doubled lattice, used as the OccupiedSet key. For a vertex all
        // three coords are even; for an edge midpoint exactly one is odd.
        struct IVec3
        {
            LCoord x, y, z;

            friend bool operator==( const IVec3 & a, const IVec3 & b ) noexcept
            {
                return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
            }
        };

        struct IVec3Hash
        {
            std::size_t operator()( const IVec3 & k ) const noexcept
            {
                std::uint64_t h = std::uint64_t( std::uint32_t(k.x) )
                    ^ ( std::uint64_t( std::uint32_t(k.y) ) * 0x9E3779B97F4A7C15ULL )
                    ^ ( std::uint64_t( std::uint32_t(k.z) ) * 0xC2B2AE3D27D4EB4FULL );
                h ^= h >> 33;
                h *= 0xFF51AFD7ED558CCDULL;
                h ^= h >> 33;
                return static_cast<std::size_t>(h);
            }
        };

        // One directed unit edge. Hot fields only; component color lives in a parallel
        // cold vector (color_), touched only on birth/export.
        struct Edge
        {
            LCoord       tx, ty, tz;  // tail vertex, real lattice coordinates
            std::uint8_t dir;         // 0:+x 1:-x 2:+y 3:-y 4:+z 5:-z
            EIdx         next;        // edge whose tail == this edge's head
            EIdx         prev;        // edge whose head == this edge's tail
        };

        using OccupiedSet_T = boost::unordered_flat_set<IVec3,IVec3Hash>;

        // Which quantity the objective-driven Step() optimizes. Enum + switch in the accept
        // path (the Reapr::Energy_T idiom); see LatticeLink/Objectives.hpp.
        enum class Objective_T : std::int8_t
        {
            MinLength    = 0,   // simulated-annealing shrink (BFACF, death-biased)
            MaxRg2Banded = 1,   // climb R_g^2 with a soft length band (all moves active)
        };

        struct Settings_T
        {
            Objective_T objective = Objective_T::MinLength;
            std::size_t max_edges = std::size_t(1) << 20;   // soft cap on births

            // MinLength: energy = length; accept ~ exp(-beta*dN) with beta annealed
            // linearly from beta_init to beta_final over anneal_steps attempts.
            Real          beta_init    = Real(0);
            Real          beta_final   = Real(2);
            std::uint64_t anneal_steps = 200000;

            // MaxRg2Banded: accept ~ exp(rg_beta*dRg2 - dBandPenalty). The band is a soft
            // wall pulling the length back toward [band_lo, band_hi]; band_lo==band_hi==0
            // disables it (pure R_g climb).
            Real rg_beta        = Real(0.01);
            Int  band_lo        = 0;
            Int  band_hi        = 0;
            Real band_stiffness = Real(0.01);
        };

    private:

        // Unit-direction lookup (indexed by Edge::dir).
        static constexpr LCoord dir_dx[6] = { +1, -1,  0,  0,  0,  0 };
        static constexpr LCoord dir_dy[6] = {  0,  0, +1, -1,  0,  0 };
        static constexpr LCoord dir_dz[6] = {  0,  0,  0,  0, +1, -1 };

        static constexpr Real coord_tol = static_cast<Real>(1) / static_cast<Real>(1024);

        std::vector<Edge> edges_;   // dense EdgeList (intrusive doubly-linked cycles)
        std::vector<Int>  color_;   // parallel cold: component color per edge

        OccupiedSet_T     occupied_;

        // R_g^2 accumulators (exact Int64; see Gyradius.hpp).
        std::int64_t S1x = 0, S1y = 0, S1z = 0, S2 = 0;
        std::int64_t Nv  = 0;

        Settings_T    settings_;
        std::uint64_t attempt_ctr_ = 0;   // move attempts, drives the annealing schedule

        bool validQ_ = false;

    public:

        LatticeLink() = default;

        explicit LatticeLink( cref<LinkEmbedding_T> emb )
        {
            BuildFromEmbedding(emb);
        }

        ~LatticeLink() = default;

        LatticeLink( const LatticeLink &  ) = default;
        LatticeLink(       LatticeLink && ) = default;
        LatticeLink & operator=( const LatticeLink &  ) = default;
        LatticeLink & operator=(       LatticeLink && ) = default;

    public:

        bool ValidQ() const { return validQ_; }

        // For a self-avoiding polygon #vertices == #edges (each vertex is one edge's tail).
        Int  EdgeCount()   const { return static_cast<Int>(edges_.size()); }
        Int  VertexCount() const { return static_cast<Int>(edges_.size()); }

        std::size_t OccupiedCount() const { return occupied_.size(); }

        cref<std::vector<Edge>> Edges()        const { return edges_; }
        cref<OccupiedSet_T>     Occupied()     const { return occupied_; }

        mref<Settings_T>        Settings()           { return settings_; }
        cref<Settings_T>        Settings()     const { return settings_; }
        std::uint64_t           AttemptCount() const { return attempt_ctr_; }
        void                    ResetSchedule()      { attempt_ctr_ = 0; }

    private:

        void Clear()
        {
            edges_.clear();
            color_.clear();
            occupied_.clear();
            S1x = S1y = S1z = S2 = 0;
            Nv  = 0;
            validQ_ = false;
        }

        // Head vertex of edge e (real lattice coords).
        void EdgeHead( const Edge & e, LCoord & hx, LCoord & hy, LCoord & hz ) const
        {
            hx = e.tx + dir_dx[e.dir];
            hy = e.ty + dir_dy[e.dir];
            hz = e.tz + dir_dz[e.dir];
        }

        // OccupiedSet key for a vertex (doubled, all-even).
        static IVec3 VertexKey( const LCoord x, const LCoord y, const LCoord z )
        {
            return IVec3{ static_cast<LCoord>(2*x),
                          static_cast<LCoord>(2*y),
                          static_cast<LCoord>(2*z) };
        }

        // OccupiedSet key for an edge's midpoint (doubled, exactly one odd coord).
        static IVec3 MidpointKey( const Edge & e )
        {
            return IVec3{ static_cast<LCoord>(2*e.tx + dir_dx[e.dir]),
                          static_cast<LCoord>(2*e.ty + dir_dy[e.dir]),
                          static_cast<LCoord>(2*e.tz + dir_dz[e.dir]) };
        }

#include "LatticeLink/Gyradius.hpp"
#include "LatticeLink/RoundTrip.hpp"
#include "LatticeLink/Moves.hpp"
#include "LatticeLink/Objectives.hpp"

    public:

        static std::string ClassName()
        {
            return std::string("LatticeLink")
            + "<" + TypeName<Real>
            + "," + TypeName<Int>
            + "," + TypeName<BReal>
            + ">";
        }

    }; // class LatticeLink

} // namespace Knoodle
