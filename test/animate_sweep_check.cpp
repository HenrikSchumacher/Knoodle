// animate_sweep_check -- the contract of Animate::Sweep (src/Animate/Sweep.hpp).
//
// For every shape: each frame runs between the shared ends, is simple, stays
// in the closed region between W and C, lies on C's side of the frame before;
// the first and last frames are W and C themselves; and the area between W
// and frame i is within 2% of the total of times[i]. The pocket shapes also
// check that the strand never jumps. Standard library only; a second or so.

#include "../src/Animate/Sweep.hpp"

#include <chrono>
#include <cstdio>
#include <random>

using namespace Knoodle::Animate;

static int checks = 0, failed = 0;
static bool verboseQ = false;

static bool Check( bool okQ, const std::string & what )
{
    ++checks;
    if( !okQ ) { ++failed; std::printf("FAILED: %s\n", what.c_str()); }
    return okQ;
}

static std::string Num( Real x ) { char b[64]; std::snprintf(b, sizeof b, "%.4g", x); return b; }

static Real DistTo( const Point & p, const Polyline & L )
{
    Real d = 1e300, t;
    for( Idx i = 0; i + 1 < L.size(); ++i ) { d = std::min(d, SweepKit::SegDist(p, L[i], L[i+1], t)); }
    return d;
}

static Polyline Scaled( const Polyline & L, Real s )
{
    Polyline Q;
    for( const Point & p : L ) { Q.push_back({ s * p[0], s * p[1] }); }
    return Q;
}

static Polyline Turned( const Polyline & L, Real angle )
{
    Polyline Q;
    for( const Point & p : L ) { Q.push_back({ std::cos(angle) * p[0] - std::sin(angle) * p[1], std::sin(angle) * p[0] + std::cos(angle) * p[1] }); }
    return Q;
}

static Real Size( const Polyline & A, const Polyline & B )
{
    Box box;
    for( const Point & p : A ) { box.Add(p); }
    for( const Point & p : B ) { box.Add(p); }
    return box.Size();
}

// The closed polygon A, then B backwards (they share both ends).
static Polyline Ring( const Polyline & A, const Polyline & B )
{
    Polyline R = A;
    R.insert(R.end(), B.rbegin() + 1, B.rend() - 1);
    return R;
}

// The piece of L from the point `from` to the point `to` (both on L, in this order).
static Polyline Piece( const Polyline & L, const Point & from, const Point & to )
{
    auto place = [&L]( const Point & p )
    {
        Real best = 1e300, t, at = 0;
        for( Idx i = 0; i + 1 < L.size(); ++i ) { const Real d = SweepKit::SegDist(p, L[i], L[i+1], t); if( d < best ) { best = d; at = i + t; } }
        return at;
    };
    const Real t0 = place(from), t1 = place(to);
    Polyline Q { from };
    for( Idx i = 0; i < L.size(); ++i ) { if( Real(i) > t0 && Real(i) < t1 ) { Q.push_back(L[i]); } }
    Q.push_back(to);
    return Q;
}

// The area between A and B, which meet at the points `cuts` (in order) besides their ends.
static Real AreaBetween( const Polyline & A, const Polyline & B, const Polyline & cuts )
{
    Polyline stops { A.front() };
    stops.insert(stops.end(), cuts.begin(), cuts.end());
    stops.push_back(A.back());
    Real area = 0;
    for( Idx k = 0; k + 1 < stops.size(); ++k ) { area += std::abs(Area2(Ring(Piece(A, stops[k], stops[k+1]), Piece(B, stops[k], stops[k+1])))) / 2; }
    return area;
}

// Do A and B cross at a place that is not just a touch within tol, nor one of the cuts?
static bool CrossQ( const Polyline & A, const Polyline & B, Real tol, const Polyline & cuts = {} )
{
    Real t;
    Point x;
    for( Idx i = 0; i + 1 < A.size(); ++i )
    {
        for( Idx j = 0; j + 1 < B.size(); ++j )
        {
            const Point & a = A[i]; const Point & b = A[i+1]; const Point & c = B[j]; const Point & d = B[j+1];
            if( std::max(a[0], b[0]) < std::min(c[0], d[0]) || std::max(c[0], d[0]) < std::min(a[0], b[0])
             || std::max(a[1], b[1]) < std::min(c[1], d[1]) || std::max(c[1], d[1]) < std::min(a[1], b[1]) ) { continue; }
            if( Side(a, b, c) * Side(a, b, d) < 0 && Side(c, d, a) * Side(c, d, b) < 0
             && SweepKit::SegDist(a, c, d, t) > tol && SweepKit::SegDist(b, c, d, t) > tol && SweepKit::SegDist(c, a, b, t) > tol && SweepKit::SegDist(d, a, b, t) > tol && Intersection(a, b, c, d, x) )
            {
                bool cutQ = false;
                for( const Point & q : cuts ) { cutQ = cutQ || Dist(q, x) <= 1000 * tol; }
                if( !cutQ ) { return true; }
            }
        }
    }
    return false;
}

