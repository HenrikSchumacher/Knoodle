/**
 * @file homfly_invariance.hpp
 * @brief Shared HOMFLY oracle: Knoodle diagram -> libhomfly polynomial, the
 *        split-link delta rule, and the simplify-must-preserve-HOMFLY
 *        invariance check.
 *
 * Extracted from homfly_check.cpp so both that Tier-1 cross-validation driver
 * and plantri_check.cpp (the plantri-generated exhaustive test) share *one*
 * oracle implementation — the same code path that homfly_check validates
 * against libhomfly's published reference polynomials.
 *
 * The bridge to libhomfly is the core library's own Jenkins code
 * (PlanarDiagram::ToJenkinsCodeString). libhomfly only handles *connected*
 * diagrams (it segfaults on split ones, even the bare unknot "1 0"), so
 * HomflyOfPossiblySplit decomposes into connected pieces and recombines with
 * the split-union delta rule. Header-only; include into one translation unit
 * each (the two tests are separate binaries, so the anonymous-namespace
 * internal linkage is intentional and ODR-safe).
 *
 * Needs the vendored libhomfly objects at link time (build/libhomfly_*.o).
 */

#pragma once

#include "../Knoodle.hpp"

extern "C" {
#include "vendor/libhomfly/homfly.h"
}
// Reclaims libhomfly's working memory; call after copying out each result.
extern "C" void knoodle_gc_free_all(void);

#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using Int   = std::int64_t;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;
using PD_T  = PDC_T::PD_T;

/// HOMFLY polynomial as a canonical map from (L-exponent, M-exponent) to
/// coefficient. libhomfly's (L,M) variables are term-for-term identical to
/// Regina's homflyLM(l,m) (verified empirically), so this representation
/// compares directly across the two oracles.
using Polynomial = std::map<std::pair<int,int>, long>;

/// Knoodle diagram -> libhomfly input string (its native Jenkins code).
inline std::string KnoodleJenkins(const PD_T& pd)
{
    return std::string(pd.ToJenkinsCodeString());
}

/// Run libhomfly on a Jenkins string, returning its polynomial string.
/// homfly_str takes a mutable char*, so hand it a private buffer.
inline std::string LibhomflyPoly(const std::string& jenkins)
{
    std::vector<char> buf(jenkins.begin(), jenkins.end());
    buf.push_back('\0');
    char* out = homfly_str(buf.data());
    std::string result = out ? std::string(out) : std::string("<null>");
    knoodle_gc_free_all();  // safe: result already copied to its own storage
    return result;
}

inline long UFFind(std::vector<long>& p, long x)
{
    while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; }
    return x;
}

/// Product of two HOMFLY polynomials (exponents add, coefficients convolve).
inline Polynomial Multiply(const Polynomial& a, const Polynomial& b)
{
    Polynomial r;
    for (const auto& [ea, ca] : a)
        for (const auto& [eb, cb] : b)
            r[{ ea.first + eb.first, ea.second + eb.second }] += ca * cb;
    for (auto it = r.begin(); it != r.end(); )
    {
        if (it->second == 0) { it = r.erase(it); } else { ++it; }
    }
    return r;
}

/// delta = HOMFLY of the 2-component unlink in libhomfly's (L,M) convention
/// (= Regina homflyLM): -(L + L^-1)/M. Each extra split component multiplies by
/// delta, so H(split union of k pieces) = delta^(k-1) * prod H(piece).
/// Verified: delta^2 = H(3-unlink) and delta * H(trefoil) = H(trefoil u unknot).
const Polynomial Delta = { {{1, -1}, -1}, {{-1, -1}, -1} };

/// A Jenkins code parsed into its components and per-crossing handedness.
/// Format: <#components>, then per component <#passings> followed by #passings
/// (crossing-id, over/under) pairs; then (crossing-id, handedness) pairs.
struct JenkinsCode
{
    long                                          ncomp = 0;
    std::vector<std::vector<std::pair<long,long>>> comps;       // per comp: (id, over)
    std::map<long,long>                            handedness;  // id -> +1/-1
    bool                                          ok = false;
};

