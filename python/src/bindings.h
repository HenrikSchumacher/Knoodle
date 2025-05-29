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
    int link_component_count;  // True number of link components
    int unlink_count;  // Number of trivial unlinks
    std::vector<KnotAnalyzer> components;  // Components split off during simplification
    
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
    bool is_unknot() const { return crossing_count == 0; }
    int get_link_component_count() const { return link_component_count; }
    int get_unlink_count() const { return unlink_count; }
    const std::vector<KnotAnalyzer>& get_components() const { return components; }
};

// Convenience functions that create a KnotAnalyzer internally
std::string get_pd_code(const std::vector<double>& coordinates, bool simplify = true);
std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify = true);
bool is_unknot(const std::vector<double>& coordinates);