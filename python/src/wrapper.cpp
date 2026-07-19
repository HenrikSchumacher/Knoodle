// Include minimal required headers from Knoodle
// (Knoodle.hpp also brings in src/Klut.hpp, used for knot identification.)
#include "Knoodle.hpp"
#ifdef __linux__
#include "submodules/Tensors/OpenBLAS.hpp"
#endif

// Conditionally include Alexander if UMFPACK is available.
// The BLAS backend header (Accelerate/OpenBLAS) must be loaded first;
// do NOT define KNOODLE_USE_UMFPACK, or Knoodle.hpp would include
// Alexander_UMFPACK.hpp before any backend is set up.
#ifdef USE_UMFPACK
#ifndef __linux__
#include "submodules/Tensors/Accelerate.hpp"
#endif
#include "src/KnotInvariants/Alexander_UMFPACK.hpp"
#endif

// Face-matrix Alexander: valid for multi-component links.
#include "src/KnotInvariants/AlexanderFaceMatrix.hpp"


// Include our bridge header
#include "bindings.h"

#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <random>

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

// Define the types we need
using Real = Real64;
using Int = Int64;
using LInt = Int;
using BReal = Real64;
using PD_T = PlanarDiagram<Int>;         // Diagram type (upstream's former PlanarDiagram2)
using PDC_T = PlanarDiagramComplex<Int>; // Diagram complex type
using Complex = Complex64;

// PD-code layout used throughout the python API: m x 5, [4 arcs + handedness].
// Upstream's PDCode() now defaults to signed + colored (m x 7), so request the
// signed, uncolored layout explicitly.
static constexpr PD_T::PDCode_TArgs_T pd5_targs {
    .signQ = true, .colorQ = false, .farfalleQ = false
};

// Knoodle's AABBTree::ComputeBoundingBoxes sets the FE_UPWARD rounding mode
// and never restores it, which would leak into the whole Python process
// (breaking, e.g., numpy). Restore the floating-point environment on exit
// from every entry point that calls into the library.
class FPEnvGuard {
public:
    FPEnvGuard() { std::fegetenv(&env_); }
    ~FPEnvGuard() { std::fesetenv(&env_); }
    FPEnvGuard(const FPEnvGuard&) = delete;
    FPEnvGuard& operator=(const FPEnvGuard&) = delete;
private:
    std::fenv_t env_;
};

// Define Alexander polynomial calculator type (only if UMFPACK available)
#ifdef USE_UMFPACK
using Alexander_T = Alexander_UMFPACK<Complex, Int>;
#endif

// AlexanderResult method implementations
std::string AlexanderResult::to_string() const {
    if (exponent == 0) {
        return std::to_string(mantissa);
    } else {
        std::ostringstream ss;
        ss << mantissa << "e" << exponent;
        return ss.str();
    }
}

double AlexanderResult::to_double() const {
    return mantissa * std::pow(10.0, exponent);
}

// Implementation class that holds the planar diagram complex
class KnotAnalyzerImpl {
public:
    PDC_T pdc;                // The diagram complex (replaces old unique_ptr<PD_T>)
    Int primary_diagram_idx;  // Index of the primary (first non-unknot) diagram, -1 if none

#ifdef USE_UMFPACK
    mutable std::unique_ptr<Alexander_T> alexander_calc;
#endif

    KnotAnalyzerImpl() : primary_diagram_idx(-1) {}

    KnotAnalyzerImpl(const KnotAnalyzerImpl& other)
        : pdc(other.pdc), primary_diagram_idx(other.primary_diagram_idx) {
        // Don't copy alexander_calc - it will be created on-demand
    }

#ifdef USE_UMFPACK
    KnotAnalyzerImpl(KnotAnalyzerImpl&& other) noexcept
        : pdc(std::move(other.pdc)),
          primary_diagram_idx(other.primary_diagram_idx),
          alexander_calc(std::move(other.alexander_calc)) {}
#else
    KnotAnalyzerImpl(KnotAnalyzerImpl&& other) noexcept
        : pdc(std::move(other.pdc)),
          primary_diagram_idx(other.primary_diagram_idx) {}
#endif

    // Get the primary diagram (first non-unknot with crossings)
    const PD_T* get_primary_diagram() const {
        if (primary_diagram_idx >= 0 && primary_diagram_idx < pdc.DiagramCount()) {
            return &pdc.Diagram(primary_diagram_idx);
        }
        return nullptr;
    }

#ifdef USE_UMFPACK
    // Get or create Alexander calculator
    Alexander_T& get_alexander_calculator() const {
        if (!alexander_calc) {
            alexander_calc = std::make_unique<Alexander_T>();
        }
        return *alexander_calc;
    }
#endif
};

// Calculate squared gyradius from coordinates
double calculate_squared_gyradius(const std::vector<double>& coords) {
    int n_points = coords.size() / 3;
    if (n_points == 0) return 0.0;

    // Calculate center of mass
    double x_sum = 0.0, y_sum = 0.0, z_sum = 0.0;
    for (int i = 0; i < n_points; ++i) {
        x_sum += coords[3*i];
        y_sum += coords[3*i+1];
        z_sum += coords[3*i+2];
    }
    double x_avg = x_sum / n_points;
    double y_avg = y_sum / n_points;
    double z_avg = z_sum / n_points;

    // Calculate average squared distance from center
    double sum_sq_dist = 0.0;
    for (int i = 0; i < n_points; ++i) {
        double dx = coords[3*i] - x_avg;
        double dy = coords[3*i+1] - y_avg;
        double dz = coords[3*i+2] - z_avg;
        sum_sq_dist += dx*dx + dy*dy + dz*dz;
    }

    return sum_sq_dist / n_points;
}