inline JenkinsCode ParseJenkins(const std::string& s)
{
    JenkinsCode j;
    std::istringstream in(s);
    if (!(in >> j.ncomp) || j.ncomp <= 0) { return j; }
    j.comps.resize(static_cast<std::size_t>(j.ncomp));
    for (long c = 0; c < j.ncomp; ++c)
    {
        long passings;
        if (!(in >> passings) || passings < 0) { return j; }
        for (long k = 0; k < passings; ++k)
        {
            long id, over;
            if (!(in >> id >> over)) { return j; }
            j.comps[static_cast<std::size_t>(c)].push_back({id, over});
        }
    }
    long id, hand;
    while (in >> id >> hand) { j.handedness[id] = hand; }
    j.ok = true;
    return j;
}

/// Group the link components into connected pieces of the *diagram*: two
/// components are joined when they share a crossing (union-find). libhomfly
/// only handles connected diagrams, so each piece is computed separately and
/// combined with the delta rule.
inline std::vector<std::vector<long>> ConnectedGroups(const JenkinsCode& j)
{
    std::vector<long> parent(static_cast<std::size_t>(j.ncomp));
    for (long i = 0; i < j.ncomp; ++i) { parent[static_cast<std::size_t>(i)] = i; }

    std::map<long,long> owner;  // crossing id -> a component holding it
    for (long c = 0; c < j.ncomp; ++c)
        for (const auto& [id, over] : j.comps[static_cast<std::size_t>(c)])
        {
            (void)over;
            auto it = owner.find(id);
            if (it == owner.end()) { owner[id] = c; }
            else
            {
                long ra = UFFind(parent, it->second), rb = UFFind(parent, c);
                if (ra != rb) { parent[static_cast<std::size_t>(ra)] = rb; }
            }
        }

    std::map<long, std::vector<long>> groups;
    for (long c = 0; c < j.ncomp; ++c) { groups[UFFind(parent, c)].push_back(c); }
    std::vector<std::vector<long>> out;
    for (auto& [root, members] : groups) { (void)root; out.push_back(members); }
    return out;
}

/// Jenkins code for one connected group, with its crossings renumbered to
/// 0..m-1 (libhomfly requires consecutive crossing names). The group must have
/// >= 1 crossing; callers handle free-unknot groups separately.
inline std::string BuildSubJenkins(const JenkinsCode& j, const std::vector<long>& group)
{
    std::set<long> ids;
    for (long c : group)
        for (const auto& [id, over] : j.comps[static_cast<std::size_t>(c)])
        { (void)over; ids.insert(id); }

    std::map<long,long> remap;
    long n = 0;
    for (long id : ids) { remap[id] = n++; }

    std::ostringstream o;
    o << group.size();
    for (long c : group)
    {
        const auto& comp = j.comps[static_cast<std::size_t>(c)];
        o << "\n" << comp.size();
        for (const auto& [id, over] : comp) { o << " " << remap[id] << " " << over; }
    }
    o << "\n";
    for (long id : ids) { o << remap[id] << " " << j.handedness.at(id) << " "; }
    return o.str();
}

/// Run libhomfly and return the polynomial as a canonical (L,M)->coef map.
inline Polynomial LibhomflyPolyMap(const std::string& jenkins)
{
    std::vector<char> buf(jenkins.begin(), jenkins.end());
    buf.push_back('\0');
    Poly* p = homfly(buf.data());

    Polynomial out;
    if (p == nullptr) { return out; }
    for (long i = 0; i < p->len; ++i)
    {
        const Term& t = p->term[i];
        out[{ static_cast<int>(t.l), static_cast<int>(t.m) }] += t.coef;
    }
    for (auto it = out.begin(); it != out.end(); )
    {
        if (it->second == 0) { it = out.erase(it); } else { ++it; }
    }
    knoodle_gc_free_all();  // safe: terms already copied into `out`
    return out;
}

/// HOMFLY of a (possibly split / disconnected) diagram from its Jenkins code.
/// libhomfly only handles connected diagrams (it segfaults on split ones, even
/// the bare unknot "1 0"), so we split into connected pieces, compute each, and
/// combine with the split-union delta rule: H = delta^(k-1) * prod H(piece). A
/// free unknot piece (no crossings) contributes the polynomial 1. Sets
/// ok=false only on a malformed Jenkins code.
inline Polynomial HomflyOfPossiblySplit(const std::string& jenkins, bool& ok)
{
    ok = true;
    JenkinsCode j = ParseJenkins(jenkins);
    if (!j.ok) { ok = false; return {}; }

    const auto groups = ConnectedGroups(j);

    Polynomial result = { {{0, 0}, 1} };  // multiplicative identity
    for (const auto& group : groups)
    {
        bool has_crossings = false;
        for (long c : group)
        {
            if (!j.comps[static_cast<std::size_t>(c)].empty()) { has_crossings = true; break; }
        }
        const Polynomial g = has_crossings
            ? LibhomflyPolyMap(BuildSubJenkins(j, group))   // connected -> safe
            : Polynomial{ {{0, 0}, 1} };                    // free unknot
        result = Multiply(result, g);
    }
    for (std::size_t i = 1; i < groups.size(); ++i) { result = Multiply(result, Delta); }
    return result;
}

