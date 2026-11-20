// klut_loader_check -- Klut's table loader refuses damaged tables.
//
// Klut::Subtable::Read (src/Klut/Subtable.hpp) reads Klut_Values_NN.tsv (knot
// names with key counts) and Klut_Keys_NN.bin (the keys, NN bytes each). A
// damaged pair must make the subtable fail -- an ERROR line, FindID returning
// Klut::error, nothing exported -- rather than load part of the table, or the
// wrong table, and answer from it. Each case below damages a copy of the real
// 3- and 4-crossing tables (embedded, so this runs without Git LFS), loads it
// with Klut directly, and checks what Klut says. knoodleidentify checks the
// tables itself before Klut sees them (test/klut_table_check.py); this test is
// for every other user of Klut.
//
// Cases: a keys file one byte short (the last key was zero-padded and
// inserted); a count that does not parse (it ended the table silently); a
// count of 0 (keys then shift onto the wrong knots); a name with no count at
// the end of the file (the unfixed loader then used an uninitialized count);
// keys left over after the last count; a key listed
// twice (the second knot became unreachable); a directory that cannot be
// searched (std::filesystem::exists threw out of the constructor); and
// SubtableNames out of range (it read past the end of the subtables).
// Also: an intact table loads, and a missing table is still silent, as Klut's
// constructor documents -- lookups at that crossing number return Klut::error.

#include "../Knoodle.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using Klut = Knoodle::Klut;

namespace
{

int checks = 0;
int failures = 0;

void Check(bool ok, const std::string& what)
{
    ++checks;
    if (ok) { return; }
    ++failures;
    std::cout << "  FAILED  " << what << "\n";
}

std::string Bytes(std::initializer_list<unsigned char> b) { return std::string(b.begin(), b.end()); }

// data/Klut/Klut_{Values,Keys}_{03,04}, byte for byte.
const std::string kValues3 = "K[3,1,1,\"e/r\"]\t1\nK[3,1,1,\"m/mr\"]\t1\n";
const std::string kKeys3   = Bytes({ 0x0f, 0x0d, 0x0f, 0x0e, 0x0c, 0x0e });
const std::string kValues4 = "K[4,1,1,\"e/m/r/mr\"]\t2\n";
const std::string kKeys4   = Bytes({ 0x17, 0x0d, 0x16, 0x0c, 0x15, 0x0f, 0x14, 0x0e });

void Write(const fs::path& p, const std::string& s)
{
    std::ofstream out(p, std::ios::binary);
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
}

fs::path MakeTables(const fs::path& root, const std::string& name,
                    const std::string& v3, const std::string& k3,
                    const std::string& v4 = kValues4, const std::string& k4 = kKeys4)
{
    const fs::path d = root / name;
    fs::create_directories(d);
    Write(d / "Klut_Values_03.tsv", v3);
    Write(d / "Klut_Keys_03.bin", k3);
    Write(d / "Klut_Values_04.tsv", v4);
    Write(d / "Klut_Keys_04.bin", k4);
    return d;
}

// What one Klut, built on `dir` for up to 4 crossings, says.
struct Outcome
{
    bool        threw = false;
    std::string cerr_text;
    std::size_t names3 = 0, ids3 = 0, names4 = 0;
    std::vector<std::string> trefoil;   // FindID of each trefoil code, as a name
};

// MacLeod codes of the trefoil's two keys, from an intact load.
std::vector<std::vector<Klut::CodeInt>> trefoil_codes;

std::string IdName(Klut& klut, Klut::ID_T id)
{
    if (id == Klut::error)     { return "error"; }
    if (id == Klut::not_found) { return "not_found"; }
    const auto names = klut.SubtableNames(3);
    return id < names.size() ? names[id] : "id " + std::to_string(id);
}

Outcome Load(const fs::path& dir)
{
    Outcome o;
    std::stringstream buf;
    std::streambuf* old = std::cerr.rdbuf(buf.rdbuf());
    try
    {
        Klut klut(dir, 4);
        klut.RequireSubtables();
        o.names3 = klut.SubtableNames(3).size();
        o.ids3   = static_cast<std::size_t>(klut.SubtableIDs(3).Size());
        o.names4 = klut.SubtableNames(4).size();
        if (trefoil_codes.empty())
        {
            const auto codes = klut.SubtableCodes(3);
            for (Knoodle::Size_T i = 0; i < codes.Dim(0); ++i)
            {
                trefoil_codes.emplace_back(codes.data(i), codes.data(i) + 3);
            }
        }
        for (const auto& code : trefoil_codes)
        {
            o.trefoil.push_back(IdName(klut, klut.FindID(code.data(), 3).second));
        }
    }
    catch (const std::exception& e)
    {
        o.threw = true;
        buf << "(exception: " << e.what() << ")";
    }
    std::cerr.rdbuf(old);
    o.cerr_text = buf.str();
    return o;
}

bool SaidError(const Outcome& o) { return o.cerr_text.find("ERROR") != std::string::npos; }

std::string Show(const Outcome& o)
{
    std::string s = o.threw ? "THREW" : "no exception";
    s += ", names3=" + std::to_string(o.names3) + ", ids3=" + std::to_string(o.ids3) + ", trefoil keys ->";
    for (const auto& t : o.trefoil) { s += " " + t; }
    if (o.threw) { s += "\n      " + o.cerr_text.substr(o.cerr_text.find("(exception: ")); }
    if (!SaidError(o)) { return s + ", no ERROR"; }
    const auto at = o.cerr_text.find("ERROR");
    const auto end = o.cerr_text.find('\n', at);
    return s + "\n      " + o.cerr_text.substr(at, end == std::string::npos ? std::string::npos : end - at);
}

// A damaged 3-crossing table: an ERROR, no exception, nothing exported, and
// lookups say error. The 4-crossing table beside it is unaffected.
void ExpectRefused(const std::string& what, const Outcome& o, const std::string& must_say)
{
    std::cout << "  " << what << ": " << Show(o) << "\n";
    Check(!o.threw, what + ": no exception");
    Check(SaidError(o), what + ": an ERROR line");
    Check(o.cerr_text.find(must_say) != std::string::npos, what + ": the ERROR says \"" + must_say + "\"");
    Check(o.names3 == 0 && o.ids3 == 0, what + ": no partial table exported");
    bool all_error = !o.trefoil.empty();
    for (const auto& t : o.trefoil) { all_error = all_error && (t == "error"); }
    Check(all_error, what + ": FindID returns Klut::error");
    Check(o.names4 == 1, what + ": the 4-crossing table still loads");
}

} // namespace

