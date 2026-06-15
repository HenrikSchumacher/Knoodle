/**
 * @file link_alexander.hpp
 * @brief Single-variable Alexander |det| fingerprint that works for LINKS.
 *
 * Knoodle's production value routine (Alexander_UMFPACK::Alexander) deliberately
 * refuses multi-component links: it normalizes the raw strand-matrix determinant
 * by dividing by its value at t = 1, and for a mu-component link (mu >= 2) the
 * reduced strand matrix degenerates at t = 1 to the incidence matrix of mu
 * disjoint strand-cycles (rank n - mu), so det(M(1)) = 0 -> division by zero.
 *
 * We sidestep the normalization entirely. The raw determinant of the reduced
 * (n-1)x(n-1) strand matrix is  +/- t^k * Delta_L(t)  -- the link's one-variable
 * Alexander polynomial up to a unit. Evaluated on the UNIT CIRCLE (|t| = 1) the
 * unit factor has modulus 1, so |det(M(t))| is a genuine diagram-independent
 * invariant. We fingerprint log10|det| at several irrational-angle points on the
 * circle (avoiding roots of unity / common Alexander roots).
 *
 * Caveats (single-variable, by design):
 *   - It carries a common (t-1)^{mu-1} factor shared by all mu-component links;
 *     that cancels in any same-link comparison, so it is fine as an invariance
 *     tripwire but is weaker than the multivariable Alexander polynomial.
 *   - It is identically 0 for split links (handled: the fingerprint reports
 *     ok=false when a determinant vanishes).
 *
 * Requires Knoodle.hpp (built with UMFPACK) to be included first.
 */
#pragma once

#include <cmath>
#include <complex>
#include <string>
#include <vector>

namespace knoodle_test {

template<typename Cplx_, typename Int_>
class LinkAlexander
{
public:
    using Cplx      = Cplx_;
    using Int       = Int_;
    using PD_T      = Knoodle::PlanarDiagram<Int>;
    using StrandMat = Knoodle::AlexanderStrandMatrix<Cplx, Int, Int>;
    using UMFPACK_T = Tensors::UMFPACK<Cplx, Int>;

    struct Value
    {
        std::vector<double> logdet;   // log10|det(reduced strand matrix at t)|
        bool                ok = false;
    };

    // Sample points on the unit circle at irrational-ish angles (dodging roots
    // of unity and the most common Alexander roots). |t| = 1 is what makes |det|
    // an invariant.
    LinkAlexander()
    :   args_{ std::polar(1.0, 0.5), std::polar(1.0, 1.0), std::polar(1.0, 1.7),
               std::polar(1.0, 2.3), std::polar(1.0, 2.9) }
    {}

    const std::vector<Cplx>& Args() const { return args_; }

    // Mirrors Alexander_Strands_Det_Sparse but skips the knots-only
    // normalization. pd is copied because Pattern() caches into it.
    Value operator()(const PD_T& pd_in) const
    {
        Value v;
        v.logdet.assign(args_.size(), 0.0);

        PD_T pd(pd_in);

        // Split DIAGRAM => the reduced strand matrix is block-diagonal with a
        // singular block per piece, so det = 0 (numerically ~1e-16 garbage, not
        // an exact zero UMFPACK would flag). The single-variable Alexander
        // polynomial of a split link is identically zero anyway, so there is no
        // meaningful fingerprint here: report ok=false rather than a bogus value.
        // (Connected diagrams of split links would also give det~0, but detecting
        // that is a genuine topological question; this catches the common case.)
        if (pd.DiagramComponentCount() > Int(1)) { return v; }

        if (pd.CrossingCount() <= Int(1)) { v.ok = true; return v; }  // trivial

        StrandMat A;
        const auto& P = A.template Pattern<false>(pd);   // (C-1)x(C-1) pattern
        if (P.RowCount() <= Int(0)) { return v; }

        UMFPACK_T umf(P.RowCount(), P.ColCount(), P.Outer().data(), P.Inner().data());

        for (std::size_t i = 0; i < args_.size(); ++i)
        {
            A.template WriteNonzeroValues<false>(pd, args_[i], umf.Values().data());
            umf.NumericFactorization();
            const auto [mant, exp10] = umf.Determinant();   // det = mant * 10^exp10
            const double mag = std::abs(mant);
            if (!(mag > 0.0)) { return v; }                 // singular -> bail
            v.logdet[i] = std::log10(mag) + static_cast<double>(exp10);
        }
        v.ok = true;
        return v;
    }

    // Relative comparison on log10|det| (tolerant of float noise at scale).
    static bool Equal(const Value& a, const Value& b, double tol = 1e-3)
    {
        if (!a.ok || !b.ok || a.logdet.size() != b.logdet.size()) { return false; }
        for (std::size_t i = 0; i < a.logdet.size(); ++i)
        {
            const double scale = std::abs(a.logdet[i]) + std::abs(b.logdet[i]) + 1.0;
            if (std::abs(a.logdet[i] - b.logdet[i]) > tol * scale) { return false; }
        }
        return true;
    }

    static std::string ToString(const Value& v)
    {
        if (!v.ok) { return "<invalid>"; }
        std::string s;
        for (std::size_t i = 0; i < v.logdet.size(); ++i)
        {
            s += (i ? ", " : "") + std::to_string(v.logdet[i]);
        }
        return s;
    }

private:
    std::vector<Cplx> args_;
};

} // namespace knoodle_test