// Convert PD_T to PD code string (5 entries: 4 arcs + handedness)
std::string pd_to_string(const PD_T& pd) {
    std::stringstream ss;
    auto pdcode = pd.PDCode<Int, pd5_targs>();

    for (Int i = 0; i < pdcode.Dimension(0); ++i) {
        ss << "[";
        for (Int j = 0; j < 5; ++j) {  // Include all 5 entries (4 arcs + handedness)
            ss << pdcode(i, j);
            if (j < 4) ss << ",";  // Comma after first 4 entries
        }
        ss << "]";
        if (i < pdcode.Dimension(0) - 1) ss << ",";
    }

    return ss.str();
}

// Convert PD_T to unsigned PD code string (4 entries only, for backward compatibility)
std::string pd_to_string_unsigned(const PD_T& pd) {
    std::stringstream ss;
    auto pdcode = pd.PDCode<Int, pd5_targs>();

    for (Int i = 0; i < pdcode.Dimension(0); ++i) {
        ss << "[";
        for (Int j = 0; j < 4; ++j) {  // Only first 4 entries (arcs only)
            ss << pdcode(i, j);
            if (j < 3) ss << ",";
        }
        ss << "]";
        if (i < pdcode.Dimension(0) - 1) ss << ",";
    }

    return ss.str();
}

// Convert PD_T to Gauss code string
std::string gauss_to_string(const PD_T& pd) {
    try {
        auto extgausscode = pd.ExtendedGaussCode();
        Int code_size = extgausscode.Size();

        if (code_size == 0) {
            return "";
        }

        std::stringstream ss;
        ss << "ext:";
        for (Int i = 0; i < code_size; ++i) {
            ss << extgausscode[i];
            if (i < code_size - 1) ss << " ";
        }

        return ss.str();
    } catch (...) {
        return "";
    }
}

// Map old simplify levels to new Simplify_Args_T
PDC_T::Simplify_Args_T make_simplify_args(int simplify_level) {
    PDC_T::Simplify_Args_T args;
    switch (simplify_level) {
        case 1:
        case 2:
        case 3:
            // Strand simplification only - no disconnect, split, or reapr
            args.disconnectQ = false;
            args.splitQ = false;
            args.embedding_trials = 0;
            args.rotation_trials = 0;
            break;
        case 4:
            // Add disconnect and split, but no reapr
            args.disconnectQ = true;
            args.splitQ = true;
            args.embedding_trials = 0;
            args.rotation_trials = 0;
            break;
        case 5:
        default:
            // Full simplification with the library defaults (including the
            // built-in Reapr rattle: rotation_trials = 25).
            break;
    }
    return args;
}

// Analyze a PDC_T to extract summary properties
struct PDCAnalysis {
    Int total_crossing_count;
    Int total_writhe;
    Int unlink_count;
    Int nontrivial_count;
    Int first_nontrivial_idx;

    static PDCAnalysis analyze(const PDC_T& pdc) {
        PDCAnalysis result = {};
        result.total_crossing_count = 0;
        result.total_writhe = 0;
        result.unlink_count = 0;
        result.nontrivial_count = 0;
        result.first_nontrivial_idx = -1;

        for (Int i = 0; i < pdc.DiagramCount(); ++i) {
            const PD_T& pd = pdc.Diagram(i);
            if (pd.ProvenUnknotQ()) {
                result.unlink_count++;
            } else if (pd.CrossingCount() > 0) {
                result.total_crossing_count += pd.CrossingCount();
                result.total_writhe += pd.Writhe();
                result.nontrivial_count++;
                if (result.first_nontrivial_idx < 0) {
                    result.first_nontrivial_idx = i;
                }
            }
        }

        return result;
    }
};

// Random rotation matrix, replicated from Reapr::RandomRotation: Rodrigues
// rotation about a Gaussian-random axis by a uniform angle in [0, pi).
static Tiny::Matrix<3,3,Real,Int> random_rotation(PRNG_T& engine)
{
    using Vector_T = Tiny::Vector<3,Real,Int>;
    using Matrix_T = Tiny::Matrix<3,3,Real,Int>;

    std::normal_distribution<Real> gaussian {Real(0), Real(1)};
    std::uniform_real_distribution<Real> angle_dist {Real(0), Scalar::Pi<Real>};

    Vector_T u;
    Real u_squared;
    do {
        u[0] = gaussian(engine);
        u[1] = gaussian(engine);
        u[2] = gaussian(engine);
        u_squared = u.NormSquared();
    } while (u_squared <= Real(0));

    u /= Sqrt(u_squared);

    const Real angle = angle_dist(engine);
    const Real cos = std::cos(angle);
    const Real sin = std::sin(angle);
    const Real d = Real(1) - cos;

    Matrix_T A;

    A[0][0] = u[0] * u[0] * d + cos       ;
    A[0][1] = u[0] * u[1] * d - sin * u[2];
    A[0][2] = u[0] * u[2] * d + sin * u[1];

    A[1][0] = u[1] * u[0] * d + sin * u[2];
    A[1][1] = u[1] * u[1] * d + cos       ;
    A[1][2] = u[1] * u[2] * d - sin * u[0];

    A[2][0] = u[2] * u[0] * d - sin * u[1];
    A[2][1] = u[2] * u[1] * d + sin * u[0];
    A[2][2] = u[2] * u[2] * d + cos       ;

    return A;
}

// Apply a rotation matrix to a flat xyz coordinate array.
static std::vector<double> rotate_coordinates(
    const std::vector<double>& coords, const Tiny::Matrix<3,3,Real,Int>& A)
{
    const size_t n = coords.size() / 3;
    std::vector<double> out(coords.size());
    for (size_t i = 0; i < n; ++i) {
        for (int r = 0; r < 3; ++r) {
            out[3 * i + static_cast<size_t>(r)] =
                A[r][0] * coords[3 * i] +
                A[r][1] * coords[3 * i + 1] +
                A[r][2] * coords[3 * i + 2];
        }
    }
    return out;
}

