// Include minimal required headers from Knoodle
#include "Knoodle.hpp"
#include "src/PolyFold.hpp"

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

// Implementation class that holds the planar diagram
class KnotAnalyzerImpl {
public:
    std::unique_ptr<PD_T> pd;
    
    KnotAnalyzerImpl() : pd(nullptr) {}
    
    KnotAnalyzerImpl(const KnotAnalyzerImpl& other) {
        if (other.pd) {
            pd = std::make_unique<PD_T>(*other.pd);
        }
    }
    
    KnotAnalyzerImpl(KnotAnalyzerImpl&& other) noexcept : pd(std::move(other.pd)) {}
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
        for (Int j = 0; j < 4; ++j) {
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
                    
                    // Process components that were split off during simplification
                    for (PD_T& comp : comps) {
                        KnotAnalyzer comp_analyzer;
                        comp_analyzer.crossing_count = comp.CrossingCount();
                        comp_analyzer.writhe = comp.Writhe();
                        comp_analyzer.pd_code = pd_to_string(comp);
                        comp_analyzer.gauss_code = gauss_to_string(comp);
                        comp_analyzer.squared_gyradius = 0.0; // Components don't have coordinates
                        comp_analyzer.link_component_count = comp.LinkComponentCount();
                        comp_analyzer.unlink_count = comp.UnlinkCount();
                        
                        components.push_back(std::move(comp_analyzer));
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
      components(other.components) {
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
      components(std::move(other.components)) {
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
        components = other.components;
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
        components = std::move(other.components);
    }
    return *this;
}

// Destructor
KnotAnalyzer::~KnotAnalyzer() = default;

// Convenience functions that create a KnotAnalyzer internally
std::string get_pd_code(const std::vector<double>& coordinates, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.get_pd_code();
}

std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify) {
    KnotAnalyzer analyzer(coordinates, simplify);
    return analyzer.get_gauss_code();
}

bool is_unknot(const std::vector<double>& coordinates) {
    KnotAnalyzer analyzer(coordinates, true);
    return analyzer.is_unknot();
}