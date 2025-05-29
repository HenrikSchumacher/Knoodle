#pragma once

#include <vector>
#include <string>
#include <memory>

// Forward declaration for the implementation
class KnotAnalyzerImpl;

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
};

// Convenience functions that create a KnotAnalyzer internally
std::string get_pd_code(const std::vector<double>& coordinates, bool simplify = true);
std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify = true);
bool is_unknot(const std::vector<double>& coordinates);