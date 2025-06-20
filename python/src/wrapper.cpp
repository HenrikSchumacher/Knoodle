// Include minimal required headers from Knoodle
#include "Knoodle.hpp"
#include "src/PolyFold.hpp"
#include "src/Alexander_UMFPACK.hpp"

// Include our bridge header
#include "bindings.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

// Define the types we need
using Real = Real64;
using Int = Int64;
using LInt = Int64;
using BReal = Real64;
using PD_T = PlanarDiagram<Int>;
using Complex = Complex64;

// Define Alexander polynomial calculator type
using Alexander_T = Alexander_UMFPACK<Complex, Int>;

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
    mutable std::unique_ptr<Alexander_T> alexander_calc;
    
    KnotAnalyzerImpl() : pd(nullptr), alexander_calc(nullptr) {}
    
    KnotAnalyzerImpl(const KnotAnalyzerImpl& other) {
        if (other.pd) {
            pd = std::make_unique<PD_T>(*other.pd);
        }
        // Don't copy alexander_calc - it will be created on-demand
    }
    
    KnotAnalyzerImpl(KnotAnalyzerImpl&& other) noexcept 
        : pd(std::move(other.pd)), alexander_calc(std::move(other.alexander_calc)) {}
    
    // Get or create Alexander calculator
    Alexander_T& get_alexander_calculator() const {
        if (!alexander_calc) {
            alexander_calc = std::make_unique<Alexander_T>();
        }
        return *alexander_calc;
    }
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
        
        alex_calc.Alexander(*impl->pd, arg, mantissa, exponent, false);
        
        // Convert complex result to real (Alexander polynomial should be real for real inputs)
        result.mantissa = std::real(mantissa);
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
            result.mantissa = std::real(mantissas[i]);
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