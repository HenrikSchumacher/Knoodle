/**
 * @file embedding_fixtures.hpp
 * @brief Shared machinery for embedding_check: fixture loading, the exact
 *        integer shear that reproduces the Prosector symbolic perturbation,
 *        the crossing fingerprint, and an independent degeneracy census.
 *
 * The classes under test (`LinkEmbedding`, `LinkEmbedding2/3/4`) turn 3D
 * polygonal coordinates into the crossing data a `PlanarDiagram` is built from.
 * `LinkEmbedding2/3/4` are backed by `Prosector2/3/4`, which all document the
 * *same* symbolic perturbation: instead of projecting along `{0,0,1}` they
 * project along `{eps, eps^3, 1}` and take the limit `eps -> 0+`. That is what
 * lets them resolve degenerate projections (vertices over vertices, vertices on
 * segments, segments projecting to points, concurrent triples, collinear
 * overlaps) consistently.
 *
 * Two consequences are the backbone of the test:
 *
 *  1. Because the perturbation direction is *documented*, the correct answer on
 *     a degenerate input is exactly computable: shear the coordinates by
 *     eps = 1/N and clear denominators (`Shear` below). For N large enough the
 *     sheared projection is generic, so any correct implementation agrees with
 *     it crossing-for-crossing. No approximate oracle needed.
 *
 *  2. Because all three share the perturbation, they must agree with *each
 *     other* crossing-for-crossing on every input.
 *
 * The census is deliberately computed here, in exact integer arithmetic,
 * independently of the classes under test: it is what proves a fixture actually
 * drives the degenerate branches rather than silently going generic.
 *
 * Header-only, internal linkage; include into one translation unit.
 */

#pragma once

#include "../Knoodle.hpp"

// Knoodle.hpp pulls in LinkEmbedding and LinkEmbedding2 but not 3 and 4.
#include "../src/LinkEmbedding3.hpp"
#include "../src/LinkEmbedding4.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <vector>

namespace knoodle_test {

using Int   = std::int64_t;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;
using PD_T  = PDC_T::PD_T;

// Wide enough for exact products of coordinate differences in the census.
using Wide  = __int128;

// ===========================================================================
//  Curves
// ===========================================================================

/// A polygonal link in 3-space: interleaved coordinates plus component
/// boundaries. Each component is implicitly closed (last vertex back to first),
/// which is the convention `LinkEmbedding*` uses.
struct Curve
{
    std::string         name;
    std::vector<double> v;          ///< 3 * VertexCount(), interleaved
    std::vector<Int>    comp_ptr;   ///< component boundaries, size ComponentCount()+1

    Int VertexCount()    const { return Int(v.size() / 3); }
    Int ComponentCount() const { return Int(comp_ptr.size()) - Int(1); }

    /// Every coordinate is an exact integer. This is what makes the census and
    /// the shear exact, and it is also what makes `LinkEmbedding2/3/4` take
    /// their `input_integralQ` branch (no scaling, no rounding).
    bool IntegralQ() const
    {
        for( double x : v )
        {
            double ipart;
            if( std::modf(x,&ipart) != 0.0 ) { return false; }
        }
        return true;
    }