// Build a diagram complex from closed-curve coordinates, retrying with
// random rotations when the plain z-projection is degenerate (upstream's
// FindIntersections rejects, e.g., intersections that fall exactly on
// segment corners and suggests rotating the input). Throws when no
// projection succeeds.
static PDC_T pdc_from_knot_coordinates(const std::vector<double>& coordinates)
{
    using Knot_T = KnotEmbedding<Real, Int, Real>;

    const Int n = static_cast<Int>(coordinates.size() / 3);

    static thread_local PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };

    constexpr int max_attempts = 10;
    Knot_T K( n );
    int err = -1;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (attempt == 0) {
            K.ReadVertexCoordinates( coordinates.data() );
        } else {
            K = Knot_T( n );
            const auto rotated =
                rotate_coordinates(coordinates, random_rotation(random_engine));
            K.ReadVertexCoordinates( rotated.data() );
        }
        err = K.template FindIntersections<false>();
        if (err == 0) { break; }
    }
    if (err != 0) {
        throw std::runtime_error(
            "could not find a non-degenerate planar projection after "
            + std::to_string(max_attempts) + " attempts (error code "
            + std::to_string(err) + ")");
    }

    PDC_T pdc { PD_T::FromKnotEmbedding(K) };

    for (Int i = 0; i < pdc.DiagramCount(); ++i) {
        if (pdc.Diagram(i).InvalidQ()) {
            throw std::runtime_error("diagram construction from the "
                                     "projection failed (invalid diagram)");
        }
    }
    return pdc;
}

// KnotAnalyzer implementation
KnotAnalyzer::KnotAnalyzer() : impl(std::make_shared<KnotAnalyzerImpl>()) {
    crossing_count = 0;
    writhe = 0;
    squared_gyradius = 0.0;
    link_component_count = 1;
    unlink_count = 0;
    is_prime = true;
    is_composite = false;
    prime_component_count = 0;
}

