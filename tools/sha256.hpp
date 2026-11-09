#pragma once

// SHA-256 (FIPS 180-4), dependency-free, for checking the KLUT tables against
// data/Klut/Klut_Manifest.txt. Not constant-time and not meant for anything
// secret: it guards against damaged or mismatched files, not attackers.
//
//   knoodle_sha256::Sha256 h;  h.Update(bytes, n);  ...;  std::string hex = h.HexDigest();
//   std::string err;  std::optional<std::string> hex = knoodle_sha256::FileHexDigest(path, err);
//
// test/sha256_check.cpp checks it against the FIPS 180-2 vectors and against
// every padding boundary near a block edge.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace knoodle_sha256
{

class Sha256
{
public:
    Sha256() { Reset(); }

    void Reset()
    {
        h_ = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
               0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };
        length_   = 0;
        buffered_ = 0;
    }

    void Update(const void* data, std::size_t n)
    {
        const auto* p = static_cast<const std::uint8_t*>(data);
        length_ += n;
        if (buffered_ > 0)
        {
            const std::size_t take = std::min(n, std::size_t(64) - buffered_);
            for (std::size_t i = 0; i < take; ++i) { buffer_[buffered_ + i] = p[i]; }
            buffered_ += take;
            p += take;
            n -= take;
            if (buffered_ < 64) { return; }
            Block(buffer_.data());
            buffered_ = 0;
        }
        while (n >= 64)
        {
            Block(p);
            p += 64;
            n -= 64;
        }
        for (std::size_t i = 0; i < n; ++i) { buffer_[i] = p[i]; }
        buffered_ = n;
    }

    void Update(std::string_view s) { Update(s.data(), s.size()); }

    /// Finish and return the digest. The object must be Reset() before reuse.
    std::array<std::uint8_t, 32> Digest()
    {
        const std::uint64_t bits = length_ * 8u;
        const std::uint8_t one = 0x80;
        Update(&one, 1);
        const std::uint8_t zero = 0;
        while (buffered_ != 56) { Update(&zero, 1); }
        std::uint8_t len[8];
        for (int i = 0; i < 8; ++i) { len[i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i)); }
        Update(len, 8);

        std::array<std::uint8_t, 32> out{};
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                out[4 * i + j] = static_cast<std::uint8_t>(h_[i] >> (24 - 8 * j));
            }
        }
        return out;
    }

    /// Finish and return the digest as 64 lowercase hex digits.
    std::string HexDigest()
    {
        static constexpr char hex[] = "0123456789abcdef";
        std::string s;
        s.reserve(64);
        for (std::uint8_t b : Digest())
        {
            s.push_back(hex[b >> 4]);
            s.push_back(hex[b & 15]);
        }
        return s;
    }

private:
    static std::uint32_t Rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    void Block(const std::uint8_t* p)
    {
        static constexpr std::uint32_t k[64] = {
            0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
            0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
            0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
            0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
            0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
            0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
            0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
            0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
        };

        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i)
        {
            w[i] = (std::uint32_t(p[4 * i]) << 24) | (std::uint32_t(p[4 * i + 1]) << 16)
                 | (std::uint32_t(p[4 * i + 2]) << 8) | std::uint32_t(p[4 * i + 3]);
        }
        for (int i = 16; i < 64; ++i)
        {
            const std::uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3];
        std::uint32_t e = h_[4], f = h_[5], g = h_[6], h = h_[7];
        for (int i = 0; i < 64; ++i)
        {
            const std::uint32_t S1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ (~e & g);
            const std::uint32_t t1 = h + S1 + ch + k[i] + w[i];
            const std::uint32_t S0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
            const std::uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = S0 + mj;
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d;
        h_[4] += e; h_[5] += f; h_[6] += g; h_[7] += h;
    }

    std::array<std::uint32_t, 8> h_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::uint64_t length_   = 0;
    std::size_t   buffered_ = 0;
};

/// SHA-256 of a whole file as lowercase hex, or nullopt with `err` set.
inline std::optional<std::string> FileHexDigest(const std::filesystem::path& path, std::string& err)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        err = "cannot be opened for reading";
        return std::nullopt;
    }
    Sha256 h;
    std::vector<char> chunk(std::size_t(1) << 20);
    while (in)
    {
        in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        const std::streamsize got = in.gcount();
        if (got > 0) { h.Update(chunk.data(), static_cast<std::size_t>(got)); }
    }
    if (in.bad())
    {
        err = "read error";
        return std::nullopt;
    }
    return h.HexDigest();
}

} // namespace knoodle_sha256
