#include "huffman/BitIO.h"
#include "huffman/Huffman.h"

#include <gtest/gtest.h>

#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace huff;

static std::vector<uint8_t> bytesOf(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

// --- round-trip correctness (the core guarantee) ----------------------------

TEST(Huffman, RoundTripEmpty) {
    std::vector<uint8_t> in;
    EXPECT_EQ(decompress(compress(in)), in);
}

TEST(Huffman, RoundTripSingleByte) {
    std::vector<uint8_t> in = {42};
    EXPECT_EQ(decompress(compress(in)), in);
}

TEST(Huffman, RoundTripSingleSymbolRepeated) {
    std::vector<uint8_t> in(1000, 'A');           // only one distinct symbol
    EXPECT_EQ(decompress(compress(in)), in);
}

TEST(Huffman, RoundTripText) {
    auto in = bytesOf("the quick brown fox jumps over the lazy dog, again and again!");
    EXPECT_EQ(decompress(compress(in)), in);
}

TEST(Huffman, RoundTripAllByteValues) {
    std::vector<uint8_t> in(256);
    std::iota(in.begin(), in.end(), 0);           // every possible byte, once
    EXPECT_EQ(decompress(compress(in)), in);
}

TEST(Huffman, RoundTripRandom) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> in(50000);
    for (auto& b : in) b = static_cast<uint8_t>(dist(rng));
    EXPECT_EQ(decompress(compress(in)), in);
}

// --- it actually compresses -------------------------------------------------

TEST(Huffman, SkewedDataShrinks) {
    // 90% 'A', rest spread out -> low entropy -> should compress well.
    std::vector<uint8_t> in;
    for (int i = 0; i < 10000; ++i) in.push_back(i % 10 == 0 ? static_cast<uint8_t>(i % 256) : 'A');
    EXPECT_LT(compress(in).size(), in.size());
}

// --- bit I/O ----------------------------------------------------------------

TEST(BitIO, WriteReadRoundTrip) {
    BitWriter bw;
    const std::vector<int> bits = {1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0};
    for (int b : bits) bw.writeBit(b);
    bw.flush();
    BitReader br(bw.bytes().data(), bw.bytes().size());
    for (int b : bits) EXPECT_EQ(br.readBit(), b);
}