KnotAnalyzer::KnotAnalyzer(const std::vector<double>& coordinates, bool simplify, int simplify_level)
    : impl(std::make_shared<KnotAnalyzerImpl>()) {

    FPEnvGuard fp_guard;

    try {
        // Create planar diagram complex from coordinates (with rotation
        // retries for degenerate projections)
        impl->pdc = pdc_from_knot_coordinates(coordinates);

        // Apply simplification if requested
        if (simplify) {
            auto args = make_simplify_args(simplify_level);
            impl->pdc.Simplify(args);
        }

        // Analyze the complex
        auto analysis = PDCAnalysis::analyze(impl->pdc);
        impl->primary_diagram_idx = analysis.first_nontrivial_idx;

        // Extract properties
        crossing_count = analysis.total_crossing_count;
        writhe = analysis.total_writhe;
        unlink_count = analysis.unlink_count;
        squared_gyradius = calculate_squared_gyradius(coordinates);

        // PD code and gauss code from the primary diagram
        const PD_T* primary = impl->get_primary_diagram();
        if (primary) {
            pd_code = pd_to_string(*primary);
            gauss_code = gauss_to_string(*primary);
            link_component_count = primary->LinkComponentCount();
        } else {
            pd_code = "";
            gauss_code = "";
            link_component_count = 1;  // unknot has 1 link component
        }

        // Prime decomposition: every non-unknot diagram is a prime component
        if (analysis.nontrivial_count > 1) {
            is_composite = true;
            is_prime = false;
            prime_component_count = analysis.nontrivial_count;

            for (Int i = 0; i < impl->pdc.DiagramCount(); ++i) {
                const PD_T& comp_pd = impl->pdc.Diagram(i);
                if (comp_pd.ProvenUnknotQ() || comp_pd.CrossingCount() == 0) continue;

                KnotAnalyzer comp;
                comp.crossing_count = comp_pd.CrossingCount();
                comp.writhe = comp_pd.Writhe();
                comp.pd_code = pd_to_string(comp_pd);
                comp.gauss_code = gauss_to_string(comp_pd);
                comp.squared_gyradius = 0.0;
                comp.link_component_count = comp_pd.LinkComponentCount();
                comp.unlink_count = 0;
                comp.is_prime = true;
                comp.is_composite = false;
                comp.prime_component_count = 1;

                // Create a PDC_T for this child from a copy of the component diagram
                comp.impl = std::make_shared<KnotAnalyzerImpl>();
                PD_T comp_copy = comp_pd;
                comp.impl->pdc = PDC_T(std::move(comp_copy));
                comp.impl->primary_diagram_idx = 0;

                prime_components.push_back(std::move(comp));
            }
        } else if (analysis.nontrivial_count == 1) {
            is_prime = true;
            is_composite = false;
            prime_component_count = 1;
        } else {
            // Unknot
            is_prime = true;
            is_composite = false;
            prime_component_count = 0;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error in KnotAnalyzer constructor: " << e.what() << std::endl;
        // Set error values
        crossing_count = -1;
        writhe = 0;
        pd_code = "Error: " + std::string(e.what());
        gauss_code = "Error: " + std::string(e.what());
        squared_gyradius = 0.0;
        link_component_count = -1;
        unlink_count = -1;
        is_prime = false;
        is_composite = false;
        prime_component_count = -1;
    }
}

// Copy constructor
KnotAnalyzer::KnotAnalyzer(const KnotAnalyzer& other)
    : impl(std::make_shared<KnotAnalyzerImpl>(*other.impl)),
      crossing_count(other.crossing_count),
      writhe(other.writhe),
      pd_code(other.pd_code),
      gauss_code(other.gauss_code),
      squared_gyradius(other.squared_gyradius),
      link_component_count(other.link_component_count),
      unlink_count(other.unlink_count),
      is_prime(other.is_prime),
      is_composite(other.is_composite),
      prime_component_count(other.prime_component_count),
      prime_components(other.prime_components) {
}

// Move constructor
KnotAnalyzer::KnotAnalyzer(KnotAnalyzer&& other) noexcept
    : impl(std::move(other.impl)),
      crossing_count(other.crossing_count),
      writhe(other.writhe),
      pd_code(std::move(other.pd_code)),
      gauss_code(std::move(other.gauss_code)),
      squared_gyradius(other.squared_gyradius),
      link_component_count(other.link_component_count),
      unlink_count(other.unlink_count),
      is_prime(other.is_prime),
      is_composite(other.is_composite),
      prime_component_count(other.prime_component_count),
      prime_components(std::move(other.prime_components)) {
}

// Copy assignment
KnotAnalyzer& KnotAnalyzer::operator=(const KnotAnalyzer& other) {
    if (this != &other) {
        impl = std::make_shared<KnotAnalyzerImpl>(*other.impl);
        crossing_count = other.crossing_count;
        writhe = other.writhe;
        pd_code = other.pd_code;
        gauss_code = other.gauss_code;
        squared_gyradius = other.squared_gyradius;
        link_component_count = other.link_component_count;
        unlink_count = other.unlink_count;
        is_prime = other.is_prime;
        is_composite = other.is_composite;
        prime_component_count = other.prime_component_count;
        prime_components = other.prime_components;
    }
    return *this;
}

// Move assignment
KnotAnalyzer& KnotAnalyzer::operator=(KnotAnalyzer&& other) noexcept {
    if (this != &other) {
        impl = std::move(other.impl);
        crossing_count = other.crossing_count;
        writhe = other.writhe;
        pd_code = std::move(other.pd_code);
        gauss_code = std::move(other.gauss_code);
        squared_gyradius = other.squared_gyradius;
        link_component_count = other.link_component_count;
        unlink_count = other.unlink_count;
        is_prime = other.is_prime;
        is_composite = other.is_composite;
        prime_component_count = other.prime_component_count;
        prime_components = std::move(other.prime_components);
    }
    return *this;
}

// Destructor
KnotAnalyzer::~KnotAnalyzer() = default;

// Helper methods for PD code analysis
std::string KnotAnalyzer::get_pd_code_unsigned() const {
    const PD_T* primary = impl->get_primary_diagram();
    if (!primary) return "";
    return pd_to_string_unsigned(*primary);
}

std::vector<std::vector<int>> KnotAnalyzer::get_pd_code_matrix() const {
    std::vector<std::vector<int>> result;

    const PD_T* primary = impl->get_primary_diagram();
    if (!primary) return result;

    auto pdcode = primary->PDCode();
    int num_crossings = pdcode.Dimension(0);

    result.reserve(num_crossings);

    for (int i = 0; i < num_crossings; ++i) {
        std::vector<int> crossing(5);  // 4 arcs + handedness
        for (int j = 0; j < 5; ++j) {
            crossing[j] = static_cast<int>(pdcode(i, j));
        }
        result.push_back(crossing);
    }

    return result;
}

std::vector<int> KnotAnalyzer::get_crossing_handedness() const {
    std::vector<int> handedness;

    const PD_T* primary = impl->get_primary_diagram();
    if (!primary) return handedness;

    auto pdcode = primary->PDCode();
    int num_crossings = pdcode.Dimension(0);

    handedness.reserve(num_crossings);

    for (int i = 0; i < num_crossings; ++i) {
        handedness.push_back(static_cast<int>(pdcode(i, 4)));  // 5th entry is handedness
    }

    return handedness;
}

// Alexander polynomial methods
#ifdef USE_UMFPACK
AlexanderResult KnotAnalyzer::alexander(const std::complex<double>& z) const {
    AlexanderResult result;

    const PD_T* primary = impl->get_primary_diagram();
    if (!primary || primary->CrossingCount() == 0) {
        result.mantissa = 1.0;
        result.exponent = 0;
        return result;
    }

    // The Alexander polynomial is not defined for multi-component links;
    // check here so the library does not print an error to the console.
    if (primary->LinkComponentCount() > Int(1)) {
        result.mantissa = 0.0;
        result.exponent = 0;
        return result;
    }

    FPEnvGuard fp_guard;

    try {
        Alexander_T& alex_calc = impl->get_alexander_calculator();

        Complex arg = Complex(z.real(), z.imag());
        Complex mantissa;
        Int exponent;

        const int status =
            alex_calc.Alexander(*primary, arg, mantissa, exponent, false);
        if (status != 0) {
            result.mantissa = 0.0;
            result.exponent = 0;
            return result;
        }

        // For real inputs, return signed real part (preserves sign of Delta(-1) etc.)
        // For complex inputs, return magnitude (true invariant on the unit circle)
        if (z.imag() == 0.0) {
            result.mantissa = std::real(mantissa);
        } else {
            result.mantissa = std::abs(mantissa);
        }
        result.exponent = exponent;

    } catch (const std::exception& e) {
        std::cerr << "Error computing Alexander polynomial: " << e.what() << std::endl;
        result.mantissa = 0.0;
        result.exponent = 0;
    }

    return result;
}

AlexanderResult KnotAnalyzer::alexander(double t) const {
    return alexander(std::complex<double>(t, 0.0));
}

std::vector<AlexanderResult> KnotAnalyzer::alexander(const std::vector<std::complex<double>>& points) const {
    std::vector<AlexanderResult> results;
    results.reserve(points.size());

    const PD_T* primary = impl->get_primary_diagram();
    if (!primary || primary->CrossingCount() == 0) {
        // Unknot case
        for (size_t i = 0; i < points.size(); ++i) {
            results.push_back({1.0, 0});
        }
        return results;
    }

    // The Alexander polynomial is not defined for multi-component links;
    // check here so the library does not print an error to the console.
    if (primary->LinkComponentCount() > Int(1)) {
        for (size_t i = 0; i < points.size(); ++i) {
            results.push_back({0.0, 0});
        }
        return results;
    }

    FPEnvGuard fp_guard;

    try {
        Alexander_T& alex_calc = impl->get_alexander_calculator();

        // Convert input points to Complex
        std::vector<Complex> args;
        args.reserve(points.size());
        for (const auto& p : points) {
            args.emplace_back(p.real(), p.imag());
        }

        // Prepare output arrays
        std::vector<Complex> mantissas(points.size());
        std::vector<Int> exponents(points.size());

        // Compute Alexander polynomial for all points
        const int status = alex_calc.Alexander(
            *primary, args.data(), static_cast<Int>(args.size()),
            mantissas.data(), exponents.data(), false);
        if (status != 0) {
            for (size_t i = 0; i < points.size(); ++i) {
                results.push_back({0.0, 0});
            }
            return results;
        }

        // Convert results
        for (size_t i = 0; i < points.size(); ++i) {
            AlexanderResult result;
            if (points[i].imag() == 0.0) {
                result.mantissa = std::real(mantissas[i]);
            } else {
                result.mantissa = std::abs(mantissas[i]);
            }
            result.exponent = exponents[i];
            results.push_back(result);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error computing Alexander polynomial batch: " << e.what() << std::endl;
        // Return zeros on error
        for (size_t i = 0; i < points.size(); ++i) {
            results.push_back({0.0, 0});
        }
    }

    return results;
}

std::vector<AlexanderResult> KnotAnalyzer::alexander(const std::vector<double>& points) const {
    std::vector<std::complex<double>> complex_points;
    complex_points.reserve(points.size());
    for (double t : points) {
        complex_points.emplace_back(t, 0.0);
    }

    return alexander(complex_points);
}

std::map<std::string, AlexanderResult> KnotAnalyzer::alexander_invariants() const {
    std::map<std::string, AlexanderResult> invariants;

    // Common evaluation points for Alexander polynomial
    std::vector<std::pair<std::string, std::complex<double>>> test_points = {
        {"at_minus_1", std::complex<double>(-1.0, 0.0)},
        {"at_1", std::complex<double>(1.0, 0.0)},
        {"at_i", std::complex<double>(0.0, 1.0)},
        {"at_minus_i", std::complex<double>(0.0, -1.0)},
        {"at_omega", std::complex<double>(-0.5, std::sqrt(3.0)/2.0)}, // primitive cube root of unity
        {"at_omega2", std::complex<double>(-0.5, -std::sqrt(3.0)/2.0)} // omega^2
    };

    for (const auto& [name, point] : test_points) {
        invariants[name] = alexander(point);
    }

    return invariants;
}
#else
// Stub implementations when UMFPACK not available
AlexanderResult KnotAnalyzer::alexander(const std::complex<double>&) const {
    std::cerr << "Alexander polynomial requires UMFPACK (not available)" << std::endl;
    return {0.0, 0};
}

AlexanderResult KnotAnalyzer::alexander(double) const {
    std::cerr << "Alexander polynomial requires UMFPACK (not available)" << std::endl;
    return {0.0, 0};
}

std::vector<AlexanderResult> KnotAnalyzer::alexander(const std::vector<std::complex<double>>& points) const {
    std::cerr << "Alexander polynomial requires UMFPACK (not available)" << std::endl;
    return std::vector<AlexanderResult>(points.size(), {0.0, 0});
}

std::vector<AlexanderResult> KnotAnalyzer::alexander(const std::vector<double>& points) const {
    std::cerr << "Alexander polynomial requires UMFPACK (not available)" << std::endl;
    return std::vector<AlexanderResult>(points.size(), {0.0, 0});
}

std::map<std::string, AlexanderResult> KnotAnalyzer::alexander_invariants() const {
    std::cerr << "Alexander polynomial requires UMFPACK (not available)" << std::endl;
    return {};
}
#endif

// Convenience functions that create a KnotAnalyzer internally
std::string get_pd_code(const std::vector<double>& coordinates, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.get_pd_code();
}

std::string get_pd_code_unsigned(const std::vector<double>& coordinates, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.get_pd_code_unsigned();
}

std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.get_gauss_code();
}

bool is_unknot(const std::vector<double>& coordinates) {
    KnotAnalyzer analyzer(coordinates, true);
    return analyzer.is_unknot();
}

AlexanderResult alexander(const std::vector<double>& coordinates, double t, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.alexander(t);
}

AlexanderResult alexander(const std::vector<double>& coordinates, const std::complex<double>& z, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.alexander(z);
}


// Both link invariants from a single diagram build (valid for links):
//  - the full m x (m+2) Alexander face matrix at z (+ two adjacent faces to
//    delete) for the determinant, and
//  - the m x 5 PD code for the pairwise linking numbers.
std::tuple<std::vector<std::vector<std::complex<double>>>,
           std::pair<int,int>,
           std::vector<std::vector<int>>>
link_invariants(
    const std::vector<double>& coordinates,
    const std::vector<long>& edges,
    const std::complex<double>& z,
    bool simplify)
{
    std::vector<std::vector<std::complex<double>>> mat;
    std::pair<int,int> drop(-1, -1);
    std::vector<std::vector<int>> pd_out;

    FPEnvGuard fp_guard;

    try {
        const long n_edges = static_cast<long>(edges.size() / 2);

        // LinkEmbedding needs per-edge colors (component ids). Derive them
        // from the edge list with a union-find over the vertices, then
        // relabel roots to 0..k-1 in first-appearance order. Types match
        // 'edges' (long) because the LinkEmbedding constructor takes both
        // arrays through the same template parameter.
        long n_vertices = 0;
        for (long v : edges) {
            n_vertices = std::max(n_vertices, v + 1);
        }
        std::vector<long> parent(static_cast<size_t>(n_vertices));
        for (long v = 0; v < n_vertices; ++v) { parent[static_cast<size_t>(v)] = v; }
        auto find = [&parent](long v) {
            while (parent[static_cast<size_t>(v)] != v) {
                parent[static_cast<size_t>(v)] =
                    parent[static_cast<size_t>(parent[static_cast<size_t>(v)])];
                v = parent[static_cast<size_t>(v)];
            }
            return v;
        };
        for (long e = 0; e < n_edges; ++e) {
            parent[static_cast<size_t>(find(edges[static_cast<size_t>(2 * e)]))] =
                find(edges[static_cast<size_t>(2 * e + 1)]);
        }
        std::vector<long> edge_colors(static_cast<size_t>(n_edges));
        std::map<long, long> color_of_root;
        for (long e = 0; e < n_edges; ++e) {
            const long root = find(edges[static_cast<size_t>(2 * e)]);
            auto [it, inserted] =
                color_of_root.emplace(root, static_cast<long>(color_of_root.size()));
            edge_colors[static_cast<size_t>(e)] = it->second;
        }

        // Build the diagram from the embedding, but retry with random
        // rotations (as Reapr's rattle does before re-projecting) when the
        // plain z-projection is degenerate, e.g. for a component lying in a
        // plane that contains the z-axis.
        using Link_T = LinkEmbedding<Real, Int, Real>;
        Link_T L( edges.data(), edge_colors.data(), n_edges );

        static thread_local PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };

        constexpr int max_attempts = 10;
        int err = -1;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            if (attempt == 0) {
                L.ReadVertexCoordinates( coordinates.data() );
            } else {
                L = Link_T( edges.data(), edge_colors.data(), n_edges );
                L.SetTransformationMatrix( random_rotation(random_engine) );
                L.template ReadVertexCoordinates<true,true>( coordinates.data() );
            }
            err = L.template FindIntersections<false>();
            if (err == 0) { break; }
        }
        if (err != 0) {
            std::cerr << "link_invariants: FindIntersections failed after "
                      << max_attempts << " projection attempts (error code "
                      << err << ")." << std::endl;
            return {mat, drop, pd_out};
        }

        auto [pd0, unlink_colors] = PD_T::FromLinkEmbedding(L);
        Int unlinks = unlink_colors.Size();

        // Simplification goes through the diagram complex, but with
        // disconnect/split turned off so the link diagram stays whole (the
        // old Simplify4 behavior). Trivial loops split off by the complex are
        // counted as unlinks.
        PD_T pd = std::move(pd0);
        if (simplify && pd.CrossingCount() > Int(0)) {
            PDC_T pdc { std::move(pd) };
            PDC_T::Simplify_Args_T args;
            args.disconnectQ = false;
            args.splitQ = false;
            args.embedding_trials = 0;
            args.rotation_trials = 0;
            pdc.Simplify(args);

            Int primary_idx = -1;
            for (Int i = 0; i < pdc.DiagramCount(); ++i) {
                const PD_T& d = pdc.Diagram(i);
                if (d.ProvenUnknotQ() || d.CrossingCount() == Int(0)) {
                    unlinks++;
                } else {
                    primary_idx = i;
                }
            }
            pd = (primary_idx >= 0) ? pdc.Diagram(primary_idx) : PD_T();
        }

        const Int m = pd.CrossingCount();
        if (m == 0) {
            return {mat, drop, pd_out};  // no crossings -> unknot/split unlink
        }

        // PD code (m x 5): [under_in, over, under_out, over_other, handedness].
        auto pdcode = pd.PDCode<Int, pd5_targs>();
        const Int rows = pdcode.Dimension(0);
        pd_out.assign(static_cast<size_t>(rows), std::vector<int>(5));
        for (Int i = 0; i < rows; ++i) {
            for (Int j = 0; j < 5; ++j) {
                pd_out[static_cast<size_t>(i)][static_cast<size_t>(j)]
                    = static_cast<int>(pdcode(i, j));
            }
        }

        // The determinant recipe needs a connected diagram of the whole link:
        // for a split diagram FaceCount != m+2, and split unknot components
        // make the determinant 0 anyway. Return the PD code alone in that case.
        if (pd.DiagramComponentCount() > Int(1) || unlinks > Int(0)) {
            return {mat, drop, pd_out};
        }

        const Int F = pd.FaceCount();
        AlexanderFaceMatrix<Complex, Int, LInt> afm;
        std::vector<Complex> buf(static_cast<size_t>(m) * static_cast<size_t>(F), Complex(0));
        afm.template WriteDenseMatrix<true>(pd, static_cast<Complex>(z), buf.data());

        // Two adjacent faces = right/left faces of the outgoing arc of the
        // first active crossing (that arc borders exactly those two faces).
        const auto & C_arcs  = pd.Crossings();
        const auto & A_faces = pd.ArcFaces();
        cptr<CrossingState_T> C_state = pd.CrossingStates().data();
        const Int c_count = pd.MaxCrossingCount();
        Int d1 = -1, d2 = -1;
        for (Int c = 0; c < c_count; ++c) {
            const CrossingState_T s = C_state[c];
            if (RightHandedQ(s) || LeftHandedQ(s)) {
                const Int a_out = C_arcs(c, PD_T::Out, PD_T::Right);
                d1 = A_faces(a_out, 0);
                d2 = A_faces(a_out, 1);
                break;
            }
        }
        if (d1 < 0 || d2 < 0 || d1 == d2) {
            return {mat, drop, pd_out};  // pd still usable for linking numbers
        }

        mat.assign(static_cast<size_t>(m),
                   std::vector<std::complex<double>>(static_cast<size_t>(F)));
        for (Int i = 0; i < m; ++i) {
            for (Int j = 0; j < F; ++j) {
                mat[static_cast<size_t>(i)][static_cast<size_t>(j)]
                    = buf[static_cast<size_t>(i) * static_cast<size_t>(F) + static_cast<size_t>(j)];
            }
        }
        drop = { static_cast<int>(d1), static_cast<int>(d2) };
    } catch (const std::exception& e) {
        std::cerr << "link_invariants error: " << e.what() << std::endl;
        mat.clear();
        drop = {-1, -1};
        pd_out.clear();
    }
    return {mat, drop, pd_out};
}

