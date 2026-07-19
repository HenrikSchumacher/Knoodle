#ifdef __linux__
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
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
#include <pybind11/complex.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "bindings.h"

namespace py = pybind11;

// Create the Python module
PYBIND11_MODULE(_knoodle, m) {
    m.doc() = "Python binding for the Knoodle knot analysis library";
    
    // Bind AlexanderResult struct
    py::class_<AlexanderResult>(m, "AlexanderResult")
        .def(py::init<>())
        .def_readwrite("mantissa", &AlexanderResult::mantissa)
        .def_readwrite("exponent", &AlexanderResult::exponent)
        .def("to_string", &AlexanderResult::to_string,
             "Convert to string representation")
        .def("to_double", &AlexanderResult::to_double,
             "Convert to double (may lose precision for large exponents)")
        .def("__str__", &AlexanderResult::to_string)
        .def("__repr__", [](const AlexanderResult& r) {
            return "AlexanderResult(mantissa=" + std::to_string(r.mantissa) + 
                   ", exponent=" + std::to_string(r.exponent) + ")";
        });
    
    py::class_<KnotAnalyzer>(m, "KnotAnalyzer")
        .def(py::init<>())
        .def(py::init<const std::vector<double>&, bool, int>(),
             py::arg("coordinates"),
             py::arg("simplify") = true,
             py::arg("simplify_level") = 5,
             "Create a KnotAnalyzer from 3D curve coordinates")
        .def_static("from_pd_code", &KnotAnalyzer::from_pd_code,
             py::arg("pd_code"),
             py::arg("simplify") = true,
             py::arg("simplify_level") = 5,
             "Create a KnotAnalyzer from a PD code (list of 4 or 5 element lists)")
        
        // Properties
        .def_readwrite("crossing_count", &KnotAnalyzer::crossing_count)
        .def_readwrite("writhe", &KnotAnalyzer::writhe)
        .def_readwrite("pd_code", &KnotAnalyzer::pd_code)
        .def_readwrite("gauss_code", &KnotAnalyzer::gauss_code)
        .def_readwrite("squared_gyradius", &KnotAnalyzer::squared_gyradius)
        .def_readwrite("link_component_count", &KnotAnalyzer::link_component_count)
        .def_readwrite("unlink_count", &KnotAnalyzer::unlink_count)
        .def_readwrite("is_prime", &KnotAnalyzer::is_prime)
        .def_readwrite("is_composite", &KnotAnalyzer::is_composite)
        .def_readwrite("prime_component_count", &KnotAnalyzer::prime_component_count)
        .def_readwrite("prime_components", &KnotAnalyzer::prime_components)
        
        // Methods
        .def("get_crossing_count", &KnotAnalyzer::get_crossing_count,
             "Get the crossing count")
        .def("get_writhe", &KnotAnalyzer::get_writhe,
             "Get the writhe")
        .def("get_pd_code", &KnotAnalyzer::get_pd_code,
             "Get the PD code (5 entries per crossing: 4 arcs + handedness)")
        .def("get_pd_code_unsigned", &KnotAnalyzer::get_pd_code_unsigned,
             "Get the unsigned PD code (4 entries per crossing: arcs only)")
        .def("get_pd_code_matrix", &KnotAnalyzer::get_pd_code_matrix,
             "Get PD code as matrix of integers [[arc1,arc2,arc3,arc4,handedness], ...]")
        .def("get_crossing_handedness", &KnotAnalyzer::get_crossing_handedness,
             "Get handedness values for all crossings (+1=right, -1=left)")
        .def("get_gauss_code", &KnotAnalyzer::get_gauss_code,
             "Get the extended Gauss code (one entry per arc, not per crossing)")
        .def("get_squared_gyradius", &KnotAnalyzer::get_squared_gyradius,
             "Get the squared radius of gyration")
        .def("is_unknot", &KnotAnalyzer::is_unknot,
             "Check if this is an unknot")
        .def("get_link_component_count", &KnotAnalyzer::get_link_component_count,
             "Get the number of link components")
        .def("get_unlink_count", &KnotAnalyzer::get_unlink_count,
             "Get the number of trivial unlinks")
        .def("get_is_prime", &KnotAnalyzer::get_is_prime,
             "Check if the knot is prime")
        .def("get_is_composite", &KnotAnalyzer::get_is_composite,
             "Check if the knot is composite")
        .def("get_prime_component_count", &KnotAnalyzer::get_prime_component_count,
             "Get the total number of prime components")
        .def("get_prime_components", &KnotAnalyzer::get_prime_components,
             "Get the prime knot components")
        
        // Alexander polynomial methods
        .def("alexander", 
             py::overload_cast<const std::complex<double>&>(&KnotAnalyzer::alexander, py::const_),
             py::arg("z"),
             "Compute Alexander polynomial at complex point z")
        .def("alexander", 
             py::overload_cast<double>(&KnotAnalyzer::alexander, py::const_),
             py::arg("t"),
             "Compute Alexander polynomial at real point t")
        .def("alexander", 
             py::overload_cast<const std::vector<std::complex<double>>&>(&KnotAnalyzer::alexander, py::const_),
             py::arg("points"),
             "Compute Alexander polynomial at multiple complex points")
        .def("alexander", 
             py::overload_cast<const std::vector<double>&>(&KnotAnalyzer::alexander, py::const_),
             py::arg("points"),
             "Compute Alexander polynomial at multiple real points")
        .def("alexander_invariants", &KnotAnalyzer::alexander_invariants,
             "Get Alexander polynomial evaluated at common invariant points")

        // MacLeod code methods for knot identification
        .def("macleod_code", &KnotAnalyzer::macleod_code,
             "Get MacLeod code as bytes (for knot table lookup)")
        .def("macleod_string", &KnotAnalyzer::macleod_string,
             "Get MacLeod code as string representation")
        .def("proven_minimal", &KnotAnalyzer::proven_minimal,
             "Check if diagram is proven to be minimal (e.g., alternating)")

        .def("__repr__", [](const KnotAnalyzer &r) {
            std::stringstream ss;
            ss << "KnotAnalyzer(crossings=" << r.crossing_count
               << ", writhe=" << r.writhe;
            if (r.is_composite) {
                ss << ", composite with " << r.prime_component_count << " prime factors";
            } else if (r.is_prime && r.crossing_count > 0) {
                ss << ", prime";
            } else if (r.crossing_count == 0) {
                ss << ", unknot";
            }
            ss << ")";
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
          "Get the PD code representation of a knot (5 entries per crossing: 4 arcs + handedness)");
    
    m.def("get_pd_code_unsigned", &get_pd_code_unsigned,
          py::arg("coordinates"), 
          py::arg("simplify") = true,
          "Get the unsigned PD code representation of a knot (4 entries per crossing: arcs only)");
    
    m.def("get_gauss_code", &get_gauss_code,
          py::arg("coordinates"),
          py::arg("simplify") = true,
          "Get the extended Gauss code representation of a knot (one entry per arc)");
    
    m.def("is_unknot", &is_unknot,
          py::arg("coordinates"),
          "Check if the given curve is an unknot");
    
    // Alexander polynomial convenience functions
    m.def("alexander", 
          py::overload_cast<const std::vector<double>&, double, bool>(&alexander),
          py::arg("coordinates"), py::arg("t"), py::arg("simplify") = true,
          "Compute Alexander polynomial at point t for given coordinates");

    m.def("alexander",
          py::overload_cast<const std::vector<double>&, const std::complex<double>&, bool>(&alexander),
          py::arg("coordinates"), py::arg("z"), py::arg("simplify") = true,
          "Compute Alexander polynomial at complex point z for given coordinates");

    // Prime knot identification via MacLeod code lookup tables
    py::class_<KnotLookupTable>(m, "KnotLookupTable")
        .def(py::init<const std::string&, int>(),
             py::arg("path"),
             py::arg("max_crossings") = 13,
             "Load prime-knot lookup tables from a directory containing "
             "Klut_Keys_NN.bin / Klut_Values_NN.tsv files (NN = 03..max_crossings, "
             "zero-padded; library maximum is 13). Loads up to the highest "
             "crossing count present; raises RuntimeError (with generation "
             "instructions) if the directory contains no tables. The tables are "
             "not shipped with pyknoodle -- see the README to generate them.")
        .def("lookup",
             [](const KnotLookupTable& t, const std::vector<uint8_t>& code) -> py::object {
                 std::string name = t.lookup(code);
                 return name.empty() ? py::object(py::none())
                                     : py::object(py::cast(name));
             },
             py::arg("macleod_code"),
             "Look up a short MacLeod code (bytes or list of ints, one byte per "
             "crossing); returns the prime knot name, or None if not in the table")
        .def("lookup",
             [](const KnotLookupTable& t, py::bytes code) -> py::object {
                 std::string s = code;
                 std::vector<uint8_t> v(s.begin(), s.end());
                 std::string name = t.lookup(v);
                 return name.empty() ? py::object(py::none())
                                     : py::object(py::cast(name));
             },
             py::arg("macleod_code"))
        .def("lookup",
             [](const KnotLookupTable& t, const KnotAnalyzer& analyzer) -> py::object {
                 std::string name = t.lookup(analyzer);
                 return name.empty() ? py::object(py::none())
                                     : py::object(py::cast(name));
             },
             py::arg("analyzer"),
             "Identify a KnotAnalyzer's (simplified, prime) diagram; returns the "
             "knot name, '0_1' for the unknot, or None if not in the table")
        .def_property_readonly("max_crossings", &KnotLookupTable::max_crossings,
             "Highest crossing count the table was built for");

    // Knot-shadow sieve for lookup-table generation
    m.def("sieve_shadows",
          [](const std::vector<long>& shadows, int crossing_count,
             int rattle_iter, int thread_count) {
              auto result = sieve_shadows(shadows, crossing_count,
                                          rattle_iter, thread_count);
              auto convert = [](auto& entries) {
                  py::list out;
                  for (auto& [code, rep] : entries) {
                      out.append(py::make_tuple(
                          py::bytes(reinterpret_cast<const char*>(code.data()),
                                    code.size()),
                          py::cast(rep)));
                  }
                  return out;
              };
              return py::make_tuple(convert(result.first),
                                    convert(result.second));
          },
          py::arg("shadows"), py::arg("crossing_count"),
          py::arg("rattle_iter") = 25, py::arg("thread_count") = 0,
          "Sieve knot shadows for lookup-table generation. shadows: flat "
          "unsigned PD codes (4 ints per crossing) of single-component "
          "shadows, all with crossing_count crossings. Enumerates every "
          "over/under assignment, simplifies with REAPR's Rattle, and returns "
          "(minimal, other): lists of (macleod_code bytes, c x 5 signed PD "
          "code) for diagrams that stayed at c crossings, split by whether "
          "minimality was proven. All four mirror/reversal transforms are "
          "included. thread_count <= 0 uses all cores.");

    // Multi-component link invariants (valid for links): determinant + linking.
    m.def("link_invariants", &link_invariants,
          py::arg("coordinates"), py::arg("edges"), py::arg("z"),
          py::arg("simplify") = true,
          "Both link invariants from one diagram. coordinates: flat AoS xyz of all "
          "components concatenated; edges: interleaved [tail,tip,...] vertex-index "
          "pairs, one closing cycle per component. Returns (A, (f1,f2), pd): A is the "
          "full m x (m+2) Alexander face matrix at z (delete adjacent columns f1,f2 "
          "-> det = Delta(z), |det| at z=-1 is the determinant); pd is the m x 5 PD "
          "code [under_in, over, under_out, over_other, handedness] for the pairwise "
          "linking numbers.");
}
