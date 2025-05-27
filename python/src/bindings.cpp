#ifdef __linux__
#include <cstdint>

#ifndef FASTINT8_DEFINED
#define FASTINT8_DEFINED
typedef std::int8_t FastInt8;
typedef std::int16_t FastInt16;
typedef std::int32_t FastInt32;
typedef std::int64_t FastInt64;
typedef std::uint8_t FastUInt8;
typedef std::uint16_t FastUInt16;
typedef std::uint32_t FastUInt32;
typedef std::uint64_t FastUInt64;
#endif

#ifndef TOOLS_PTIMER
#define TOOLS_PTIMER(name, description) do {} while(0)
#endif

#endif 

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "bindings.h"

namespace py = pybind11;

// Create the Python module
PYBIND11_MODULE(_knoodle, m) {
    m.doc() = "Python binding for the Knoodle knot analysis library";
    
    py::class_<KnotAnalyzer>(m, "KnotAnalyzer")
        .def(py::init<>())
        .def(py::init<const std::vector<double>&, bool, int>(),
             py::arg("coordinates"),
             py::arg("simplify") = true,
             py::arg("simplify_level") = 5,
             "Create a KnotAnalyzer from 3D curve coordinates")
        
        // Properties
        .def_readwrite("crossing_count", &KnotAnalyzer::crossing_count)
        .def_readwrite("writhe", &KnotAnalyzer::writhe)
        .def_readwrite("pd_code", &KnotAnalyzer::pd_code)
        .def_readwrite("gauss_code", &KnotAnalyzer::gauss_code)
        .def_readwrite("squared_gyradius", &KnotAnalyzer::squared_gyradius)
        .def_readwrite("components", &KnotAnalyzer::components)
        
        // Methods
        .def("get_crossing_count", &KnotAnalyzer::get_crossing_count,
             "Get the crossing count")
        .def("get_writhe", &KnotAnalyzer::get_writhe,
             "Get the writhe")
        .def("get_pd_code", &KnotAnalyzer::get_pd_code,
             "Get the PD code")
        .def("get_gauss_code", &KnotAnalyzer::get_gauss_code,
             "Get the Gauss code")
        .def("get_squared_gyradius", &KnotAnalyzer::get_squared_gyradius,
             "Get the squared radius of gyration")
        .def("is_unknot", &KnotAnalyzer::is_unknot,
             "Check if this is an unknot")
        .def("get_components", &KnotAnalyzer::get_components,
             "Get the knot components")
        
        .def("__repr__", [](const KnotAnalyzer &r) {
            std::stringstream ss;
            ss << "KnotAnalyzer(crossings=" << r.crossing_count
               << ", writhe=" << r.writhe
               << ", pd_code='" << r.pd_code << "'"
               << ", gauss_code='" << r.gauss_code << "'"
               << ", components=[" << r.components.size() << "])";
            return ss.str();
        });
    
    // Legacy function that creates a KnotAnalyzer (for backward compatibility)
    m.def("analyze_curve", [](const std::vector<double>& coordinates, bool simplify, int simplify_level) {
            return KnotAnalyzer(coordinates, simplify, simplify_level);
        },
        py::arg("coordinates"),
        py::arg("simplify") = true,
        py::arg("simplify_level") = 5,
        "Analyze a 3D curve to extract knot properties");
    
    // Convenience functions
    m.def("get_pd_code", &get_pd_code,
          py::arg("coordinates"),
          py::arg("simplify") = true,
          "Get the PD code representation of a knot");
    
    m.def("get_gauss_code", &get_gauss_code,
          py::arg("coordinates"),
          py::arg("simplify") = true,
          "Get the Extended Gauss code representation of a knot");
    
    m.def("is_unknot", &is_unknot,
          py::arg("coordinates"),
          "Check if the given curve is an unknot");
}