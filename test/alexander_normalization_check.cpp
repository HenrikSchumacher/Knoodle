/**
 * @file alexander_normalization_check.cpp
 * @brief Regression test: Alexander_UMFPACK must normalize correctly when the
 *        Alexander polynomial vanishes at sqrt(2).
 *
 * The bug this pins: Alexander_Strands_Normalization finds the power of t that
 * makes the strand determinant symmetric by evaluating it at x = sqrt(2) and
 * 1/x, and reading k off det(1/x) / det(x) = x^(-2k). If Delta(sqrt(2)) = 0,
 * both determinants are rounding noise, the logarithm of their quotient was NaN
 * or a non-integer, and static_cast<Int64>(round(NaN)) is undefined behaviour:
 *   - arm64 gave 0, so every value at |t| != 1 came back off by a power of t
 *     (Alexander(2) = -896 = 2^8 * Delta(2) on the diagram below);
 *   - x86-64 gave INT64_MIN, the result became infinite, and
 *     ProductAccumulator::MantissaExponent10 looped forever (94 of 1000 cluster
 *     tasks in the fourier-knots run hung until the 24 h limit).
 * Delta(sqrt(2)) = 0 exactly when 2t^2 - 5 + 2t^-2 = Delta_{6_1}(t^2) divides
 * Delta; the 15-crossing diagram below has exactly that Alexander polynomial.
 * It is a summand that Simplify produced from a random Fourier knot
 * (fourier-knots hunt, curve 20122042).
 *
 * Every value is checked against the exact symmetric polynomial (Delta(1) = 1),
 * at real and complex arguments, on both the dense and the sparse code paths.
 * The trefoil and the figure-eight are controls for the ordinary path. The
 * exact polynomial of the 15-crossing diagram was computed independently, by
 * Fox calculus over the rationals.
 *
 * Build: see test/Makefile (target: alexander_normalization_check).
 */

#include "../Knoodle.hpp"   // KNOODLE_USE_UMFPACK supplied by the Makefile (-D)

#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Int    = std::int64_t;
using Cplx   = std::complex<double>;
using PD_T   = Knoodle::PlanarDiagram<Int>;
using Alex_T = Knoodle::Alexander_UMFPACK<Cplx, Int>;

namespace {

int failures = 0;

void Check(bool ok, const std::string& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok) { ++failures; }
}

struct Knot
{
    std::string                 name;
    std::vector<Int>            pd;      // signed PD code, 5 entries per crossing
    std::function<Cplx(Cplx)>   delta;   // exact symmetric Alexander polynomial
};

const std::vector<Knot> kKnots = {
    { "trefoil",
      { 0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1 },
      [](Cplx t) { return t - 1.0 + 1.0 / t; } },
    { "figure-eight",
      { 3,1,4,0,1, 7,5,0,4,1, 5,2,6,3,-1, 1,6,2,7,-1 },
      [](Cplx t) { return -t + 3.0 - 1.0 / t; } },
    { "15-crossing, Delta(sqrt 2) = 0",
      { 24,0,25,29,1,   0,24,1,23,1,    22,2,23,1,1,    2,22,3,21,1,    20,4,21,3,1,
         4,18,5,17,1,   5,10,6,11,-1,   11,6,12,7,-1,   7,12,8,13,-1,   19,9,20,8,1,
         9,19,10,18,1,  28,13,29,14,-1, 14,27,15,28,-1, 26,15,27,16,-1, 16,25,17,26,-1 },
      [](Cplx t) { return -2.0 * t * t + 5.0 - 2.0 / (t * t); } },
};

const std::vector<Cplx> kArgs = {
    Cplx(-1.0), Cplx(2.0), Cplx(3.0), Cplx(0.5), Cplx(-2.0), Cplx(std::sqrt(2.0)),
    Cplx(0.0, 1.0), Cplx(1.3, 0.4)
};

void CheckKnot(const Knot& k, const Alex_T& alex, const std::string& path)
{
    const Int n = static_cast<Int>(k.pd.size() / 5);
    PD_T pd = PD_T::FromSignedPDCode(k.pd.data(), n, false, true);
    if (!pd.ValidQ() || pd.LinkComponentCount() != 1)
    {
        Check(false, k.name + ": could not build the diagram");
        return;
    }

    std::vector<Cplx>    mant(kArgs.size());
    std::vector<Int>     expo(kArgs.size());
    const int rc = alex.Alexander(pd, kArgs.data(), static_cast<Int>(kArgs.size()),
                                  mant.data(), expo.data(), false);
    Check(rc == 0, k.name + " (" + path + "): Alexander returns 0");

    for (std::size_t i = 0; i < kArgs.size(); ++i)
    {
        const Cplx got   = mant[i] * std::pow(10.0, static_cast<double>(expo[i]));
        const Cplx exact = k.delta(kArgs[i]);
        const bool ok    = std::isfinite(got.real()) && std::isfinite(got.imag())
                        && std::abs(got - exact) <= 1e-9 * (1.0 + std::abs(exact));
        std::ostringstream s;
        s << k.name << " (" << path << "): Delta" << kArgs[i] << " = " << got
          << ", exact " << exact;
        Check(ok, s.str());
    }
}

} // namespace

int main()
{
    const Alex_T dense;             // default threshold: these diagrams take the dense path
    const Alex_T sparse (Int(2));   // threshold 2: diagrams with more than 3 crossings go sparse

    for (const Knot& k : kKnots)
    {
        CheckKnot(k, dense,  "dense");
        CheckKnot(k, sparse, "sparse");
    }

    std::cout << (failures == 0
                  ? "PASS: Alexander normalization is correct when Delta(sqrt 2) = 0\n"
                  : "FAIL: " + std::to_string(failures) + " check(s) failed\n");
    return (failures == 0) ? 0 : 1;
}
