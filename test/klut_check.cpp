/**
 * @file klut_check.cpp
 * @brief Exhaustive internal-consistency test of the KLUT (Knot LookUp Table).
 *        Tier 1: reconstruct every key, compute knot invariants, and verify the
 *        table is self-consistent — without any external reference data.
 *
 * For every key in data/Klut/Klut_Keys_NN.bin (mapped to its name via the
 * corresponding Klut_Values_NN.tsv), reconstruct the diagram from its MacLeod
 * code (PlanarDiagram::FromMacLeodCode) and check:
 *
 *  (A) Alexander agrees within (c,i): all keys whose name shares the crossing
 *      number c and KnotInfo index i have the same Alexander value. Alexander
 *      is mirror/reverse-invariant, so every symmetry coset of one knot agrees.
 *      Catches a key landing in the wrong knot's bucket.
 *
 *  (B) HOMFLY agrees within a name-bucket: all keys with the exact same name
 *      K[c,i,j,"coset"] have the same HOMFLY. Catches a key in the wrong coset
 *      (e.g. a mirror placed with its non-mirror sibling — same Alexander, but
 *      HOMFLY differs).
 *
 *  (C) HOMFLY honors chirality across a knot's buckets: for one (c,i), every
 *      bucket's HOMFLY is either equal to or the mirror of a reference (mirror
 *      sends L -> L^-1, i.e. (l,m) -> (-l,m)). {e,r} share a HOMFLY, {m,mr}
 *      share its mirror, amphichiral knots are palindromic.
 *
 * Knots only (KLUT is knots-only; Alexander is knots-only). Build: test/Makefile
 * (target klut_check) — needs UMFPACK + BLAS/LAPACK and links vendored libhomfly.
 */

#define KNOODLE_USE_UMFPACK
#include "../Knoodle.hpp"

extern "C" {
#include "vendor/libhomfly/homfly.h"
}
extern "C" void knoodle_gc_free_all(void);

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using Int   = std::int64_t;
using Cplx  = std::complex<double>;
using PD_T  = Knoodle::PlanarDiagram<Int>;
using Alex_T = Knoodle::Alexander_UMFPACK<Cplx, Int>;
using Clock = std::chrono::steady_clock;

