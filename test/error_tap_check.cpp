/**
 * @file error_tap_check.cpp
 * @brief Regression test for the CLI tools' fail-loudly machinery.
 *
 * Background: the core library reports unrecoverable trouble via Tools' eprint()
 * and then returns a diagram it has already disclaimed -- e.g.
 * PlanarDiagramComplex::Rattle's "returned invalid status flag for 10 random
 * rotation matrices. ... Returning an invalid diagram." The PDC writers then skip
 * invalid diagrams silently, so a run could finish, write a quietly truncated
 * file, and exit 0. The tools now tap std::cerr to count "ERROR: " lines, stage
 * output in a ".partial" file, and refuse to commit it if anything errored.
 *
 * This test pins the two pieces that logic rests on:
 *   1. CerrErrorTap counts eprint() but NOT wprint(), and passes text through.
 *   2. AtomicOutFile publishes on Commit, and on Abort leaves no partial file and
 *      does not disturb a pre-existing file of the same name.
 *
 * Build: see test/Makefile (target: error_tap_check). Exit 0 if all pass.
 */

#include "../tools/knoodle_io.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void Check(bool ok, const std::string& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok) { ++failures; }
}

} // namespace

int main()
{
    std::cout << "error_tap_check\n";

    // ---- 1. the cerr tap -----------------------------------------------------
    {
        // Capture what the tap forwards, so we can assert it is passed through
        // untouched as well as counted.
        std::ostringstream sink;
        std::streambuf* real_cerr = std::cerr.rdbuf(sink.rdbuf());

        long counted = 0;
        {
            CerrErrorTap tap;
            g_cerr_tap = &tap;

            Tools::wprint("a warning that must not count");
            Check(tap.Count() == 0, "wprint does not count as an error");

            Tools::eprint("a genuine library error");
            Check(tap.Count() == 1, "eprint counts as one error");

            Tools::eprint("a second one");
            Check(tap.Count() == 2, "a second eprint counts");

            Check(ErrorsSeen(), "ErrorsSeen() is true once the library errored");

            // The report quotes what the library actually said, so the text must
            // be retained, not just tallied.
            Check(tap.Messages().size() == 2, "both error lines are retained");
            Check(tap.Messages().at(0).find("a genuine library error") != std::string::npos,
                  "retained text is the error message itself");
            Check(tap.Messages().at(0).starts_with("ERROR:"),
                  "retained line keeps its ERROR: prefix");

            counted = tap.Count();
            g_cerr_tap = nullptr;
        }
        std::cerr.rdbuf(real_cerr);

        const std::string forwarded = sink.str();
        Check(counted == 2, "final count is 2");
        Check(forwarded.find("a genuine library error") != std::string::npos,
              "tapped text is still forwarded to the real stream");
        Check(forwarded.find("a warning that must not count") != std::string::npos,
              "warnings are forwarded too");
    }

    // ---- 2. atomic output ----------------------------------------------------
    {
        namespace fs = std::filesystem;
        const fs::path dir  = fs::temp_directory_path() / "knoodle_error_tap_check";
        fs::remove_all(dir);
        fs::create_directories(dir);

        // Commit publishes.
        {
            const fs::path p = dir / "committed.tsv";
            {
                AtomicOutFile f(p);
                Check(f.Good(), "AtomicOutFile opens");
                f.Stream() << "payload\n";
                Check(!fs::exists(p), "nothing at the final path before Commit");
                Check(fs::exists(p.string() + ".partial"), "staged as .partial");
                Check(f.Commit(), "Commit succeeds");
            }
            Check(fs::exists(p), "file exists after Commit");
            Check(!fs::exists(p.string() + ".partial"), "no .partial left after Commit");
            std::ifstream in(p);
            std::string line;
            std::getline(in, line);
            Check(line == "payload", "committed content is intact");
        }

        // Abort leaves a pre-existing file untouched.
        {
            const fs::path p = dir / "preexisting.tsv";
            { std::ofstream seed(p); seed << "ORIGINAL\n"; }
            {
                AtomicOutFile f(p);
                f.Stream() << "REPLACEMENT\n";
                f.Abort();
            }
            Check(!fs::exists(p.string() + ".partial"), "no .partial left after Abort");
            std::ifstream in(p);
            std::string line;
            std::getline(in, line);
            Check(line == "ORIGINAL", "Abort leaves the pre-existing file untouched");
        }

        // Destruction without Commit behaves like Abort.
        {
            const fs::path p = dir / "dropped.tsv";
            {
                AtomicOutFile f(p);
                f.Stream() << "never committed\n";
            }
            Check(!fs::exists(p), "no file published when Commit was never called");
            Check(!fs::exists(p.string() + ".partial"), "no .partial left behind either");
        }

        fs::remove_all(dir);
    }

    std::cout << (failures == 0 ? "PASS: fail-loudly machinery behaves\n"
                                : "FAIL: " + std::to_string(failures) + " check(s) failed\n");
    return (failures == 0) ? 0 : 1;
}
