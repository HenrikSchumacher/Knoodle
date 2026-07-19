// Include minimal required headers from Knoodle
#include "Knoodle.hpp"
#ifdef __linux__
#include "submodules/Tensors/OpenBLAS.hpp"
#endif
#include "src/PolyFold.hpp"

// Conditionally include Alexander if UMFPACK is available
#ifdef USE_UMFPACK
#ifndef __linux__
#include "submodules/Tensors/Accelerate.hpp"
#endif
#include "src/KnotInvariants/Alexander_UMFPACK.hpp"
#endif

// Face-matrix Alexander: valid for multi-component links.
#include "src/KnotInvariants/AlexanderFaceMatrix.hpp"

// Prime knot identification from MacLeod codes.
#include "src/PrimeKnotLookupTable.hpp"

// Classic Reapr (Rattle): used by the knot-shadow sieve.
#include "src/Reapr.hpp"


// Include our bridge header
#include "bindings.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <random>
#include <thread>

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

// Define the types we need
using Real = Real64;
using Int = Int64;
using LInt = Int;
using BReal = Real64;
using PD_T = PlanarDiagram<Int>;
using Complex = Complex64;

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

// Implementation class that holds the planar diagram
class KnotAnalyzerImpl {
public:
    std::unique_ptr<PD_T> pd;
#ifdef USE_UMFPACK
    mutable std::unique_ptr<Alexander_T> alexander_calc;
#endif

#ifdef USE_UMFPACK
    KnotAnalyzerImpl() : pd(nullptr), alexander_calc(nullptr) {}
#else
    KnotAnalyzerImpl() : pd(nullptr) {}
#endif

