#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

// Forward declarations to avoid including everything
namespace Knoodle {
    template<typename Int>
    class PlanarDiagram;
}

// Create the bridge functions
namespace py = pybind11;

// Result structure
struct KnotAnalyzer {
    int crossing_count;
    int writhe;
    std::string pd_code;
    std::string gauss_code;
    double squared_gyradius;
    std::vector<KnotAnalyzer> components;
};

KnotAnalyzer analyze_curve(const std::vector<double>& coordinates, 
                                        bool simplify,
                                        int simplify_level);

std::string get_pd_code(const std::vector<double>& coordinates, bool simplify);

std::string get_gauss_code(const std::vector<double>& coordinates, bool simplify);

bool is_unknot(const std::vector<double>& coordinates);

// Create the Python module
PYBIND11_MODULE(_knoodle, m) {
    m.doc() = "Python binding for the Knoodle knot analysis library";
    
    py::class_<KnotAnalyzer>(m, "KnotAnalyzer")
        .def(py::init<>())
        .def_readwrite("crossing_count", &KnotAnalyzer::crossing_count)
        .def_readwrite("writhe", &KnotAnalyzer::writhe)
        .def_readwrite("pd_code", &KnotAnalyzer::pd_code)
        .def_readwrite("gauss_code", &KnotAnalyzer::gauss_code)
        .def_readwrite("squared_gyradius", &KnotAnalyzer::squared_gyradius)
        .def_readwrite("components", &KnotAnalyzer::components)
        .def("__repr__", [](const KnotAnalyzer &r) {
            std::stringstream ss;
            ss << "KnotAnalyzer(crossings=" << r.crossing_count
               << ", writhe=" << r.writhe
               << ", pd_code='" << r.pd_code << "'"
               << ", gauss_code='" << r.gauss_code << "'"
               << ", components=[" << r.components.size() << "])";
            return ss.str();
        });
    
    m.def("analyze_curve", &analyze_curve,
          py::arg("coordinates"),
          py::arg("simplify") = true,
          py::arg("simplify_level") = 5,
          "Analyze a 3D curve to extract knot properties");
    
    m.def("get_pd_code", &get_pd_code,
          py::arg("coordinates"),
          py::arg("simplify") = true,
          "Get the PD code representation of a knot");
    
    m.def("get_gauss_code", &get_gauss_code,
          py::arg("coordinates"),
          py::arg("simplify") = true,
          "Get the Gauss code representation of a knot");
    
    m.def("is_unknot", &is_unknot,
          py::arg("coordinates"),
          "Check if the given curve is an unknot");
}