    double MaxAbsCoordinate() const
    {
        double m = 0.0;
        for( double x : v ) { m = std::max(m,std::abs(x)); }
        return m;
    }
};

/// Read a fixture: three whitespace-separated columns per vertex, a blank line
/// starting a new component. This is exactly the format
/// the library reader (`FromFile`) accepts, and the format `ndmansfield` emits.
/// Lines whose first non-space character is '#' are comments (our extension;
/// keep them out of files handed to the library reader).
inline bool LoadCurve( const std::string & path, Curve & c, std::string & error )
{
    std::ifstream in (path);
    if( !in )
    {
        error = "cannot open '" + path + "'";
        return false;
    }

    c = Curve{};
    c.name = path;
    c.comp_ptr.push_back(Int(0));

    std::string line;
    Int         n = 0;
    Int         line_no = 0;

    while( std::getline(in,line) )
    {
        ++line_no;

        const std::size_t first = line.find_first_not_of(" \t\r");
        if( first != std::string::npos && line[first] == '#' ) { continue; }

        std::istringstream iss (line);
        double x,y,z;

        if( !(iss >> x >> y >> z) )
        {
            if( first != std::string::npos )
            {
                error = path + ":" + std::to_string(line_no) + ": expected three numbers";
                return false;
            }
            // Blank line: component break (ignore repeats and a trailing blank).
            if( n > c.comp_ptr.back() ) { c.comp_ptr.push_back(n); }
            continue;
        }

        c.v.push_back(x); c.v.push_back(y); c.v.push_back(z);
        ++n;
    }

    if( n > c.comp_ptr.back() ) { c.comp_ptr.push_back(n); }

    if( c.ComponentCount() <= Int(0) )
    {
        error = path + ": no vertices";
        return false;
    }

    for( Int k = 0; k < c.ComponentCount(); ++k )
    {
        const Int len = c.comp_ptr[k+1] - c.comp_ptr[k];
        if( len < Int(3) )
        {
            error = path + ": component " + std::to_string(k) + " has "
                  + std::to_string(len) + " vertices (need >= 3)";
            return false;
        }
    }
    return true;
}

/// The exact integer realization of the Prosector symbolic perturbation.
///
/// Projecting along `{eps, eps^3, 1}` sends `(x,y,z)` to `(x - eps*z, y - eps^3*z)`.
/// Put `eps = 1/N` and multiply through by `N^3` to clear denominators:
///
///     X = N^3*x - N^2*z ,   Y = N^3*y - z ,   Z = N^3*z
///
/// This is a linear isomorphism of 3-space (determinant `N^9 > 0`), so it changes
/// neither the link type nor the edge numbering; and for N large enough its
/// *vertical* projection is generic and realizes the `eps -> 0+` limit. Feeding
/// the result back through the same class must therefore reproduce, exactly, the
/// crossings the class computed symbolically on the raw coordinates.
///
/// The caller escalates N until the answer stabilizes rather than assuming a
/// value is large enough — see `kShearLadder`.
inline Curve Shear( const Curve & c, double N )
{
    const double N2 = N*N;
    const double N3 = N2*N;

    Curve out = c;
    out.name = c.name + "[shear N=" + std::to_string(std::llround(N)) + "]";

    for( std::size_t i = 0; i < c.v.size(); i += 3 )
    {
        const double x = c.v[i], y = c.v[i+1], z = c.v[i+2];
        out.v[i  ] = N3*x - N2*z;
        out.v[i+1] = N3*y -     z;
        out.v[i+2] = N3*z;
    }
    return out;
}

/// The shear ladder. Powers of two so the products stay exact in `double`
/// (integral inputs remain integral, hence the classes stay on their exact
/// `input_integralQ` path). The largest entry costs a factor `N^3 = 2^36` on
/// top of the input magnitude, so a fixture with coordinates up to ~1e3 stays
/// far inside the 2^53 exactly-representable range.
inline const std::vector<double> & ShearLadder()
{
    static const std::vector<double> ladder { 4.0, 8.0, 16.0, 64.0, 256.0, 4096.0 };
    return ladder;
}

/// Largest coordinate magnitude a shear by `N` will produce, so the caller can
/// stop before leaving exact-integer territory instead of silently rounding.
inline double ShearMagnitude( const Curve & c, double N )
{
    return (N*N*N + N*N) * c.MaxAbsCoordinate();
}

/// A uniformly random rotation matrix (Shoemake's quaternion method), row-major.
/// Uniform over SO(3), so successive draws sample projection directions evenly
/// rather than clustering.
inline std::array<double,9> RandomRotationMatrix( std::uint64_t seed )
{
    std::mt19937_64 rng (seed);
    std::uniform_real_distribution<double> U (0.0,1.0);

    const double u1 = U(rng), u2 = U(rng), u3 = U(rng);
    const double s1 = std::sqrt(1.0-u1), s2 = std::sqrt(u1);
    const double t1 = 2.0*M_PI*u2,       t2 = 2.0*M_PI*u3;

    const double qx = s1*std::sin(t1), qy = s1*std::cos(t1);
    const double qz = s2*std::sin(t2), qw = s2*std::cos(t2);

    return {
        1-2*(qy*qy+qz*qz),   2*(qx*qy-qz*qw),   2*(qx*qz+qy*qw),
          2*(qx*qy+qz*qw), 1-2*(qx*qx+qz*qz),   2*(qy*qz-qx*qw),
          2*(qx*qz-qy*qw),   2*(qy*qz+qx*qw), 1-2*(qx*qx+qy*qy)
    };
}

/// A random integer matrix of determinant +1.
///
/// The integer counterpart of `RandomRotationMatrix`, and the reason it exists:
/// a rotation of a lattice point is not a lattice point, so a curve whose
/// `Real_` is integral cannot be randomly rotated -- casting back to the grid
/// gives a different, badly degenerate curve (and the rotation matrix itself
/// truncates to something singular). A matrix with INTEGER entries and
/// POSITIVE determinant has neither problem: it maps integer coordinates to
/// integer coordinates exactly, and because `GL+(3,R)` is connected it is
/// isotopic to the identity, so the image is the same knot type. It is simply
/// no longer a *lattice* configuration -- the edges stop being unit vectors --
/// which is exactly what makes the projection generic.
///
/// Built as a product of elementary integer shears, each of determinant 1, so
/// the product is unimodular by construction rather than by hoping. Entries
/// stay small: on the fixtures here the coordinates grow from 4 to about 36,
/// against a budget of `m = 60` bits (see `LinkEmbedding_Int/EdgeCoordinates.hpp`),
/// so exactness is never at risk for a single application.
///
/// CAUTION: do NOT compose these. Coordinates grow geometrically under
/// repeated application, and the exact path holds only while
/// `scaling_exponent >= 0`. Apply each transform to the ORIGINAL curve.
inline std::array<double,9> UnimodularIntegerMatrix( std::uint64_t seed )
{
    std::mt19937_64 rng (seed);
    std::uniform_int_distribution<int>  off (1,3);
    std::uniform_int_distribution<int>  dir (0,1);

    std::array<double,9> M { 1,0,0, 0,1,0, 0,0,1 };

    auto compose = [&M]( const std::array<double,9> & A )
    {
        std::array<double,9> P {};
        for( int r = 0; r < 3; ++r )
        for( int c = 0; c < 3; ++c )
        {
            double acc = 0;
            for( int k = 0; k < 3; ++k ) { acc += A[3*r+k] * M[3*k+c]; }
            P[3*r+c] = acc;
        }
        M = P;
    };

    constexpr int planes[3][2] = { {0,1}, {1,2}, {0,2} };

    for( int t = 0; t < 3; ++t )
    {
        std::array<double,9> S { 1,0,0, 0,1,0, 0,0,1 };
        const int i = planes[t][0], j = planes[t][1];
        // Either an upper or a lower shear in this plane; both have det 1, and
        // mixing them keeps the result from being triangular.
        if( dir(rng) ) { S[3*i+j] = off(rng); } else { S[3*j+i] = off(rng); }
        compose(S);
    }
    return M;
}

/// The 24 rotations of the cube: signed permutation matrices of determinant +1.
///
/// These map the cubic lattice onto itself, so a lattice configuration stays a
/// lattice configuration -- and its projection stays *maximally degenerate*.
/// That is the point. They are not a source of generic projections (measured:
/// the shared-column count of `lattice_04` is 96 before and after), they are 24
/// independent hard cases per fixture, every one of which must give the same
/// knot.
inline const std::vector<std::array<double,9>> & OctahedralRotations()
{
    static const std::vector<std::array<double,9>> table = []{
        std::vector<std::array<double,9>> out;
        constexpr int perms[6][3] = {
            {0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}
        };
        for( const auto & p : perms )
        for( int signs = 0; signs < 8; ++signs )
        {
            std::array<double,9> M {};
            const int sg[3] = { (signs&1)?-1:1, (signs&2)?-1:1, (signs&4)?-1:1 };
            for( int r = 0; r < 3; ++r ) { M[3*r + p[r]] = sg[r]; }

            // det of a signed permutation = sign(perm) * product(signs).
            const int parity = ((p[0]>p[1]) + (p[0]>p[2]) + (p[1]>p[2])) % 2;
            const int det = (parity ? -1 : 1) * sg[0] * sg[1] * sg[2];
            if( det > 0 ) { out.push_back(M); }
        }
        return out;
    }();
    return table;
}

/// Apply a matrix to a curve, keeping the entries exact.
inline Curve ApplyMatrix( const Curve & c, const std::array<double,9> & M,
                          const std::string & tag )
{
    Curve out = c;
    out.name = c.name + "[" + tag + "]";
    for( std::size_t i = 0; i < c.v.size(); i += 3 )
    {
        const double x = c.v[i], y = c.v[i+1], z = c.v[i+2];
        out.v[i  ] = M[0]*x + M[1]*y + M[2]*z;
        out.v[i+1] = M[3]*x + M[4]*y + M[5]*z;
        out.v[i+2] = M[6]*x + M[7]*y + M[8]*z;
    }
    return out;
}

/// A uniformly random rotation applied to the whole curve. Used for the knot-type
/// tier: a random rotation is degeneracy-free with probability 1, so it is an
/// independent generic projection of the same link.
inline Curve Rotate( const Curve & c, std::uint64_t seed )
{
    const auto R = RandomRotationMatrix(seed);

    Curve out = c;
    out.name = c.name + "[rot " + std::to_string(seed) + "]";

    for( std::size_t i = 0; i < c.v.size(); i += 3 )
    {
        const double x = c.v[i], y = c.v[i+1], z = c.v[i+2];
        out.v[i  ] = R[0]*x + R[1]*y + R[2]*z;
        out.v[i+1] = R[3]*x + R[4]*y + R[5]*z;
        out.v[i+2] = R[6]*x + R[7]*y + R[8]*z;
    }
    return out;
}

/// An independent generic projection of the same link, whatever the coordinate
/// type. Floating point gets a random rotation; integral coordinates get a
/// random unimodular integer matrix, for the reasons above.
inline Curve GenericImage( const Curve & c, std::uint64_t seed, bool integralQ )
{
    if( !integralQ ) { return Rotate(c,seed); }

    return ApplyMatrix( c, UnimodularIntegerMatrix(seed),
                        "int " + std::to_string(seed) );
}


/// Radius of gyration: the RMS distance of the vertices from their centroid.
///
/// This is the honest measure of "did the knot grow". It is exactly invariant
/// under any rigid motion -- rotation *and* translation -- so it is unaffected by
/// the internal Sterbenz shift, which a bounding box or a centroid norm would
/// pick up as spurious motion. Repeated re-aiming must leave it alone; if it
/// drifts, the shape itself is inflating or collapsing and precision is being
/// spent on nothing.
template<class Real_T>
double RadiusOfGyration( const std::vector<Real_T> & v )
{
    const std::size_t n = v.size() / 3;
    if( n == 0 ) { return 0.0; }

    double cx = 0.0, cy = 0.0, cz = 0.0;
    for( std::size_t i = 0; i < n; ++i )
    {
        cx += double(v[3*i]); cy += double(v[3*i+1]); cz += double(v[3*i+2]);
    }
    cx /= double(n); cy /= double(n); cz /= double(n);

    double s = 0.0;
    for( std::size_t i = 0; i < n; ++i )
    {
        const double dx = double(v[3*i  ]) - cx;
        const double dy = double(v[3*i+1]) - cy;
        const double dz = double(v[3*i+2]) - cz;
        s += dx*dx + dy*dy + dz*dz;
    }
    return std::sqrt( s / double(n) );
}

// ===========================================================================
//  Crossing fingerprint
// ===========================================================================

/// One crossing as seen from one of its two edges, read straight out of the
/// embedding's own arrays — before any `PlanarDiagram` is built, so a mismatch
/// localizes to the intersection computation rather than to PD assembly.
struct Slot
{
    Int edge       = 0;
    Int partner    = 0;
    int overQ      = 0;   ///< does `edge` pass over `partner` here?
    int handedness = 0;   ///< +1 right-handed, -1 left-handed

