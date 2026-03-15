#pragma once

#include <vector>
#include <string>
#include <memory>
#include <complex>
#include <map>

// Forward declaration for the implementation
class KnotAnalyzerImpl;

// Alexander polynomial result structure
struct AlexanderResult {
    double mantissa;
    int64_t exponent;
    
    // Convert to string representation
    std::string to_string() const;
    
    // Convert to double (may lose precision for large exponents)
    double to_double() const;
};

// Result structure that now owns the planar diagram
class KnotAnalyzer {
private:
    std::shared_ptr<KnotAnalyzerImpl> impl;
    
public:
    // Public properties
    int crossing_count;
    int writhe;
    std::string pd_code;
    std::string gauss_code;  // Note: This is extended Gauss code (one entry per arc)
    double squared_gyradius;
    int link_component_count;  // Number of link components (always 1 for single polygons)
    int unlink_count;  // Number of trivial unlinks
    bool is_prime;  // True if the knot is prime (not composite)
    bool is_composite;  // True if the knot is composite
    int prime_component_count;  // Total number of prime components (including remainder)
    std::vector<KnotAnalyzer> prime_components;  // Prime knot factors split off
    
    // Constructors
    KnotAnalyzer();
    KnotAnalyzer(const std::vector<double>& coordinates, bool simplify = true, int simplify_level = 5);

    // Factory method to create from PD code
    static KnotAnalyzer from_pd_code(const std::vector<std::vector<int>>& pd_code, bool simplify = true, int simplify_level = 5);
    
    // Copy and move constructors
    KnotAnalyzer(const KnotAnalyzer& other);
    KnotAnalyzer(KnotAnalyzer&& other) noexcept;
    KnotAnalyzer& operator=(const KnotAnalyzer& other);
    KnotAnalyzer& operator=(KnotAnalyzer&& other) noexcept;
    
    // Destructor
    ~KnotAnalyzer();
    
    // Methods to access properties without recreating the diagram
    int get_crossing_count() const { return crossing_count; }
    int get_writhe() const { return writhe; }
    const std::string& get_pd_code() const { return pd_code; }
    const std::string& get_gauss_code() const { return gauss_code; }
    double get_squared_gyradius() const { return squared_gyradius; }
    bool is_unknot() const { return crossing_count == 0 && link_component_count == 1; }
    int get_link_component_count() const { return link_component_count; }
    int get_unlink_count() const { return unlink_count; }
    bool get_is_prime() const { return is_prime; }
    bool get_is_composite() const { return is_composite; }
    int get_prime_component_count() const { return prime_component_count; }
    const std::vector<KnotAnalyzer>& get_prime_components() const { return prime_components; }
    
    // Helper methods for PD code analysis
    std::string get_pd_code_unsigned() const;  // Get 4-entry PD code (backward compatibility)
    std::vector<std::vector<int>> get_pd_code_matrix() const;  // Get as matrix of integers
    std::vector<int> get_crossing_handedness() const;  // Get just the handedness values
    
    // Alexander polynomial methods
    AlexanderResult alexander(const std::complex<double>& z) const;
    AlexanderResult alexander(double t) const;
    std::vector<AlexanderResult> alexander(const std::vector<std::complex<double>>& points) const;
    std::vector<AlexanderResult> alexander(const std::vector<double>& points) const;
    std::map<std::string, AlexanderResult> alexander_invariants() const;

    // MacLeod code for knot identification
    std::vector<uint8_t> macleod_code() const;
    std::string macleod_string() const;
    bool proven_minimal() const;
};

// Convenience functions that create a KnotAnalyzer internally
std::string get_pd_code(const std::vector<double>& coordinates, bool simplify = true);
std::string get_pd_code_unsigned(const std::vector<double>& coordinates, bool simplify = true);
std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify = true);
bool is_unknot(const std::vector<double>& coordinates);

// Alexander polynomial convenience functions
AlexanderResult alexander(const std::vector<double>& coordinates, double t, bool simplify = true);
AlexanderResult alexander(const std::vector<double>& coordinates, const std::complex<double>& z, bool simplify = true);