namespace {

double Secs(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}

//==============================================================================
// Invariants
//==============================================================================

const std::vector<Cplx> kAlexArgs = {
    Cplx(-1.0), Cplx(2.0), Cplx(3.0), Cplx(5.0), Cplx(-2.0)
};

struct AlexFP { std::vector<Cplx> v; };

AlexFP Alexander(const Alex_T& alex, const PD_T& pd)
{
    std::vector<Cplx>         mant(kAlexArgs.size());
    std::vector<std::int64_t> exp(kAlexArgs.size());
    alex.Alexander(pd, kAlexArgs.data(), static_cast<std::int64_t>(kAlexArgs.size()),
                   mant.data(), exp.data(), false);
    AlexFP f;
    f.v.resize(kAlexArgs.size());
    for (std::size_t i = 0; i < mant.size(); ++i)
    {
        f.v[i] = mant[i] * std::pow(10.0, static_cast<double>(exp[i]));
    }
    return f;
}

bool AlexEqual(const AlexFP& a, const AlexFP& b)
{
    constexpr double tol = 1e-3;
    for (std::size_t i = 0; i < a.v.size(); ++i)
    {
        const double scale = std::abs(a.v[i]) + std::abs(b.v[i]) + 1.0;
        if (std::abs(a.v[i] - b.v[i]) > tol * scale) { return false; }
    }
    return true;
}

std::string AlexStr(const AlexFP& f)
{
    std::string s;
    for (std::size_t i = 0; i < f.v.size(); ++i)
        s += (i ? "," : "") + std::to_string(f.v[i].real());
    return s;
}

using Polynomial = std::map<std::pair<int,int>, long>;

// HOMFLY of a connected knot via vendored libhomfly. Returns ok=false if pd is
// not a single connected component (a knot always is; defensive).
Polynomial HomflyKnot(const PD_T& pd, bool& ok)
{
    ok = false;
    if (pd.LinkComponentCount() > Int(1) || pd.DiagramComponentCount() > Int(1))
        return {};
    std::string jenkins = std::string(pd.ToJenkinsCodeString());
    std::vector<char> buf(jenkins.begin(), jenkins.end());
    buf.push_back('\0');
    Poly* p = homfly(buf.data());
    Polynomial out;
    if (p != nullptr)
        for (long i = 0; i < p->len; ++i)
            out[{ static_cast<int>(p->term[i].l), static_cast<int>(p->term[i].m) }]
                += p->term[i].coef;
    knoodle_gc_free_all();
    ok = true;
    return out;
}

// Mirror image of a HOMFLY polynomial: L -> L^-1, i.e. (l,m) -> (-l,m).
Polynomial Mirror(const Polynomial& p)
{
    Polynomial out;
    for (const auto& [lm, c] : p) { out[{ -lm.first, lm.second }] = c; }
    return out;
}

//==============================================================================
// Name parsing:  K[c, i, j, "coset"]
//==============================================================================

struct NameFields { int c = 0, i = 0, j = 0; std::string coset; bool ok = false; };

NameFields ParseName(const std::string& name)
{
    NameFields f;
    if (!name.starts_with("K[") || name.back() != ']') { return f; }
    const std::string in = name.substr(2, name.size() - 3);
    std::size_t c1 = in.find(','), c2 = c1==std::string::npos?c1:in.find(',',c1+1),
                c3 = c2==std::string::npos?c2:in.find(',',c2+1);
    if (c3 == std::string::npos) { return f; }
    try {
        f.c = std::stoi(in.substr(0, c1));
        f.i = std::stoi(in.substr(c1+1, c2-c1-1));
        f.j = std::stoi(in.substr(c2+1, c3-c2-1));
    } catch (...) { return f; }
    f.coset = in.substr(c3+1);
    f.ok = true;
    return f;
}

//==============================================================================
// Per-crossing-number sweep
//==============================================================================

struct Stats
{
    long keys = 0, recon_fail = 0, alex_mismatch = 0,
         homfly_bucket_mismatch = 0, homfly_chirality_mismatch = 0,
         reader_mismatch = 0, reader_notfound = 0;
};

Alex_T alex;  // reused across the run (caches per-diagram normalization)

void CheckCrossing(const std::string& dir, int c, bool do_alex, bool do_homfly,
                   bool do_reader, Knoodle::Klut& klut, Stats& st)
{
    const std::string cc = (c < 10 ? "0" : "") + std::to_string(c);
    std::ifstream vstream(dir + "/Klut_Values_" + cc + ".tsv");
    std::ifstream kstream(dir + "/Klut_Keys_" + cc + ".bin", std::ios::binary);
    if (!vstream || !kstream)
    {
        std::cout << "  c=" << c << ": (data missing, skipping)\n";
        return;
    }

    // References, scoped to this crossing number (c is part of every name).
    // A knot is identified by (i, j): i = index, j = alternating flag (12a_i
    // and 12n_i are different knots that share i but differ in j).
    using KnotKey = std::pair<int,int>;                       // (i, j)
    std::map<KnotKey, AlexFP>                      alex_by_knot;
    std::map<std::string, Polynomial>             homfly_by_name;
    std::map<KnotKey, std::vector<Polynomial>>    homfly_by_knot;  // one per bucket

    const auto t0 = Clock::now();
    long keys = 0;
    std::string name;
    long count;
    std::vector<Knoodle::UInt8> key(static_cast<std::size_t>(c));

    while (vstream >> name >> count)
    {
        NameFields nf = ParseName(name);
        if (!nf.ok) { std::cout << "  c=" << c << ": unparsable name '" << name << "'\n"; }

        for (long n = 0; n < count; ++n)
        {
            if (!kstream.read(reinterpret_cast<char*>(key.data()), c))
            {
                std::cout << "  c=" << c << ": key file ended early\n";
                return;
            }
            ++keys; ++st.keys;

            // (Tier 2) Reader: Klut::FindName(key) must return the value-file
            // name. Tests the hash-table loader + lookup in src/Klut.hpp.
            if (do_reader)
            {
                std::string found = klut.FindName(key.data(), Int(c));
                if (found != name)
                {
                    ++st.reader_mismatch;
                    if (found == "NotFound") { ++st.reader_notfound; }
                    if (st.reader_mismatch <= 20)
                        std::cout << "  FAIL c=" << c << " reader: FindName -> '"
                                  << found << "', expected '" << name << "'\n";
                }
            }

            if (!do_alex && !do_homfly) { continue; }   // Tier-2-only run

            PD_T pd = PD_T::FromMacLeodCode(key.data(), Int(c), Int(0));
            if (!pd.ValidQ() || pd.LinkComponentCount() != Int(1))
            {
                ++st.recon_fail;
                std::cout << "  FAIL c=" << c << " name=" << name
                          << ": key did not reconstruct to a knot.\n";
                continue;
            }

            const KnotKey knot{nf.i, nf.j};

            // (A) Alexander consistency within a knot (c,i,j).
            if (do_alex)
            {
                AlexFP a = Alexander(alex, pd);
                auto ai = alex_by_knot.find(knot);
                if (ai == alex_by_knot.end()) { alex_by_knot.emplace(knot, a); }
                else if (!AlexEqual(a, ai->second))
                {
                    ++st.alex_mismatch;
                    std::cout << "  FAIL c=" << c << " name=" << name
                              << ": Alexander differs within (c,i,j)\n"
                              << "    this key: " << AlexStr(a) << "\n"
                              << "    knot    : " << AlexStr(ai->second) << "\n";
                }
            }

            if (!do_homfly) { continue; }

            // (B) HOMFLY consistency within the exact name bucket.
            bool ok = false;
            Polynomial h = HomflyKnot(pd, ok);
            if (!ok) { continue; }
            auto hb = homfly_by_name.find(name);
            if (hb == homfly_by_name.end())
            {
                homfly_by_name.emplace(name, h);
                homfly_by_knot[knot].push_back(h);   // first key of a new bucket
            }
            else if (h != hb->second)
            {
                ++st.homfly_bucket_mismatch;
                std::cout << "  FAIL c=" << c << " name=" << name
                          << ": HOMFLY differs within the name bucket.\n";
            }
        }
    }

    // (C) Chirality: across the buckets of one knot (c,i,j), every HOMFLY is
    // equal to or the mirror of the first.
    if (do_homfly)
    {
        for (const auto& [knot, hlist] : homfly_by_knot)
        {
            const Polynomial& h0 = hlist.front();
            const Polynomial  h0m = Mirror(h0);
            for (const Polynomial& h : hlist)
            {
                if (h != h0 && h != h0m)
                {
                    ++st.homfly_chirality_mismatch;
                    std::cout << "  FAIL c=" << c << " i=" << knot.first
                              << " j=" << knot.second
                              << ": a bucket's HOMFLY is neither equal to nor the"
                                 " mirror of its sibling buckets.\n";
                    break;
                }
            }
        }
    }

    const auto t1 = Clock::now();
    std::cout << "  c=" << c << ": " << keys << " keys, "
              << alex_by_knot.size() << " distinct knots, "
              << homfly_by_name.size() << " name buckets  ["
              << Secs(t0, t1) << "s]\n";
}

} // namespace

