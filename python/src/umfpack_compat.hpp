#pragma once

// Compatibility stubs for single-precision UMFPACK functions
// UMFPACK only supports double precision, but the Tensors library has code paths
// for single precision. These stubs prevent compile errors for unused code paths.

#ifndef UMFPACK_SINGLE_PRECISION_STUBS
#define UMFPACK_SINGLE_PRECISION_STUBS

// Single precision (float) - not actually supported by UMFPACK
// These are stubs that should never be called at runtime

// Long int versions (sl = single float, long int)
inline void umfpack_sl_free_symbolic(void**) {}
inline void umfpack_sl_free_numeric(void**) {}
inline int umfpack_sl_symbolic(int, int, const long*, const long*, const float*, void**, const double*, double*) { return -1; }
inline int umfpack_sl_numeric(const long*, const long*, const float*, void*, void**, const double*, double*) { return -1; }
inline int umfpack_sl_solve(int, const long*, const long*, const float*, float*, const float*, void*, const double*, double*) { return -1; }

// Int versions (si = single float, int)
inline void umfpack_si_free_symbolic(void**) {}
inline void umfpack_si_free_numeric(void**) {}
inline int umfpack_si_symbolic(int, int, const int*, const int*, const float*, void**, const double*, double*) { return -1; }
inline int umfpack_si_numeric(const int*, const int*, const float*, void*, void**, const double*, double*) { return -1; }
inline int umfpack_si_solve(int, const int*, const int*, const float*, float*, const float*, void*, const double*, double*) { return -1; }

// Complex float long int versions (cl = complex float, long int)
inline void umfpack_cl_free_symbolic(void**) {}
inline void umfpack_cl_free_numeric(void**) {}
inline int umfpack_cl_symbolic(int, int, const long*, const long*, const float*, const float*, void**, const double*, double*) { return -1; }
inline int umfpack_cl_numeric(const long*, const long*, const float*, const float*, void*, void**, const double*, double*) { return -1; }
inline int umfpack_cl_solve(int, const long*, const long*, const float*, const float*, float*, float*, const float*, const float*, void*, const double*, double*) { return -1; }

// Complex float int versions (ci = complex float, int)
inline void umfpack_ci_free_symbolic(void**) {}
inline void umfpack_ci_free_numeric(void**) {}
inline int umfpack_ci_symbolic(int, int, const int*, const int*, const float*, const float*, void**, const double*, double*) { return -1; }
inline int umfpack_ci_numeric(const int*, const int*, const float*, const float*, void*, void**, const double*, double*) { return -1; }
inline int umfpack_ci_solve(int, const int*, const int*, const float*, const float*, float*, float*, const float*, const float*, void*, const double*, double*) { return -1; }

#endif // UMFPACK_SINGLE_PRECISION_STUBS
