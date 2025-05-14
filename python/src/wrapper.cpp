// #define POLYFOLD_NO_QUATERNIONS

// Include minimal required headers from Knoodle
// We'll try to avoid the problematic headers
#include "Knoodle.hpp"
#include "src/PolyFold.hpp"

// Include our bridge header
#include "bindings.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>


using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

// Define the types we need
using Real = Real64;
using Int = Int64;
using LInt = Int64;
using BReal = Real64;
using PD_T = PlanarDiagram<Int>;

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

// Create a PlanarDiagram and apply simplification
PD_T create_planar_diagram(const std::vector<double>& coords, bool simplify, int simplify_level) {
    try {
        Int n = coords.size() / 3;
        
        // Create planar diagram from coordinates
        PD_T pd(coords.data(), n);
        
        // Apply simplification if requested
        if (simplify) {
            std::vector<PD_T> components;
            switch (simplify_level) {
                case 1:
                    pd.Simplify1();
                    break;
                case 2:
                    pd.Simplify2();
                    break;
                case 3:
                    pd.Simplify3(4);
                    break;
                case 4:
                    pd.Simplify4();
                    break;
                case 5:
                default:
                    pd.Simplify5(components);
                    break;
            }
        }
        
        return pd;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating planar diagram: " << e.what() << std::endl;
        return PD_T(); // Return empty diagram
    }
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
    auto gausscode = pd.ExtendedGaussCode();
    
    for (Int i = 0; i < gausscode.Size(); ++i) {
        ss << gausscode[i];
        if (i < gausscode.Size() - 1) ss << " ";
    }
    
    return ss.str();
}

// Implementation of bridge functions
KnotAnalyzer analyze_curve(const std::vector<double>& coordinates, 
                                       bool simplify,
                                       int simplify_level) {
    KnotAnalyzer result;
    
    try {
        // Create planar diagram with simplification
        PD_T pd = create_planar_diagram(coordinates, simplify, simplify_level);
        
        // Fill basic properties
        result.crossing_count = pd.CrossingCount();
        result.writhe = pd.Writhe();
        result.pd_code = pd_to_string(pd);
        result.gauss_code = gauss_to_string(pd);
        result.squared_gyradius = calculate_squared_gyradius(coordinates);
        
        // If we used Simplify5, get the components
        if (simplify && simplify_level == 5) {
            std::vector<PD_T> components;
            pd.Simplify5(components); // This will update components
            
            // Add components to the result
            for (PD_T& comp : components) {
                KnotAnalyzer comp_result;
                comp_result.crossing_count = comp.CrossingCount();
                comp_result.writhe = comp.Writhe();
                comp_result.pd_code = pd_to_string(comp);
                comp_result.gauss_code = gauss_to_string(comp);
                
                result.components.push_back(comp_result);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error in analyze_knoodle_curve: " << e.what() << std::endl;
        // Return default values on error
        result.crossing_count = -1;
        result.writhe = 0;
        result.pd_code = "Error: " + std::string(e.what());
    }
    
    return result;
}

std::string get_pd_code(const std::vector<double>& coordinates, bool simplify) {
    try {
        PD_T pd = create_planar_diagram(coordinates, simplify, 5);
        return pd_to_string(pd);
    }
    catch (const std::exception& e) {
        return "Error: " + std::string(e.what());
    }
}

std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify) {
    try {
        PD_T pd = create_planar_diagram(coordinates, simplify, 5);
        return gauss_to_string(pd);
    }
    catch (const std::exception& e) {
        return "Error: " + std::string(e.what());
    }
}

bool is_unknot(const std::vector<double>& coordinates) {
    try {
        PD_T pd = create_planar_diagram(coordinates, true, 5);
        return pd.CrossingCount() == 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in is_knoodle_unknot: " << e.what() << std::endl;
        return false;
    }
}