// Is every point and every segment's middle of F in the closed region between A and B?
static bool BetweenQ( const Polyline & F, const Polyline & A, const Polyline & B, Real tol )
{
    const Polyline ring = Ring(A, B);
    for( Idx i = 0; i < F.size(); ++i )
    {
        for( int half = 0; half < 2; ++half )
        {
            if( half && i + 1 == F.size() ) { continue; }
            const Point p = half ? Lerp(F[i], F[i+1], 0.5) : F[i];
            if( !Inside(ring, p) && DistTo(p, A) > tol && DistTo(p, B) > tol ) { return false; }
        }
    }
    return true;
}

static bool SimpleQ( const Polyline & F )
{
    std::vector<std::vector<Idx>> chain (1);
    for( Idx i = 0; i < F.size(); ++i ) { chain[0].push_back(i); }
    std::string why;
    return EmbeddedQ(F, chain, why);
}

struct Shape
{
    std::string name;
    Polyline    W, C, cuts;       // cuts: where W and C meet between their ends, other than along shared stretches
    Idx         lobes = 1;
    int         steps = 8;        // frames at 0, 1/steps, ..., 1
    Real        jump  = 0;        // if positive: no step of the strand may pass this multiple of the average step
    Real        turn  = 0;        // if positive: no frame may turn by more than this (degrees) at its first or last three corners
};

// All the checks of the contract on one shape. Returns the frames.
static std::vector<Polyline> Run( const Shape & S )
{
    std::vector<Real> times;
    for( int i = 0; i <= S.steps; ++i ) { times.push_back(Real(i) / S.steps); }
    SweepResult R;
    std::string why;
    if( !Check(Sweep(S.W, S.C, times, R, why), S.name + ": Sweep failed: " + why) ) { return {}; }
    if( !Check(R.frames.size() == times.size(), S.name + ": wrong number of frames") ) { return {}; }
    const Real size = Size(S.W, S.C), tol = 1e-9 * size, total = AreaBetween(S.W, S.C, S.cuts);
    Check(R.frames.front() == S.W && R.frames.back() == S.C, S.name + ": the first and last frames are not W and C exactly");
    Check(std::abs(R.area - total) <= 1e-9 * total && R.lobes == S.lobes,
          S.name + ": area " + Num(R.area) + " in " + std::to_string(R.lobes) + " lobes, expected " + Num(total) + " in " + std::to_string(S.lobes));
    std::string ends, simple, inside, nested, timing, hook;
    Real worst = 0, biggest = 0, sum = 0, sharpest = 0, closest = 1e300;
    Idx most = 0;
    for( Idx i = 0; i < R.frames.size(); ++i )
    {
        const Polyline & F = R.frames[i];
        const std::string at = " (frame " + std::to_string(i) + ")";
        if( F.size() < 2 || F.front() != S.W.front() || F.back() != S.W.back() ) { ends = at; continue; }
        if( !SimpleQ(F) ) { simple = at; }
        if( !BetweenQ(F, S.W, S.C, tol) || CrossQ(F, S.W, tol, S.cuts) || CrossQ(F, S.C, tol, S.cuts) ) { inside = at; }
        if( i > 0 && F != R.frames[i-1] )
        {
            const Polyline & F0 = R.frames[i-1];
            if( CrossQ(F, F0, tol, S.cuts) || !BetweenQ(F, F0, S.C, tol) ) { nested = at; }
            Real step = 0;
            for( const Point & p : F ) { step = std::max(step, DistTo(p, F0)); }
            biggest = std::max(biggest, step); sum += step;
        }
        const Real err = std::abs(AreaBetween(S.W, F, S.cuts) - times[i] * total) / total;
        if( err > worst ) { worst = err; timing = at; }
        if( i == 0 || i + 1 == R.frames.size() ) { continue; }
        most = std::max(most, F.size());
        for( Idx k = 0; k + 1 < F.size(); ++k ) { closest = std::min(closest, Dist(F[k], F[k+1]) / size); }
        for( Idx k = 1; k + 1 < F.size(); ++k )
        {
            if( k > 3 && k + 4 < F.size() ) { continue; }
            const Real dot = (F[k][0] - F[k-1][0]) * (F[k+1][0] - F[k][0]) + (F[k][1] - F[k-1][1]) * (F[k+1][1] - F[k][1]);
            const Real turn = std::abs(std::atan2(Cross(F[k-1], F[k], F[k+1]), dot)) * 57.29577951;
            if( turn > sharpest ) { sharpest = turn; hook = at; }
        }
    }
    Check(ends.empty(), S.name + ": a frame does not run between the shared ends" + ends);
    Check(simple.empty(), S.name + ": a frame crosses itself" + simple);
    Check(inside.empty(), S.name + ": a frame leaves the region between W and C" + inside);
    Check(nested.empty(), S.name + ": a frame is not on C's side of the frame before" + nested);
    Check(worst <= 0.02, S.name + ": swept area off by " + Num(100 * worst) + "% of the total" + timing);
    Check(most <= 400 * std::max<Idx>(S.lobes, 1) + S.W.size() + S.C.size(), S.name + ": a frame has " + std::to_string(most) + " points");
    if( S.turn > 0 ) { Check(sharpest <= S.turn, S.name + ": a frame turns by " + Num(sharpest) + " degrees next to a shared end" + hook); }
    const Real mean = sum / S.steps;
    if( S.jump > 0 ) { Check(biggest <= S.jump * mean, S.name + ": the strand jumps by " + Num(biggest) + ", " + Num(biggest / mean) + " times the average step " + Num(mean)); }
    if( verboseQ )
    {
        std::printf("%-28s area err %.3f%%  most points %3zu  biggest step %.4g = %.2f x mean  end turn %.1f  least step 1/%.0f\n", S.name.c_str(), 100 * worst, most, biggest, biggest / mean, sharpest, 1 / closest);
    }
    return R.frames;
}