// Prime knot identification via the Klut MacLeod-code lookup tables
class KnotLookupTableImpl {
public:
    Klut table;

    KnotLookupTableImpl(const std::string& path, int max_crossings)
        : table(std::filesystem::path(path), static_cast<Size_T>(max_crossings)) {
        // Load eagerly so IO problems surface at construction, not first use.
        table.LoadSubtables();
    }
};

// Resolve the table directory for an empty user path: $KNOODLE_KLUT_DIR,
// then the repository data/Klut baked in at build time, then ./data/Klut.
static std::string resolve_klut_dir(const std::string& path) {
    if (!path.empty()) {
        return path;
    }
    if (const char* env = std::getenv("KNOODLE_KLUT_DIR")) {
        if (env[0] != '\0') {
            return std::string(env);
        }
    }
#ifdef KNOODLE_KLUT_DEFAULT
    if (std::filesystem::exists(std::filesystem::path(KNOODLE_KLUT_DEFAULT))) {
        return std::string(KNOODLE_KLUT_DEFAULT);
    }
#endif
    return std::string("data/Klut");
}

// Convert a raw Klut label K[c,i,j,"coset"] to the KnotInfo-convention short
// name: "c_i" for c <= 10, "ca_i" / "cn_i" for c >= 11 (j = alternating
// flag). Labels that do not parse are returned unchanged.
static std::string short_knot_name(const std::string& raw) {
    if (raw.size() < 4 || raw.compare(0, 2, "K[") != 0) {
        return raw;
    }
    long c = 0, i = 0, j = 0;
    if (std::sscanf(raw.c_str(), "K[%ld,%ld,%ld,", &c, &i, &j) != 3) {
        return raw;
    }
    if (c <= 10) {
        return std::to_string(c) + "_" + std::to_string(i);
    }
    return std::to_string(c) + (j ? "a" : "n") + "_" + std::to_string(i);
}