int main(int argc, char* argv[])
{
    std::string dir = "../data/Klut";
    int from_c = 3, to_c = 13;
    bool do_alex = true, do_homfly = true, do_reader = true;

    for (int k = 1; k < argc; ++k)
    {
        std::string a(argv[k]);
        if      (a.rfind("--data-dir=", 0) == 0)        dir = a.substr(11);
        else if (a.rfind("--up-to-crossing=", 0) == 0)  to_c = std::stoi(a.substr(17));
        else if (a.rfind("--from-crossing=", 0) == 0)    from_c = std::stoi(a.substr(16));
        else if (a == "--no-alex")                       do_alex = false;
        else if (a == "--no-homfly")                     do_homfly = false;
        else if (a == "--no-reader")                     do_reader = false;
        else if (a == "--reader-only")                   { do_alex = do_homfly = false; }
        else if (a == "-h" || a == "--help")
        {
            std::cout <<
                "klut_check — exhaustive consistency test of the KLUT.\n\n"
                "Tier 1 (invariants): reconstruct every key and check\n"
                "  (A) Alexander agrees within each knot (c,i,j),\n"
                "  (B) HOMFLY agrees within each name bucket,\n"
                "  (C) HOMFLY honors chirality across a knot's buckets.\n"
                "Tier 2 (reader): Klut::FindName(key) returns the value-file name.\n\n"
                "  --data-dir=PATH       KLUT data dir (default ../data/Klut)\n"
                "  --from-crossing=N     first crossing number (default 3)\n"
                "  --up-to-crossing=N    last crossing number (default 13)\n"
                "  --no-alex             skip Alexander check (A)\n"
                "  --no-homfly           skip HOMFLY checks (B,C)\n"
                "  --no-reader           skip the Klut::FindName check\n"
                "  --reader-only         only the reader check (no reconstruction)\n";
            return 0;
        }
    }

    std::string tiers;
    if (do_alex)   tiers += "Alexander ";
    if (do_homfly) tiers += "HOMFLY ";
    if (do_reader) tiers += "reader ";
    std::cout << "=== KLUT consistency (c=" << from_c << ".." << to_c
              << ", " << tiers << ") ===\n";

    Knoodle::Klut klut(std::filesystem::path(dir), static_cast<Knoodle::Size_T>(to_c));

    Stats st;
    const auto t0 = Clock::now();
    for (int c = from_c; c <= to_c; ++c)
        CheckCrossing(dir, c, do_alex, do_homfly, do_reader, klut, st);
    const auto t1 = Clock::now();

    const long fails = st.recon_fail + st.alex_mismatch + st.homfly_bucket_mismatch
                     + st.homfly_chirality_mismatch + st.reader_mismatch;
    std::cout << "\nklut_check: " << st.keys << " keys in " << Secs(t0, t1) << "s\n"
              << "  reconstruction failures     : " << st.recon_fail << "\n";
    if (do_alex)
        std::cout << "  Alexander (c,i,j) mismatches: " << st.alex_mismatch << "\n";
    if (do_homfly)
        std::cout << "  HOMFLY bucket mismatches    : " << st.homfly_bucket_mismatch << "\n"
                  << "  HOMFLY chirality mismatches : " << st.homfly_chirality_mismatch << "\n";
    if (do_reader)
        std::cout << "  reader (FindName) mismatches: " << st.reader_mismatch
                  << "  (of which not-found: " << st.reader_notfound << ")\n";
    std::cout << (fails == 0 ? "PASS: KLUT is consistent.\n"
                             : (std::to_string(fails) + " failure(s).\n"));
    return fails == 0 ? 0 : 1;
}