// The same shape a million times larger and smaller: the same frames, up to the scale.
static void RunScaled( const Shape & S, const std::vector<Polyline> & frames )
{
    for( Real s : { 1e6, 1e-6 } )
    {
        Shape T = S;
        T.name += (s > 1) ? " x 1e6" : " x 1e-6";
        T.W = Scaled(S.W, s); T.C = Scaled(S.C, s); T.cuts = Scaled(S.cuts, s);
        const std::vector<Polyline> scaled = Run(T);
        const Real tol = 1e-6 * Size(S.W, S.C);
        bool sameQ = (scaled.size() == frames.size());
        Real worst = 0;
        for( Idx i = 0; sameQ && i < frames.size(); ++i )
        {
            sameQ = (scaled[i].size() == frames[i].size());
            for( Idx k = 0; sameQ && k < frames[i].size(); ++k ) { worst = std::max(worst, Dist(frames[i][k], { scaled[i][k][0] / s, scaled[i][k][1] / s })); }
        }
        Check(sameQ && worst <= tol, T.name + ": not the same frames up to the scale" + (sameQ ? " (off by " + Num(worst) + ")" : " (different point counts)"));
    }
}

static void Refused( const std::string & name, const Polyline & W, const Polyline & C )
{
    SweepResult R;
    std::string why;
    const bool okQ = Sweep(W, C, { 0.0, 0.5, 1.0 }, R, why);
    Check(!okQ && !why.empty(), name + ": should have been refused with a reason");
    if( verboseQ ) { std::printf("%-28s refused: %s\n", name.c_str(), why.c_str()); }
}

// A random region between two staircases from (0,0) to (n,H): column i holds a_i < y < b_i.
static Shape Staircase( std::mt19937 & rng, int id )
{
    auto roll = [&rng]( int lo, int hi ) { return lo + static_cast<int>(rng() % static_cast<unsigned>(hi - lo + 1)); };
    const int n = roll(3, 8);
    std::vector<int> a (n, 0), b (n, 0);
    b[0] = roll(1, 4);
    for( int i = 1; i < n; ++i )
    {
        a[i] = a[i-1] + roll(0, b[i-1] - a[i-1] - 1);
        b[i] = std::max(b[i-1], a[i] + 1) + roll(0, 3);
    }
    Shape S;
    S.name = "staircase " + std::to_string(id);
    auto add = []( Polyline & L, Real x, Real y ) { if( L.empty() || L.back() != Point{ x, y } ) { L.push_back({ x, y }); } };
    for( int i = 0; i < n; ++i )
    {
        add(S.W, i, a[i]); add(S.W, i + 1, a[i]);
        add(S.C, i, i ? b[i-1] : 0); add(S.C, i, b[i]);
    }
    add(S.W, n, b[n-1]); add(S.C, n, b[n-1]);
    return S;
}