inline PD_T BuildPD(const std::vector<Int>& pd, Int crossings)
{
    return PD_T::FromSignedPDCode(pd.data(), crossings, false, true);
}

/// Human-readable form of a (L,M)->coef polynomial, for diagnostics.
inline std::string PolyToString(const Polynomial& p)
{
    if (p.empty()) { return "0"; }
    std::string s;
    for (const auto& [lm, c] : p)
    {
        if (!s.empty()) { s += " + "; }
        s += std::to_string(c) + " L^" + std::to_string(lm.first)
                               + " M^" + std::to_string(lm.second);
    }
    return s;
}

//==============================================================================
// Invariance check
//==============================================================================
//
// HOMFLY is a link invariant, so simplification must not change it. We compare
// homfly(input) to homfly(simplified complex).
//
// === How a simplified PlanarDiagramComplex encodes a link (the model) ===
//
// After Simplify, the result is a PlanarDiagramComplex whose pd_list holds one
// or more diagrams, and every arc carries a COLOR (PD_T::ArcColors()). Color and
// storage connectivity distinguish how the pieces combine:
//
//   * Same color  => CONNECT-SUMMED — prime summands of one knotted loop;
//     HOMFLY multiplies, NO delta: H(A # B) = H(A)·H(B).
//   * Distinct colors co-occurring in one CONNECTED piece => LINKED (e.g. a Hopf
//     link); not split, no delta.
//   * Distinct colors in disjoint pieces sharing no color => SPLIT (disjoint,
//     unlinked); each split boundary adds one delta: H(A ⊔ B) = delta·H(A)·H(B).
//
// `ToSingleDiagram()` reassembles the complex into one diagram via
// Splitting() -> AnelliToFarfalle() -> Connect(); the anelli->farfalle step
// encodes the split-union delta factors, so the result is HOMFLY-faithful for
// knots, connect-sums, non-split links, AND split links. We rely on it here.
//
// (Verified 2026-06-14: an explicit color-aware delta computation —
// delta^(S-1)·prod H(Splitting piece), S = color-connected split components —
// agrees with ToSingleDiagram on 2-way and 3-way split links and on all 8.5M
// 8-crossing plantri diagrams. So ToSingleDiagram needs no "split-link fix"; a
// HOMFLY change on a split link is a genuine Simplify bug, not an oracle
// artifact. We kept the simpler library-canonical path.)

/// Simplify args mirroring knoodlesimplify's default (Reapr / level-6) path;
/// the struct's defaults already set splitQ/rerouteQ/disconnectQ = true.
inline PDC_T::Simplify_Args_T SimplifyArgs()
{
    return PDC_T::Simplify_Args_T{};
}

enum class InvStatus { Preserved, Changed, Unsupported };

/// HOMFLY of the simplified diagram, honoring its split structure (via
/// ToSingleDiagram's faithful reassembly + HomflyOfPossiblySplit's delta rule).
/// Returns the unknot polynomial {(0,0):1} when the input simplifies away
/// entirely. Sets supported=false only when the complex cannot be reassembled.
inline Polynomial SimplifiedHomfly(const PD_T& pd, bool& supported)
{
    supported = true;
    PD_T  copy(pd);
    PDC_T pdc(std::move(copy));
    pdc.Simplify(SimplifyArgs());

    if (!pdc.ValidQ() || pdc.DiagramCount() == Int(0))
    {
        return Polynomial{ {{0, 0}, 1} };  // unknot
    }
    PD_T single = pdc.ToSingleDiagram();
    if (!single.ValidQ())
    {
        supported = false;
        return {};
    }
    return HomflyOfPossiblySplit(KnoodleJenkins(single), supported);
}

