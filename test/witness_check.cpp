// The feasibility-witness verifier, tools/witness_check.hpp.
//
// A middlepass record may carry a `#feas`/`#fvar` witness: the claimed sweep
// disk on one side of the move, and a labelling of every piece on that side.
// The emitter checks V1/V2/V3/V5 against its own reading of the disk; V0 (the
// disk and the pieces, rebuilt independently) and V4 (no stray merges) are
// ours. This test checks them on
//
//   1. four real witnesses from middlestrands' v2 fixture, which must pass;
//   2. corruptions of one of them, each of which a check must catch -- a green
//      tick means nothing unless the check can fail;
//   3. malformed witness headers, which the READER must refuse;
//   4. the Whitehead link whose witness lies by omission: a clasped circle sits
//      inside the disk, and the witness leaves it out. This is also the one
//      place the side convention is cross-checked on a link, against the disk
//      middlestrands' own region flood computes for it.
//
// Build: `make witness_check` in test/ (a row in test/manifest.tsv).

#include "../Knoodle.hpp"
#include "../tools/witness_check.hpp"
#include "witness_fixtures.hpp"
#include "knot_determinant.hpp"

#include <cstdint>
#include <cstdio>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using PD_T    = Knoodle::PlanarDiagram<Int>;
using Trace_T = Knoodle::MoveTrace<PD_T>;
using Desc_T  = Knoodle::PassDescriptor<Int>;
using Feas_T  = Trace_T::Feasibility;

using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
using Deco_T      = Knoodle::OrthoDecorate<PD_T>;

static bool ok = true;

// Checks that passed. Printed at the end so the manifest's work pattern can
// tell a run that examined something from one that examined nothing.
static int checks_passed = 0;

static void check( bool passedQ, const std::string & what )
{
    std::printf("  %-68s %s\n", what.c_str(), passedQ ? "OK" : "FAILED");
    if( passedQ ) { ++checks_passed; } else { ok = false; }
}

static bool Contains( const std::string & s, const char * needle )
{
    return s.find(needle) != std::string::npos;
}

struct Loaded_T
{
    std::optional<PD_T> pd;
    Desc_T              mv;
    Feas_T              feas;
};

static bool Load( const char * text, Loaded_T & out, std::string & why )
{
    std::istringstream in (text);
    Trace_T::Reader reader (in);
    Trace_T::Record rec;

    if( reader.Next(rec,why) != Trace_T::Status::Record ) { return false; }
    if( !rec.state || !rec.move || !rec.feas )
    {
        why = "record lacks #state, #move or #feas";
        return false;
    }
    if( !Desc_T::Parse(*rec.move, out.mv, why) ) { return false; }

    out.pd   = std::move(*rec.state);
    out.feas = *rec.feas;
    return true;
}

static bool ReaderRefuses( const std::string & text, std::string & why )
{
    std::istringstream in (text);
    Trace_T::Reader reader (in);
    Trace_T::Record rec;
    return (reader.Next(rec,why) == Trace_T::Status::Error) && !why.empty();
}

static KnoodleWitness::WitnessReport_T Run( const Loaded_T & L, const Feas_T & f )
{
    return KnoodleWitness::CheckWitness<PD_T>(*L.pd, L.mv, f);
}