int main()
{
    std::cout << "klut_loader_check -- Klut::Subtable::Read on damaged tables\n";

    const fs::path root = fs::temp_directory_path()
        / ("klut_loader_check_" + std::to_string(static_cast<long>(::getpid())));
    fs::remove_all(root);
    fs::create_directories(root);

    // Intact: both trefoil keys found under their own names.
    {
        const Outcome o = Load(MakeTables(root, "intact", kValues3, kKeys3));
        std::cout << "  intact: " << Show(o) << "\n";
        Check(!o.threw && !SaidError(o), "intact: loads without an ERROR");
        Check(o.names3 == 2 && o.ids3 == 2 && o.names4 == 1, "intact: 2 + 1 names, 2 keys");
        Check(o.trefoil == std::vector<std::string>({ "K[3,1,1,\"e/r\"]", "K[3,1,1,\"m/mr\"]" })
              || o.trefoil == std::vector<std::string>({ "K[3,1,1,\"m/mr\"]", "K[3,1,1,\"e/r\"]" }),
              "intact: each trefoil key finds its own name");
        Check(trefoil_codes.size() == 2, "intact: two trefoil codes to look up (setup)");
    }

    ExpectRefused("keys file one byte short",
        Load(MakeTables(root, "short_key", kValues3, kKeys3.substr(0, 5))), "ends after 1 keys");

    ExpectRefused("a count that does not parse",
        Load(MakeTables(root, "bad_count", "K[3,1,1,\"e/r\"]\tx\nK[3,1,1,\"m/mr\"]\t1\n", kKeys3)),
        "no valid key count");

    ExpectRefused("a count of 0",
        Load(MakeTables(root, "zero_count", "K[3,1,1,\"e/r\"]\t0\nK[3,1,1,\"m/mr\"]\t2\n", kKeys3)),
        "no valid key count");

    ExpectRefused("a name with no count at the end",
        Load(MakeTables(root, "no_count", "K[3,1,1,\"e/r\"]\t1\nK[3,1,1,\"m/mr\"]\n", kKeys3)),
        "no valid key count");

    ExpectRefused("a key left over after the last count",
        Load(MakeTables(root, "extra_key", kValues3, kKeys3 + kKeys3.substr(0, 3))),
        "holds more than the 2 keys");

    ExpectRefused("a key listed twice",
        Load(MakeTables(root, "duplicate", kValues3, kKeys3.substr(0, 3) + kKeys3.substr(0, 3))),
        "repeats a key of K[3,1,1,\"e/r\"]");

    // A directory that cannot be searched: the files may well exist, so this
    // is an error, not a missing table. Skipped where permissions are not
    // enforced (e.g., running as root).
    {
        const fs::path d = MakeTables(root, "locked", kValues3, kKeys3);
        ::chmod(d.c_str(), 0);
        const bool enforced = !std::ifstream(d / "Klut_Values_03.tsv").good();
        if (enforced)
        {
            const Outcome o = Load(d);
            std::cout << "  a directory that cannot be searched: " << Show(o) << "\n";
            Check(!o.threw, "unsearchable directory: no exception");
            Check(o.cerr_text.find("Could not check for") != std::string::npos,
                  "unsearchable directory: an ERROR says it could not check for the files");
            Check(o.names3 == 0 && o.names4 == 0, "unsearchable directory: nothing loaded");
        }
        else
        {
            std::cout << "  a directory that cannot be searched: skipped (permissions not enforced)\n";
        }
        ::chmod(d.c_str(), 0755);
    }

    // A missing 4-crossing table is not an error (Klut's constructor: tables
    // load "if the corresponding files exist"), and the 3s still work.
    {
        const fs::path d = MakeTables(root, "no_4s", kValues3, kKeys3);
        fs::remove(d / "Klut_Values_04.tsv");
        fs::remove(d / "Klut_Keys_04.bin");
        const Outcome o = Load(d);
        std::cout << "  no 4-crossing table: " << Show(o) << "\n";
        Check(!o.threw && !SaidError(o), "missing table: still silent");
        Check(o.names3 == 2 && o.names4 == 0, "missing table: the 3s load, the 4s are empty");
    }

    // SubtableNames out of range: empty, not a read past the end. Last, since
    // the unfixed loader can crash here.
    {
        // (The unfixed loader indexed past the end of its subtables here; it
        // was seen trying to open a Subtable with an empty path.)
        const fs::path d = MakeTables(root, "range", kValues3, kKeys3);
        std::stringstream buf;
        std::streambuf* old = std::cerr.rdbuf(buf.rdbuf());
        bool empty = false;
        bool ids_empty = false;
        {
            Klut klut(d, 4);
            empty = klut.SubtableNames(5).empty() && klut.SubtableNames(1000).empty();
            ids_empty = klut.SubtableIDs(5).Size() == 0;
        }
        std::cerr.rdbuf(old);
        std::cout << "  SubtableNames out of range: " << (empty ? "empty" : "NOT empty")
                  << (buf.str().empty() ? ", nothing printed" : ", printed: " + buf.str().substr(0, buf.str().find('\n'))) << "\n";
        Check(empty && buf.str().find("ERROR") == std::string::npos,
              "SubtableNames out of range is empty, with no ERROR");
        Check(ids_empty, "SubtableIDs out of range is empty");
    }

    fs::remove_all(root);

    std::cout << (failures == 0 ? "KLUT LOADER CHECK OK" : "KLUT LOADER CHECK FAILED")
              << " (" << checks << " checks, " << failures << " failed)\n";
    return failures == 0 ? 0 : 1;
}