    bool operator==( const Slot & o ) const
    {
        return edge == o.edge && partner == o.partner
            && overQ == o.overQ && handedness == o.handedness;
    }
};

/// The full crossing set, in the embedding's canonical order: edges ascending,
/// and within an edge, crossings in the order they are met along it.
///
/// Two runs on the same combinatorial link are directly comparable, because a
/// linear change of coordinates leaves the edge numbering alone. That is what
/// makes the shear comparison and the cross-class comparison exact.
struct Fingerprint
{
    std::vector<Slot> slots;
    Int               crossing_count = 0;

    bool operator==( const Fingerprint & o ) const
    {
        return crossing_count == o.crossing_count && slots == o.slots;
    }
    bool operator!=( const Fingerprint & o ) const { return !(*this == o); }

    /// Human-readable account of the *first* disagreement, so a failure names a
    /// concrete edge rather than dumping two long strings.
    std::string DiffAgainst( const Fingerprint & o ) const
    {
        if( crossing_count != o.crossing_count )
        {
            return "crossing count " + std::to_string(crossing_count)
                 + " vs " + std::to_string(o.crossing_count);
        }
        const std::size_t n = std::min(slots.size(),o.slots.size());
        for( std::size_t i = 0; i < n; ++i )
        {
            if( !(slots[i] == o.slots[i]) )
            {
                auto show = [](const Slot & s)
                {
                    return "edge " + std::to_string(s.edge)
                         + " x edge " + std::to_string(s.partner)
                         + " (" + (s.overQ ? "over" : "under")
                         + ", handedness " + std::to_string(s.handedness) + ")";
                };
                return "first difference at position " + std::to_string(i)
                     + ": " + show(slots[i]) + "  vs  " + show(o.slots[i]);
            }
        }
        if( slots.size() != o.slots.size() )
        {
            return "slot count " + std::to_string(slots.size())
                 + " vs " + std::to_string(o.slots.size());
        }
        return "identical";
    }
};

/// Build the fingerprint from an embedding that has already computed its
/// intersections. `EdgeStates()[k]` packs `(handedness << 1) | overQ`; each
/// intersection index occupies exactly two slots, which is how the partner edge
/// is recovered.
///
/// Takes a mutable reference: on LinkEmbedding2/3/4 these accessors are not
/// const (they can still trigger the lazy computation).
template<class LE_T>
Fingerprint TakeFingerprint( LE_T & L )
{
    Fingerprint f;
    f.crossing_count = Int(L.IntersectionCount());

    const Int ec   = Int(L.EdgeCount());
    auto      cptr = L.EdgePointers().data();
    auto      ci   = L.EdgeIntersections().data();
    auto      cs   = L.EdgeStates().data();

    std::vector<std::pair<Int,Int>> owner (
        static_cast<std::size_t>(f.crossing_count), std::pair<Int,Int>{-1,-1} );

    for( Int e = 0; e < ec; ++e )
    {
        for( Int k = Int(cptr[e]); k < Int(cptr[e+1]); ++k )
        {
            auto & o = owner[static_cast<std::size_t>(ci[k])];
            if( o.first < Int(0) ) { o.first = e; } else { o.second = e; }
        }
    }

    f.slots.reserve( static_cast<std::size_t>(Int(2)*f.crossing_count) );

    for( Int e = 0; e < ec; ++e )
    {
        for( Int k = Int(cptr[e]); k < Int(cptr[e+1]); ++k )
        {
            const auto & o = owner[static_cast<std::size_t>(ci[k])];
            const int state = int(cs[k]);

            Slot s;
            s.edge       = e;
            s.partner    = (o.first == e) ? o.second : o.first;
            s.overQ      = state & 1;
            s.handedness = state >> 1;
            f.slots.push_back(s);
        }
    }
    return f;
}

// ===========================================================================
//  Running an embedding
// ===========================================================================

/// Load a fixture with the *library's own* reader, whatever it is currently
/// called. The name has moved between `FromFile` and `ReadFromFile` across
/// revisions of this branch, and the point of the reader tier is to check the
/// parse, not to pin the spelling -- so probe for both rather than break the
/// build on a rename.
template<class LE_T>
LE_T LoadWithLibraryReader( const std::filesystem::path & file )
{
    if constexpr ( requires { LE_T::FromFile(file); } )
    {
        return LE_T::FromFile(file);
    }
    else
    {
        return LE_T::ReadFromFile(file);
    }
}

/// Compute the intersections and say plainly whether it worked.
///
/// Compute the intersections and say plainly whether it worked.
///
/// Both families return an `int` error code with 0 = success as of `1d8761c7`,
/// which unified a convention split that had `LinkEmbedding2/3/4` returning
/// `bool`. The `bool` branch below is kept because the return type is inspected
/// rather than assumed, which is what made this function survive that change
/// without edits.
///
/// It also enforces the INTROSPECTION CONTRACT on the way through: a class that
/// reports success must report zero 3-space intersections. Those two statements
/// are the same statement, and a build where they disagree is broken in a way
/// no tier would otherwise notice -- every tier asks "did it work", none asks
/// "and does the object agree that it worked".
template<class LE_T>
bool RequireIntersectionsOK( LE_T & L, std::string & message, int & err )
{
    const auto raw_status = L.template RequireIntersections<false>();

    using Status_T = std::remove_cvref_t<decltype(raw_status)>;

    /// Success must mean IntersectionCount3D() == 0. Returns false and fills
    /// `message` if the object contradicts its own success return.
    auto agrees_with_itselfQ = [&L,&message,&err]() -> bool
    {
        if constexpr ( requires { L.IntersectionCount3D(); } )
        {
            const auto n3d = L.IntersectionCount3D();
            if( n3d > 0 )
            {
                err     = -2;
                message = "RequireIntersections reported SUCCESS but "
                          "IntersectionCount3D() is " + std::to_string(Int(n3d))
                        + "; a projection cannot both succeed and contain "
                          "3-space intersections";
                return false;
            }
        }
        else
        {
            // This is to suppress warning message by the compiler.
            (void)L;
            (void)message;
            (void)err;
        }
        return true;
    };

    if constexpr ( std::is_same_v<Status_T,bool> )
    {
        if( raw_status ) { return agrees_with_itselfQ(); }

        err     = -1;
        message = "RequireIntersections returned false";

        if constexpr ( requires { L.IntersectionCount3D(); } )
        {
            const auto n3d = L.IntersectionCount3D();
            if( n3d > 0 )
            {
                message += " (" + std::to_string(Int(n3d))
                         + " line segment pair(s) intersect in 3D)";
            }
        }
        return false;
    }
    else
    {
        err = int(raw_status);
        if( err == 0 ) { return agrees_with_itselfQ(); }

        message = "RequireIntersections returned error code " + std::to_string(err)
                + (err == 6 ? " (line segments intersect in 3D)" : "");
        return false;
    }
}

/// Everything one projection produced, including the ways it can fail.
struct RunResult
{
    bool        ok             = false;
    int         err            = 0;      ///< class's own error code (6 = 3D intersection)
    bool        threw          = false;  ///< LinkEmbedding4 throws where 2/3 return a code
    std::string message;