int main()
{
    // ---- 1. real witnesses ---------------------------------------------------
    std::printf("=== real witnesses from middlestrands' v2 fixture ===\n");

    const std::vector<std::pair<const char *, const char *>> fixtures = {
        { "8-crossing disk",            witness_rec_small_disk },
        { "far-side disk, anchors in",  witness_rec_far_disk   },
        { "empty disk",                 witness_rec_empty_disk },
        { "candidate, 1-crossing disk", witness_rec_candidate  },
    };

    for( const auto & [name, text] : fixtures )
    {
        Loaded_T L;
        std::string why;
        if( !Load(text, L, why) )
        {
            check(false, std::string(name) + ": load (" + why + ")");
            continue;
        }

        const auto r = Run(L, L.feas);

        check(r.v0_okQ, std::string(name) + ": V0, side " + std::to_string(L.feas.side)
            + ", " + std::to_string(r.disk_size) + " crossings, "
            + std::to_string(r.piece_count) + " pieces");
        if( !r.v0_okQ ) { std::printf("      %s\n", r.v0_why.c_str()); }

        check(r.v4_okQ, std::string(name) + ": V4, "
            + std::to_string(r.class_count) + " classes");
        if( !r.v4_okQ ) { std::printf("      %s\n", r.v4_why.c_str()); }

        check(r.labels_okQ, std::string(name) + ": V2/V3/V5, "
            + std::to_string(r.germ_count) + " germs, "
            + std::to_string(r.order_count) + " orderings, "
            + std::to_string(r.tag_count) + " tags");
        if( !r.labels_okQ ) { std::printf("      %s\n", r.labels_why.c_str()); }
    }

    // ---- 2. corruptions ------------------------------------------------------
    std::printf("=== corruptions of the 8-crossing witness ===\n");
    {
        Loaded_T L;
        std::string why;
        if( !Load(witness_rec_small_disk, L, why) )
        {
            check(false, "load the 8-crossing record (" + why + ")");
        }
        else
        {
            const Feas_T base = L.feas;
            const int    s    = base.side;

            KnoodleWitness::Sides_T<PD_T> S;
            check(KnoodleWitness::ReconstructSides<PD_T>(*L.pd, L.mv, S, why),
                  "the sides reconstruct");

            auto expect_v0 = [&]( const Feas_T & f, const char * needle,
                                  const char * what )
            {
                const auto r = Run(L, f);
                const bool caughtQ = !r.v0_okQ && Contains(r.v0_why, needle);
                check(caughtQ, std::string("V0 catches: ") + what);
                std::printf("      %s\n", r.v0_okQ ? "(passed V0)" : r.v0_why.c_str());
            };

            auto expect_v4 = [&]( const Feas_T & f, const char * needle,
                                  const char * what )
            {
                const auto r = Run(L, f);
                const bool caughtQ = r.v0_okQ && !r.v4_okQ && Contains(r.v4_why, needle);
                check(caughtQ, std::string("V4 catches: ") + what);
                std::printf("      %s\n", !r.v0_okQ ? r.v0_why.c_str()
                                        : (r.v4_okQ ? "(passed V4)" : r.v4_why.c_str()));
            };

            {
                Feas_T f = base;
                f.disk.erase(f.disk.begin());
                expect_v0(f, "omits", "a crossing dropped from disk=");
            }
            {
                Feas_T f = base;
                for( Int c = 0; c < L.pd->MaxCrossingCount(); ++c )
                {
                    if( !L.pd->CrossingActiveQ(c) ) { continue; }
                    if( std::find(S.disk[s].begin(), S.disk[s].end(), c) == S.disk[s].end() )
                    {
                        f.disk.push_back(c);
                        break;
                    }
                }
                expect_v0(f, "not interior", "a crossing added to disk=");
            }
            {
                Feas_T f = base;
                f.classes[1].pieces.pop_back();
                expect_v0(f, "in no #fvar class", "a piece left out of every class");
            }
            {
                Feas_T f = base;
                for( Int key = 0; key < Int(S.side_of.size()); ++key )
                {
                    if( S.side_of[std::size_t(key)] == 1 - s )
                    {
                        f.classes[0].pieces.push_back({ key / Int(3), int(key % Int(3)) - 1 });
                        break;
                    }
                }
                expect_v0(f, "lies on side", "a piece from the other side");
            }
            {
                Feas_T f = base;
                f.classes[0].pieces.push_back({ Desc_T::ArcOf(L.mv.strand[1]), -1 });
                expect_v0(f, "arc of W", "an arc of W named as a piece");
            }
            {
                Feas_T f = base;
                for( auto & cls : f.classes )
                {
                    auto it = std::find_if(cls.pieces.begin(), cls.pieces.end(),
                                           []( const auto & p ) { return p.half >= 0; });
                    if( it != cls.pieces.end() ) { it->half = -1; break; }
                }
                expect_v0(f, "crossed by the corridor", "a crossed arc named whole");
            }
            {
                Feas_T f = base;
                f.side = 1 - f.side;
                expect_v0(f, "side", "the side flipped");
            }
            {
                Feas_T f = base;
                auto & into = f.classes[1].pieces;
                into.insert(into.end(), f.classes[2].pieces.begin(), f.classes[2].pieces.end());
                f.classes.erase(f.classes.begin() + 2);
                expect_v4(f, "stray merge", "two classes merged");
            }
            {
                Feas_T f = base;
                Trace_T::FeasClass lone;
                lone.label = f.classes[4].label;
                lone.pieces.push_back(f.classes[4].pieces.back());
                f.classes[4].pieces.pop_back();
                f.classes.push_back(lone);
                expect_v4(f, "must share a class", "a class split in two");
            }
        }
    }

    // ---- 2b. label corruptions ---------------------------------------------
    std::printf("=== label corruptions V2, V3 and V5 must catch ===\n");
    {
        Loaded_T L;
        std::string why;
        if( !Load(witness_rec_small_disk, L, why) )
        {
            check(false, "load the 8-crossing record (" + why + ")");
        }
        else
        {
            const Feas_T base = L.feas;

            auto run = [&]( const Desc_T & mv, const Feas_T & f )
            {
                return KnoodleWitness::CheckWitness<PD_T>(*L.pd, mv, f);
            };

            // Every class of this witness is forced by a germ or a tag, so
            // flipping any single label must be caught.
            bool v2_seen = false, v5_seen = false;
            for( std::size_t ci = 0; ci < base.classes.size(); ++ci )
            {
                Feas_T f = base;
                f.classes[ci].label = (f.classes[ci].label == 'a') ? 'b' : 'a';

                const auto r = run(L.mv, f);
                const bool caughtQ = r.v0_okQ && r.v4_okQ && !r.labels_okQ;
                check(caughtQ, "a label flip on class " + std::to_string(ci) + " is caught");
                std::printf("      %s\n", caughtQ ? r.labels_why.c_str() : "(passed)");

                v2_seen = v2_seen || Contains(r.labels_why, "(V2)");
                v5_seen = v5_seen || Contains(r.labels_why, "(V5)");
            }
            check(v2_seen, "some flip is caught as a germ violation (V2)");
            check(v5_seen, "some flip is caught as a tag violation (V5)");

            {
                Desc_T mv = L.mv;
                mv.over[0] = !mv.over[0];
                const auto r = run(mv, base);
                check(!r.labels_okQ && Contains(r.labels_why, "(V5)"),
                      "V5 catches: a route tag flipped");
                std::printf("      %s\n", r.labels_okQ ? "(passed)" : r.labels_why.c_str());
            }

            // V3 needs a disk crossing whose two strands lie in different
            // classes: make the under-strand's class above and the over's below.
            {
                KnoodleWitness::Sides_T<PD_T> S;
                KnoodleWitness::ReconstructSides<PD_T>(*L.pd, L.mv, S, why);

                auto class_index = []( const Feas_T & f, Int key ) -> int
                {
                    for( std::size_t ci = 0; ci < f.classes.size(); ++ci )
                    {
                        for( const auto & p : f.classes[ci].pieces )
                        {
                            if( KnoodleWitness::PieceKey(p.arc, p.half) == key ) { return int(ci); }
                        }
                    }
                    return -1;
                };

                bool builtQ = false;
                for( Int c : S.disk[base.side] )
                {
                    const auto k = KnoodleWitness::StrandKeysAt<PD_T>(*L.pd, S, c);
                    if( (k[0] < 0) || (k[1] < 0) || (k[2] < 0) || (k[3] < 0) ) { continue; }

                    const int cu = class_index(base, k[0]);
                    const int co = class_index(base, k[2]);
                    if( (cu < 0) || (co < 0) || (cu == co) ) { continue; }

                    Feas_T f = base;
                    f.classes[std::size_t(cu)].label = 'a';
                    f.classes[std::size_t(co)].label = 'b';

                    const auto r = run(L.mv, f);
                    check(!r.labels_okQ && Contains(r.labels_why, "(V3)"),
                          "V3 catches: under above, over below at crossing " + std::to_string(c));
                    std::printf("      %s\n", r.labels_okQ ? "(passed)" : r.labels_why.c_str());
                    builtQ = true;
                    break;
                }
                check(builtQ, "a disk crossing with its two strands in two classes exists");
            }
        }
    }

    // ---- 2c. chords and anchors ---------------------------------------------
    // No route arc in middlestrands' fixture is a chord of W, and none of their
    // witnesses breaks an ordering at an anchor, so these three records come
    // from a search over data/diagrams with a scratch solver; see
    // witness_fixtures.hpp for where each came from.
    std::printf("=== chords and anchors: synthetic witnesses ===\n");
    {
        // A crossed arc whose tail and head are both interior crossings of W.
        auto chord_of = []( const Loaded_T & L ) -> Int
        {
            std::vector<Int> interior;
            for( std::size_t i = 1; i < L.mv.strand.size(); ++i )
            {
                interior.push_back(Desc_T::DarcHeadCrossing(*L.pd, L.mv.strand[i-1]));
            }
            auto inQ = [&]( Int c ) { return std::find(interior.begin(), interior.end(), c) != interior.end(); };
            for( Int da : L.mv.cross )
            {
                const Int a = Desc_T::ArcOf(da);
                if( inQ(L.pd->Arcs()(a, PD_T::Tail)) && inQ(L.pd->Arcs()(a, PD_T::Head)) ) { return a; }
            }
            return Int(-1);
        };

        std::string why;

        Loaded_T agree;
        if( !Load(witness_rec_chord_agree, agree, why) )
        {
            check(false, "load the chord-agree record (" + why + ")");
        }
        else
        {
            const Int b = chord_of(agree);
            check(b >= 0, "agree: the corridor crosses a chord of W (arc " + std::to_string(b) + ")");
            const auto r = Run(agree, agree.feas);
            check(r.v0_okQ && r.v4_okQ && r.labels_okQ,
                  "agree: a witness honouring the chord rule passes V0-V5");
            if( !r.labels_okQ ) { std::printf("      %s\n", r.labels_why.c_str()); }
        }

        Loaded_T conflict;
        if( !Load(witness_rec_chord_conflict, conflict, why) )
        {
            check(false, "load the chord-conflict record (" + why + ")");
        }
        else
        {
            const Int b = chord_of(conflict);
            check(b >= 0, "conflict: the corridor crosses a chord of W (arc " + std::to_string(b) + ")");
            const auto r = Run(conflict, conflict.feas);
            check(r.v0_okQ && r.v4_okQ && r.labels_okQ,
                  "conflict: a disagreeing chord's witness passes V0-V5 (no chord rule)");
            std::printf("      %s\n", r.labels_okQ ? "(passed)" : r.labels_why.c_str());

            // Each half is still bound by its own germ: flip the class of the
            // chord's side-s half and V2 must name that half.
            int ci_half = -1;
            std::string half_name;
            for( std::size_t ci = 0; ci < conflict.feas.classes.size(); ++ci )
            {
                for( const auto & p : conflict.feas.classes[ci].pieces )
                {
                    if( (p.arc == b) && (p.half >= 0) )
                    {
                        ci_half   = int(ci);
                        half_name = std::to_string(b) + (p.half == 0 ? "t" : "h");
                    }
                }
            }
            check(ci_half >= 0, "conflict: the witness names a half of the chord");
            if( ci_half >= 0 )
            {
                Feas_T f = conflict.feas;
                auto & lab = f.classes[std::size_t(ci_half)].label;
                lab = (lab == 'a') ? 'b' : 'a';
                const auto rf = Run(conflict, f);
                check(!rf.labels_okQ && Contains(rf.labels_why, "(V2)")
                      && Contains(rf.labels_why, (half_name + " must be").c_str()),
                      "conflict: flipping " + half_name + " is caught by its own germ (V2)");
                std::printf("      %s\n", rf.labels_okQ ? "(passed)" : rf.labels_why.c_str());
            }
        }

        Loaded_T sound;
        if( !Load(witness_rec_chord_sound, sound, why) )
        {
            check(false, "load the sound disagreeing-chord record (" + why + ")");
        }
        else
        {
            const Int b = chord_of(sound);
            check(b >= 0, "sound: the corridor crosses a chord of W (arc " + std::to_string(b) + ")");
            const auto r = Run(sound, sound.feas);
            check(r.v0_okQ && r.v4_okQ && r.labels_okQ,
                  "sound: Theorem B's system, chord rule off, passes V0-V5");
            if( !r.labels_okQ ) { std::printf("      %s\n", r.labels_why.c_str()); }

            // By Theorem B the move is an isotopy; the determinant agrees.
            OrthoDraw_T H (*sound.pd, Int(-1), OrthoDraw_T::Settings_T{});
            Deco_T deco (H, Int(2));
            std::vector<Int> freed;
            std::string awhy;
            const PD_T after = deco.AfterDiagram(*sound.pd, sound.mv, awhy, freed);

            const Int P  = 1000003;
            const Int d0 = DeterminantModP(*sound.pd);
            const Int d1 = awhy.empty() ? DeterminantModP(after) : Int(0);
            const bool sameQ = awhy.empty() && ((d0 == d1) || ((d0 + d1) % P == 0));
            check(sameQ, "sound: and the move it prescribes keeps the determinant");
            std::printf("      %s\n", !awhy.empty() ? awhy.c_str()
                : ("det " + std::to_string(std::min(d0, P - d0)) + " -> "
                   + std::to_string(std::min(d1, P - d1))).c_str());
        }

        Loaded_T lie;
        if( !Load(witness_rec_anchor_lie, lie, why) )
        {
            check(false, "load the anchor-ordering record (" + why + ")");
        }
        else
        {
            const auto r = Run(lie, lie.feas);
            check(r.v0_okQ && r.v4_okQ && !r.labels_okQ
                  && Contains(r.labels_why, "(an anchor)") && Contains(r.labels_why, "(V3)"),
                  "anchor: V3 refuses a witness that breaks an ordering at an anchor");
            std::printf("      %s\n", r.labels_okQ ? "(passed)" : r.labels_why.c_str());

            // Why the anchor orderings matter: the move this witness prescribes
            // is not an isotopy.
            OrthoDraw_T H (*lie.pd, Int(-1), OrthoDraw_T::Settings_T{});
            Deco_T deco (H, Int(2));
            std::vector<Int> freed;
            std::string awhy;
            const PD_T after = deco.AfterDiagram(*lie.pd, lie.mv, awhy, freed);

            const Int P  = 1000003;
            const Int d0 = DeterminantModP(*lie.pd);
            const Int d1 = awhy.empty() ? DeterminantModP(after) : Int(0);
            const bool changedQ = awhy.empty() && (d0 != d1) && ((d0 + d1) % P != 0);
            check(changedQ, "anchor: and the move it prescribes changes the determinant");
            std::printf("      %s\n", !awhy.empty() ? awhy.c_str()
                : ("det " + std::to_string(std::min(d0, P - d0)) + " -> "
                   + std::to_string(std::min(d1, P - d1))).c_str());
        }
    }

    // ---- 2d. Theorem B's strand hypothesis ------------------------------------
    // W's crossings must be pairwise distinct. Well-formedness does not ask for
    // that, so V0 reports UNCHECKED instead: walk a knot from W's first arc
    // until the walk returns to a crossing it has already passed, go one arc
    // further, and hand that strand to the reconstruction.
    std::printf("=== a strand that passes through a crossing twice ===\n");
    {
        Loaded_T L;
        std::string why;
        if( !Load(witness_rec_chord_agree, L, why) )
        {
            check(false, "load the chord-agree record (" + why + ")");
        }
        else
        {
            Desc_T mv = L.mv;
            mv.strand.clear();
            mv.cross.clear();
            mv.over.clear();

            Int cur = Desc_T::ArcOf(L.mv.strand.front());
            std::vector<Int> seen { L.pd->Arcs()(cur, PD_T::Tail) };
            bool repeatQ = false;
            while( !repeatQ && (mv.strand.size() < std::size_t(L.pd->ArcCount())) )
            {
                mv.strand.push_back(Int(2) * cur + Int(1));
                const Int c = L.pd->Arcs()(cur, PD_T::Head);
                repeatQ = std::find(seen.begin(), seen.end(), c) != seen.end();
                seen.push_back(c);
                cur = L.pd->NextArc(cur, PD_T::Head);
            }
            mv.strand.push_back(Int(2) * cur + Int(1));
            mv.depart = mv.strand.front();
            mv.land   = mv.strand.back() - Int(1);

            KnoodleWitness::Sides_T<PD_T> S;
            const bool builtQ = KnoodleWitness::ReconstructSides<PD_T>(*L.pd, mv, S, why);
            check(repeatQ && !builtQ && Contains(why, "twice; Theorem B"),
                  "V0 is UNCHECKED on a strand of " + std::to_string(mv.strand.size())
                  + " arcs that repeats a crossing");
            std::printf("      %s\n", why.c_str());
        }
    }

    // ---- 3. the reader refuses malformed witnesses ----------------------------
    std::printf("=== malformed witness headers ===\n");
    {
        const std::string head = "#trace v=1\n#move kind=middlepass strand=1,3 depart=0 land=2\n";

        const std::vector<std::pair<const char *, std::string>> cases = {
            { "#fvar before #feas",       head + "#fvar 5=a\n" },
            { "side other than 0 or 1",   head + "#feas side=2 disk=\n" },
            { "no disk= field",           head + "#feas side=0\n" },
            { "an empty disk= entry",     head + "#feas side=0 disk=4,,6\n" },
            { "a label other than a/b/f", head + "#feas side=0 disk=\n#fvar 5=x\n" },
            { "a piece in two classes",   head + "#feas side=0 disk=\n#fvar 5=a\n#fvar 5=b\n" },
            { "a piece twice in a class", head + "#feas side=0 disk=\n#fvar 5,5=a\n" },
            { "a malformed piece",        head + "#feas side=0 disk=\n#fvar 5q=a\n" },
            { "a second #feas",           head + "#feas side=0 disk=\n#feas side=1 disk=\n" },
        };

        for( const auto & [what, text] : cases )
        {
            std::string why;
            const bool refusedQ = ReaderRefuses(text, why);
            check(refusedQ, std::string("refused: ") + what);
            if( refusedQ ) { std::printf("      %s\n", why.c_str()); }
        }
    }

    // ---- 4. the Whitehead witness that lies by omission -------------------------
    std::printf("=== the Whitehead link: a witness that leaves out a trapped circle ===\n");
    {
        // handoff middlepass-descriptor-emission/link-trapped-whitehead.tsv
        std::vector<Int> code = {
            13, 11,  0, 10,  1,
             0,  7,  1,  8, -1,
             6,  1,  7,  2, -1,
             2, 11,  3, 12, -1,
             3, 13,  4, 12,  1,
             4, 16,  5, 15,  1,
            14,  6, 15,  5,  1,
            17,  8, 14,  9, -1,
             9, 16, 10, 17, -1,
        };
        PD_T pd = PD_T::FromSignedPDCode(code.data(), Int(9), false, false);

        // Their move (move.json): W = arcs 0,1,2; route arcs 10 then 4 with
        // entry bits 1,0; blind tags u on arc 10, o on arc 4.
        Desc_T mv;
        mv.middlepassQ = true;
        mv.strand = { 1, 3, 5 };
        mv.cross  = { 21, 8 };
        mv.over   = { false, true };
        for( Int d : { Int(0), Int(1) } )
        {
            if( Desc_T::LeftFace(pd, d) == Desc_T::LeftFace(pd, mv.cross.front()) ) { mv.depart = d; }
        }
        for( Int d : { Int(4), Int(5) } )
        {
            if( Desc_T::LeftFace(pd, d) == Desc_T::RightFace(pd, mv.cross.back()) ) { mv.land = d; }
        }

        std::string why;
        const bool wfQ = mv.WellFormedQ(pd, why);
        check(wfQ, "their move is a well-formed middlepass descriptor");
        if( !wfQ ) { std::printf("      %s\n", why.c_str()); }

        KnoodleWitness::Sides_T<PD_T> S;
        const bool rebuiltQ = KnoodleWitness::ReconstructSides<PD_T>(pd, mv, S, why);
        check(rebuiltQ, "the sides reconstruct on a link");
        if( !rebuiltQ ) { std::printf("      %s\n", why.c_str()); }

        if( rebuiltQ )
        {
            const std::vector<Int> clasps = { 5, 6, 7, 8 };
            std::string d1;
            for( Int c : S.disk[1] ) { d1 += " " + std::to_string(c); }
            check(S.disk[1] == clasps,
                  "side 1's disk is C's clasps {5,6,7,8}, as their F.1 flood finds");
            std::printf("      side-1 disk:%s\n", d1.c_str());

            bool c_on_1 = true;
            for( Int a = 14; a <= 17; ++a )
            {
                c_on_1 = c_on_1 && (S.side_of[std::size_t(KnoodleWitness::PieceKey(a,-1))] == 1);
            }
            check(c_on_1, "the clasped circle C (arcs 14-17) lies on side 1");

            // The blind witness: side 1, an empty disk, and every side-1 piece
            // except C's.
            Feas_T lie;
            lie.side = 1;
            for( Int key = 0; key < Int(S.side_of.size()); ++key )
            {
                const Int a = key / Int(3);
                if( (S.side_of[std::size_t(key)] != 1) || (a >= 14 && a <= 17) ) { continue; }
                Trace_T::FeasClass cls;
                cls.label = 'b';
                cls.pieces.push_back({ a, int(key % Int(3)) - 1 });
                lie.classes.push_back(cls);
            }

            const auto r = KnoodleWitness::CheckWitness<PD_T>(pd, mv, lie);
            check(!r.v0_okQ && Contains(r.v0_why, "interior on side 1"),
                  "V0 refuses the witness that leaves C out");
            std::printf("      %s\n", r.v0_okQ ? "(passed V0)" : r.v0_why.c_str());
        }
    }

    if( ok ) { std::printf("WITNESS CHECK OK (%d checks passed)\n", checks_passed); }
    else     { std::printf("WITNESS CHECK FAILED\n"); }
    return ok ? 0 : 1;
}
