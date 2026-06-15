/**
 * @file link_split_check.cpp
 * @brief Direct test of the SPLIT-link corner case: split unions of several
 *        components, including a component that collapses to a free (0-crossing)
 *        loop under Simplify.
 *
 * Split links are where the HOMFLY path is most fragile: libhomfly segfaults on
 * split input (even the bare unknot "1 0"). The oracle in homfly_invariance.hpp
 * (HomflyOfPossiblySplit) is supposed to defuse that by union-find-decomposing
 * the diagram into connected pieces, running libhomfly on each, and recombining
 * with the split-union delta rule  H(A u B) = delta * H(A) * H(B). This test
 * exercises that path head-on:
 *
 *   (1) Build split unions by arc-offsetting connected pieces (trefoil, fig-8,
 *       Hopf, a 9-crossing unknot) into one disconnected diagram.
 *   (2) Oracle correctness: HomflyOfPossiblySplit(union) must equal the
 *       INDEPENDENT delta-rule product of the pieces' own HOMFLYs.
 *   (3) Simplify must preserve HOMFLY and component count -- including when a
 *       piece (the unknot) reduces to a free loop, which is exactly the "1 0"
 *       input that crashes raw libhomfly.
 *   (4) The single-variable |det| fingerprint must DEGRADE GRACEFULLY on split
 *       links (det = 0 -> ok = false), never crash or report a bogus value.
 *
 * Build: see test/Makefile (target: link_split_check). Needs UMFPACK + Accelerate
 * + the vendored libhomfly objects.
 */

#include "../Knoodle.hpp"
#include "homfly_invariance.hpp"   // Int, PDC_T, PD_T, Polynomial, oracle, Delta, Multiply
#include "link_alexander.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

using Cplx       = std::complex<double>;
using LinkAlex_T = knoodle_test::LinkAlexander<Cplx, Int>;

// A piece = its flat 5-col signed PD code (arcs 0..2c-1) and crossing count.
struct Piece
{
    std::vector<Int> code;
    Int              crossings = 0;
};

Piece PieceFromInts(std::vector<Int> flat)
{
    Piece p;
    p.crossings = static_cast<Int>(flat.size() / 5);
    p.code = std::move(flat);
    return p;
}

Piece PieceFromFile(const std::string& path)
{
    std::ifstream in(path);
    std::vector<Int> ints;
    Int v;
    while (in >> v) { ints.push_back(v); }
    return PieceFromInts(std::move(ints));
}

// Build one diagram that is the split (disjoint) union of the pieces, by
// offsetting each piece's arc labels past the previous pieces. Disjoint arc sets
// => disconnected diagram => DiagramComponentCount() == #pieces.
PD_T SplitUnion(const std::vector<Piece>& pieces)
{
    std::vector<Int> all;
    Int arc_offset = 0;
    Int crossings  = 0;
    for (const auto& p : pieces)
    {
        const Int rows = p.crossings;
        for (Int r = 0; r < rows; ++r)
        {
            for (int k = 0; k < 4; ++k) { all.push_back(p.code[5 * r + k] + arc_offset); }
            all.push_back(p.code[5 * r + 4]);   // sign unchanged
        }
        arc_offset += Int(2) * p.crossings;     // #arcs of a 4-valent diagram = 2c
        crossings  += p.crossings;
    }
    return PD_T::FromSignedPDCode(all.data(), crossings, false, true);
}

// delta^(k-1) * prod H(piece): the textbook split-union HOMFLY, computed from the
// pieces independently (each piece is connected, so libhomfly handles it direct).
Polynomial IndependentSplitHomfly(const std::vector<Piece>& pieces, bool& ok)
{
    ok = true;
    Polynomial prod = { {{0, 0}, 1} };   // multiplicative identity
    for (const auto& p : pieces)
    {
        PD_T pd = PD_T::FromSignedPDCode(p.code.data(), p.crossings, false, true);
        bool ok_i = false;
        Polynomial h = HomflyOfPossiblySplit(KnoodleJenkins(pd), ok_i);
        if (!ok_i) { ok = false; return {}; }
        prod = Multiply(prod, h);
    }
    for (std::size_t i = 1; i < pieces.size(); ++i) { prod = Multiply(prod, Delta); }
    return prod;
}

struct Case
{
    std::string                name;
    std::vector<std::string>   piece_names;   // keys into the piece table
};