    Fingerprint fp;
    Int         crossing_count = 0;
    Int         component_count = 0;
    double      rounding_error = 0.0;
    double      scaling_factor = 1.0;

    /// Populated only when the caller asks for a diagram.
    bool        pd_built       = false;
    PD_T        pd;
    Int         unlink_count   = 0;
};

/// The one adapter that covers all four classes.
///
/// `PD_T::FromLinkEmbedding` only has overloads for `LinkEmbedding` and
/// `LinkEmbedding2`, but every class in the family exposes the same seven
/// accessors, and `FromLinkEmbedding_Raw` is public precisely so tests can
/// reach it ("Testing makes it necessary to make this public").
template<class LE_T>
std::pair<PD_T,Knoodle::Tensor1<Int,Int>> PDFromEmbedding( LE_T & L )
{
    return PD_T::FromLinkEmbedding_Raw(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.ComponentColors().data(),
        L.IntersectionCount(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeStates().data()
    );
}

/// Project `c` with class `LE_T` and report what came out.
///
/// Failures are captured, never fatal: `LinkEmbedding4` currently *throws* where
/// `LinkEmbedding2/3` return error code 6, and a test that aborts on the first
/// awkward fixture cannot report on the rest.
template<class LE_T>
RunResult RunEmbedding( const Curve & c, bool want_pd = false )
{
    RunResult r;

    try
    {
        Knoodle::Tensor1<Int,Int> cp ( c.comp_ptr.data(), Int(c.comp_ptr.size()) );
        Knoodle::Tensor1<Int,Int> col =
            Knoodle::iota<Int,Int>( c.comp_ptr.size() - std::size_t(1) );

        LE_T L ( std::move(cp), std::move(col) );

        // The curve is stored in double; narrow to whatever coordinate type this
        // instantiation reads. For an f32 instantiation this is a real narrowing
        // -- that is the point of testing it -- and `RoundingError()` reports what
        // it cost.
        using Real_T = typename LE_T::Real;

        std::vector<Real_T> coords ( c.v.size() );
        for( std::size_t i = 0; i < c.v.size(); ++i )
        {
            coords[i] = static_cast<Real_T>(c.v[i]);
        }

        // Coordinates are handed over already transformed; no in-class transform.
        L.template ReadVertexCoordinates<false>( coords.data() );

        if( !RequireIntersectionsOK(L,r.message,r.err) ) { return r; }

        r.fp              = TakeFingerprint(L);
        r.crossing_count  = Int(L.IntersectionCount());
        r.component_count = Int(L.ComponentCount());

        if constexpr ( requires { L.RoundingError(); } )
        {
            r.rounding_error = double(L.RoundingError());
        }
        if constexpr ( requires { L.ScalingFactor(); } )
        {
            r.scaling_factor = double(L.ScalingFactor());
        }

        if( want_pd )
        {
            auto [pd,unlinks] = PDFromEmbedding(L);
            r.pd           = std::move(pd);
            r.unlink_count = Int(unlinks.Size());
            r.pd_built     = true;
        }

        r.ok = true;
    }
    catch( const std::exception & e )
    {
        r.threw   = true;
        r.message = std::string("threw: ") + e.what();
    }
    catch( ... )
    {
        r.threw   = true;
        r.message = "threw: (unknown exception)";
    }
    return r;
}

// ===========================================================================
//  Degeneracy census
// ===========================================================================
//
// Computed here, in exact integer arithmetic, with no help from the classes
// under test. Its job is to make "we exercised the corner cases" checkable: a
// fixture declares the degeneracies it is supposed to contain, and the test
// fails if the fixture has quietly become generic.

/// Counts of the degenerate configurations a vertical projection can present.
struct Census
{
    Int vertices             = 0;
    Int edges                = 0;

    Int zero_length_edges    = 0;  ///< an edge whose endpoints coincide in 3D
    Int vertical_edges       = 0;  ///< projects to a single point
    Int coincident_vertices  = 0;  ///< pairs of vertices with equal (x,y)
    Int max_point_multiplicity = 0;///< most edges through one projected point (>=3 is a multiple point)

    Int vertex_on_edge       = 0;  ///< endpoint in the relative interior of another edge
    Int corner_corner        = 0;  ///< endpoints of two non-adjacent edges coincide
    Int collinear_overlap    = 0;  ///< projections share more than a point
    Int multiple_points      = 0;  ///< projected points with >= 3 edges through them
    Int transversal          = 0;  ///< ordinary interior crossings
    Int spatial              = 0;  ///< non-adjacent edges meeting in 3D (unfixable; must be 0)

    /// Any degeneracy at all.
    Int DegenerateTotal() const
    {
        return zero_length_edges + vertical_edges + coincident_vertices
             + vertex_on_edge + corner_corner + collinear_overlap + multiple_points;
    }

    /// Lookup by the key names used in `.expect` files.
    Int Get( const std::string & key, bool & known ) const
    {
        known = true;
        if( key == "vertices"              ) { return vertices; }
        if( key == "edges"                 ) { return edges; }
        if( key == "zero_length_edges"     ) { return zero_length_edges; }
        if( key == "vertical_edges"        ) { return vertical_edges; }
        if( key == "coincident_vertices"   ) { return coincident_vertices; }
        if( key == "max_point_multiplicity") { return max_point_multiplicity; }
        if( key == "vertex_on_edge"        ) { return vertex_on_edge; }
        if( key == "corner_corner"         ) { return corner_corner; }
        if( key == "collinear_overlap"     ) { return collinear_overlap; }
        if( key == "multiple_points"       ) { return multiple_points; }
        if( key == "transversal"           ) { return transversal; }
        if( key == "spatial"               ) { return spatial; }
        if( key == "degenerate_total"      ) { return DegenerateTotal(); }
        known = false;
        return 0;
    }

    std::string ToString() const
    {
        std::string s;
        auto add = [&s](const char * k, Int v)
        {
            if( v != Int(0) ) { s += (s.empty() ? "" : " ") + std::string(k) + "=" + std::to_string(v); }
        };
        add("zero_length_edges",    zero_length_edges);
        add("vertical_edges",       vertical_edges);
        add("coincident_vertices",  coincident_vertices);
        add("vertex_on_edge",       vertex_on_edge);
        add("corner_corner",        corner_corner);
        add("collinear_overlap",    collinear_overlap);
        add("multiple_points",      multiple_points);
        add("max_point_multiplicity", max_point_multiplicity);
        add("transversal",          transversal);
        add("spatial",              spatial);
        return s.empty() ? "(generic)" : s;
    }
};

namespace census_detail {

struct P2 { Wide x, y; };
struct P3 { Wide x, y, z; };

inline Wide Cross( const P2 & a, const P2 & b ) { return a.x*b.y - a.y*b.x; }

inline Wide Orient( const P2 & a, const P2 & b, const P2 & c )
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

inline int Sgn( Wide v ) { return (v > 0) - (v < 0); }

inline Wide Gcd( Wide a, Wide b )
{
    if( a < 0 ) { a = -a; }
    if( b < 0 ) { b = -b; }
    while( b != 0 ) { Wide t = a % b; a = b; b = t; }
    return a;
}

/// A projected point as an exact reduced rational pair, usable as a map key.
struct RatPoint
{
    Wide nx = 0, ny = 0, d = 1;

    void Normalize()
    {
        if( d < 0 ) { nx = -nx; ny = -ny; d = -d; }
        Wide g = Gcd(Gcd(nx,ny),d);
        if( g > 1 ) { nx /= g; ny /= g; d /= g; }
    }
    bool operator<( const RatPoint & o ) const
    {
        if( nx != o.nx ) { return nx < o.nx; }
        if( ny != o.ny ) { return ny < o.ny; }
        return d < o.d;
    }
};

/// Is `p` on segment `a`-`b` (assumed collinear with it)?
inline bool WithinBox( const P2 & p, const P2 & a, const P2 & b )
{
    return std::min(a.x,b.x) <= p.x && p.x <= std::max(a.x,b.x)
        && std::min(a.y,b.y) <= p.y && p.y <= std::max(a.y,b.y);
}

inline bool SamePoint( const P2 & a, const P2 & b ) { return a.x == b.x && a.y == b.y; }

} // namespace census_detail

/// Classify every projected edge pair of an *integral* curve exactly.
///
/// `pair_budget` caps the O(m^2) pair scan; the census is a fixture property, so
/// on very large fixtures (the benchmark lattices) it is skipped rather than
/// allowed to dominate the run. A skipped census reports `edges = 0`.
inline Census TakeCensus( const Curve & c, Int pair_budget = Int(40'000'000) )
{
    using namespace census_detail;

    Census cen;

    if( !c.IntegralQ() ) { return cen; }

    const Int n = c.VertexCount();

    // Edge e runs from vertex e to `next[e]`; within a component the last vertex
    // wraps to the first. This is exactly the convention LinkEmbedding* uses when
    // constructed from component pointers, so edge indices line up with the
    // fingerprint's.
    std::vector<Int> next ( static_cast<std::size_t>(n) );
    for( Int k = 0; k < c.ComponentCount(); ++k )
    {
        const Int b = c.comp_ptr[k], e = c.comp_ptr[k+1];
        for( Int i = b; i < e; ++i ) { next[static_cast<std::size_t>(i)] = (i+1 < e) ? i+1 : b; }
    }

    std::vector<P3> V ( static_cast<std::size_t>(n) );
    for( Int i = 0; i < n; ++i )
    {
        V[static_cast<std::size_t>(i)] = P3{
            Wide(std::llround(c.v[static_cast<std::size_t>(3*i  )])),
            Wide(std::llround(c.v[static_cast<std::size_t>(3*i+1)])),
            Wide(std::llround(c.v[static_cast<std::size_t>(3*i+2)]))
        };
    }
    auto Pt = [&V](Int i){ return P2{ V[static_cast<std::size_t>(i)].x, V[static_cast<std::size_t>(i)].y }; };

    // Vertices joined by a zero-length edge are the same point of 3-space, so
    // the edges on either side of one are *adjacent* even though their indices
    // are not consecutive. Without collapsing them, every zero-length edge would
    // be reported as a genuine (unfixable) 3D intersection between its two
    // neighbours, which is exactly backwards: a zero-length edge is a degeneracy
    // the classes are supposed to absorb.
    std::vector<Int> root ( static_cast<std::size_t>(n) );
    for( Int i = 0; i < n; ++i ) { root[static_cast<std::size_t>(i)] = i; }

    std::function<Int(Int)> Find = [&](Int i) -> Int
    {
        while( root[static_cast<std::size_t>(i)] != i )
        {
            root[static_cast<std::size_t>(i)] = root[static_cast<std::size_t>(root[static_cast<std::size_t>(i)])];
            i = root[static_cast<std::size_t>(i)];
        }
        return i;
    };

    for( Int e = 0; e < n; ++e )
    {
        const Int    f = next[static_cast<std::size_t>(e)];
        const auto & a = V[static_cast<std::size_t>(e)];
        const auto & b = V[static_cast<std::size_t>(f)];

        if( a.x == b.x && a.y == b.y && a.z == b.z )
        {
            const Int ra = Find(e), rb = Find(f);
            if( ra != rb ) { root[static_cast<std::size_t>(rb)] = ra; }
        }
    }

    /// Do these two edges share an endpoint (zero-length edges collapsed)?
    auto AdjacentQ = [&](Int e, Int f)
    {
        const Int e0 = Find(e), e1 = Find(next[static_cast<std::size_t>(e)]);
        const Int f0 = Find(f), f1 = Find(next[static_cast<std::size_t>(f)]);
        return e0 == f0 || e0 == f1 || e1 == f0 || e1 == f1;
    };

    cen.vertices = n;
    cen.edges    = n;   // one edge per vertex: every component is closed

    // --- per-edge degeneracies -------------------------------------------
    for( Int e = 0; e < n; ++e )
    {
        const auto & a = V[static_cast<std::size_t>(e)];
        const auto & b = V[static_cast<std::size_t>(next[static_cast<std::size_t>(e)])];

        if( a.x == b.x && a.y == b.y )
        {
            if( a.z == b.z ) { ++cen.zero_length_edges; } else { ++cen.vertical_edges; }
        }
    }

    // --- coincident projected vertices ------------------------------------
    {
        std::map<std::pair<Wide,Wide>,Int> bucket;
        for( Int i = 0; i < n; ++i ) { ++bucket[{V[static_cast<std::size_t>(i)].x, V[static_cast<std::size_t>(i)].y}]; }
        for( const auto & [key,cnt] : bucket )
        {
            (void)key;
            if( cnt > Int(1) ) { cen.coincident_vertices += cnt*(cnt-Int(1))/Int(2); }
        }
    }

    // --- pairwise classification ------------------------------------------
    if( (Wide(n)*Wide(n))/2 > Wide(pair_budget) )
    {
        cen.edges = 0;   // census skipped; too big to be worth it
        return cen;
    }

    // Projected points where edges meet, so multiplicity can be counted.
    std::map<RatPoint,std::set<Int>> incidence;

    for( Int e = 0; e < n; ++e )
    {
        const Int e1 = next[static_cast<std::size_t>(e)];
        const P2  p0 = Pt(e), p1 = Pt(e1);

        for( Int f = e+1; f < n; ++f )
        {
            const Int f1 = next[static_cast<std::size_t>(f)];

            // Adjacent edges share a vertex by construction; only a genuine
            // overlap of their projections is a degeneracy worth counting.
            const bool adjacent = AdjacentQ(e,f);

            const P2 q0 = Pt(f), q1 = Pt(f1);

            const bool p_pt = SamePoint(p0,p1);
            const bool q_pt = SamePoint(q0,q1);

            // ---- one or both project to a point ----
            if( p_pt || q_pt )
            {
                bool meets = false;
                RatPoint at;

                if( p_pt && q_pt )
                {
                    meets = SamePoint(p0,q0);
                    at = RatPoint{ p0.x, p0.y, 1 };
                }
                else
                {
                    const P2 pt = p_pt ? p0 : q0;
                    const P2 a  = p_pt ? q0 : p0;
                    const P2 b  = p_pt ? q1 : p1;
                    meets = (Orient(a,b,pt) == 0) && WithinBox(pt,a,b);
                    at = RatPoint{ pt.x, pt.y, 1 };
                }

                if( meets && !adjacent )
                {
                    at.Normalize();
                    incidence[at].insert(e);
                    incidence[at].insert(f);
                    ++cen.vertex_on_edge;   // a point-edge incidence
                }
                continue;
            }

            const Wide d1 = Orient(p0,p1,q0);
            const Wide d2 = Orient(p0,p1,q1);
            const Wide d3 = Orient(q0,q1,p0);
            const Wide d4 = Orient(q0,q1,p1);

            // ---- collinear ----
            if( d1 == 0 && d2 == 0 && d3 == 0 && d4 == 0 )
            {
                // Overlap in more than a point?
                const bool q0_in = WithinBox(q0,p0,p1);
                const bool q1_in = WithinBox(q1,p0,p1);
                const bool p0_in = WithinBox(p0,q0,q1);
                const bool p1_in = WithinBox(p1,q0,q1);

                const int inside = int(q0_in)+int(q1_in)+int(p0_in)+int(p1_in);

                if( inside >= 2 )
                {
                    // Two shared endpoints only (touching at one point) is not an interval.
                    const bool touch_only =
                        (inside == 2) &&
                        ( SamePoint(p0,q0) || SamePoint(p0,q1) ||
                          SamePoint(p1,q0) || SamePoint(p1,q1) );

                    if( !touch_only ) { ++cen.collinear_overlap; }
                    else if( !adjacent ) { ++cen.corner_corner; }
                }
                continue;
            }

            if( adjacent ) { continue; }

            const int s1 = Sgn(d1), s2 = Sgn(d2), s3 = Sgn(d3), s4 = Sgn(d4);

            const bool straddle = (s1*s2 <= 0) && (s3*s4 <= 0);
            if( !straddle ) { continue; }

            // The intersection point, exactly: p0 + t*(p1-p0) with t = d3/(d3-d4).
            const P2   r { p1.x-p0.x, p1.y-p0.y };
            const P2   s { q1.x-q0.x, q1.y-q0.y };
            const Wide denom = Cross(r,s);

            if( denom == 0 ) { continue; }   // parallel but not collinear: no meet

            const Wide tnum = Cross(P2{q0.x-p0.x, q0.y-p0.y}, s);

            RatPoint at;
            at.nx = p0.x*denom + tnum*r.x;
            at.ny = p0.y*denom + tnum*r.y;
            at.d  = denom;
            at.Normalize();

            incidence[at].insert(e);
            incidence[at].insert(f);

            const bool endpoint_p = (s1 == 0) || (s2 == 0);   // q's endpoint on p's line
            const bool endpoint_q = (s3 == 0) || (s4 == 0);

            if( endpoint_p && endpoint_q ) { ++cen.corner_corner;  }
            else if( endpoint_p || endpoint_q ) { ++cen.vertex_on_edge; }
            else { ++cen.transversal; }

            // A genuine 3D intersection is the one case no perturbation can fix.
            // Heights on the two edges at the meet: compare z along each.
            {
                const auto & P0 = V[static_cast<std::size_t>(e )];
                const auto & P1 = V[static_cast<std::size_t>(e1)];
                const auto & Q0 = V[static_cast<std::size_t>(f )];
                const auto & Q1 = V[static_cast<std::size_t>(f1)];

                // z_p = P0.z + t*(P1.z-P0.z) with t = tnum/denom
                // z_q = Q0.z + u*(Q1.z-Q0.z) with u = cross(q0-p0, r)/denom
                const Wide unum = Cross(P2{q0.x-p0.x, q0.y-p0.y}, r);

                const Wide zp = P0.z*denom + tnum*(P1.z-P0.z);
                const Wide zq = Q0.z*denom + unum*(Q1.z-Q0.z);

                if( zp == zq ) { ++cen.spatial; }
            }
        }
    }

    for( const auto & [pt,edges] : incidence )
    {
        (void)pt;
        const Int mult = Int(edges.size());
        cen.max_point_multiplicity = std::max(cen.max_point_multiplicity,mult);
        if( mult >= Int(3) ) { ++cen.multiple_points; }
    }

    return cen;
}

// ===========================================================================
//  .expect files
// ===========================================================================

/// One declared expectation about a fixture, e.g. `vertical_edges >= 1`.
struct Expectation
{
    std::string key;
    std::string op;    ///< "==" or ">="
    Int         value = 0;
};

/// A known defect in the code under test, optionally scoped to some classes.
///
/// Written in a `.expect` file as either
///     xfail_exact = reason                 (all classes)
///     xfail_exact = 2,3,4 | reason         (only LinkEmbedding2/3/4)
struct Marker
{
    std::set<int>         classes;  ///< empty = applies to every class
    std::set<std::string> coords;   ///< empty = applies to every coordinate type
    std::string           reason;

    /// A defect can be specific to a coordinate type as well as to a class, and
    /// the two are independent. deg_stacked_points is the case that forced this:
    /// the same root cause (upstream issue 11) CRASHES the rotation tier under
    /// i64, where the generic image is a unimodular integer matrix that
    /// preserves the degenerate configuration, but merely FAILS under f64,
    /// where a random rotation separates the two points before the library ever
    /// pairs them. One class list cannot say both.
    bool AppliesTo( int cls, const std::string & coord_tag ) const
    {
        if( !classes.empty() && classes.count(cls) == 0 )      { return false; }
        if( !coords.empty()  && coords.count(coord_tag) == 0 ) { return false; }
        return true;
    }
};

/// A fixture's sidecar file. Lines are `key >= n`, `key == n`, `knot = NAME`,
/// `note = ...`, `xfail_<tier> = ...`, or `xcrash_<tier> = ...`; `#` starts a
/// comment. Everything is optional.
///
/// `xfail_<tier>` records a known *wrong answer*: the check still runs and is
/// still reported, but it does not fail the suite -- and if it starts passing,
/// that is an XPASS failure, so a stale marker gets removed rather than quietly
/// masking a later regression.
///
/// `xcrash_<tier>` records a known *crash* -- the library takes the process down,
/// so the check cannot simply be run and scored. By default the combination is
/// skipped (with the reason printed) so the rest of the suite completes; under
/// `--isolate` it is run in a child process and scored properly.
struct Expectations
{
    std::vector<Expectation> census;
    std::string              knot;      ///< documentation only
    std::string              note;
    // A tier may carry several markers with disjoint class lists -- one defect
    // in LinkEmbedding and a different one in LinkEmbedding2/3/4 are separate
    // facts about separate code and each deserves its own reason. They are held
    // in declaration order and the first one that applies to the class wins,
    // with a fixture's own markers ahead of any inherited default.
    std::map<std::string,std::vector<Marker>> xfail;    ///< tier -> markers
    std::map<std::string,std::vector<Marker>> xcrash;   ///< tier -> markers

    static std::string FirstApplicable(
        const std::map<std::string,std::vector<Marker>> & m,
        const std::string & tier, int cls, const std::string & coord_tag )
    {
        auto it = m.find(tier);
        if( it == m.end() ) { return {}; }

        for( const auto & marker : it->second )
        {
            if( marker.AppliesTo(cls,coord_tag) ) { return marker.reason; }
        }
        return {};
    }

    /// The reason this tier is expected to fail here, or "".
    std::string XFail( const std::string & tier, int cls,
                       const std::string & coord_tag ) const
    {
        return FirstApplicable(xfail,tier,cls,coord_tag);
    }

    /// The reason this tier is expected to crash here, or "".
    std::string XCrash( const std::string & tier, int cls,
                        const std::string & coord_tag ) const
    {
        return FirstApplicable(xcrash,tier,cls,coord_tag);
    }
};

/// Parse `"2,3,4 | reason"`, `"2,3,4 @i64 | reason"`, `"@f32,f64 | reason"`,
/// or plain `"reason"`. The class list and the `@` coordinate list are both
/// optional and both mean "every one of them" when absent.
inline Marker ParseMarker( const std::string & value )
{
    Marker m;
    const std::size_t bar = value.find('|');

    if( bar == std::string::npos ) { m.reason = value; return m; }

    std::string list = value.substr(0,bar);

    if( const std::size_t at = list.find('@'); at != std::string::npos )
    {
        std::string tags = list.substr(at+1);
        list = list.substr(0,at);

        std::string tag;
        for( char ch : tags + "," )
        {
            if( ch == ',' )
            {
                const std::size_t b = tag.find_first_not_of(" \t");
                const std::size_t e = tag.find_last_not_of(" \t");
                if( b != std::string::npos ) { m.coords.insert(tag.substr(b,e-b+1)); }
                tag.clear();
            }
            else { tag += ch; }
        }
    }
    m.reason = value.substr(bar+1);

    const std::size_t b = m.reason.find_first_not_of(" \t");
    m.reason = (b == std::string::npos) ? "" : m.reason.substr(b);

    std::string cur;
    for( char ch : list + "," )
    {
        if( ch == ',' )
        {
            const std::size_t s = cur.find_first_not_of(" \t");
            if( s != std::string::npos ) { m.classes.insert(std::stoi(cur.substr(s))); }
            cur.clear();
        }
        else { cur += ch; }
    }
    return m;
}

inline bool LoadExpectations( const std::string & path, Expectations & x, std::string & error )
{
    std::ifstream in (path);
    if( !in ) { return true; }   // absent is fine

    std::string line;
    Int line_no = 0;

    while( std::getline(in,line) )
    {
        ++line_no;
        const std::size_t hash = line.find('#');
        if( hash != std::string::npos ) { line.erase(hash); }

        std::istringstream iss (line);
        std::string key, op;
        if( !(iss >> key) ) { continue; }

        if( key == "knot" || key == "note"
            || key.rfind("xfail_",0) == 0 || key.rfind("xcrash_",0) == 0 )
        {
            std::string rest;
            std::getline(iss,rest);
            const std::size_t b = rest.find_first_not_of(" \t=");
            const std::string val = (b == std::string::npos) ? "" : rest.substr(b);

            if     ( key == "knot" ) { x.knot = val; }
            else if( key == "note" ) { x.note = val; }
            else if( key.rfind("xfail_",0) == 0 )
            {
                x.xfail[key.substr(6)].push_back(ParseMarker(val));
            }
            else
            {
                x.xcrash[key.substr(7)].push_back(ParseMarker(val));
            }
            continue;
        }

        Int value = 0;
        if( !(iss >> op >> value) || (op != "==" && op != ">=") )
        {
            error = path + ":" + std::to_string(line_no)
                  + ": expected `key ==|>= n`, `knot = NAME`, or `note = ...`";
            return false;
        }
        x.census.push_back( Expectation{ key, op, value } );
    }
    return true;
}

/// Merge `base` into `x` for any tier `x` does not already speak about.
///
/// Used for a `DEFAULT.expect` alongside the fixtures: some defects belong to a
/// *class*, not to any one curve -- a stale cache makes every fixture behave the
/// same way -- and copying the same marker into fourteen sidecars would be noise
/// that nobody would remember to delete together. A per-fixture marker always
/// wins over the default for the same tier.
inline void InheritDefaults( Expectations & x, const Expectations & base )
{
    // Appended, not substituted: a fixture that speaks about classes 2,3,4 must
    // not silently drop an inherited marker about class 1.
    for( const auto & [tier,markers] : base.xfail )
    {
        auto & dst = x.xfail[tier];
        dst.insert(dst.end(),markers.begin(),markers.end());
    }
    for( const auto & [tier,markers] : base.xcrash )
    {
        auto & dst = x.xcrash[tier];
        dst.insert(dst.end(),markers.begin(),markers.end());
    }
}

/// Check a census against its declared expectations; returns the failures.
inline std::vector<std::string> CheckExpectations( const Census & cen, const Expectations & x )
{
    std::vector<std::string> failures;

    for( const auto & e : x.census )
    {
        bool known = false;
        const Int got = cen.Get(e.key,known);

        if( !known )
        {
            failures.push_back("unknown census key `" + e.key + "`");
            continue;
        }
        const bool pass = (e.op == "==") ? (got == e.value) : (got >= e.value);
        if( !pass )
        {
            failures.push_back(e.key + " " + e.op + " " + std::to_string(e.value)
                               + " but census says " + std::to_string(got));
        }
    }
    return failures;
}

} // namespace knoodle_test