// Shared raw lookup: returns the K[...] label, or "" for the Klut sentinels.
static std::string klut_lookup_raw(Klut& table,
                                   const std::vector<uint8_t>& macleod_code) {
    // One byte per crossing; codes longer than the library maximum cannot be
    // in any table (and must not reach MacLeodCodeToKey, whose key buffer
    // holds only 16 bytes).
    if (macleod_code.empty() ||
        macleod_code.size() > Klut::max_crossing_count) {
        return "";
    }

    std::string name = table.FindName(
        macleod_code.data(), macleod_code.size());

    if (name == "NotFound" || name == "Invalid" || name == "Error") {
        return "";
    }
    return name;
}

KnotLookupTable::KnotLookupTable(const std::string& path, int max_crossings) {
    namespace fs = std::filesystem;

    const std::string dir = resolve_klut_dir(path);

    const int limit = std::min(
        max_crossings, static_cast<int>(Klut::max_crossing_count));

    // Probe which table files are present so we can load up to the highest
    // available crossing count and give a clear error when there are none.
    int highest_found = 0;
    for (int c = 3; c <= limit; ++c) {
        const std::string s = (c < 10 ? "0" : "") + std::to_string(c);
        const fs::path k = fs::path(dir) / ("Klut_Keys_" + s + ".bin");
        const fs::path v = fs::path(dir) / ("Klut_Values_" + s + ".tsv");
        if (fs::exists(k) && fs::exists(v)) {
            highest_found = c;
        }
    }

    if (highest_found == 0) {
        throw std::runtime_error(
            "KnotLookupTable: no lookup tables (Klut_Keys_NN.bin / "
            "Klut_Values_NN.tsv, NN = 03..13) found in '" + dir + "'. "
            "The tables ship in the repository at data/Klut via git LFS; "
            "run 'git lfs pull' in the repository (or point the path "
            "argument or $KNOODLE_KLUT_DIR at a table directory).");
    }

    impl = std::make_shared<KnotLookupTableImpl>(dir, highest_found);
}