/// Number of link components of a complex = number of distinct arc colors
/// (connect-sum summands share a color, so they count once; each unlink was
/// created with its own color). Simplify must preserve this; a change is a
/// component-loss/gain bug. The empty (DiagramCount 0) complex is a single
/// unknot = 1 component.
inline Int ComplexComponentCount(const PDC_T& pdc)
{
    return (pdc.DiagramCount() == Int(0)) ? Int(1) : pdc.ColorCount();
}

/// Distinct active-arc colors of a diagram = the labels of its link components
/// (a color is a *persistent* identifier preserved across simplification).
inline std::set<Int> DiagramColors(const PD_T& d)
{
    std::set<Int> colors;
    const auto& col = d.ArcColors();
    const Int A = d.ArcCount();
    for (Int a = 0; a < A; ++a)
        if (d.ArcActiveQ(a) && col[a] != PD_T::InvalidColor) { colors.insert(col[a]); }
    return colors;
}

/// HOMFLY of the SUBLINK of `d` formed by the components whose color is in
/// `subset`. SubdiagramByColors splits pure-unknot components off into the
/// returned unlink-color tensor, so we fold them back in with the delta rule
/// (each is one extra split unknot component).
inline Polynomial SublinkHomfly(const PD_T& d, const std::vector<Int>& subset, bool& ok)
{
    ok = true;
    auto pr = d.SubdiagramByColors(subset.data(), static_cast<Int>(subset.size()));
    const PD_T& sub  = pr.first;
    const Int   extra = pr.second.Size();    // unknot components split off

    Polynomial h;
    if (sub.ValidQ())
    {
        h = HomflyOfPossiblySplit(KnoodleJenkins(sub), ok);
        for (Int i = 0; i < extra; ++i) { h = Multiply(h, Delta); }   // + split unknots
    }
    else   // the sublink is all unknots: H = delta^(extra-1)
    {
        h = Polynomial{ {{0,0},1} };
        for (Int i = 1; i < extra; ++i) { h = Multiply(h, Delta); }
    }
    return h;
}

/// One Simplify, measuring the result's component count and (if needed for the
/// HOMFLY/sublink checks) reassembling it into a single colored diagram. The
/// component count is a cheap, HOMFLY-free invariant — a high-crossing torture
/// run can catch component-loss bugs with need_single=false and skip the
/// expensive polynomial work entirely.
struct SimplifyMeasure
{
    Int  comp_after        = 0;
    PD_T single_after;                 // reassembled diagram (need_single only)
    bool reduced_to_unknot = false;    // simplified away entirely (1 unknot)
    bool reassembly_failed = false;    // ToSingleDiagram returned invalid
    std::vector<PD_T> pieces;          // diagrammatically-prime pieces (need_pieces)
};

inline SimplifyMeasure SimplifyAndMeasure(const PD_T& pd, bool need_single,
                                          bool need_pieces = false)
{
    SimplifyMeasure m;
    PD_T  copy(pd);
    PDC_T pdc(std::move(copy));
    pdc.Simplify(SimplifyArgs());

    if (!pdc.ValidQ() || pdc.DiagramCount() == Int(0))
    {
        m.comp_after = 1;               // reduced to a single unknot
        m.reduced_to_unknot = true;
        return m;
    }
    m.comp_after = ComplexComponentCount(pdc);
    if (need_single)
    {
        m.single_after = pdc.ToSingleDiagram();
        if (!m.single_after.ValidQ()) { m.reassembly_failed = true; }
    }
    if (need_pieces)   // each is a diagrammatically-prime summand of the result
    {
        m.pieces.reserve(static_cast<std::size_t>(pdc.DiagramCount()));
        for (Int i = 0; i < pdc.DiagramCount(); ++i) { m.pieces.push_back(pdc.Diagram(i)); }
    }
    return m;
}

struct InvarianceResult
{
    InvStatus  status;
    Polynomial before;
    Polynomial after;
};

inline InvarianceResult CheckInvariance(const PD_T& pd)
{
    bool ok_before = true;
    Polynomial before = HomflyOfPossiblySplit(KnoodleJenkins(pd), ok_before);

    bool ok_after = true;
    Polynomial after = SimplifiedHomfly(pd, ok_after);

    const InvStatus status =
        (!ok_before || !ok_after) ? InvStatus::Unsupported
        : (before == after)       ? InvStatus::Preserved
                                  : InvStatus::Changed;
    return { status, std::move(before), std::move(after) };
}

} // namespace
