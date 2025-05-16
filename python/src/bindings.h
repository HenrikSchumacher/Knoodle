#pragma once

#include <vector>
#include <string>

// Result structure
struct KnotAnalyzer {
    int crossing_count;
    int writhe;
    std::string pd_code;
    std::string gauss_code;
    double squared_gyradius;
    std::vector<KnotAnalyzer> components;
};

// Bridge functions declaration
KnotAnalyzer analyze_curve(const std::vector<double>& coordinates, 
                                        bool simplify = true,
                                        int simplify_level = 5);

std::string get_pd_code(const std::vector<double>& coordinates, bool simplify = true);

std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify = true);

bool is_unknot(const std::vector<double>& coordinates);