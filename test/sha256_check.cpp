// sha256_check -- tools/sha256.hpp against known digests.
//
// knoodleidentify checks the KLUT tables against the SHA-256 sums in
// data/Klut/Klut_Manifest.txt, which scripts/klut-manifest.py writes with
// Python's hashlib. So the C++ digest must agree with hashlib byte for byte,
// on every message length -- the padding near a 64-byte block edge (55, 56,
// 63, 64, 119, 120 bytes...) is where a hand-written SHA-256 goes wrong.
//
// The expected digests are the FIPS 180-2 examples plus hashlib's digests of
// prefixes of a fixed byte pattern. Each message is hashed whole, one byte at
// a time, and in uneven chunks, which must all agree; and FileHexDigest is
// checked on a file larger than its 1 MiB read buffer.

#include "../tools/sha256.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

int checks = 0;
int failures = 0;

void Check(bool ok, const std::string& what, const std::string& got = "", const std::string& want = "")
{
    ++checks;
    if (ok) { return; }
    ++failures;
    std::cout << "  FAILED  " << what << "\n";
    if (!got.empty()) { std::cout << "          got  " << got << "\n          want " << want << "\n"; }
}

std::string Whole(const std::string& msg)
{
    knoodle_sha256::Sha256 h;
    h.Update(msg);
    return h.HexDigest();
}

std::string ByteAtATime(const std::string& msg)
{
    knoodle_sha256::Sha256 h;
    for (char ch : msg) { h.Update(&ch, 1); }
    return h.HexDigest();
}

std::string Chunked(const std::string& msg)
{
    // Chunk sizes chosen to straddle block edges in different ways.
    static constexpr std::size_t sizes[] = { 1, 63, 2, 64, 65, 7, 128, 13 };
    knoodle_sha256::Sha256 h;
    std::size_t pos = 0;
    for (std::size_t i = 0; pos < msg.size(); ++i)
    {
        const std::size_t n = std::min(sizes[i % std::size(sizes)], msg.size() - pos);
        h.Update(msg.data() + pos, n);
        pos += n;
    }
    return h.HexDigest();
}

void CheckMessage(const std::string& label, const std::string& msg, const std::string& want)
{
    const std::string whole = Whole(msg);
    Check(whole == want, label + ": whole message", whole, want);
    Check(ByteAtATime(msg) == want, label + ": one byte at a time");
    Check(Chunked(msg) == want, label + ": uneven chunks");
}

std::string Pattern(std::size_t n)
{
    std::string s(n, '\0');
    for (std::size_t i = 0; i < n; ++i) { s[i] = static_cast<char>((i * 7 + 3) & 0xff); }
    return s;
}

} // namespace

int main()
{
    std::cout << "sha256_check -- tools/sha256.hpp vs FIPS 180-2 and hashlib digests\n";

    CheckMessage("FIPS \"abc\"", "abc",
                 "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CheckMessage("FIPS 448-bit", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                 "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CheckMessage("FIPS 896-bit",
                 "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
                 "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
                 "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
    CheckMessage("FIPS a million a's", std::string(1000000, 'a'),
                 "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    // hashlib.sha256(bytes((i*7+3) & 0xff for i in range(n))).hexdigest()
    static const struct { std::size_t n; const char* hex; } prefixes[] = {
        {    0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },
        {    1, "084fed08b978af4d7d196a7446a86b58009e636b611db16211b65a9aadff29c5" },
        {    3, "6ab0dba1f4f1dfbb37b4f9eeb092c09fca4900ad32bdcd147d8dde35d6c87c35" },
        {   55, "e7313d333c272e639f790978283f9eb392e843d0f29b7016828bb1daa4aac70b" },
        {   56, "4324d65f3c103567f5589c710bc08f8523f929a9272e3af36fc968e52abc6c27" },
        {   57, "35df609437dcfea3279283ab79fd554e2bf78f8f7ae2de532d8ee300b09e8f73" },
        {   63, "81c80242132f230c3bd41b3e63bbcff16107339549214a99614ff26664625055" },
        {   64, "39e3d7b6b5d075d37d053ad89b24b41bef4f3c29760c84447cab3f3be1882241" },
        {   65, "aacca6ff74fdbb296d165a45cecfa04e5127bc008770fbbdd48006f2d2fae95e" },
        {  111, "67d9492e628fd376e0b2efec8ca2b99b123e202cf620deb270728df979b2f73e" },
        {  112, "96b928cff8528dbb99602c709a65b846cb6467acb8b722f0d758e4dc27bfc508" },
        {  119, "9ce7368e4daf32341631b492e80359dc9f594b48453cd0dd5bf0b19279cc177e" },
        {  120, "7836b787757e95e58b3ca5aec90b1b004e8deba1e50e9675af9cabf1a13a04b5" },
        {  127, "a8d23e75d936f303d248888d9b165ee543f4cbafcad3c9dd2a79bd84faa11d07" },
        {  128, "d2742f1f4ac6bb7ca2b239ee18402ba8b3f9f8e652d2a72973c2b9ba11c08cf6" },
        {  129, "307f8fc2c1622b92762e818d39a185d4d667ad49a4b07ceae1f4afa008a93ec4" },
        { 1000, "1e9bc38cbf860b9ec31918b065f9b52476c549a782e0e7990bed8ce3868d2371" },
        { 2000, "125282f6f95ac691d3c7bcbad682fba56f43302283037780c5de3bcab68ed0ff" },
    };
    for (const auto& p : prefixes)
    {
        CheckMessage("pattern, " + std::to_string(p.n) + " bytes", Pattern(p.n), p.hex);
    }

    // FileHexDigest reads in 1 MiB chunks; a 2.5 MB file of a's crosses two of
    // them. The digest must equal the in-memory one.
    {
        const std::filesystem::path f = std::filesystem::temp_directory_path()
            / ("sha256_check_" + std::to_string(std::rand()) + ".bin");
        const std::string big(2500000, 'a');
        {
            std::ofstream out(f, std::ios::binary);
            out.write(big.data(), static_cast<std::streamsize>(big.size()));
        }
        std::string err;
        const auto got = knoodle_sha256::FileHexDigest(f, err);
        const std::string want = Whole(big);
        Check(got.has_value() && *got == want, "FileHexDigest on a 2.5 MB file",
              got.value_or("(nullopt: " + err + ")"), want);
        std::filesystem::remove(f);

        const auto missing = knoodle_sha256::FileHexDigest(f, err);
        Check(!missing.has_value() && !err.empty(), "FileHexDigest on a missing file fails with a reason");
    }

    std::cout << (failures == 0 ? "SHA256 CHECK OK" : "SHA256 CHECK FAILED")
              << " (" << checks << " checks, " << failures << " failed)\n";
    return failures == 0 ? 0 : 1;
}