std::string KnotLookupTable::lookup(const std::vector<uint8_t>& macleod_code) const {
    return short_knot_name(klut_lookup_raw(impl->table, macleod_code));
}

std::string KnotLookupTable::lookup(const KnotAnalyzer& analyzer) const {
    // A fully simplified unknot leaves no diagram with crossings: at most one
    // trivial component remains, counted as an unlink by the diagram complex
    // (a split unlink of several circles has unlink_count > 1 and is not the
    // unknot).
    if (analyzer.crossing_count == 0 && analyzer.unlink_count <= 1) {
        return "0_1";
    }
    return lookup(analyzer.macleod_code());
}

std::string KnotLookupTable::lookup_raw(const std::vector<uint8_t>& macleod_code) const {
    return klut_lookup_raw(impl->table, macleod_code);
}

std::string KnotLookupTable::lookup_raw(const KnotAnalyzer& analyzer) const {
    if (analyzer.crossing_count == 0 && analyzer.unlink_count <= 1) {
        return "0_1";
    }
    return klut_lookup_raw(impl->table, analyzer.macleod_code());
}

int KnotLookupTable::max_crossings() const {
    return static_cast<int>(impl->table.CrossingCount());
}

// MacLeod code methods
std::vector<uint8_t> KnotAnalyzer::macleod_code() const {
    std::vector<uint8_t> result;

    const PD_T* primary = impl->get_primary_diagram();
    if (!primary || primary->CrossingCount() == 0) {
        return result;  // Empty for unknot
    }

    if (primary->LinkComponentCount() > 1) {
        std::cerr << "MacLeod code not defined for links with multiple components" << std::endl;
        return result;
    }

    try {
        auto code = primary->template MacLeodCode<UInt8>();
        result.reserve(code.Size());
        for (Int i = 0; i < code.Size(); ++i) {
            result.push_back(code[i]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error computing MacLeod code: " << e.what() << std::endl;
    }

    return result;
}

std::string KnotAnalyzer::macleod_string() const {
    const PD_T* primary = impl->get_primary_diagram();
    if (!primary || primary->CrossingCount() == 0) {
        return "";
    }

    if (primary->LinkComponentCount() > 1) {
        return "";
    }

    try {
        return primary->MacLeodString();
    } catch (const std::exception& e) {
        std::cerr << "Error computing MacLeod string: " << e.what() << std::endl;
        return "";
    }
}

bool KnotAnalyzer::proven_minimal() const {
    const PD_T* primary = impl->get_primary_diagram();
    // No primary diagram: a fully simplified unknot is trivially minimal.
    if (!primary) return crossing_count == 0;
    return primary->ProvenMinimalQ();
}

// Factory method to create from PD code
KnotAnalyzer KnotAnalyzer::from_pd_code(const std::vector<std::vector<int>>& pd_code, bool simplify, int simplify_level) {
    KnotAnalyzer result;

    if (pd_code.empty()) {
        // Empty PD code = unknot
        result.crossing_count = 0;
        result.writhe = 0;
        result.link_component_count = 1;
        result.unlink_count = 0;
        result.is_prime = true;
        result.is_composite = false;
        result.prime_component_count = 0;
        return result;
    }

    FPEnvGuard fp_guard;

    try {
        Int n = static_cast<Int>(pd_code.size());  // Number of crossings

        // Determine if PD code has 4 or 5 entries per crossing
        bool signed_pd = (pd_code[0].size() == 5);

        // Flatten PD code
        std::vector<Int> flat_pd;
        int entries_per_crossing = signed_pd ? 5 : 4;
        flat_pd.reserve(n * entries_per_crossing);

        for (const auto& crossing : pd_code) {
            if (static_cast<int>(crossing.size()) != entries_per_crossing) {
                throw std::runtime_error("Inconsistent PD code entry sizes");
            }
            for (int val : crossing) {
                flat_pd.push_back(static_cast<Int>(val));
            }
        }

        // Create the diagram from the PD code, then wrap it in a complex
        result.impl = std::make_shared<KnotAnalyzerImpl>();
        PD_T pd = signed_pd
            ? PD_T::FromSignedPDCode(flat_pd.data(), n, false, false)
            : PD_T::FromUnsignedPDCode(flat_pd.data(), n, false, false);
        result.impl->pdc = PDC_T(std::move(pd));

        // Validate
        if (result.impl->pdc.DiagramCount() == 0 ||
            result.impl->pdc.Diagram(0).InvalidQ()) {
            throw std::runtime_error("Invalid PD code - could not create valid diagram");
        }

        // Apply simplification if requested
        if (simplify) {
            auto args = make_simplify_args(simplify_level);
            result.impl->pdc.Simplify(args);
        }

        // Analyze the complex
        auto analysis = PDCAnalysis::analyze(result.impl->pdc);
        result.impl->primary_diagram_idx = analysis.first_nontrivial_idx;

        // Extract properties
        result.crossing_count = analysis.total_crossing_count;
        result.writhe = analysis.total_writhe;
        result.unlink_count = analysis.unlink_count;
        result.squared_gyradius = 0.0;  // No coordinates available

        const PD_T* primary = result.impl->get_primary_diagram();
        if (primary) {
            result.pd_code = pd_to_string(*primary);
            result.gauss_code = gauss_to_string(*primary);
            result.link_component_count = primary->LinkComponentCount();
        } else {
            result.pd_code = "";
            result.gauss_code = "";
            result.link_component_count = 1;
        }

        // Prime decomposition
        if (analysis.nontrivial_count > 1) {
            result.is_composite = true;
            result.is_prime = false;
            result.prime_component_count = analysis.nontrivial_count;

            for (Int i = 0; i < result.impl->pdc.DiagramCount(); ++i) {
                const PD_T& comp_pd = result.impl->pdc.Diagram(i);
                if (comp_pd.ProvenUnknotQ() || comp_pd.CrossingCount() == 0) continue;

                KnotAnalyzer comp;
                comp.impl = std::make_shared<KnotAnalyzerImpl>();
                PD_T comp_copy = comp_pd;
                comp.impl->pdc = PDC_T(std::move(comp_copy));
                comp.impl->primary_diagram_idx = 0;
                comp.crossing_count = comp_pd.CrossingCount();
                comp.writhe = comp_pd.Writhe();
                comp.pd_code = pd_to_string(comp_pd);
                comp.gauss_code = gauss_to_string(comp_pd);
                comp.squared_gyradius = 0.0;
                comp.link_component_count = comp_pd.LinkComponentCount();
                comp.unlink_count = 0;
                comp.is_prime = true;
                comp.is_composite = false;
                comp.prime_component_count = 1;

                result.prime_components.push_back(std::move(comp));
            }
        } else if (analysis.nontrivial_count == 1) {
            result.is_prime = true;
            result.is_composite = false;
            result.prime_component_count = 1;
        } else {
            result.is_prime = true;
            result.is_composite = false;
            result.prime_component_count = 0;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error creating KnotAnalyzer from PD code: " << e.what() << std::endl;
        result.crossing_count = -1;
        result.pd_code = "Error: " + std::string(e.what());
    }

    return result;
}