    KnotAnalyzerImpl(const KnotAnalyzerImpl& other) {
        if (other.pd) {
            pd = std::make_unique<PD_T>(*other.pd);
        }
        // Don't copy alexander_calc - it will be created on-demand
    }

#ifdef USE_UMFPACK
    KnotAnalyzerImpl(KnotAnalyzerImpl&& other) noexcept
        : pd(std::move(other.pd)), alexander_calc(std::move(other.alexander_calc)) {}
#else
    KnotAnalyzerImpl(KnotAnalyzerImpl&& other) noexcept
        : pd(std::move(other.pd)) {}
#endif

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

// Convert PD_T to PD code string
std::string pd_to_string(PD_T& pd) {
    std::stringstream ss;
    auto pdcode = pd.PDCode();
    
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
std::string pd_to_string_unsigned(PD_T& pd) {
    std::stringstream ss;
    auto pdcode = pd.PDCode();
    
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
std::string gauss_to_string(PD_T& pd) {
    std::stringstream ss;
    
    // Get the extended Gauss code (one entry per arc)
    auto extgausscode = pd.ExtendedGaussCode();
    Int code_size = extgausscode.Size();
    
    if (code_size == 0) {
        return "";
    }
    
    // The ExtendedGaussCode has one entry per arc (not per crossing)
    // For a knot with n crossings, there are 2n arcs
    // Each crossing is visited twice during the traversal
    
    // For now, return the extended code but mark it as such
    // A proper standard Gauss code would require different logic
    ss << "ext:";
    for (Int i = 0; i < code_size; ++i) {
        ss << extgausscode[i];
        if (i < code_size - 1) ss << " ";
    }
    
    return ss.str();
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
    
    try {
        Int n = coordinates.size() / 3;
        
        // Create planar diagram from coordinates
        impl->pd = std::make_unique<PD_T>(coordinates.data(), n);
        
        // Apply simplification if requested
        if (simplify && impl->pd) {
            std::vector<PD_T> comps;
            switch (simplify_level) {
                case 1:
                    impl->pd->Simplify1();
                    break;
                case 2:
                    impl->pd->Simplify2();
                    break;
                case 3:
                    impl->pd->Simplify3(4);
                    break;
                case 4:
                    impl->pd->Simplify4();
                    break;
                case 5:
                default:
                    impl->pd->Simplify5(comps);
                    
                    // Process prime components that were split off
                    for (PD_T& comp : comps) {
                        KnotAnalyzer comp_analyzer;
                        comp_analyzer.crossing_count = comp.CrossingCount();
                        comp_analyzer.writhe = comp.Writhe();
                        comp_analyzer.pd_code = pd_to_string(comp);
                        comp_analyzer.gauss_code = gauss_to_string(comp);
                        comp_analyzer.squared_gyradius = 0.0; // Components don't have coordinates
                        comp_analyzer.link_component_count = comp.LinkComponentCount();
                        comp_analyzer.unlink_count = comp.UnlinkCount();
                        comp_analyzer.is_prime = true; // Components from DisconnectSummands are prime
                        comp_analyzer.is_composite = false;
                        comp_analyzer.prime_component_count = 1;

                        // Keep the component's diagram so that methods like
                        // macleod_code() and alexander() work on it.
                        comp_analyzer.impl->pd = std::make_unique<PD_T>(std::move(comp));

                        prime_components.push_back(std::move(comp_analyzer));
                    }
                    break;
            }
        }
        
        // Extract properties once
        if (impl->pd) {
            crossing_count = impl->pd->CrossingCount();
            writhe = impl->pd->Writhe();
            pd_code = pd_to_string(*impl->pd);
            gauss_code = gauss_to_string(*impl->pd);
            squared_gyradius = calculate_squared_gyradius(coordinates);
            
            // Get the true link component count and unlink count
            link_component_count = impl->pd->LinkComponentCount();
            unlink_count = impl->pd->UnlinkCount();
            
            // Determine if knot is prime or composite
            if (prime_components.empty()) {
                // No components were split off
                is_prime = true;
                is_composite = false;
                prime_component_count = (crossing_count > 0) ? 1 : 0; // 1 if non-trivial, 0 if unknot
            } else {
                // Components were split off - this is a composite knot
                is_composite = true;
                is_prime = false;
                
                // Total prime components = split-off components + remainder (if non-trivial)
                prime_component_count = prime_components.size();
                if (crossing_count > 0) {
                    // The remainder after splitting is also a prime component
                    prime_component_count++;
                }
            }
        } else {
            throw std::runtime_error("Failed to create planar diagram");
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
    if (!impl->pd) return "";
    return pd_to_string_unsigned(*impl->pd);
}

std::vector<std::vector<int>> KnotAnalyzer::get_pd_code_matrix() const {
    std::vector<std::vector<int>> result;
    
    if (!impl->pd) return result;
    
    auto pdcode = impl->pd->PDCode();
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
    
    if (!impl->pd) return handedness;
    
    auto pdcode = impl->pd->PDCode();
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

    if (!impl->pd || impl->pd->CrossingCount() == 0) {
        result.mantissa = 1.0;
        result.exponent = 0;
        return result;
    }

    try {
        Alexander_T& alex_calc = impl->get_alexander_calculator();

        Complex arg = Complex(z.real(), z.imag());
        Complex mantissa;
        Int exponent;

        // Use batch API (single element) because the single-value Alexander()
        // takes mantissa/exponent by value, so results are never written back.
        alex_calc.Alexander(*impl->pd, &arg, Int(1), &mantissa, &exponent, false);

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

    if (!impl->pd || impl->pd->CrossingCount() == 0) {
        // Unknot case
        for (size_t i = 0; i < points.size(); ++i) {
            results.push_back({1.0, 0});
        }
        return results;
    }

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
        alex_calc.Alexander(*impl->pd, args.data(), static_cast<Int>(args.size()),
                           mantissas.data(), exponents.data(), false);

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
    try {
        // 'long' matches the element type of 'edges' so the Link constructor's
        // ExtInt template parameter deduces consistently for both arguments.
        const long n_edges = static_cast<long>(edges.size() / 2);

        // Build the diagram as PlanarDiagram(x, edges, n) would, but retry
        // with random rotations (as Reapr::Rattle does before re-projecting)
        // when the plain z-projection is degenerate, e.g. for a component
        // lying in a plane that contains the z-axis.
        using Link_T = Link_2D<Real, Int, Real>;
        Link_T L( edges.data(), n_edges );

        static thread_local PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };

        constexpr int max_attempts = 10;
        int err = -1;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            if (attempt == 0) {
                L.ReadVertexCoordinates( coordinates.data() );
            } else {
                // Link_2D's intersection error counters are never reset
                // between FindIntersections calls, so each retry needs a
                // fresh link, as Reapr::Rattle builds one per iteration.
                L = Link_T( edges.data(), n_edges );
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

        PD_T pd( L );

        if (simplify) {
            // Simplify4 handles multi-component links and keeps the diagram
            // whole. (Simplify5 is knot-only and would split off connect
            // summands, which we would have to discard here.)
            pd.Simplify4();
        }

        const Int m = pd.CrossingCount();
        if (m == 0) {
            return {mat, drop, pd_out};  // no crossings -> unknot/split unlink
        }

        // PD code (m x 5): [under_in, over, under_out, over_other, handedness].
        auto pdcode = pd.PDCode();
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
        // absorbed into UnlinkCount make the determinant 0 anyway. Return the
        // PD code alone in that case.
        if (pd.DiagramComponentCount() > Int(1) || pd.UnlinkCount() > Int(0)) {
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
        const Int c_count = C_arcs.Dim(0);
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

// Prime knot identification via MacLeod code lookup tables
class KnotLookupTableImpl {
public:
    PrimeKnotLookupTable table;

    KnotLookupTableImpl(const std::string& path, int max_crossings)
        : table(std::filesystem::path(path), static_cast<Size_T>(max_crossings)) {}
};

KnotLookupTable::KnotLookupTable(const std::string& path, int max_crossings) {
    namespace fs = std::filesystem;

    const int limit = std::min(
        max_crossings, static_cast<int>(PrimeKnotLookupTable::max_c_count));

    // Probe which table files are present so we can load up to the highest
    // available crossing count and give a clear error when there are none
    // (the tables are not shipped with pyknoodle).
    int highest_found = 0;
    for (int c = 3; c <= limit; ++c) {
        const std::string s = (c < 10 ? "0" : "") + std::to_string(c);
        const fs::path k = fs::path(path) / ("Klut_Keys_" + s + ".bin");
        const fs::path v = fs::path(path) / ("Klut_Values_" + s + ".tsv");
        if (fs::exists(k) && fs::exists(v)) {
            highest_found = c;
        }
    }

    if (highest_found == 0) {
        throw std::runtime_error(
            "KnotLookupTable: no lookup tables (Klut_Keys_NN.bin / "
            "Klut_Values_NN.tsv, NN = 03..13) found in '" + path + "'. "
            "The tables are not shipped with pyknoodle. To generate them: "
            "enumerate 4-valent planar graphs with plantri, sieve the minimal "
            "diagrams with Knoodle's PlantriSiever, and name the resulting "
            "classes with SnapPy or Regina; see the 'Knot identification' "
            "section of python/README.md for details.");
    }

    impl = std::make_shared<KnotLookupTableImpl>(path, highest_found);
}

std::string KnotLookupTable::lookup(const std::vector<uint8_t>& macleod_code) const {
    // One byte per crossing; codes longer than the library maximum cannot be
    // in any table (and must not reach KeyFromMacLeodCode, whose key buffer
    // holds only 16 bytes).
    if (macleod_code.empty() ||
        macleod_code.size() > PrimeKnotLookupTable::max_c_count) {
        return "";
    }

    std::string name = impl->table.LookupName(
        macleod_code.data(), static_cast<Int>(macleod_code.size()));

    return (name == "NotFound") ? std::string() : name;
}

std::string KnotLookupTable::lookup(const KnotAnalyzer& analyzer) const {
    // A fully simplified unknot has no active diagram left: its component is
    // absorbed into unlink_count and link_component_count drops to 0. (Do not
    // use is_unknot() here: it requires link_component_count == 1.)
    if (analyzer.crossing_count == 0 &&
        analyzer.link_component_count + analyzer.unlink_count == 1) {
        return "0_1";
    }
    return lookup(analyzer.macleod_code());
}

int KnotLookupTable::max_crossings() const {
    return static_cast<int>(impl->table.CrossingCount());
}

// Knot-shadow sieve for lookup-table generation, modeled on
// PlantriSiever::LoadPDCodes, but keeping one representative signed PD code
// per MacLeod code so the resulting knots can be identified downstream.
std::pair<
    std::vector<std::pair<std::vector<uint8_t>, std::vector<std::vector<int>>>>,
    std::vector<std::pair<std::vector<uint8_t>, std::vector<std::vector<int>>>>>
sieve_shadows(
    const std::vector<long>& shadows,
    int crossing_count,
    int rattle_iter,
    int thread_count)
{
    using Reapr_T = Reapr<Real, Int>;
    using Key_T   = std::string;                    // crossing_count bytes
    using Rep_T   = std::vector<std::vector<int>>;  // c x 5 signed PD code
    using Map_T   = std::map<Key_T, Rep_T>;

    if (crossing_count < 1 || crossing_count > 16) {
        throw std::invalid_argument(
            "sieve_shadows: crossing_count must be in [1, 16].");
    }
    const Int c = static_cast<Int>(crossing_count);
    const size_t shadow_size =
        size_t(4) * static_cast<size_t>(crossing_count);
    if (shadows.empty() || (shadows.size() % shadow_size != 0)) {
        throw std::invalid_argument(
            "sieve_shadows: shadows length must be a positive multiple of "
            "4 * crossing_count.");
    }
    const size_t shadow_count = shadows.size() / shadow_size;

    size_t n_threads = (thread_count > 0)
        ? static_cast<size_t>(thread_count)
        : static_cast<size_t>(std::max(1u, std::thread::hardware_concurrency()));
    n_threads = std::min(n_threads, shadow_count);

    std::vector<Map_T> thread_minimal(n_threads);
    std::vector<Map_T> thread_other(n_threads);

    Reapr_T reapr;

    ParallelDo(
        [&](const Size_T thread)
        {
            Reapr_T local_reapr = reapr;
            local_reapr.Reseed();

            const size_t job_begin = (shadow_count * thread) / n_threads;
            const size_t job_end = (shadow_count * (thread + 1)) / n_threads;
            const UInt64 i_max = UInt64(1) << crossing_count;

            typename PD_T::CrossingStateContainer_T C_state(c);

            Map_T& local_minimal = thread_minimal[thread];
            Map_T& local_other = thread_other[thread];

            for (size_t job = job_begin; job < job_end; ++job) {
                PD_T pd_0 = PD_T::FromUnsignedPDCode(
                    &shadows[shadow_size * job], long(c), long(0), true, false);

                if (pd_0.CrossingCount() != c) { continue; }  // invalid shadow

                for (UInt64 i = 0; i < i_max; ++i) {
                    for (Int j = 0; j < c; ++j) {
                        C_state[j] = BooleanToCrossingState(get_bit(i, j));
                    }

                    PD_T pd(
                        pd_0.Crossings().data(), C_state.data(),
                        pd_0.Arcs().data(), pd_0.ArcStates().data(),
                        c, Int(0), false);

                    auto pd_list = local_reapr.Rattle(pd, rattle_iter);

                    // Keep only diagrams Rattle could not reduce below c
                    // crossings (candidates for minimal diagrams).
                    if ((pd_list.size() != 1) ||
                        (pd_list[0].CrossingCount() != c)) {
                        continue;
                    }
                    const bool minimalQ = pd_list[0].ProvenMinimalQ();

                    // As in PlantriSiever: record the four chirality/reversal
                    // transforms of the _original_ diagram.
                    for (bool mirrorQ : {false, true}) {
                        for (bool reverseQ : {false, true}) {
                            PD_T pd_1 = pd.CachelessCopy();
                            pd_1.ChiralityTransform(mirrorQ, reverseQ);

                            auto code = pd_1.template MacLeodCode<UInt8>();
                            if (code.Size() != c) { continue; }  // not a knot

                            Key_T key(reinterpret_cast<const char*>(code.data()),
                                      static_cast<size_t>(c));
                            Map_T& target = minimalQ ? local_minimal : local_other;
                            if (target.count(key) == 0) {
                                auto pdcode = pd_1.PDCode();
                                Rep_T rep(static_cast<size_t>(c),
                                          std::vector<int>(5));
                                for (Int r = 0; r < c; ++r) {
                                    for (Int s = 0; s < 5; ++s) {
                                        rep[static_cast<size_t>(r)][static_cast<size_t>(s)]
                                            = static_cast<int>(pdcode(r, s));
                                    }
                                }
                                target.emplace(std::move(key), std::move(rep));
                            }
                        }
                    }
                }
            }
        },
        n_threads);

    // Merge; codes proven minimal by any thread trump "other" entries.
    Map_T minimal;
    Map_T other;
    for (auto& m : thread_minimal) { minimal.merge(m); }
    for (auto& m : thread_other) {
        for (auto& kv : m) {
            if (minimal.count(kv.first) == 0) {
                other.emplace(kv.first, std::move(kv.second));
            }
        }
    }

    auto to_output = [](Map_T& src) {
        std::vector<std::pair<std::vector<uint8_t>, Rep_T>> out;
        out.reserve(src.size());
        for (auto& kv : src) {
            std::vector<uint8_t> code(kv.first.begin(), kv.first.end());
            out.emplace_back(std::move(code), std::move(kv.second));
        }
        return out;
    };

    return { to_output(minimal), to_output(other) };
}

// MacLeod code methods
std::vector<uint8_t> KnotAnalyzer::macleod_code() const {
    std::vector<uint8_t> result;

    if (!impl->pd || impl->pd->CrossingCount() == 0) {
        return result;  // Empty for unknot
    }

    if (impl->pd->LinkComponentCount() > 1) {
        std::cerr << "MacLeod code not defined for links with multiple components" << std::endl;
        return result;
    }

    try {
        auto code = impl->pd->template MacLeodCode<UInt8>();
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
    if (!impl->pd || impl->pd->CrossingCount() == 0) {
        return "";
    }

    if (impl->pd->LinkComponentCount() > 1) {
        return "";
    }

    try {
        return impl->pd->MacLeodString();
    } catch (const std::exception& e) {
        std::cerr << "Error computing MacLeod string: " << e.what() << std::endl;
        return "";
    }
}

bool KnotAnalyzer::proven_minimal() const {
    if (!impl->pd) return false;
    return impl->pd->ProvenMinimalQ();
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

        // Create PlanarDiagram from PD code
        std::unique_ptr<PD_T> pd;
        if (signed_pd) {
            pd = std::make_unique<PD_T>(
                PD_T::FromSignedPDCode(flat_pd.data(), n, Int(0), true, false)
            );
        } else {
            pd = std::make_unique<PD_T>(
                PD_T::FromUnsignedPDCode(flat_pd.data(), n, Int(0), true, false)
            );
        }

        if (!pd->ValidQ()) {
            throw std::runtime_error("Invalid PD code - could not create valid diagram");
        }

        result.impl = std::make_shared<KnotAnalyzerImpl>();
        result.impl->pd = std::move(pd);

        // Apply simplification if requested
        if (simplify && result.impl->pd) {
            std::vector<PD_T> comps;
            switch (simplify_level) {
                case 1:
                    result.impl->pd->Simplify1();
                    break;
                case 2:
                    result.impl->pd->Simplify2();
                    break;
                case 3:
                    result.impl->pd->Simplify3(4);
                    break;
                case 4:
                    result.impl->pd->Simplify4();
                    break;
                case 5:
                default:
                    result.impl->pd->Simplify5(comps);

                    // Process prime components that were split off
                    for (PD_T& comp : comps) {
                        KnotAnalyzer comp_analyzer;
                        comp_analyzer.impl = std::make_shared<KnotAnalyzerImpl>();
                        comp_analyzer.impl->pd = std::make_unique<PD_T>(std::move(comp));
                        comp_analyzer.crossing_count = comp_analyzer.impl->pd->CrossingCount();
                        comp_analyzer.writhe = comp_analyzer.impl->pd->Writhe();
                        comp_analyzer.pd_code = pd_to_string(*comp_analyzer.impl->pd);
                        comp_analyzer.gauss_code = gauss_to_string(*comp_analyzer.impl->pd);
                        comp_analyzer.squared_gyradius = 0.0;
                        comp_analyzer.link_component_count = comp_analyzer.impl->pd->LinkComponentCount();
                        comp_analyzer.unlink_count = comp_analyzer.impl->pd->UnlinkCount();
                        comp_analyzer.is_prime = true;
                        comp_analyzer.is_composite = false;
                        comp_analyzer.prime_component_count = 1;

                        result.prime_components.push_back(std::move(comp_analyzer));
                    }
                    break;
            }
        }

        // Extract properties
        if (result.impl->pd) {
            result.crossing_count = result.impl->pd->CrossingCount();
            result.writhe = result.impl->pd->Writhe();
            result.pd_code = pd_to_string(*result.impl->pd);
            result.gauss_code = gauss_to_string(*result.impl->pd);
            result.squared_gyradius = 0.0;  // No coordinates available
            result.link_component_count = result.impl->pd->LinkComponentCount();
            result.unlink_count = result.impl->pd->UnlinkCount();

            if (result.prime_components.empty()) {
                result.is_prime = true;
                result.is_composite = false;
                result.prime_component_count = (result.crossing_count > 0) ? 1 : 0;
            } else {
                result.is_composite = true;
                result.is_prime = false;
                result.prime_component_count = result.prime_components.size();
                if (result.crossing_count > 0) {
                    result.prime_component_count++;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error creating KnotAnalyzer from PD code: " << e.what() << std::endl;
        result.crossing_count = -1;
        result.pd_code = "Error: " + std::string(e.what());
    }

    return result;
}