bool RunCase(const Case& c, const std::map<std::string, Piece>& table,
             const LinkAlex_T& linkalex)
{
    std::cout << "=== " << c.name << " ===\n";

    std::vector<Piece> pieces;
    for (const auto& nm : c.piece_names)
    {
        auto it = table.find(nm);
        if (it == table.end() || it->second.crossings <= 0)
        {
            std::cout << "  FAIL: piece '" << nm << "' unavailable.\n";
            return false;
        }
        pieces.push_back(it->second);
    }

    PD_T link = SplitUnion(pieces);
    if (!link.ValidQ())
    {
        std::cout << "  FAIL: SplitUnion produced an invalid diagram.\n";
        return false;
    }

    const Int diag_comps = link.DiagramComponentCount();
    const Int link_comps = link.LinkComponentCount();
    std::cout << "  crossings=" << link.CrossingCount()
              << "  diagram-components=" << diag_comps
              << "  link-components=" << link_comps << "\n";

    if (diag_comps != static_cast<Int>(pieces.size()))
    {
        std::cout << "  FAIL: expected " << pieces.size()
                  << " split diagram-components, got " << diag_comps << ".\n";
        return false;
    }

    bool pass = true;

    // (2) Oracle correctness vs the independent delta-rule product.
    bool ok_oracle = false, ok_indep = false;
    const Polynomial h_oracle = HomflyOfPossiblySplit(KnoodleJenkins(link), ok_oracle);
    const Polynomial h_indep  = IndependentSplitHomfly(pieces, ok_indep);
    if (!ok_oracle || !ok_indep)
    {
        std::cout << "  FAIL: HOMFLY unavailable (oracle ok=" << ok_oracle
                  << ", independent ok=" << ok_indep << ").\n";
        pass = false;
    }
    else if (h_oracle != h_indep)
    {
        std::cout << "  FAIL: split-union HOMFLY disagrees with delta-rule product.\n"
                  << "    oracle     : " << PolyToString(h_oracle) << "\n"
                  << "    delta-rule : " << PolyToString(h_indep)  << "\n";
        pass = false;
    }
    else
    {
        std::cout << "  HOMFLY(union) = " << PolyToString(h_oracle) << "\n"
                  << "  oracle == delta-rule product: OK\n";
    }

    // (4) |det| fingerprint must degrade gracefully on a split link: the reduced
    // strand matrix is singular (det ~ 1e-16), so it must report ok=false rather
    // than a numerically-bogus value marked ok=true.
    const LinkAlex_T::Value fp = linkalex(link);
    if (fp.ok)
    {
        std::cout << "  FAIL: |det| fingerprint reported ok=true on a split link "
                     "(must be ok=false; det is singular).\n";
        pass = false;
    }
    else
    {
        std::cout << "  |det| fingerprint ok=false on split link: graceful\n";
    }

    // (3) Simplify must preserve HOMFLY and component count -- including the free
    // loop that the unknot piece collapses to.
    SimplifyMeasure m = SimplifyAndMeasure(link, /*need_single=*/true);
    if (m.reassembly_failed)
    {
        std::cout << "  FAIL: ToSingleDiagram failed to reassemble the simplified "
                     "complex.\n";
        return false;
    }
    Polynomial h_after;
    bool ok_after = true;
    if (m.reduced_to_unknot)
    {
        h_after = Polynomial{ {{0, 0}, 1} };
    }
    else
    {
        h_after = HomflyOfPossiblySplit(KnoodleJenkins(m.single_after), ok_after);
    }

    std::cout << "  Simplify: " << link.CrossingCount() << " -> "
              << (m.reduced_to_unknot ? Int(0) : m.single_after.CrossingCount())
              << " crossings, components " << link_comps << " -> " << m.comp_after << "\n";

    if (m.comp_after != link_comps)
    {
        std::cout << "  FAIL: component count changed under Simplify ("
                  << link_comps << " -> " << m.comp_after << ").\n";
        pass = false;
    }
    if (!ok_after)
    {
        std::cout << "  FAIL: HOMFLY of simplified diagram unavailable.\n";
        pass = false;
    }
    else if (ok_oracle && h_after != h_oracle)
    {
        std::cout << "  FAIL: HOMFLY changed under Simplify.\n"
                  << "    before: " << PolyToString(h_oracle) << "\n"
                  << "    after : " << PolyToString(h_after)  << "\n";
        pass = false;
    }
    else
    {
        std::cout << "  HOMFLY + component count preserved under Simplify: OK\n";
    }

    std::cout << (pass ? "  PASS\n" : "  FAIL\n");
    return pass;
}

} // namespace

int main(int argc, char* argv[])
{
    std::string linktable  = "../data/diagrams/linktable";
    std::string unknot_pd  = "../data/diagrams/hardunknots/H.tsv";  // 9-crossing unknot
    for (int i = 1; i < argc; ++i)
    {
        std::string a(argv[i]);
        if      (a.rfind("--linktable=", 0) == 0) { linktable = a.substr(12); }
        else if (a.rfind("--unknot=", 0) == 0)    { unknot_pd = a.substr(9); }
    }

    std::map<std::string, Piece> table;
    table["trefoil"] = PieceFromInts({0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1});
    table["fig8"]    = PieceFromInts({3,1,4,0,1, 7,5,0,4,1, 5,2,6,3,-1, 1,6,2,7,-1});
    table["hopf"]    = PieceFromFile(linktable + "/L2a1_0.tsv");
    table["unknot"]  = PieceFromFile(unknot_pd);

    std::cout << "link_split_check: direct split-link corner-case test\n"
              << "  pieces: trefoil(" << table["trefoil"].crossings << "c) "
              << "fig8(" << table["fig8"].crossings << "c) "
              << "hopf(" << table["hopf"].crossings << "c) "
              << "unknot(" << table["unknot"].crossings << "c)\n\n";

    const std::vector<Case> cases = {
        { "trefoil u trefoil",              {"trefoil", "trefoil"} },
        { "trefoil u hopf",                 {"trefoil", "hopf"} },
        { "hopf u hopf",                    {"hopf", "hopf"} },
        { "trefoil u fig8 u hopf",          {"trefoil", "fig8", "hopf"} },
        { "trefoil u trefoil u trefoil",    {"trefoil", "trefoil", "trefoil"} },
        { "trefoil u unknot (free loop)",   {"trefoil", "unknot"} },
        { "hopf u unknot (free loop)",      {"hopf", "unknot"} },
        { "unknot u unknot (two free loops)", {"unknot", "unknot"} },
    };

    LinkAlex_T linkalex{};
    int fails = 0;
    for (const auto& c : cases)
    {
        if (!RunCase(c, table, linkalex)) { ++fails; }
        std::cout << "\n";
    }

    std::cout << "link_split_check: " << (static_cast<int>(cases.size()) - fails)
              << "/" << cases.size() << " cases passed\n";
    return fails == 0 ? 0 : 1;
}