int main( int argc, char ** )
{
    verboseQ = (argc > 1);
    const auto t0 = std::chrono::steady_clock::now();

    Shape rect;
    rect.name = "rectangle"; rect.steps = 20; rect.turn = 15;
    rect.W = { {0,0}, {4,0} };
    rect.C = { {0,0}, {0,3}, {4,3}, {4,0} };
    const auto rectFrames = Run(rect);
    RunScaled(rect, rectFrames);

    // An L: a room, a neck 1 wide and 10 long up its right wall, and a room behind the neck.
    // Past the neck the harmonic function is within 1e-14 of 1.
    Shape ell;
    ell.name = "L with a deep pocket"; ell.steps = 80; ell.jump = 8;
    ell.W = { {0,0}, {10,0} };
    ell.C = { {0,0}, {0,4}, {9,4}, {9,14}, {2,14}, {2,18}, {10,18}, {10,0} };
    const auto ellFrames = Run(ell);
    RunScaled(ell, ellFrames);

    // The same with the neck 0.1 wide, a quarter of a mesh edge: the function gets down to 1e-41 behind it.
    // (No jump check: a step's worth of area is more than the whole neck holds.)
    Shape slit = ell;
    slit.name = "L with a slit for a neck"; slit.steps = 40; slit.jump = 0;
    slit.C[2][0] = slit.C[3][0] = 9.9;
    Run(slit);

    Shape back = ell;                   // the same region swept the other way: now the pocket is behind W
    back.name = "L, pocket behind W";
    std::swap(back.W, back.C);
    Run(back);

    // A comb: four teeth 1 wide and 8 long, one of them with a head.
    Shape comb;
    comb.name = "comb with deep teeth"; comb.steps = 80; comb.jump = 8;
    comb.W = { {0,0}, {13,0} };
    comb.C = { {0,0}, {0,2}, {1,2}, {1,10}, {2,10}, {2,2}, {4,2}, {4,10}, {5,10}, {5,2}, {7,2}, {7,6}, {6,6}, {6,9}, {9,9}, {9,6}, {8,6}, {8,2},
               {11,2}, {11,10}, {12,10}, {12,2}, {13,2}, {13,0} };
    Run(comb);

    std::mt19937 rng (20261110u);
    for( int id = 0; id < 30; ++id ) { Run(Staircase(rng, id)); }

    // W and C share a first and a last stretch, cut up differently in the two.
    Shape shared;
    shared.name = "shared prefix and suffix"; shared.steps = 12;
    shared.W = { {0,0}, {8,0} };
    shared.C = { {0,0}, {1,0}, {2,0}, {2,3}, {6,3}, {6,0}, {7,0}, {8,0} };
    const auto sharedFrames = Run(shared);
    RunScaled(shared, sharedFrames);
    Shape turned = shared;               // the same turned, so that the shared points are on W only up to rounding
    turned.name = "shared stretches, turned";
    turned.W = Turned(shared.W, 0.3); turned.C = Turned(shared.C, 0.3);
    Run(turned);

    Shape two;
    two.name = "two lobes"; two.steps = 12; two.lobes = 2;
    two.W = { {0,0}, {8,0} };
    two.C = { {0,0}, {1,2}, {3,2}, {5,-2}, {7,-2}, {8,0} };
    two.cuts = { {4,0} };
    const auto twoFrames = Run(two);
    RunScaled(two, twoFrames);

    Shape three;
    three.name = "three lobes"; three.steps = 12; three.lobes = 3;
    three.W = { {0,0}, {2,0.5}, {12,0} };
    three.C = { {0,0}, {1,2}, {3,2}, {5,-2}, {7,-2}, {9,2}, {11,2}, {12,0} };
    three.cuts.resize(2);
    Intersection(three.W[1], three.W[2], three.C[2], three.C[3], three.cuts[0]);
    Intersection(three.W[1], three.W[2], three.C[4], three.C[5], three.cuts[1]);
    Run(three);

    Refused("meeting points out of order", { {0,0}, {10,0} }, { {0,0}, {1,3}, {7,0}, {5,0.5}, {3,0}, {3,-2}, {10,-2}, {10,0} });
    Refused("a region inside another", { {0,0}, {5,0} }, { {0,0}, {0,2}, {7,2}, {7,-2}, {3,-2}, {3,1}, {5,1}, {5,0} });
    Refused("C crosses itself", { {0,0}, {4,0} }, { {0,0}, {3,3}, {1,3}, {4,0} });
    Refused("different ends", { {0,0}, {4,0} }, { {0,0}, {0,3}, {4,3}, {4,1} });

    if( verboseQ ) { std::printf("%.3f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count()); }
    std::printf("animate_sweep_check: (%d checks, %d failed)\n", checks, failed);
    return failed ? 1 : 0;
}
